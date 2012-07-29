//
//  enquire.cpp
//  Argos
//
//  Created by Windoze on 12-7-6.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <boost/bind.hpp>
#include "common/evaluatable.h"
#include "index/full_index.h"
#include "query/enquire.h"

namespace argos {
    namespace query {
        namespace detail {
            class filtered_doc_iterator_impl : public doc_iterator_impl {
            public:
                filtered_doc_iterator_impl(common::eva_ptr_t filter, doc_iterator_impl_ptr_t filtee, common::ExecutionContext &ctx)
                : filter_(filter)
                , filtee_(filtee)
                , ctx_(ctx)
                {
                    first_valid();
                }
                
                virtual bool end() const
                {
                    return filtee_->end();
                }
                
                virtual void next()
                {
                    filtee_->next();
                    first_valid();
                }
                
                virtual docid skip_to(docid did)
                {
                    filtee_->skip_to(did);
                    first_valid();
                    return filtee_->get_docid();
                }
                
                virtual docid get_docid() const
                {
                    return filtee_->get_docid();
                }
                
                virtual size_t estimate_size() const
                {
                    return filtee_->estimate_size();
                }
                
                virtual void save_match_info(docid did, common::match_info_t &match_info)
                {
                    if (did==get_docid()) {
                        filtee_->save_match_info(did, match_info);
                    }
                }

            private:
                inline bool check(docid did) const {
                    if (ctx_.get_index()->is_deleted(did)) {
                        return false;
                    }
                    return bool(filter_->evaluate(did, ctx_));
                }
                inline void first_valid()
                {
                    while (!end() && !check(get_docid())) {
                        filtee_->next();
                    }
                }
                
                common::eva_ptr_t filter_;
                doc_iterator_impl_ptr_t filtee_;
                common::ExecutionContext &ctx_;
            };
            
            class Enquire_impl {
            public:
                Enquire_impl(const Query &q)
                : match_(q.match_)
                , filter_(q.filter_)
                {}
                
                doc_iterator_impl_ptr_t match(common::ExecutionContext &ctx)
                {
                    return match_->match(ctx);
                }
                
                doc_iterator_impl_ptr_t filter(doc_iterator_impl_ptr_t filtee, common::ExecutionContext &ctx)
                {
                    return doc_iterator_impl_ptr_t(new filtered_doc_iterator_impl(filter_, filtee, ctx));
                }
                
                doc_iterator_impl_ptr_t execute(common::ExecutionContext &ctx)
                {
                    // NOTE: Don't run filter here as we update per-doc match_info *after* we have a doc from returned doc_iterator
                    // This makes impossible to use match_info info in filter as filter is executed *before* returning
                    // NOTE1: Do we really need to use match info in filter?
                    
                    //return filter(match(ctx), ctx);
                    return match(ctx);
                }
                
                match_ptr_t match_;
                common::eva_ptr_t filter_;
            };
        }   // End of namespace detail
        
        doc_iterator Enquire::execute(const Query &q, common::ExecutionContext &ctx)
        {
            doc_iterator ret=doc_iterator(detail::Enquire_impl(q).execute(ctx));
            return ret;
        }
        
        inline size_t value_length(const common::Value &v)
        {
            switch (v.type_) {
                case common::VT_ARRAY:
                    return v.array.size();
                case common::VT_EMPTY:
                    return 0;
                default:
                    break;
            }
            return 1;
        }
        
        inline common::Value get_value_at(const common::Value &v, size_t i)
        {
            return v.type_==common::VT_ARRAY ? v.array.get_element(i) : v;
        }

        typedef std::vector<size_t> comb_t;
        
        bool incr(comb_t::iterator begin, comb_t::iterator end, comb_t::const_iterator obegin, comb_t::const_iterator oend)
        {
            if (begin==end) {
                // Nothing to incr
                return false;
            }
            
            comb_t::iterator last=end-1;
            comb_t::const_iterator olast=oend-1;
            (*last)++;
            if (*last>=*olast) {
                // Overflow
                *last=0;
                return incr(begin, end-1, obegin, oend-1);
            }
            
            return true;
        }
        
        void update_histo(docid did, const group_key_t &key, histogram_t &histo, common::ExecutionContext &ctx)
        {
            if (key.empty()) {
                return;
            }
            if (histo.empty()) {
                // Reduce memory re-allocation
                histo.max_load_factor(3);
                histo.reserve(500);
            }
            common::value_list_t vl(key.size(), common::Value());
            // TODO:
            comb_t overflows(key.size());
            comb_t currents(key.size(), 0);
            for (int i=0; i<key.size(); i++) {
                if (key[i].empty()) return;
                size_t len=value_length(key[i]);
                if (len==0) {
                    // No value combinations as there is an empty value
                    return;
                }
                overflows[i]=len;
            }
            // Uses do-while as we at least have 1 comb at here
            comb_t::iterator begin=currents.begin();
            comb_t::iterator end=currents.end();
            comb_t::const_iterator obegin=overflows.begin();
            comb_t::const_iterator oend=overflows.end();
            do {
                for (size_t i=0; i<currents.size(); i++) {
                    vl[i]=get_value_at(key[i], currents[i]);
                    //std::cout << common::Constant(vl[i]).serialize() << ',';
                }
                //std::cout << std::endl;
                histogram_t::iterator i=histo.find(vl);
                if (i==histo.end()) {
                    histo[vl]=1;
                } else {
                    (i->second)++;
                }
            } while (incr(begin, end, obegin, oend));
        }
        
