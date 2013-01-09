//
//  doc_iterator.h
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <vector>
#include <boost/shared_ptr.hpp>
#include "common/argos_consts.h"
#include "common/heaper.h"
#include "common/execution_context.h"

#ifndef Argos_doc_iterator_h
#define Argos_doc_iterator_h

namespace argos {
    namespace query {
        const int OP_AND=1;
        const int OP_OR=2;
        const int OP_IGNORED=3;
        // TODO:
        // const int OP_PHRASE=2;
        // const int OP_NEAR=2;
        
        namespace detail {
            /**
             * Abstract base class of all doc iterator implementation
             *
             * A doc_iterator_impl must return doci in ascending order, one same id may *not* be returned twice
             */
            class doc_iterator_impl {
            public:
                /**
                 * Virtual destrutor
                 */
                virtual ~doc_iterator_impl(){}            

                /**
                 * Return true if the iteration is ended
                 */
                virtual bool end() const=0;

                /**
                 * Move iterator to the next valid doc, set the end stat if there is no more
                 *
                 * NOTE: This method may be called even if iteration ends
                 */
                virtual void next()=0;

                /**
                 * Move iterator to a valid doc with docid not less than did, set the end stat if there is no more
                 *
                 * NOTE: This method may be called even if iteration ends
                 */
                virtual docid skip_to(docid did)=0;

                /**
                 * Return current valid docid, otherwise INVALID_DOC
                 *
                 * NOTE:
                 * 1. The method must return same value if the iterator has not been moved
                 * 2. The method may be called even if the iteration ends, it must return INVALID_DOC under the situation
                 * 3. The method must be consist with end(), that is, if end() returns true, this method must return INVALID_DOC; and vice versa
                 */
                virtual docid get_docid() const=0;
                
                /**
                 * Return true if we don't care about if there is any match
                 *
                 * NOTE: end(), get_docid(), and estimate_size() will be never called if this function returns true
                 * next(), skip_to() will still be called, which makes this query has a chance to record what happened with the docid
                 * Also, save_match_info() will be called to save info if input docid is valid for this query
                 */
                virtual bool ignored() const { return false; }
                
                /**
                 * Return an estimation of result doc set size
                 *
                 * The return value needs not to be accurate, but an accurate value may help for optimization
                 */
                virtual size_t estimate_size() const=0;
                
                /**
                 * Save match info
                 */
                virtual void save_match_info(docid did, common::match_info_t &match_info)=0;
            };
            
            /**
             * A doc_iterator_impl implementation, it returns all docid, include max_doc
             */
            class match_all_doc_iterator_impl : public doc_iterator_impl {
            public:
                match_all_doc_iterator_impl(docid max_doc)
                : current_(1)
                , max_doc_(max_doc+1)
                {}
                
                virtual bool end() const { return current_==max_doc_; }
                virtual void next() { current_++; }
                virtual docid skip_to(docid did) { current_=did; return did; }
                virtual docid get_docid() const { return current_; }
                virtual size_t estimate_size() const { return size_t(-1); }
                virtual void save_match_info(docid did, common::match_info_t &match_info) {}
            private:
                docid current_;
                docid max_doc_;
            };

            /**
             * A doc_iterator_impl implementation, it always ends and returns no docid
             */
            class match_none_doc_iterator_impl : public doc_iterator_impl {
            public:
                match_none_doc_iterator_impl() {}
                virtual bool end() const { return true; }
                virtual void next() {}
                virtual docid skip_to(docid did) { return INVALID_DOC; }
                virtual docid get_docid() const { return INVALID_DOC; }
                virtual size_t estimate_size() const { return 0; }
                virtual void save_match_info(docid did, common::match_info_t &match_info){};
            };
        }   // End of namespace detail
        
        typedef boost::shared_ptr<detail::doc_iterator_impl> doc_iterator_impl_ptr_t;
        
        extern const doc_iterator_impl_ptr_t match_none_ptr;

        namespace detail {
            /**
             * This doc_iterator_imple will always ignored when validating docid
             */
            class ignored_doc_iterator_impl : public doc_iterator_impl {
            public:
                ignored_doc_iterator_impl(){}
                
