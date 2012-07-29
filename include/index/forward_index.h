//
//  forward_index.h
//  Argos
//
//  Created by Windoze on 12-6-19.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "index/doc_storage.h"

#ifndef Argos_forward_index_h
#define Argos_forward_index_h

namespace argos {
    class FieldEvaluator;
    namespace index {
        /**
         * class ForwardIndex stores all documents and their field values
         */
        class ForwardIndex {
        public:
            /**
             * Constructor, create or load a forward index with given FieldConfig
             */
            ForwardIndex(const char *name, const common::FieldConfig *fc);
            
            /**
             * Destructor
             */
            ~ForwardIndex();
            
            /**
             * Save all pending changes into persistant storage
             */
            void flush();
            
            /**
             * Returns logical docid for the doc specified by pk
             */
            inline docid get_docid(primary_key pk) const
            {
                docid ret=INVALID_DOC;
                idmap_.find(pk, ret);
                return ret;
            }
            
            /**
             * Returns the maximum docid in use, even it's marked deleted
             */
            inline docid get_last_doc() const { return storage_.get_last_doc(); }
            
            /**
             * Create a new document, allocate new docid and storage
             */
            inline docid new_document() { return storage_.new_document(); }
                        
            /**
             * Mark document as deleted
             */
            inline void delete_document(docid did) { storage_.mark_delete(did); }

            /**
             * Return true if the doc is marked deleted or doc doesn't exist
             */
            inline bool is_deleted(docid did) const { return storage_.is_deleted(did); }

            /**
             * Return primary key for the doc
             */
            inline primary_key get_primary_key(docid did) const { return storage_.get_primary_key(did); }

            /**
             * Set primary key for the doc
             */
            inline void set_primary_key(docid did, primary_key pk) {
                storage_.set_primary_key(did, pk);
                docid old_id;
                size_t index;
                idmap_.insert_or_replace(pk, did, old_id, index);
            }

            /**
             * Return count of documents have beed deleted
             */
            inline size_t erased() const { return storage_.get_erased_count(); }
            
            /**
             * Add a document into the forward index
             */
            docid add_document(const common::value_list_t &vl);

            /**
             * Update the content of the specific document with new values
             *
             * NOTE: This can cause data inconsistance if there is any other thread reads the index
             */
            bool set_document(docid did, const common::value_list_t &vl);
            
            size_t get_field_length(docid did, int fid) const;

            /**
             * Return the underlying doc storage
             */
            inline const detail::doc_storage *get_storage() const { return &storage_; }
            inline detail::doc_storage *get_storage() { return &storage_; }

            /**
             * Return the FieldConfig the index is using
             */
            inline const common::FieldConfig *get_field_config() const { return field_config_; }
        private:
            const common::FieldConfig *field_config_;
            detail::doc_storage storage_;
            typedef common::hash_table<primary_key, docid> idmap_t;
            idmap_t idmap_;
        };
    }   // End of namespace index
}   // End of namespace argos

#endif
