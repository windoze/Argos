//
//  dummy_ri_backend.cpp
//  Argos
//
//  Created by Windoze on 12-7-4.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "index/reverse_index.h"
#include "parser.h"

/**
 * Dummy reversed index
 * Always returns all docs
 */

namespace argos {
    namespace index {
        namespace detail {
            class DummyReverseIndex : public ReverseIndex{
            public:
                DummyReverseIndex(const char *name)
                {}
                
                virtual void flush() {
                }
                
                virtual bool add_doc(docid did, const term_list_t &terms)
                { return true; }
            };

        }   // End of namespace detail
        ReverseIndex *create_reverse_index(const char *name)
        {
            return new detail::DummyReverseIndex(name);
        }
    }   // End of namespace index

    namespace parser {
        namespace detail {
            class DummyMatchParser_impl : public MatchParser_impl {
            public:
                query::match_ptr_t parse(const char *&str, size_t &len, common::ExecutionContext &context)
                {
                    return query::match_ptr_t(new query::MatchAll());
                }
            };

            MatchParser_impl *create_match_parser_impl()
            {
                return new DummyMatchParser_impl;
            }
        }   // End of namespace detail
    }   // End of namespace parser
}   // End of namespace argos
