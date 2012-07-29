//
//  field_evaluator.h
//  Argos
//
//  Created by Windoze on 12-6-26.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/evaluatable.h"
#include "common/field_config.h"

#ifndef Argos_field_evaluator_h
#define Argos_field_evaluator_h

namespace argos {
    namespace index {
        class FieldEvaluator : public common::Evaluatable {
        public:
            FieldEvaluator(const common::FieldConfig *fc, const char *name);
            
            FieldEvaluator(const common::FieldConfig *fc, const char *name, size_t sz);
            
            FieldEvaluator(const common::FieldConfig *fc, int fid);
            
            FieldEvaluator(const FieldEvaluator &fe)
            : field_config_(fe.field_config_)
            , fid_(fe.fid_)
            , fi_(fe.fi_)
            {}
            
            virtual int eva_type() const { return common::ET_ATTR; }
            virtual bool is_const() const { return false; }
            virtual bool uses_match_info() const { return false; }
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const;
            virtual std::string to_string() const;
            virtual size_t buffer_size() const;
            virtual size_t serialize(char *buf) const;
            virtual std::ostream &serialize(std::ostream &os) const;
            
            int field_id() const { return fid_; }
        private:
            const common::FieldConfig *field_config_;
            int fid_;
            common::FieldInfo fi_;
            mutable docid cached_docid_;
            mutable common::Value cached_value_;
        };
    }   // End of namespace index
}   // End of namespace argos

#endif
