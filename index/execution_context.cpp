//
//  execution_context.cpp
//  Argos
//
//  Created by Windoze on 12-6-28.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/field_config.h"
#include "common/execution_context.h"
#include "index/full_index.h"

namespace argos {
    namespace common {
        const common::FieldConfig *ExecutionContext::get_field_config() const
        {
            return index->get_field_config();
        }
        
        index::ForwardIndex *ExecutionContext::get_forward_index() const
        {
            return index->get_forward_index();
        }
        
        index::ReverseIndex *ExecutionContext::get_reverse_index() const
        {
            return index->get_reverse_index();
        }
    }   // End of namespace common
}   // End of namespace argos
