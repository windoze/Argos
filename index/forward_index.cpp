//
//  forward_index.cpp
//  Argos
//
//  Created by Windoze on 12-6-28.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "index/forward_index.h"

namespace argos {
    namespace index {
        const size_t FI_INITIAL_MAP_SIZE=5000000;
        using namespace common;
        ForwardIndex::ForwardIndex(const char *name, const common::FieldConfig *fc)
        : field_config_(fc)
        , storage_(name, fc)
        , idmap_(FI_INITIAL_MAP_SIZE, name)
        {}
        
        ForwardIndex::~ForwardIndex()
        {}
        
        void ForwardIndex::flush()
        {
            storage_.flush();
            idmap_.serialize();
        }
        
        docid ForwardIndex::add_document(const value_list_t &vl)
        {
            if (vl.size()!=field_config_->count()) {
                return INVALID_DOC;
            }
            primary_key pk=(primary_key)(vl[0].cast(VT_INTEGER).number);
            docid old_did=get_docid(pk);
            docid did=new_document();
            if (!storage_.is_valid(did)) {
                // New doc failed
                return did;
            }
            bool set=set_document(did, vl);
            if (set && storage_.is_valid(old_did)) {
                // Doc exists, delete old one
                delete_document(old_did);
            }
            return did;
        }
        
        bool ForwardIndex::set_document(docid did, const value_list_t &vl)
        {
            if (vl.size()!=field_config_->count()) {
                return false;
            }
            for (int i=0; i<vl.size(); i++) {
                if(!storage_.set_field_value(i, did, vl[i]))
                    return false;
            }
            set_primary_key(did, (primary_key)(vl[0].cast(VT_INTEGER).number));
            return true;
        }
        
        size_t ForwardIndex::get_field_length(docid did, int fid) const
        {
            return storage_.get_field_len(fid, did);
        }
        
    }   // End of namespace index
}   // End of namespace argos
