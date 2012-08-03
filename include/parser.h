//
//  parser.h
//  Argos
//
//  Created by Windoze on 12-7-6.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/evaluatable.h"
#include "query/argos_query.h"

#ifndef Argos_parser_h
#define Argos_parser_h

namespace argos {
    namespace parser {
        /**
         * Parse string in CSV-like format into a value list
         */
        bool parse_value_list(const char *&str, size_t &len, common::value_list_t &vl, common::ExecutionContext &context);
        /**
         * Parse expression
         */
        common::eva_ptr_t parse_expr(const char *&str, size_t &len, common::ExecutionContext &context);

        /**
         * Parse field list, each field can be a field name or an expression
         */
        bool parse_field_list(const char *&str, size_t &len, common::field_list_t &fl, common::ExecutionContext &context);

        namespace detail {
            /**
             * class MatchParser_impl is the abstract base for all match parsers
             *
             * A match parser parses string and generate Match instances, which will be used for reverse index query
             */
            class MatchParser_impl {
            public:
                virtual ~MatchParser_impl(){}
                virtual query::match_ptr_t parse(const char *&str, size_t &len, common::ExecutionContext &context)=0;
            };

            /**
             * Create MatchParser_impl instance
             */
            MatchParser_impl *create_match_parser_impl();
        }
        
        /**
         * Concrate wrapper class for MatchParser_impl
         */
        class MatchParser {
        public:
            MatchParser();
            ~MatchParser();
            query::match_ptr_t parse(const char *&str, size_t &len, common::ExecutionContext &context);
        private:
            detail::MatchParser_impl *impl_;
        };

        /**
         * Parse break-down query parameters
         */
        bool parse_query(const char *match,
                         const char *filter,
                         const char *sort,
                         const char *sort_order,
                         const char *nr,
                         const char *sk,
                         const char *field_list,
                         query::Query &q,
                         common::ExecutionContext &context);

        /**
         * Parse query string, which is the parameter part of an URL
         */
        bool parse_query(const char *req, size_t len, query::Query &q, common::ExecutionContext &ctx);
    }   // End of namespace parser
}   // End of namespace argos

#endif
