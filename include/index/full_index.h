//
//  full_index.h
//  Argos
//
//  Created by Windoze on 12-7-2.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <iostream>
#include <boost/shared_ptr.hpp>
#include "common/value.h"
#include "common/execution_context.h"
#include "common/evaluatable.h"

#ifndef Argos_full_index_h
#define Argos_full_index_h

namespace argos {
    namespace common {
        class FieldConfig;
    }
    namespace index {
        const int INDEX_CREATE_OR_OPEN=0;
        const int INDEX_CREATE=1;
        const int INDEX_OPEN=2;
        
        class ForwardIndex;
        class ReverseIndex;
        
        /**
         * class Index is a facade class for ForwardIndex and ReverseIndex
         *
         * It calles internal forward and reverse indices to do actual work
         */
        class Index {
        public:
            /**
             * Constructor, load an index
             */
            Index(const char *path);

            /**
             * Constructor, create or load an index
             */
            Index(const char *conf_file, const char *path, int open_flag);

            /**
             * Constructor, create or load an index
             */
            Index(std::istream &is, const char *path, int open_flag);

            /**
             * Destructor
             */
            ~Index();
            
            /**
             * Save all pending changes into persistant storage
             */
            void flush();
            
            /**
             * Return true if the index is opened
             */
            inline bool is_open() const { return opened_; }
            
            /**
             * Return index name
             */
            inline const char *get_name() const {
                return name_;
            }

            /**
             * Return count of documents in the index
             */
            size_t get_doc_count() const;

            /**
             * Return primary key for doc
             */
            primary_key get_primary_key(docid did) const;

            /**
             * Return docid for doc
             */
            docid get_docid(primary_key pk) const;
            
            /**
             * Add a document into index
             */
            docid add_document(const common::value_list_t &vl, common::ExecutionContext &ctx);

            /**
             * Mark document as deleted
             */
            bool delete_document(primary_key pk);

            /**
             * Return true if document is marked deleted or doesn't exist
             */
            bool is_deleted(docid did) const;

            /**
             * Return true if document is marked deleted or doesn't exist
             */
            bool is_deleted(primary_key pk) const;
            
            /**
             * Return value of the field of the document
             */
            common::Value get_doc_field(primary_key pk, int fid) const;

            /**
             * Return value of the field of the document
             */
            common::Value get_doc_field(docid did, int fid) const;
            
            /**
             * Set value of the field of the document
             */
            bool set_doc_field(primary_key pk, int fid, common::Value v);
            
            /**
             * Set value of the field of the document
             */
            bool set_doc_field(docid did, int fid, common::Value v);
            
            /**
             * Return a value list for the document
             */
            bool get_value_list(primary_key pk, const common::field_list_t &fl, common::value_list_t &vl, common::ExecutionContext &ctx) const;

            /**
             * Return a value list for the document
             */
            bool get_value_list(docid did, const common::field_list_t &fl, common::value_list_t &vl, common::ExecutionContext &ctx) const;
            
            /**
             * Return the FieldConfig
             */
            const common::FieldConfig *get_field_config() const { return fc_; }
            
            /**
             * Return the forward index
             */
            index::ForwardIndex *get_forward_index() const { return fi_; }
            
            /**
             * Return the reverse index
             */
            index::ReverseIndex *get_reverse_index() const { return ri_; }
            
            /**
             * Create a new ExecutionContext with all necessary info set
             */
            common::ExecutionContext *create_context();

        private:
            bool init(std::istream &is, const char *path, int open_flag);
            
            char name_[PATH_MAX];
            bool opened_;
            common::FieldConfig *fc_;
            index::ReverseIndex *ri_;
            index::ForwardIndex *fi_;
            size_t auto_commit_count_;
        };

        typedef boost::shared_ptr<Index> index_ptr_t;
    }   // End of namespace index
}   // End of namespace argos

#endif