                template<typename Iterator_T>
                ignored_doc_iterator_impl(Iterator_T begin, Iterator_T end)
                : children_(begin, end)
                {}
                
                template<typename Container_T>
                ignored_doc_iterator_impl(const Container_T &c)
                : children_(c.begin(), c.end())
                {}
                
                virtual bool ignored() const { return true; }
                // Never called
                virtual bool end() const { return true; }
                virtual void next() {
                    for (children_t::iterator i=children_.begin(); i!=children_.end(); ++i) {
                        (*i)->next();
                    }
                }
                virtual docid skip_to(docid did) {
                    for (children_t::iterator i=children_.begin(); i!=children_.end(); ++i) {
                        (*i)->skip_to(did);
                    }
                    // Never checked
                    return INVALID_DOC;
                }
                // Never called
                virtual docid get_docid() const { return INVALID_DOC; }
                // Never called
                virtual size_t estimate_size() const { return 0; }
                virtual void save_match_info(docid did, common::match_info_t &match_info){
                    for (children_t::iterator i=children_.begin(); i!=children_.end(); ++i) {
                        (*i)->save_match_info(did, match_info);
                    }
                }
            private:
                typedef std::vector<doc_iterator_impl_ptr_t> children_t;
                children_t children_;
            };
            
            /**
             * and_doc_iterator_impl returns a docid only if all sub iterators have it
             */
            class and_doc_iterator_impl : public doc_iterator_impl
            {
            public:
                and_doc_iterator_impl()
                : current_(INVALID_DOC)
                {}
                
                template<typename Iterator_T>
                and_doc_iterator_impl(Iterator_T begin, Iterator_T end)
                : current_(INVALID_DOC)
                {
                    for (Iterator_T i=begin; i!=end; ++i) {
                        if ((*i)->ignored()) {
                            ignored_children_.push_back(*i);
                        } else {
                            children_.push_back(*i);
                        }
                    }
                    if(children_.size()==0) {
                        // TODO: Should we add a match_all here?
                        throw argos_syntax_error("Syntax error in match");
                    }
                    current_=first_valid();
                }
                
                template<typename Container_T>
                and_doc_iterator_impl(const Container_T &c)
                : current_(INVALID_DOC)
                {
                    for (typename Container_T::const_iterator i=c.begin(); i!=c.end(); ++i) {
                        if ((*i)->ignored()) {
                            ignored_children_.push_back(*i);
                        } else {
                            children_.push_back(*i);
                        }
                    }
                    if(children_.size()==0) {
                        // TODO: Should we add a match_all here?
                        throw argos_syntax_error("Syntax error in match");
                    }
                   current_=first_valid();
                }
                
                virtual bool end() const
                {
                    return !is_valid(current_);
                }
                
                virtual void next()
                {
                    children_[0]->next();
                    current_=first_valid();
                }
                
                virtual docid skip_to(docid did) {
                    for (children_t::iterator i=ignored_children_.begin(); i!=ignored_children_.end(); ++i) {
                        (*i)->skip_to(did);
                    }
                    children_[0]->skip_to(did);
                    return current_=first_valid();
                }
                
                virtual docid get_docid() const { return current_; }
                
                virtual size_t estimate_size() const
                {
                    // Return smallest size of all children
                    size_t ret=size_t(-1);
                    for (children_t::const_iterator i=children_.begin(); i!=children_.end(); ++i) {
                        ret=std::min((*i)->estimate_size(), ret);
                    }
                    return ret;
                }

                virtual void save_match_info(docid did, common::match_info_t &match_info)
                {
                    if (did != get_docid()) {
                        return;
                    }
                    for (children_t::iterator i=children_.begin(); i!=children_.end(); ++i) {
                        (*i)->save_match_info(did, match_info);
                    }
                    for (children_t::iterator i=ignored_children_.begin(); i!=ignored_children_.end(); ++i) {
                        (*i)->save_match_info(did, match_info);
                    }
                }
                
