//
//  argos_query.h
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <string>
#include "common/evaluatable.h"
#include "query/match.h"

#ifndef Argos_argos_query_h
#define Argos_argos_query_h

namespace argos {
    namespace query {
        struct sort_criterion {
            common::eva_ptr_t sort_key;
            bool desc;
        };
        
        typedef std::vector<sort_criterion> sort_criteria;
        
        inline void evaluate(docid did, const sort_criteria& sc, common::value_list_t &vl, common::ExecutionContext &ctx) {
            vl.resize(sc.size(), common::Value());
            for (size_t i=0; i<sc.size(); i++) {
                vl[i]=sc[i].sort_key->evaluate(did, ctx);
            }
        }
        
        typedef std::vector<common::field_list_t> histos_t;
        
        /**
         * Allocator for primary keys
         */
        typedef common::mem_pool_tl_allocator<primary_key> pk_allocator_t;
        
        /**
         * A list of primary keys
         */
        typedef std::vector<primary_key, pk_allocator_t> pk_list_t;
        
        /**
         * class Query is a wrapper for all query parameters
         */
        class Query {
        public:
            Query();
            
            std::string to_string() const;

            bool uses_match_info() const;
            
            const common::field_list_t &get_field_list() const { return fl_; }

            // HACK: Match part
            std::string match_param_;
            // Output Format
            std::string fmt;
            // Item query
            pk_list_t pks_;
            // Match
            match_ptr_t match_;
            // Filter
            common::eva_ptr_t filter_;
            // Sort criterion
            sort_criteria sort_crit_;
            // Histograms
            histos_t histos_;
            // Number of doc to return
            size_t nr_;
            // Skip top n doc
            size_t sk_;
            // Output Field List
            common::field_list_t fl_;
            // Query ID
            std::string query_id_;
            // Compress?
            bool comp_;
        };
    }   // End of namespace query
}   // End of namespace argos

#endif
