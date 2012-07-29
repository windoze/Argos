//
//  enquire.h
//  Argos
//
//  Created by Windoze on 12-7-6.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>
#include <utility>
#include <vector>
#include <memory>
#include <boost/unordered_map.hpp>
#include "common/mem_pool_allocator.h"
#include "common/argos_consts.h"
#include "query/argos_query.h"

#ifndef Argos_enquire_h
#define Argos_enquire_h

namespace argos {
    namespace query {
        // Allocator for common::Value
        typedef common::mem_pool_tl_allocator<common::Value> value_allocator_t;
        // histo_key can be composited
        typedef std::vector<common::Value, value_allocator_t> group_key_t;
        // Allocator for group_key_t
        typedef common::mem_pool_tl_allocator<group_key_t> group_key_allocator_t;
        typedef std::vector<group_key_t, group_key_allocator_t> group_keys_t;
        // histo_key comparator
        inline bool operator==(const group_key_t &a, const group_key_t &b)
        {
            if (a.size()!=b.size()) {
                return false;
            }
            for (size_t i=0; i<a.size(); i++) {
                if (!bool(a[i]==b[i])) {
                    return false;
                }
            }
            return true;
        }
        
        inline void evaluate(docid did, const common::field_list_t &fl, group_key_t &vl, common::ExecutionContext &ctx) {
            vl.resize(fl.size(), common::Value());
            for (size_t i=0; i<fl.size(); i++) {
                vl[i]=fl[i]->evaluate(did, ctx);
            }
        }
        
        inline void evaluate(docid did, const std::vector<common::field_list_t> &histo, group_keys_t &vl, common::ExecutionContext &ctx) {
            vl.resize(histo.size());
            for (size_t i=0; i<histo.size(); i++) {
                evaluate(did, histo[i], vl[i], ctx);
            }
        }
        
        struct group_key_equal {
            inline bool operator()(const group_key_t &a, const group_key_t &b) const
            {
                return operator==(a, b);
            }
        };
        
        struct group_key_hash : public std::unary_function<group_key_t, size_t> {
            size_t operator()(const group_key_t &k) const
            {
                size_t ret=0;
                for (group_key_t::const_iterator i=k.begin(); i!=k.end(); ++i) {
                    ret = ret ^ common::hash_function(*i);
                }
                return ret;
            }
        };
        
        typedef common::mem_pool_tl_allocator<std::pair<const group_key_t, size_t> > histogram_value_allocator_t;
        //typedef std::allocator<std::pair<const group_key_t, size_t> > histogram_value_allocator_t;
        // Histogram is a map from group_key_t to a doc count
        typedef boost::unordered_map<group_key_t, size_t, group_key_hash, group_key_equal, histogram_value_allocator_t> histogram_t;
        // Allocator for histogram
        typedef common::mem_pool_tl_allocator<histogram_t> histogram_allocator_t;
        // We can do multiple histogram in one query
        typedef std::vector<histogram_t, histogram_allocator_t> histograms_t;
        
        // TODO: Group is a map from group_key to common::OFFSET, which points to a simple_doc_vector_t in mem_pool
        // typedef boost::unordered_map<group_key_t, common::OFFSET, boost::hash<group_key_t>, std::equal_to<group_key_t>, group_key_allocator_t> group_t;
        // Allocator for group
        // typedef common::mem_pool_tl_allocator<group_t> group_allocator_t;
        // We can do multiple grouping in one query
        // typedef std::vector<group_t, group_allocator_t> groups_t;
        
        /**
         * struct doc_value_pair_t contains docid, sortkey, and match_info for each matched doc
         *
         * This struct is used by Enquire to sort result set
         */
        struct doc_value_pair_t {
            /**
             * Default constructor
             */
            doc_value_pair_t()
            : did(UNUSED_DOC)
            , match_info(common::mem_pool_tl_allocator<common::Value>())
            {}
            