                docid first_valid()
                {
                    bool found=false;
                    docid last_doc=children_[0]->get_docid();
                    while (!found) {
                        found=true;
                        for (children_t::iterator i=children_.begin(); i!=children_.end(); ++i) {
                            if ((*i)->end()) {
                                // Ends if any child ends
                                return INVALID_DOC;
                            }
                            docid did=(*i)->skip_to(last_doc);
                            if (did!=last_doc) {
                                found=false;
                                last_doc=did;
                                break;
                            }
                        }
                    }
                    for (children_t::iterator i=ignored_children_.begin(); i!=ignored_children_.end(); ++i) {
                        (*i)->skip_to(last_doc);
                    }
                    return last_doc;
                }
                
                typedef std::vector<doc_iterator_impl_ptr_t> children_t;
                children_t children_;
                children_t ignored_children_;
                docid current_;
            };
            
            /**
             * and_doc_iterator_impl returns a docid of all sub iterators
             */
            class or_doc_iterator_impl : public doc_iterator_impl
            {
            public:
                struct di_less {
                    bool operator()(const doc_iterator_impl_ptr_t &l, const doc_iterator_impl_ptr_t &r) const {
                        return l->get_docid() < r->get_docid();
                    }
                };
                
                or_doc_iterator_impl()
                : current_(INVALID_DOC)
                {}
                
                template<typename Iterator_T>
                or_doc_iterator_impl(Iterator_T begin, Iterator_T end)
                : current_(INVALID_DOC)
                {
                    children_.resize(end-begin);
                    if (children_.size()>0) {
                        children_heap_=common::heaper<children_t::iterator, di_less>(children_.begin(), children_.end(), true);
                        for (Iterator_T i=begin; i!=end; ++i) {
                            if ((*i)->ignored()) {
                                ignored_children_.push_back(*i);
                            } else {
                                children_heap_.push(*i);
                            }
                        }
                    }
                    if(children_heap_.size()==0) {
                        // TODO: Should we add a match_all here?
                        throw argos_syntax_error("Syntax error in match");
                    }
                    current_=children_heap_.top()->get_docid();
                }
                
                template<typename Container_T>
                or_doc_iterator_impl(const Container_T &c)
                : current_(INVALID_DOC)
                {
                    children_.resize(c.size());
                    if (children_.size()>0) {
                        children_heap_=common::heaper<children_t::iterator, di_less>(children_.begin(), children_.end(), true);
                        for (typename Container_T::const_iterator i=c.begin(); i!=c.end(); ++i) {
                            if ((*i)->ignored()) {
                                ignored_children_.push_back(*i);
                            } else {
                                children_heap_.push(*i);
                            }
                        }
                    }
                    if(children_heap_.size()==0) {
                        // TODO: Should we add a match_all here?
                        throw argos_syntax_error("Syntax error in match");
                    }
                    current_=children_heap_.top()->get_docid();
                }
                
                virtual bool end() const
                {
                    if (children_.size()==0) {
                        return true;
                    }
                    return !is_valid(current_);
                }
                
                virtual void next()
                {
                    current_=skip_to(current_+1);
                }
                
                virtual docid skip_to(docid did) {
                    if (end()) {
                        return INVALID_DOC;
                    }
                    for (children_t::iterator i=ignored_children_.begin(); i!=ignored_children_.end(); ++i) {
                        (*i)->skip_to(did);
                    }
                    while (children_heap_.top()->get_docid()<did) {
                        children_heap_.top()->skip_to(did);
                        children_heap_.heapify();
                    }
                    current_=children_heap_.top()->get_docid();
                    return current_;
                }
                
                virtual docid get_docid() const {
                    if (end()) {
                        return INVALID_DOC;
                    }
                    return current_;
                }
                
                virtual size_t estimate_size() const
                {
                    // Return sum of all children sizes
                    size_t ret=0;
                    for (size_t n=0; n<children_heap_.size(); n++) {
                        ret+=children_[n]->estimate_size();
                    }
                    return ret;
                }
                
                virtual void save_match_info(docid did, common::match_info_t &match_info)
                {
                    if (did != get_docid()) {
                        return;
                    }
                    for (size_t n=0; n<children_heap_.size(); n++) {
                        children_[n]->save_match_info(did, match_info);
                    }
                    for (children_t::iterator i=ignored_children_.begin(); i!=ignored_children_.end(); ++i) {
                        (*i)->save_match_info(did, match_info);
                    }
                }
                
