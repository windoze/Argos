//
//  reverse_index.h
//  Argos
//
//  Created by Windoze on 12-6-30.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <string>
#include <vector>
#include <map>
#include "common/value.h"
#include "common/execution_context.h"
#include "common/term_list.h"

#ifndef Argos_reversed_index_h
#define Argos_reversed_index_h

namespace argos {
    namespace index {
        /**
         * class ReverseIndex is the abstract base of reverse index implementation
         */
        class ReverseIndex {
        public:
            virtual ~ReverseIndex(){}

            /**
             * Save all pending changes to persistant storage
             */
            virtual void flush()=0;
            
            /**
             * Add a document into reverse index
             *
             * @param did docid of the document, caller *must* it's greater than all existing docid in the reverse index
             * @param vl is all fields of the document
             * @param context contains all needed info
             * @return true if document is successfully added
             */
            bool add_document(docid did, const common::value_list_t &vl, common::ExecutionContext &context);
        protected:
            /**
             * add_doc is called by add_document, with processed term list
             */
            virtual bool add_doc(docid did, const common::term_list_t &terms)=0;
        };

        /**
         * Create a reverse index instance
         */
        ReverseIndex *create_reverse_index(const char *name);
    }   // End of namespace index
}   // End of namespace argos

#endif
