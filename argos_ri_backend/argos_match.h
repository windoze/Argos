//
//  argos_match.h
//  Argos
//
//  Created by Windoze on 12-7-21.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifndef Argos_argos_match_h
#define Argos_argos_match_h

#include <boost/shared_ptr.hpp>
#include "common/execution_context.h"
#include "common/term_list.h"

namespace argos {
    namespace query {
        namespace detail {
            class ArgosMatch : public Match {
            public:
                typedef std::vector<ArgosMatch> children_t;
                
                ArgosMatch();
                ArgosMatch(const common::term_t &term);
                ArgosMatch(int op, const children_t &children);
                virtual void collect_match_term(int &next_pos, common::match_term_dict_t &dict);
                virtual doc_iterator_impl_ptr_t match(common::ExecutionContext &ctx);
                
                int op_;
                common::term_t term_;
                children_t children_;
                int match_info_pos_;
            };
        }
    }
}

#endif
