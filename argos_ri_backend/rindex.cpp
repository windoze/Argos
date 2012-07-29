//
//  rindex.cpp
//  Argos
//
//  Created by Windoze on 12-7-10.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "rindex.h"

namespace argos {
    namespace index {
        namespace detail {
            const size_t INITIAL_MAP_SIZE=5000000;
            ArgosReverseIndex::ArgosReverseIndex(const char *name)
            : term_map_(INITIAL_MAP_SIZE, true, 1, name)
            , doc_vector_pool_(name, ".data")
            {}
            
            void ArgosReverseIndex::flush()
            {
                doc_vector_pool_.save();
                term_map_.serialize();
            }
            
            doc_vector_t ArgosReverseIndex::term_doc_vector(const char *term)
            {
                common::OFFSET off;
                if (term_map_.find(term, off)) {
                    return doc_vector_t(&doc_vector_pool_, off);
                }
                // Term doesn't exist, create new doc vector
                return add_term(term);
            }
            
            bool ArgosReverseIndex::has_term(const char *term) const {
                common::OFFSET off;
                return term_map_.find(term, off);
            }

            bool ArgosReverseIndex::add_doc(docid did, const common::term_list_t &terms)
            {
                for (common::term_list_t::const_iterator i=terms.begin(); i!=terms.end(); ++i) {
                    term_doc_vector(i->first.c_str()).push_back(common::doc_freq_pair(did,i->second));
                }
                return true;
            }
            
            doc_vector_t ArgosReverseIndex::add_term(const char *term)
            {
                // Actually add
                doc_vector_t ret(&doc_vector_pool_);
                common::OFFSET off=ret.get_offset();
                term_map_.insert(term, off);
                return ret;
            }
        }   // End of namespace detail

        ReverseIndex *create_reverse_index(const char *name)
        {
            return new detail::ArgosReverseIndex(name);
        }
    }   // End of namespace index
}   // End of namespace argos
