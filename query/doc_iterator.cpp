//
//  doc_iterator.cpp
//  Argos
//
//  Created by Windoze on 12-7-6.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "query/doc_iterator.h"

namespace argos {
    namespace query {
        const doc_iterator_impl_ptr_t match_none_ptr(new detail::match_none_doc_iterator_impl);        
    }   // End of namespace query
}   // End of namespace argos
