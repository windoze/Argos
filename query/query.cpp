//
//  query.cpp
//  Argos
//
//  Created by Windoze on 12-7-2.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "query/argos_query.h"

namespace argos {
    namespace query {
        /*
        Query::Query(const char *match, const char *filter, query::ExecutionContext &ctx)
        : riq_(0)
        , filter_(0)
        , sort_key_(0)
        , desc_(false)
        , nr_(0)
        , sk_(0)
        {
            size_t len=strlen(match);
            riq_=parse_ri_query(match, len, ctx);
            len=strlen(filter);
            filter_=parse(filter, len, ctx);
        }
        
        Query::Query(const char *match, const char *filter, const char *sortkey, bool desc, size_t nr, size_t sk, query::ExecutionContext &ctx)
        : riq_(0)
        , filter_(0)
        , sort_key_(0)
        , desc_(desc)
        , nr_(nr)
        , sk_(sk)
        {
            size_t len=strlen(match);
            riq_=parse_ri_query(match, len, ctx);
            len=strlen(filter);
            filter_=parse(filter, len, ctx);
            len=strlen(sortkey);
            sort_key_=parse(sortkey, len, ctx);
        }
        
        Query::Query(const char *match,
                     const char *filter,
                     const char *sortkey,
                     bool desc,
                     size_t nr,
                     size_t sk,
                     const char *field_list,
                     query::ExecutionContext &ctx)
        : riq_(0)
        , filter_(0)
        , sort_key_(0)
        , desc_(desc)
        , nr_(nr)
        , sk_(sk)
        {
            size_t len=strlen(match);
            riq_=parse_ri_query(match, len, ctx);
            if (!riq_) {
                throw argos_bad_operator(match);
            }
            len=strlen(filter);
            filter_=parse(filter, len, ctx);
            if (!filter_) {
                throw argos_bad_field(filter);
            }
            len=strlen(sortkey);
            sort_key_=parse(sortkey, len, ctx);
            if (!sort_key_) {
                throw argos_bad_field(sortkey);
            }
            len=strlen(field_list);
            if (!parse_field_list(field_list, len, ctx, fl_)) {
                throw argos_bad_field(field_list);
            }
        }
         */
        Query::Query()
        : nr_(0)
        , sk_(0)
        {}
        
        bool Query::uses_match_info() const {
            bool ret=filter_->uses_match_info();
            for (sort_criteria::const_iterator i=sort_crit_.begin(); i!=sort_crit_.end(); ++i) {
                ret = ret || i->sort_key->uses_match_info();
                if (ret) {
                    return true;
                }
            }
            for (common::field_list_t::const_iterator i=fl_.begin(); i!=fl_.end(); ++i) {
                ret = ret || (*i)->uses_match_info();
                if (ret) {
                    return true;
                }
            }
            return ret;
        }
    }   // End of namespace query
}   // End of namespace argos
