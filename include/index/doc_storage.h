//
//  doc_storage.h
//  Argos
//
//  Created by Windoze on 12-6-18.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/value.h"
#include "common/field_config.h"

#ifndef Argos_doc_storage_h
#define Argos_doc_storage_h

namespace argos {
    namespace index {
        class FieldEvaluator;
        
        namespace detail {
            const int PRIMARY_KEY_FIELD_ID=0;

            class doc_storage {
            public:
                // Create forward index
                // TODO: calc an appropriate segment size for mem pool
                doc_storage(const char *name, const common::FieldConfig *fc)
                : field_config_(fc)
                , data_pool_(sizeof(data_t)+fc->count()*sizeof(size_t), name, ".fields")
                , data_(0)
                , delete_marks_(name, ".deleted")
                , dmptr_(&delete_marks_, 0)
                {
                    name_[0]=0;
                    if (name && name[0]) {
                        strcpy(name_, name);
                    }
                    data_pool_.expand(sizeof(data_t)+fc->count()*sizeof(size_t));
                    data_=(data_t *)data_pool_.get_addr(0);
                    init_field_pool();
                }
                
                ~doc_storage();
                
                void flush();
                
                bool is_valid(docid did) const {
                    return argos::is_valid(did) && did<=get_last_doc();
                }
                
                inline docid new_document()
                {
                    docid new_did=get_last_doc()+1;
                    delete_marks_.expand(new_did*sizeof(bool));
                    for(int i=0; i<field_pool_.size(); i++)
                    {
                        if(field_pool_[i]) field_pool_[i]->expand(get_field_size(i)*(new_did+1));
                        if(field_length_pool_[i]) field_length_pool_[i]->expand(sizeof(size_t)*(new_did+1));
                    }
                    data_->last_doc_=new_did;
                    data_->doc_count_++;
                    return new_did;
                }
                
                inline void mark_delete(docid did)
                {
                    if (!is_valid(did)) {
                        return;
                    }
                    if(!dmptr_[did]) {
                        dmptr_[did]=true;
                        // update sum of field length
                        for (int i=0; i<field_pool_.size(); i++) {
                            data_->field_total_len_[i]-=get_field_len(i, did);
                        }
                        data_->erased_count_++;
                    }
                }
                
                inline bool is_deleted(docid did) const
                {
                    if (!is_valid(did)) {
                        return true;
                    }
                    return dmptr_[did];
                }
                
                inline size_t get_doc_count() const {
                    return data_->doc_count_;
                }
                
                inline docid get_last_doc() const {
                    return data_->last_doc_;
                }
                
                inline size_t get_erased_count() const {
                    return data_->erased_count_;
                }
                
                inline common::mem_pool *get_field_pool(const char *name) const
                {
                    int fid=get_field_id(name);
                    return get_field_pool(fid);
                }

                inline common::mem_pool *get_field_pool(const char *name, size_t sz) const
                {
                    int fid=get_field_id(name, sz);
                    return get_field_pool(fid);
                }

                inline common::mem_pool *get_field_pool(int fid) const
                {
                    return field_pool_[fid];
                }
                
                inline common::mem_pool *get_field_data_pool(const char *name) const
                {
                    int fid=get_field_id(name);
                    return get_field_data_pool(fid);
                }
                
                inline common::mem_pool *get_field_data_pool(const char *name, size_t sz) const
                {
                    int fid=get_field_id(name, sz);
                    return get_field_data_pool(fid);
                }
                
                inline common::mem_pool *get_field_data_pool(int fid) const
                {
                    return field_data_pool_[fid];
                }
                
                inline int get_field_id(const char *name) const
                {
                    return field_config_->get_field_id(name);
                }

                int get_field_id(const char *name, size_t sz) const;
                
                common::FieldInfo get_field_info(int fid) const
                {
                    return field_config_->get_field(fid);
                }
                
                size_t get_field_size(int fid) const {
                    return get_field_info(fid).size();
                }
                
                bool get_field_stored(int fid) const {
                    return get_field_info(fid).type_!=common::FT_INVALID;
                }
                
                primary_key get_primary_key(docid did) const {
                    if (!is_valid(did)) {
                        return INVALID_PK;
                    }
                    common::offptr_t<primary_key> p(field_pool_[PRIMARY_KEY_FIELD_ID],0);
                    return p[did];
                }
                
                void set_primary_key(docid did, primary_key pk) {
                    if (!is_valid(did)) {
                        return;
                    }
                    common::offptr_t<primary_key> p(field_pool_[PRIMARY_KEY_FIELD_ID],0);
                    p[did]=pk;
                }
                
                bool set_field_value(const char *field_name, docid did, common::Value v)
                {
                    return set_field_value(get_field_id(field_name), did, v);
                }
                
                bool set_field_value(const char *field_name, size_t sz, docid did, common::Value v)
                {
                    return set_field_value(get_field_id(field_name, sz), did, v);
                }
                
                bool set_field_value(int fid, docid did, common::Value v);
                
                bool set_field_len(int fid, docid did, size_t len);
                
                size_t get_field_len(int fid, docid did) const ;
                
                size_t get_field_total_len(int fid) const { return data_->field_total_len_[fid]; }
                
                const common::FieldConfig *get_field_config() const { return field_config_; }
                
                // Set a checksum in index, uses docid(0) as it's invalid id and never used
                void set_checksum(primary_key cs);
                // Check checksum against index, this makes sure we're using the correct field config
                bool check_checksum(primary_key cs) const;
            private:
                void init_field_pool();

                // Data need to be serialized
                struct data_t {
                    docid last_doc_;
                    size_t doc_count_;
                    size_t erased_count_;
                    size_t field_total_len_[1];
                };
                char name_[PATH_MAX];
                const common::FieldConfig *field_config_;
                common::mem_pool data_pool_;
                data_t *data_;
                typedef common::offptr_t<bool> dmptr_t;
                common::mem_pool delete_marks_;
                dmptr_t dmptr_;
                std::vector<common::mem_pool *> field_pool_;
                std::vector<common::mem_pool *> field_data_pool_;
                std::vector<common::mem_pool *> field_length_pool_;
                
                friend class FieldEvaluator;
            };
        }   // End of namespace detail

        
        class FieldWriter {
            // TODO:
        };
    }   // End of namespace index
}   // End of namespace argos

#endif
