//
//  field_evaluator.cpp
//  Argos
//
//  Created by Windoze on 12-6-26.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/argos_exception.h"
#include "index/field_evaluator.h"
#include "index/full_index.h"

namespace argos {
    namespace index {
        
        int get_fid(const common::FieldConfig *fc, const char *name)
        {
            if (name[0]=='A' && name[1]=='T' && name[2]=='T' && name[3]=='R') {
                const char *p=name+4;
                while (*p) {
                    if (*p<'0' || *p>'9') {
                        return fc->get_field_id(name);
                    }
                    p++;
                }
                return ::atoi(name+4);
            }
            return fc->get_field_id(name);
        }
        
        int get_fid(const common::FieldConfig *fc, const char *name, size_t sz)
        {
            char buf[256];
            if (sz>255) {
                sz=255;
            }
            memcpy(buf, name, sz);
            buf[sz]=0;
            return get_fid(fc, buf);
        }
        
        FieldEvaluator::FieldEvaluator(const common::FieldConfig *fc, const char *name)
        : field_config_(fc)
        , fid_(get_fid(fc, name))
        , fi_(fc->get_field(fid_))
        , cached_docid_(0)
        , cached_value_()
        {
            if (fid_<0) {
                throw argos_bad_field(name);
            }
            if (!fc->get_field_def(fid_)->stored()) {
                throw argos_bad_field(fc->get_field_def(fid_)->get_name());
            }
        }
        
        FieldEvaluator::FieldEvaluator(const common::FieldConfig *fc, const char *name, size_t sz)
        : field_config_(fc)
        , fid_(get_fid(fc, name, sz))
        , fi_(fc->get_field(fid_))
        , cached_docid_(0)
        , cached_value_()
        {
            if (fid_<0) {
                throw argos_bad_field(std::string(name, sz).c_str());
            }
            if (!fc->get_field_def(fid_)->stored()) {
                throw argos_bad_field(fc->get_field_def(fid_)->get_name());
            }
        }
        
        FieldEvaluator::FieldEvaluator(const common::FieldConfig *fc, int fid)
        : field_config_(fc)
        , fid_(fid)
        , fi_(fc->get_field(fid_))
        {
            if (!fc->get_field_def(fid_)->stored()) {
                throw argos_bad_field(fc->get_field_def(fid_)->get_name());
            }
        }
        
        std::string FieldEvaluator::to_string_impl() const
        {
            return field_config_->get_field_def(fid_)->get_name();
        }
        
        common::Value FieldEvaluator::evaluate(docid did, common:: ExecutionContext &context) const
        {
            if (fi_.type_==common::FT_INVALID) {
                return common::Value();
            }
            if (did==cached_docid_) {
                return cached_value_;
            }
            cached_docid_=did;
            return cached_value_=context.get_index()->get_doc_field(did, fid_);
        }
    }   // End of namespace index
}   // End of namespace argos
