//
//  match.cpp
//  Argos
//
//  Created by Windoze on 12-7-6.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "index/forward_index.h"
#include "query/match.h"

namespace argos {
    namespace query {
        doc_iterator_impl_ptr_t MatchNone::match(common::ExecutionContext &ctx)
        {
            return match_none_ptr;
        }
        doc_iterator_impl_ptr_t MatchAll::match(common::ExecutionContext &ctx)
        {
            return doc_iterator_impl_ptr_t(new detail::match_all_doc_iterator_impl(ctx.get_forward_index()->get_last_doc()));
        }
    }   // End of namespace query
}   // End of namespace argos
