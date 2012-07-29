//
//  ri_query.h
//  Argos
//
//  Created by Windoze on 12-6-30.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <boost/shared_ptr.hpp>
#include "common/execution_context.h"
#include "query/doc_iterator.h"

#ifndef Argos_ri_query_h
#define Argos_ri_query_h

namespace argos {
    namespace query {
        /**
         * Abstract base class of Match implementation
         *
         * Match is reverse index query, it generates a docid serial for matched docs
         */
        class Match {
        public:
            virtual ~Match(){}
            /**
             * Collect all terms need to be matched into dict, record next_pos
             */
            virtual void collect_match_term(int &next_pos, common::match_term_dict_t &dict)=0;

            virtual doc_iterator_impl_ptr_t match(common::ExecutionContext &ctx)=0;

            inline void collect_match_term(common::match_term_dict_t &dict)
            {
                int next_pos=0;
                collect_match_term(next_pos, dict);
            }
        };

        /**
         * class MatchNone doesn't match any doc
         */
        class MatchNone : public Match {
        public:
            MatchNone(){}
            virtual void collect_match_term(int &next_pos, common::match_term_dict_t &dict){}
            virtual doc_iterator_impl_ptr_t match(common::ExecutionContext &ctx);
        };

        /**
         * class MatchAll matches all docs in a reverse index
         */
        class MatchAll : public Match {
        public:
            MatchAll(){}
            virtual void collect_match_term(int &next_pos, common::match_term_dict_t &dict){}
            virtual doc_iterator_impl_ptr_t match(common::ExecutionContext &ctx);
        };
        
        typedef boost::shared_ptr<Match> match_ptr_t;
    }   // End of namespace query
}   // End of namespace argos


#endif