        bool update_histos(docid did, const group_keys_t &keys, histograms_t &histos, common::ExecutionContext &ctx)
        {
            if (histos.empty()) {
                return true;
            }
            if (histos.size()!=keys.size()) {
                return false;
            }
            for (size_t i=0; i<histos.size(); i++) {
                update_histo(did, keys[i], histos[i], ctx);
            }
            //std::cout << std::endl;
            return true;
        }
        
        inline bool update_histos(const doc_value_pair_t &dvp, histograms_t &histos, common::ExecutionContext &ctx)
        {
            return update_histos(dvp.did, dvp.histo_keys, histos, ctx);
        }
        
        size_t Enquire::execute(const Query &q, results_t &results, histograms_t &histos, common::ExecutionContext &ctx)
        {
            // Prepare result and a heap to sort results
            results.resize(q.nr_+q.sk_);
            //typedef common::heaper<results_t::iterator> result_heap_t;
            typedef common::heaper<results_t::iterator, sort_comprarator> result_heap_t;
            result_heap_t rh(results.begin(), results.end(), true, sort_comprarator(q.sort_crit_));

            // Get doc iterator, which will return all docs match terms
            doc_iterator di=execute(q, ctx);
            
            // this is total hits
            size_t ret=0;
            doc_value_pair_t dvp(UNUSED_DOC, common::Value());
            
            bool use_match_info=q.uses_match_info();
            
            for (; di; di++) {
                if (ctx.get_index()->is_deleted(*di)) {
                    // Skip deleted docs
                    continue;
                }
                
                // Record matched docid
                dvp.did=*di;

                // Check if match_info is really needed
                if (use_match_info) {
                    // Make sure match_info for every doc is properly prepared
                    dvp.match_info.resize(ctx.get_match_info_size(), common::Value(int64_t(0)));
                    for (size_t i=0; i<dvp.match_info.size(); i++) {
                        dvp.match_info[i]=common::Value(int64_t(0));
                    }
                    
                    // Update match_info for this doc
                    di.save_match_info(dvp.did, dvp.match_info);
                }
                
#if 0
                // Debug code
                std::cout << dvp.did << ':' << ctx.get_index()->get_primary_key(dvp.did) <<" - ";
                std::cout.flush();
                for (common::match_term_dict_t::const_iterator i=ctx.match_term_dict.begin(); i!=ctx.match_term_dict.end(); ++i) {
                    std::cout << i->first << '[' << i->second << ']';
                    std::cout.flush();
                    std::cout << ":" << common::Constant(dvp.match_info[i->second]).to_string() << '\t';
                }
                std::cout << std::endl;
#endif
                
                // Save match_info in context, make sure it can be used by filter and sortkey
                if (use_match_info) {
                    ctx.set_match_info(&dvp.match_info);
                }

                // Now do filter, discard false docs
                if (bool(q.filter_->evaluate(dvp.did, ctx))) {
                    // Calculate sort keys
                    evaluate(dvp.did, q.sort_crit_, dvp.sort_keys, ctx);
                    // Clear previous histgrams in dvp
                    dvp.histo_keys.clear();
                    // Calculate histogram keys
                    evaluate(dvp.did, q.histos_, dvp.histo_keys, ctx);
                    // Update histograms
                    update_histos(dvp, histos, ctx);
#if 0
                    // Debug code
                    for (histograms_t::const_iterator i=histos.begin(); i!=histos.end(); ++i) {
                        std::cout << "-------------------\n";
                        for (histogram_t::const_iterator j=i->begin(); j!=i->end(); ++j) {
                            std::cout << '[';
                            for (size_t k=0; k<j->first.size(); ++k) {
                                std::cout << common::Constant((j->first)[k]).serialize() << ',';
                            }
                            std::cout << "]:" << j->second << std::endl;
                        }
                    }
#endif
                    // Heap sort
                    rh.push(dvp);
                    // Increment total hits
                    ret++;
                }
            }
            // rh_sz is size of result heap, it'll change after partial_sort, save the value for later use
            size_t rh_sz=rh.size();
            if (q.sk_>=rh_sz) {
                // All results are skipped, just return total hits number with empty result set
                results.clear();
            } else {
                // Partial sort result heap
                rh.partial_sort(q.sk_);

                // Erase unused part at the end, in case total hits is smaller than nr
                results.erase(results.begin()+rh_sz, results.end());
                if (q.sk_>0) {
                    // Erase skipped part
                    results.erase(results.begin(), results.begin()+q.sk_);
                }
                // Now results only contains actually needed docs
            }
            // Return total hits
            return ret;
        }
        
        size_t Enquire::execute(const pk_list_t &pks, results_t &results, common::ExecutionContext &ctx)
        {
            results.resize(pks.size());
            size_t ret=0;
            for (size_t i=0; i<pks.size(); i++) {
                if (!ctx.get_index()->is_deleted(pks[i])) {
                    results[i].did=ctx.get_index()->get_docid(pks[i]);;
                    if (results[i].did!=INVALID_DOC) {
                        ret++;
                    }
                }
            }
            return ret;
        }
    }   // End of namespace query
}   // End of namespace argos
