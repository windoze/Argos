//
//  rindex.h
//  Argos
//
//  Created by Windoze on 12-7-10.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/block_chain.h"
#include "common/hash_table.h"
#include "index/reverse_index.h"

#ifndef Argos_rindex_h
#define Argos_rindex_h

namespace argos {
    namespace index {
        namespace detail {
            typedef common::doc_freq_vector doc_vector_t;
            
            class ArgosReverseIndex : public ReverseIndex{
            public:
                ArgosReverseIndex(const char *name);
                virtual void flush();
                doc_vector_t term_doc_vector(const char *term);
                bool has_term(const char *term) const;
                
            protected:
                virtual bool add_doc(docid did, const common::term_list_t &terms);
                doc_vector_t add_term(const char *term);
            private:
                char name_[PATH_MAX];
                common::hash_table<const char *, common::OFFSET> term_map_;
                common::mem_pool doc_vector_pool_;
            };
        }   // End of namespace detail
    }
}

#endif