            /**
             * Constructor
             *
             * @param d docid
             * @param v value of sortkey for the doc
             */
            doc_value_pair_t(docid d, common::Value v)
            : did(d)
            , value(v)
            {}
            
            /**
             * docid
             */
            docid did;
            /**
             * value of sortkey for the doc
             */
            common::Value value;
            /**
             * value of sortkey for the doc
             */
            common::value_list_t sort_keys;
            /**
             * value of histogram key for the doc
             */
            group_keys_t histo_keys;
            /**
             * value of group key for the doc
             */
            group_keys_t group_keys;
            /**
             * match_info contains WDF for all matched term
             */
            common::match_info_t match_info;
        };
        
        /**
         * Comparator to sort result set
         */
        inline bool operator<(const doc_value_pair_t &l, const doc_value_pair_t &r)
        {
            return l.value< r.value;
        }

        struct sort_comprarator {
            inline sort_comprarator(const sort_criteria &c)
            : sort_orders_(c.size())
            {
                for (size_t i=0; i<c.size(); i++) {
                    sort_orders_[i]=c[i].desc;
                }
            }
            
            inline bool less(size_t i, const doc_value_pair_t &a, const doc_value_pair_t &b) const {
                bool ret;
                if (sort_orders_[i]) {
                    ret=bool(a.sort_keys[i]<b.sort_keys[i]);
                } else {
                    ret=bool(b.sort_keys[i]<a.sort_keys[i]);
                }
                if (!ret) {
                    return false;
                }
                return true;
            }
            
            inline bool operator()(const doc_value_pair_t &a, const doc_value_pair_t &b) const {
                for (size_t i=0; i<sort_orders_.size(); i++) {
                    if(!less(i, a, b))
                    {
                        if (!less(i, b, a)) {
                            // a[i]==b[i], compare next value
                            continue;
                        } else {
                            // a[i]>b[i], return false
                            return false;
                        }
                    } else {
                        // a[i]<b[i], return true
                        return true;
                    }
                }
                return true;
            }
            
            std::vector<bool> sort_orders_;
        };
        
        /**
         * We save result set in mem_pool, this is the allocator for result vector
         */
        typedef common::mem_pool_tl_allocator<doc_value_pair_t> dvp_allocator_t;

        /**
         * Result vector
         */
        typedef std::vector<doc_value_pair_t, dvp_allocator_t> results_t;
        
        /**
         * class Enquire executes query on index
         */
        class Enquire {
        public:
            /**
             * Execute query, return a doc_iterator
             *
             * Returned iterator contains *all* matched doc, regardless deletion-mark and filter
             *
             * @param q the query to be executed
             * @param ctx the context needed during execution
             * @return doc_iterator of all valid docs 
             */
            doc_iterator execute(const Query &q, common::ExecutionContext &ctx);

            /**
             * Execute query, generate result set, returns total hits
             *
             * Size of the result set and sortkey are specified in query
             *
             * @param q the query to be executed
             * @param resutls contain returned docid/sortkey/match_info
             * @param ctx the context needed during execution
             * @return size of total hits
             */
            size_t execute(const Query &q, results_t &results, histograms_t &histo, common::ExecutionContext &ctx);
            
            size_t execute(const pk_list_t &pks, results_t &results, common::ExecutionContext &ctx);
        };
    }   // End of namespace query
}   // End of namespace enquire

namespace std {
    /**
     * Specialized function template std::swap for doc_value_pair_t
     */
    template<>
    inline void swap<argos::query::doc_value_pair_t>(argos::query::doc_value_pair_t &a, argos::query::doc_value_pair_t &b)
    {
        std::swap(a.did, b.did);
        std::swap(a.value, b.value);
        std::swap(a.match_info, b.match_info);
        std::swap(a.sort_keys, b.sort_keys);
        std::swap(a.histo_keys, b.histo_keys);
        std::swap(a.group_keys, b.group_keys);
    }
}   // End of namespace std

#endif