                typedef std::vector<doc_iterator_impl_ptr_t> children_t;
                typedef common::heaper<children_t::iterator, di_less> children_heap_t;
                children_t children_;
                children_heap_t children_heap_;
                children_t ignored_children_;
                docid current_;
            };
            
            /**
             * A wrapper class of and_doc_iterator_impl and or_doc_iterator_impl
             */
            template<typename Iterator_T>
            doc_iterator_impl_ptr_t multi_doc_iterator_impl(int op, Iterator_T begin, Iterator_T end)
            {
                std::vector<doc_iterator_impl_ptr_t> sub;
                for (Iterator_T i=begin; i!=end; ++i) {
                    sub.push_back(*i);
                }
                
                switch (op) {
                    case OP_AND:
                        return doc_iterator_impl_ptr_t(new detail::and_doc_iterator_impl(sub));
                        break;
                    case OP_OR:
                        return doc_iterator_impl_ptr_t(new detail::or_doc_iterator_impl(sub));
                        break;
                    case OP_IGNORED:
                        return doc_iterator_impl_ptr_t(new detail::ignored_doc_iterator_impl(sub));
                        break;
                }
                return doc_iterator_impl_ptr_t();
            }
            
            inline doc_iterator_impl_ptr_t multi_doc_iterator_impl(int op)
            {
                std::vector<doc_iterator_impl_ptr_t> sub;
                switch (op) {
                    case OP_AND:
                    case OP_OR:
                        return match_none_ptr;
                    case OP_IGNORED:
                        return doc_iterator_impl_ptr_t(new detail::ignored_doc_iterator_impl(sub));
                        break;
                }
                return doc_iterator_impl_ptr_t();
            }
            
            template<typename Container_T>
            doc_iterator_impl_ptr_t multi_doc_iterator_impl(int op, const Container_T &c)
            {
                return multi_doc_iterator_impl(op, c.begin(), c.end());
            }
        }
        
        /**
         * A facade class of doc_iterator_impl
         */
        class doc_iterator {
        public:
            doc_iterator()
            {}
            
            doc_iterator(doc_iterator_impl_ptr_t impl)
            : impl_(impl)
            {}
            
            template<typename Iterator_T>
            doc_iterator(int op, Iterator_T begin, Iterator_T end)
            {
                std::vector<doc_iterator_impl_ptr_t> sub;
                for (Iterator_T i=begin; i!=end; ++i) {
                    sub.push_back(i->impl_);
                }

                switch (op) {
                    case OP_AND:
                        impl_=doc_iterator_impl_ptr_t(new detail::and_doc_iterator_impl(sub));
                        break;
                    case OP_OR:
                        impl_=doc_iterator_impl_ptr_t(new detail::or_doc_iterator_impl(sub));
                        break;
                    default:
                        break;
                }
            }
            
            doc_iterator(int op, const std::vector<doc_iterator> &children)
            {
                std::vector<doc_iterator_impl_ptr_t> sub;
                for (std::vector<doc_iterator>::const_iterator i=children.begin(); i!=children.end(); ++i) {
                    sub.push_back(i->impl_);
                }
                
                switch (op) {
                    case OP_AND:
                        impl_=doc_iterator_impl_ptr_t(new detail::and_doc_iterator_impl(sub));
                        break;
                    case OP_OR:
                        impl_=doc_iterator_impl_ptr_t(new detail::or_doc_iterator_impl(sub));
                        break;
                    default:
                        break;
                }
            }
            
            inline docid operator*() const {
                return impl_ ? impl_->get_docid() : INVALID_DOC;
            }
            
            inline doc_iterator &operator++() {
                if(impl_) impl_->next();
                return *this;
            }
            
            inline void operator++(int) {
                if(impl_) impl_->next();
            }
            
            inline operator bool() const {
                return impl_ && !(impl_->end());
            }
            
            virtual void save_match_info(docid did, common::match_info_t &match_info)
            {
                if ((!impl_) || (did==INVALID_DOC) || (did==UNUSED_DOC) ||(did != operator*())) {
                    return;
                }
                impl_->save_match_info(did, match_info);
            }
            
            doc_iterator_impl_ptr_t impl_;
        };
    }   // End of namespace query
}   // End of namespace argos

#endif
