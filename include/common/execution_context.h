//
//  execution_context.h
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <map>
#include "common/argos_consts.h"
#include "common/mem_pool_allocator.h"
#include "common/value.h"
#include "common/evaluatable.h"

#ifndef Argos_execution_context_h
#define Argos_execution_context_h

namespace argos {
    namespace index {
        class ForwardIndex;
        class ReverseIndex;
        class Index;
    }
    
    namespace common {
        class FieldConfig;
        
        typedef std::vector<Value, mem_pool_tl_allocator<Value> > match_info_t;
        typedef std::map<std::string, int> match_term_dict_t;
        typedef std::map<int, eva_ptr_t> field_eva_cache_t;

        /**
         * struct ExecutionContext contains necessary info during execution of a query or document operations
         */
        struct ExecutionContext {
            /**
             * Default constructor
             */
            ExecutionContext()
            : temp_pool(0)
            , index()
            , match_info_(0)
            {}
            
            /**
             * Copy constructor
             */
            ExecutionContext(const ExecutionContext &other)
            : temp_pool(other.temp_pool)
            , index(other.index)
            , match_info_(other.match_info_)
            {}
            
            /**
             * Destructor
             */
            ~ExecutionContext()
            {}
            
            /**
             * Allocate memory from temp_pool
             */
            template<typename T>
            common::mem_pool_allocator<T> allocator() const {
                return common::mem_pool_allocator<T>(temp_pool);
            }
            
            /**
             * Allocate given number of element of memory from temp_pool
             *
             * @param sz number of elements to allocate
             */
            template<typename T>
            inline T *allocate(size_t sz)
            {
                return (T *)(temp_pool->get_addr(temp_pool->allocate(sz * sizeof(T))));
            }
            
            /**
             * Allocate given number of element of memory from temp_pool
             *
             * @param p stores returned value
             * @param sz number of elements to allocate
             */
            template<typename T>
            inline T *allocate(T *&p, size_t sz)
            {
                return p=(T *)(temp_pool->get_addr(temp_pool->allocate(sz * sizeof(T))));
            }

            /**
             * Clear content of temp_pool
             *
             * reset() should be called only if all allocated objects are not needed anymore
             * Any object inside should be destructed properly before this function call
             */
            void reset()
            {
                temp_pool->reset();
            }
            
            /**
             * return associated FieldConfig
             */
            const FieldConfig *get_field_config() const;
            /**
             * return associated ForwardIndex
             */
            index::ForwardIndex *get_forward_index() const;
            /**
             * return associated ReverseIndex
             */
            index::ReverseIndex *get_reverse_index() const;
            /**
             * return associated Index
             */
            const index::Index *get_index() const {
                return index;
            }
            /**
             * match_term_dict contains all terms involved in the query
             */
            match_term_dict_t match_term_dict;
            /**
             * returns the number of terms involved in the query
             */
            size_t get_match_info_size() const { return match_term_dict.size(); }
            /**
             * Use for bookkeeping during the match, each term query will record term count in match_info
             */
            match_info_t *get_match_info() const {
                return match_info_;
            }
            /**
             * Use for bookkeeping during the match, match_info is per-doc and will be replaced before processing next doc
             */
            void set_match_info(match_info_t *l){
                match_info_=l;
            }
            
            field_eva_cache_t field_eva_cache;
            
            mem_pool *temp_pool;
        private:
            const index::Index *index;
            match_info_t *match_info_;
            friend class index::Index;
        };
    }   // End of namespace common
}   // End of namespace argos

#endif
