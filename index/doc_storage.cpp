//
//  doc_storage.cpp
//  Argos
//
//  Created by Windoze on 12-6-26.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "index/doc_storage.h"

namespace argos {
    namespace index {
        namespace detail {
            doc_storage::~doc_storage()
            {
                for (int i=0; i<field_pool_.size(); i++) {
                    if (field_pool_[i]) {
                        delete field_pool_[i];
                    }
                    if (field_data_pool_[i]) {
                        delete field_data_pool_[i];
                    }
                    if (field_length_pool_[i]) {
                        delete field_length_pool_[i];
                    }
                }
            }
            
            void doc_storage::flush()
            {
                for (int i=0; i<field_config_->count(); i++) {
                    if (!get_field_stored(i)) {
                        continue;
                    }
                    field_pool_[i]->save();
                    if (field_data_pool_[i]) {
                        field_data_pool_[i]->save();
                    }
                    if (field_length_pool_[i]) {
                        field_length_pool_[i]->save();
                    }
                }
                data_pool_.save();
            }

            void doc_storage::init_field_pool()
            {
                char buf[PATH_MAX];
                field_pool_.resize(field_config_->count());
                field_data_pool_.resize(field_config_->count());
                field_length_pool_.resize(field_config_->count());
                for (int i=0; i<field_config_->count(); i++) {
                    buf[0]=0;
                    if (name_[0]) {
                        sprintf(buf, "%s.%i", name_, i);
                    }
                    if (get_field_config()->get_field_def(i)->indexed()) {
                        field_length_pool_[i]=new common::mem_pool(buf, ".len");
                    } else {
                        field_length_pool_[i]=NULL;
                    }
                    if (get_field_stored(i)) {
                        field_pool_[i]=new common::mem_pool(buf);
                        if (get_field_info(i).has_data()) {
                            field_data_pool_[i]=new common::mem_pool(buf, ".data");
                        } else {
                            field_data_pool_[i]=NULL;
                        }
                    }
                }
            }

            int doc_storage::get_field_id(const char *name, size_t sz) const
            {
                char buf[256];
                if (sz>255) {
                    sz=255;
                }
                memcpy(buf, name, sz);
                buf[sz]=0;
                return get_field_id(buf);
            }
            
            bool doc_storage::set_field_value(int fid, docid did, common::Value v)
            {
                if (!is_valid(did)) {
                    return false;
                }
                if (!get_field_pool(fid)) {
                    if (!get_field_stored(fid)) {
                        // TODO: Log event
                        return true;
                    }
                    return false;
                }
                common::FieldInfo fi=get_field_info(fid);
                void *p=get_field_pool(fid)->get_addr(fi.size()*did);
                if ((fi.type_ & common::FT_MULTI) == common::FT_MULTI) {
                    if (v.type_==common::VT_ARRAY) {
                        if (!get_field_data_pool(fid)) {
                            return false;
                        }
                        common::OFFSET off=make_array(get_field_data_pool(fid), fi.type_, v.array);
                        if (off==common::INVALID_OFFSET) {
                            return false;
                        }
                        *((common::OFFSET *)p)=off;
                        return true;
                    }
                    return false;
                }
                switch (fi.type_) {
                    case common::FT_INT8:
                        *((int8_t *)p)=(int8_t)v.cast(common::VT_INTEGER).number;
                        break;
                    case common::FT_INT16:
                        *((int16_t *)p)=(int16_t)v.cast(common::VT_INTEGER).number;
                        break;
                    case common::FT_INT32:
                        *((int32_t *)p)=(int32_t)v.cast(common::VT_INTEGER).number;
                        break;
                    case common::FT_INT64:
                        *((int64_t *)p)=v.cast(common::VT_INTEGER).number;
                        break;
                    case common::FT_FLOAT:
                        *((float *)p)=(float)v.cast(common::VT_DOUBLE).dnumber;
                        break;
                    case common::FT_DOUBLE:
                        *((double *)p)=v.cast(common::VT_DOUBLE).dnumber;
                        break;
                    case common::FT_GEOLOC:
                        *((common::GeoLocationValue *)p)=v.cast(common::VT_GEOLOCATION).geolocation;
                        break;
                    case common::FT_STRING:
                    {
                        if (!get_field_data_pool(fid)) {
                            return false;
                        }
                        const char *str=v.cast(common::VT_STRING).string;
                        if (str) {
                            *((common::OFFSET *)p)=get_field_data_pool(fid)->add_string(str);
                        }
                    }
                        break;
                    default:
                        break;
                }
                return true;
            }
            
            bool doc_storage::set_field_len(int fid, docid did, size_t len)
            {
                if (!field_length_pool_[fid]) {
                    return false;
                }
                common::offptr_t<primary_key> p(field_length_pool_[fid],0);
                p[did]=len;
                data_->field_total_len_[fid]+=len;
                return true;
            }
            
            size_t doc_storage::get_field_len(int fid, docid did) const
            {
                if (!field_length_pool_[fid]) {
                    return 0;
                }
                common::offptr_t<primary_key> p(field_length_pool_[fid],0);
                return p[did];
            }
            
            void doc_storage::set_checksum(primary_key cs)
            {
                common::offptr_t<primary_key> p(field_pool_[PRIMARY_KEY_FIELD_ID],0);
                p[0]=cs;
            }
            bool doc_storage::check_checksum(primary_key cs) const
            {
                common::offptr_t<primary_key> p(field_pool_[PRIMARY_KEY_FIELD_ID],0);
                return cs==p[0];
            }
        }   // End of namespace detail
    }   // End of namespace index
}   // End of namespace argos