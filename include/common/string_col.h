//
//  string_col.h
//  Argos
//
//  Created by Windoze on 12-6-13.
//  Copyright(c) 2012 0d0a.com. All rights reserved.
//

#ifndef Argos_string_col_h
#define Argos_string_col_h

#include "common/mem_pool.h"

namespace argos {
    namespace common {
        
        static const size_t DEFAULT_STRING_COL_SEG_SIZE=4*1024*1024;   // 4M
        
        /**
         * string_collection class can store strings and binary chuncks, grows as needed
         *
         * This class is *not* a full-functional memory management tool, it's mainly used
         * by hash_table to store string keys, another usage is to store variable-length
         * data without deletion.
         * NOTE: for binary data, string_collection does *not* store length information,
         *       and there is no way to retrieve it within this class, we need to record
         *       length somewhere else
         */
        class string_collection
        {
        public:
            typedef mem_pool pool_t;
            
            /**
             * Create new string collection
             */
            string_collection(const char *name=NULL)
            : m_pool(DEFAULT_STRING_COL_SEG_SIZE, name)
            {}
            
            /**
             * Clear the string collection, pool is marked as empty but not shrinked
             */
            inline void clear() { m_pool.clear(); }

            /**
             * Reset the string collection, pool is reset and all mmaps are closed
             */
            inline void reset() { m_pool.reset(); }

            /**
             * Expand the underlying pool
             */
            inline void expand(size_t sz=0) { m_pool.expand(sz); }
            
            /**
             * Add a string into the collection
             */
            inline bool add(const char *str, OFFSET &offset)
            {
                if(!str) {
                    return false;
                }
                
                offset=m_pool.add_string(str);
                return offset!=INVALID_OFFSET;
            }
            
            /**
             * Add a piece of binary data into the collection
             */
            inline bool add(const void *binary_data, size_t length, OFFSET &offset)
            {
                offset=m_pool.add_chunk(binary_data, length);
                return offset!=INVALID_OFFSET;
            }
            
            /**
             * Return the string at offset
             */
            inline const char *get(OFFSET offset) const
            {
                return (const char *)m_pool.get_addr(offset);
            }
            
            /**
             * Write string collection to disk
             */
            inline void serialize() { m_pool.save(); }

            /**
             * Load string collection from disk
             */
            inline void deserialize() { m_pool.load(); }

            /**
             * Return the name of string collection, it's could be the name of
             * on-disk file
             */
            inline const char *name() const { return m_pool.get_name(); }
            
            /**
             * Rename underlying file
             *
             * @param new_name new name of underlying file
             * @return true if success, otherwise false
             */
            bool rename(const char *name) { return m_pool.rename(name); }

            /**
             * Return the number of bytes used by the string collection
             */
            inline size_t size() const { return m_pool.get_used_size(); }
            
        private:
            pool_t m_pool;
        };
    }   // End of namespace common
}   // End of namespace argos

#endif
