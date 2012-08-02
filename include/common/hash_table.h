//
//  hash_table.h
//  Argos
//
//  Created by Windoze on 12-6-12.
//  Copyright(c) 2012 0d0a.com. All rights reserved.
//

#ifndef Argos_hash_table_h
#define Argos_hash_table_h

#include <stdio.h>
#include <string.h>
#include <utility>
#include <fcntl.h>
#include <unistd.h>
#include "common/primes.h"
#include "common/hash_function.h"
#include "common/string_col.h"

namespace argos {
    namespace common {
        const uint64_t VALID_TAG=0x484153485441424CL;

        static const size_t HASH_TABLE_INVALID_INDEX_ =(size_t)-1;
        
        template<typename KEY, typename VALUE>
        class hash_table;
        
        namespace detail {
            template<typename HASH_TABLE>
            class hash_iterator
            {
            public:
                typedef typename HASH_TABLE::key_type KEY;
                typedef typename HASH_TABLE::mapped_type T;
                typedef hash_iterator<HASH_TABLE> this_t;
                
                hash_iterator()
                {
                    m_index = HASH_TABLE_INVALID_INDEX_;
                    m_table = NULL;
                }
                
                hash_iterator(size_t index, HASH_TABLE *table)
                : m_index(index)
                , m_table(table)
                {
                }
                
                inline hash_iterator operator++(int);
                inline void operator++();
                
                inline bool operator==(const this_t &it)
                {
                    return(m_table == it.m_table) && (m_index == it.m_index);
                }
                
                inline bool operator!=(const this_t &it)
                {
                    return !((*this) == it) ;
                }
                
                inline size_t get_index() const { return m_index; }
                inline KEY get_key() const;
                inline T &get_value() const;
                OFFSET get_offset() const;
                
                std::pair<KEY, T> operator*() const
                {
                    return std::make_pair(get_key(), get_value());
                }
                
            protected:
                size_t m_index;
                HASH_TABLE *m_table;
            };
            
            /**
             * write nelem of T into fd
             *
             * This implementation is for POD only, need specialize for non-POD types
             */
            template<typename T>
            inline bool swrite(int fd, const T *p, size_t nelem, size_t &written_bytes) {
                size_t nbytes=sizeof(T)*nelem;
                const char *pc=(const char *)p;
                size_t total=0;
                while (total<nbytes) {
                    // Handle short write
                    ssize_t r=write(fd, pc+total, nbytes-total);
                    if (r<0) {
                        return false;
                    }
                    total+=r;
                }
                written_bytes+=total;
                return true;
            }
            
            /**
             * read nelem of T into fd
             *
             * This implementation is for POD only, need specialize for non-POD types
             */
            template<typename T>
            inline bool sread(int fd, T *p, size_t nelem, size_t &read_bytes) {
                size_t nbytes=sizeof(T)*nelem;
                char *pc=(char *)p;
                size_t total=0;
                while (total<nbytes) {
                    // Handle short read
                    ssize_t r=read(fd, pc+total, nbytes-total);
                    if (r<0) {
                        return false;
                    }
                    total+=r;
                }
                read_bytes+=total;
                return true;
            }
        }   // End of namespace detail
        
        template<typename KEY, typename VALUE>
        class hash_table
        {
        public:
            typedef hash_table<KEY, VALUE> this_t;
            typedef detail::hash_iterator<this_t> iterator;
            friend class detail::hash_iterator<this_t>;
            
            typedef KEY key_type;
            typedef VALUE mapped_type;
            
            hash_table(size_t size = 17, const char *name = 0)
            : m_data_pool(new mem_pool(sizeof(data_t), name))
            , m_data(m_data_pool->offptr<data_t>(0).get())
            //, m_size(get_prime_number(size))
            , m_value_pool(0)
            , m_key_pool(0)
            , m_occupation_pool(0)
            , m_next_index_pool(0)
            //, m_quantity(0)
            //, m_collisions(0)
            , m_values(0)
            , m_keys(0)
            , m_occupied(0)
            //, m_erased(0)
            , m_next_index(0)
            //, m_first_index(-1)
            //, m_last_index(-1)
            {
                m_name[0]=0;
                if(name) strcpy(m_name, name);
                if (m_data && (m_data->valid_tag==VALID_TAG)) {
                    // we already have a hash_table
                    alloc(false);
                } else {
                    m_data->valid_tag=VALID_TAG;
                    m_data->m_size=get_prime_number(size);
                    m_data->m_quantity=0;
                    m_data->m_collisions=0;
                    m_data->m_erased=0;
                    m_data->m_first_index=-1;
                    m_data->m_last_index=-1;
                    m_data_pool->save();
                    alloc();
                }
            }
            
            ~hash_table()
            {
                serialize();
            }
            
            iterator begin() 
            {
                return iterator(first(), this);
            }
            
            iterator end() 
            {
                return iterator(last(), this);
            }
            
            size_t first() 
            {
                //if some deletions have been made,
                //then even begin might be invalid
                if(m_data->m_first_index == HASH_TABLE_INVALID_INDEX_) return HASH_TABLE_INVALID_INDEX_;
                if(m_occupied[m_data->m_first_index] == false)
                {
                    m_data->m_first_index = next(m_data->m_first_index);
                }
                return m_data->m_first_index;
            }
            
            size_t last() const { return HASH_TABLE_INVALID_INDEX_; }
            
            //given an index, return the next populated index
            size_t next(size_t index)
            {
                if(index == HASH_TABLE_INVALID_INDEX_) return HASH_TABLE_INVALID_INDEX_;
                
                size_t next_index = m_next_index[index];
                
                while(next_index != HASH_TABLE_INVALID_INDEX_ &&
                      m_occupied[next_index] == false)  //deleted value ?
                {
                    next_index = m_next_index[next_index];
                }
                m_next_index[index] = next_index;
                if(next_index == HASH_TABLE_INVALID_INDEX_) m_data->m_last_index = index;
                return next_index;
            }
            
            iterator find(KEY key)
            {
                size_t index;
                if(get_index(key, index))
                {
                    return iterator(index, this);
                }
                return end();
            }
            
            bool erase(iterator &i)
            {
                if(m_occupied[i.get_index()] == false) return true; //already erased
                m_occupied[i.get_index()] = false;
                //m_values[i.get_index()] = VALUE(); is this needed ??
                m_data->m_quantity--;
                m_data->m_erased++;
                return true;
            }
            
            inline bool get_index(KEY key, size_t &index, size_t &collisions, bool &full) const
            {
                int incr;
                collisions = 0;
                full = false;
                index = hash(key, incr);
                if(m_occupied[index] == false)
                {
                    return false;
                }
                size_t index_start = index;
                do
                {
                    if(key == m_keys[index])
                    {
                        return true;
                    }
                    collisions++;
                    index += incr;
                    while(index>= m_data->m_size) index -= m_data->m_size;
                }
                while(!(index == index_start || 
                        m_occupied[index] == false));
                
                if(index == index_start)
                {
                    // TODO: hash table full logging
                    full = true;
                }
                return false;
            }                                                                    
            
            bool get_index(KEY key, size_t &index, size_t &collisions) const
            {
                bool full = false;
                return get_index(key, index, collisions, full);
            }
            
            inline
            bool get_index(KEY key, size_t &index) const
            {
                size_t collisions(0);
                return get_index(key, index, collisions);
            }
            
            bool find(KEY key, VALUE &value, size_t &index) const
            {
                if(get_index(key, index))
                {
                    value = m_values[index];
                    return true;
                }
                return false;
            }
            
            inline bool find(KEY key, VALUE &value) const
            {
                size_t index;
                return find(key, value, index);
            }
            
            inline bool insert_or_replace(KEY key, const VALUE &value, VALUE &old_value, size_t &index)
            {
                bool full = false;
                size_t collisions(0);
                if(get_index(key, index, collisions, full))
                {
                    old_value = m_values[index];
                    m_values[index] = value;
                    return false;
                }
                
                if((m_data->m_quantity  *4)>(m_data->m_size  *3))
                {
                    resize();
                    return insert_or_replace(key, value, old_value, index);
                }
                
                m_data->m_collisions += collisions;
                actually_insert(key, value, index);
                return true;
            }   
            
            inline bool insert(KEY key, const VALUE &value, size_t &index)
            {
                bool full = false;
                size_t collisions(0);
                if(get_index(key, index, collisions, full))
                {
                    //already exists
                    return false;
                }
                if((m_data->m_quantity  *4)>(m_data->m_size  *3))
                {
                    resize();
                    return insert(key, value, index);
                }
                m_data->m_collisions += collisions;
                actually_insert(key, value, index);
                return true;
            }
            
            inline bool insert(KEY key, const VALUE &value)
            {
                size_t index;
                return insert(key, value, index);
            }
            
            inline bool insert(KEY key, const VALUE &value, iterator &i)
            {
                size_t index;
                bool b = insert(key, value, index);
                i = iterator(index, this);
                return b;
            }
            
            inline bool erase(KEY key)
            {
                size_t index;
                if(!get_index(key, index)) return false;
                if(m_occupied[index] == false) return true; //already erased
                m_occupied[index] = false;
                m_keys[index] = KEY();
                m_values[index] = VALUE();
                m_data->m_quantity--;
                m_data->m_erased++;
                return true;
            }
            
            inline VALUE &operator[](KEY key)
            {
                size_t index;
                insert(key, VALUE(), index); //will insert if doesnt exist
                return m_values[index];
            }
            
            inline KEY get_key(size_t index) const
            {
                return m_keys[index];
            }
            
            inline VALUE &get_value(size_t index)
            {
                return m_values[index];
            }
            
            inline size_t get_offset(size_t index) const
            {
                return index;
            }
            
            inline void nuke()
            {
                //reset all entries
                memset(m_values, 0, sizeof(VALUE)  *m_data->m_size);
                memset(m_occupied, 0, sizeof(bool)  *m_data->m_size);
                //TODO: problem when using non basic data-types
                memset(m_keys, 0, sizeof(KEY)  *m_data->m_size);
                //if(m_key_pool) delete m_key_pool;
                //m_key_pool=new mem_pool(m_data->m_size*sizeof(KEY) /* TODO: name */);
                //m_keys=offptr_t<KEY>(m_key_pool, 0).get();
                
                memset(m_next_index, 0xFF, sizeof(size_t)  *m_data->m_size);
                
                m_data->m_quantity = 0;
                m_data->m_collisions = 0;
                m_data->m_erased = 0;
                
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
            }
            
            size_t size() const { return m_data->m_quantity; }
            size_t capacity() const { return m_data->m_size; }
            size_t collisions() const { return m_data->m_collisions; }
            size_t erased() const { return m_data->m_erased; }
            const char *name() const { return m_name; }
            size_t memory_usage() const {return m_data->m_size *(sizeof(KEY)+sizeof(VALUE));}
            double erased_percentage() const
            {
                return (m_data->m_quantity == 0) ? 0.0 :(double)m_data->m_erased/(double)m_data->m_quantity;
            }
            
            bool serialize() {
                m_data_pool->save();
                m_value_pool->save();
                m_key_pool->save();
                m_occupation_pool->save();
                m_next_index_pool->save();
                return true;
            }
            
            bool deserialize() {
                // TODO: For now we can call constructor with a name to load existing ht
                return true;
            }
        private:
            // Unfinished
            hash_table(const hash_table &table)
            : m_data_pool(0)
//            , m_size(table.m_size)
//            , m_quantity(table.m_quantity)
//            , m_collisions(table.m_collisions)
            , m_values(0)
            , m_keys(0)
            , m_occupied(0)
//            , m_erased(table.m_erased)
            , m_next_index(0)
//            , m_first_index(table.m_first_index)
//            , m_last_index(table.m_last_index)
            {
                m_name[0]=0;
                if(table.m_name) strcpy(m_name, table.m_name);
                
                alloc(false);
                memcpy(m_data, table.m_data, sizeof(data_t));
                memcpy(m_values, table.m_values, sizeof(VALUE)  *m_data->m_size);
                memcpy(m_keys, table.m_keys, sizeof(KEY)  *m_data->m_size);
                memcpy(m_occupied, table.m_occupied, sizeof(bool)  *m_data->m_size);
                memcpy(m_next_index, table.m_next_index, sizeof(size_t)  *m_data->m_size);
            }
            
            void alloc(bool clear_content=true)
            {
                char buf[PATH_MAX];
                
                m_value_pool=new mem_pool(m_data->m_size*sizeof(VALUE), add_suffix(buf, name(), ".value"));
                m_values=offptr_t<VALUE>(m_value_pool, 0).get();
                
                m_occupation_pool=new mem_pool(m_data->m_size*sizeof(bool), add_suffix(buf, name(), ".use"));
                m_occupied=offptr_t<bool>(m_occupation_pool, 0).get();
                
                m_key_pool=new mem_pool(m_data->m_size*sizeof(KEY), add_suffix(buf, name(), ".key"));
                m_keys=offptr_t<KEY>(m_key_pool, 0).get();
                
                m_next_index_pool=new mem_pool(m_data->m_size*sizeof(size_t), add_suffix(buf, name(), ".next"));
                m_next_index=offptr_t<size_t>(m_next_index_pool, 0).get();

                if (clear_content) {
                    memset(m_values, 0, sizeof(VALUE)  *m_data->m_size);
                    memset(m_occupied, 0, sizeof(bool)  *m_data->m_size);
                    // problem when using non basic data-types
                    // memset(m_keys, 0, sizeof(KEY)  *m_data->m_size); // ...?
                    memset(m_next_index, 0xFF, sizeof(size_t)  *m_data->m_size);
                }
            }
            
            void clear()
            {
                m_data->m_quantity = 0;

                m_values = NULL;
                if(m_value_pool) delete m_value_pool;
                m_value_pool = NULL;

                m_occupied = NULL;
                if(m_occupation_pool) delete m_occupation_pool;
                m_occupation_pool = NULL;
                
                m_keys = NULL;                
                if(m_key_pool) delete m_key_pool;
                m_key_pool = NULL;
                
                m_next_index = NULL;
                if(m_next_index_pool) delete m_next_index_pool;
                m_next_index_pool = NULL;
                
                m_data->m_collisions = 0;
                m_data->m_erased = 0;
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
            }
            
            hash_table(hash_table &table, bool shallow)
            : m_data_pool(0)
//            , m_size(0)
//            , m_quantity(0)
//            , m_collisions(0)
            , m_values(0)
            , m_keys(0)
            , m_occupied(0)
//            , m_erased(0)
            , m_next_index(0)
//            , m_first_index(0)
//            , m_last_index(0)
            {
                m_name[0]=0;
                if(shallow)
                {
                    memcpy((void*)this,(void*)&table, sizeof(hash_table));
                }
                else
                {
                    // TODO: Deep copy
                }
            }
            
            void rename(const char *new_name)
            {
                // Only rename named hash_table
                if (m_name[0] && new_name && new_name[0])
                {
                    strcpy(m_name, new_name);
                    char buf[PATH_MAX];
                    if(m_data_pool) m_data_pool->rename(name());
                    if(m_value_pool) m_value_pool->rename(add_suffix(buf, name(), ".value"));
                    if(m_key_pool) m_key_pool->rename(add_suffix(buf, name(), ".key"));
                    if(m_occupation_pool) m_occupation_pool->rename(add_suffix(buf, name(), ".use"));
                    if(m_next_index_pool) m_next_index_pool->rename(add_suffix(buf, name(), ".next"));
                }
            }
            
            void resize(size_t new_size = 0)
            {
                //shallow copy this hash table to another one
                hash_table table(*this, true); //shallow
                
                size_t old_size=m_data->m_size;
                
                // Rename newly created hash_table
                char buf[PATH_MAX];
                table.rename(add_suffix(buf, name(), ".temp"));
                
                m_data_pool=new mem_pool(sizeof(data_t), m_name);
                m_data=m_data_pool->offptr<data_t>(0).get();
                
                //reset your own parameters
                m_data->valid_tag=VALID_TAG;
                m_data->m_quantity = 0; 
                m_data->m_collisions = 0;
                m_data->m_erased = 0;
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
                
                if(new_size == 0)
                {
                    // new size should be at least 1.5 times bigger
                    m_data->m_size = get_prime_number(old_size +(old_size>> 1));
                }
                else
                {
                    m_data->m_size = get_prime_number(new_size);
                }
                //m_size_factor unchanged
                alloc();
                
                //now copy everything from other table to urself
                
                size_t index;
                size_t collisions;
                size_t orig_index;
                bool first = true;
                for(iterator i = table.begin(); i != table.end(); i++)
                {
                    bool b = get_index(i.get_key(), index, collisions);
                    if(b)
                    {
                        // TODO: Error, hash table resizing problem
                    }
                    if(first)
                    {
                        m_data->m_first_index = index;
                        
                        first = false;
                    }
                    else
                    {
                        m_next_index[m_data->m_last_index] = index;
                    }
                    
                    m_data->m_last_index = index;
                    m_next_index[index] = HASH_TABLE_INVALID_INDEX_;
                    
                    table.get_index(i.get_key(), orig_index);
                    m_values[index] = i.get_value();
                    m_occupied[index] = true;
                    m_keys[index] = table.m_keys[orig_index];
                    m_data->m_collisions += collisions;
                    m_data->m_quantity++;
                }
            }
            
            //use after erasing a lot
            void compact() //changes string ptrs/offsets !!
            {
                //shallow copy this hash table to another one
                hash_table table(*this, true); //shallow
                
                //reset your own parameters
                m_data->m_quantity = 0; 
                m_data->m_collisions = 0;
                m_data->m_erased = 0;
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
                
                //m_own_keys unchanged
                //m_size_factor unchanged
                alloc();
                
                //now copy everything from other table to urself
                
                for(iterator i = table.begin(); i != table.end(); i++)
                {
                    bool b = insert(i.get_key(), i.get_value());
                    if(b)
                    {
                        // TODO: Error, hash table resizing problem
                    }
                }
            }
            
            inline void actually_insert(KEY key, const VALUE &value, size_t &index)
            {
                m_keys[index] = key;
                m_occupied[index] = true;
                
                m_values[index] = value;
                m_data->m_quantity++;
                
                //update the iterator info
                if(m_data->m_first_index ==(size_t)-1)
                {
                    m_data->m_first_index = index;
                    m_data->m_last_index = index;
                }
                else
                {
                    m_next_index[m_data->m_last_index] = index;
                    m_data->m_last_index = index;
                }
                m_next_index[index] = HASH_TABLE_INVALID_INDEX_;
                return;
            }
            
            inline size_t hash(KEY key, int &incr) const
            {
                size_t hn = hash_function(key);
                size_t size = m_data->m_size;
                incr =int((hn %(size - 1)) + 1);
                if(incr <0) incr = -incr;
                return hn % size;
            }
            
            mem_pool *m_data_pool;

            struct data_t {
                uint64_t valid_tag;
                size_t m_size;
                size_t m_quantity;
                size_t m_collisions;
                size_t m_erased;            
                size_t  m_first_index;
                size_t  m_last_index;
            };
            data_t *m_data;
            
            //size
            //size_t m_size;
            //size_t m_quantity;
            //size_t m_collisions;
            //size_t m_erased;            
            //size_t  m_first_index;
            //size_t  m_last_index;
            
            mem_pool *m_value_pool;
            mem_pool *m_key_pool;
            mem_pool *m_occupation_pool;
            mem_pool *m_next_index_pool;
            VALUE *m_values;
            KEY *m_keys;
            bool *m_occupied;
            size_t *m_next_index;
            
            char m_name[PATH_MAX];
        };
        
        template<typename VALUE>
        class hash_table<const char*, VALUE>
        {
        public:
            typedef hash_table<const char*, VALUE> this_t;
            typedef const char *KEY;
            typedef detail::hash_iterator<this_t> iterator;
            
            typedef const char *key_type;
            typedef VALUE mapped_type;
            
            hash_table(size_t size = 17, bool own_keys = true, int size_factor = 1, const char *name = 0)
            : m_data_pool(new mem_pool(sizeof(data_t), name))
            , m_data(m_data_pool->offptr<data_t>(0).get())
//            , m_size(get_prime_number(size))
//            , m_quantity(0)
//            , m_collisions(0)
            , m_values(0)
            , m_offsets(0)
            , m_strings(0)
//            , m_own_keys(own_keys)
//            , m_size_factor(size_factor)
//            , m_erased(0)
            , m_next_index(0)
//            , m_first_index(-1)
//            , m_last_index(-1)
//            , m_caseless(false)
            , m_compare_fn(strcmp)
            {
                m_name[0]=0;
                if(name) strcpy(m_name, name);
                if (m_data && (m_data->valid_tag==VALID_TAG)) {
                    // we already have a hash_table
                    alloc(false);
                } else {
                    m_data->valid_tag=VALID_TAG;
                    m_data->m_size=get_prime_number(size);
                    m_data->m_quantity=0;
                    m_data->m_collisions=0;
                    m_data->m_own_keys=own_keys;
                    m_data->m_size_factor=size_factor;
                    m_data->m_erased=0;
                    m_data->m_first_index=-1;
                    m_data->m_last_index=-1;
                    m_data->m_caseless=false;
                    m_data_pool->save();
                    alloc();
                }
            }
            
            hash_table(bool own_keys, size_t size = 17 , int size_factor = 1, const char *name = 0)
            : m_data_pool(new mem_pool(sizeof(data_t), name))
            , m_data(m_data_pool->offptr<data_t>(0))
//            , m_size(get_prime_number(size))
//            , m_quantity(0)
//            , m_collisions(0)
            , m_values(0)
            , m_offsets(0)
            , m_strings(0)
//            , m_own_keys(own_keys)
//            , m_size_factor(size_factor)
//            , m_erased(0)
            , m_next_index(0)
//            , m_first_index(-1)
//            , m_last_index(-1)
//            , m_caseless(false)
            , m_compare_fn(strcmp)
            {
                m_name[0]=0;
                if(name) strcpy(m_name, name);
                if (m_data && (m_data->valid_tag==VALID_TAG)) {
                    // we already have a hash_table
                    alloc(false);
                } else {
                    m_data->valid_tag=VALID_TAG;
                    m_data->m_size=get_prime_number(size);
                    m_data->m_quantity=0;
                    m_data->m_collisions=0;
                    m_data->m_own_keys=own_keys;
                    m_data->m_size_factor=size_factor;
                    m_data->m_erased=0;
                    m_data->m_first_index=-1;
                    m_data->m_last_index=-1;
                    m_data->m_caseless=false;
                    m_data_pool->save();
                    alloc();
                }
            }
            
            ~hash_table()
            {
                serialize();
            }
            
            iterator begin() 
            {
                return iterator(first(), this);
            }
            
            iterator end() 
            {
                return iterator(last(), this);
            }
            
            size_t first() 
            {
                //if some deletions have been made,
                //then even begin might be invalid
                if(m_data->m_first_index == HASH_TABLE_INVALID_INDEX_) return HASH_TABLE_INVALID_INDEX_;
                if(m_offsets[m_data->m_first_index] == INVALID_OFFSET)
                {
                    m_data->m_first_index = next(m_data->m_first_index);
                }
                return m_data->m_first_index;
            }
            
            size_t last() const { return HASH_TABLE_INVALID_INDEX_; }
            
            //given an index, return the next populated index
            size_t next(size_t index)
            {
                if(index == HASH_TABLE_INVALID_INDEX_) return HASH_TABLE_INVALID_INDEX_;
                
                size_t next_index = m_next_index[index];
                
                while(next_index != HASH_TABLE_INVALID_INDEX_ &&
                      m_offsets[next_index] == INVALID_OFFSET)  //deleted value ?
                {
                    next_index = m_next_index[next_index];
                }
                m_next_index[index] = next_index;
                if(next_index == HASH_TABLE_INVALID_INDEX_) m_data->m_last_index = index;
                return next_index;
            }
            
            iterator find(KEY key)
            {
                size_t index;
                if(get_index(key, index))
                {
                    return iterator(index, this);
                }
                return end();
            }
            
            bool erase(iterator &i)
            {
                if(m_offsets[i.get_index()] == INVALID_OFFSET) return true; //already erased
                m_offsets[i.get_index()] = INVALID_OFFSET;
                //m_values[i.get_index()] = VALUE(); is this needed ??
                m_data->m_quantity--;
                m_data->m_erased++;
                return true;
            }
            
            void set_caseless()
            {
                m_data->m_caseless = true;
                m_compare_fn = strcasecmp;
            }
            
            inline bool get_index(KEY key, size_t &index, size_t &collisions, bool &full) const
            {
                size_t incr;
                collisions = 0;
                full = false;
                index = hash(key, incr);
                if(m_offsets[index] == INVALID_OFFSET)
                {
                    return false;
                }
                size_t index_start = index;
                do
                {
                    int r=0;
                    if (m_data->m_own_keys) {
                        r=m_compare_fn(key, m_strings->get(m_offsets[index]));
                    } else {
                        r=m_compare_fn(key,(const char*)(m_offsets[index]));
                    }
//                    if((m_data->m_own_keys && 0 == m_compare_fn(key, m_strings->get(m_offsets[index]))) 
//                       ||(!m_data->m_own_keys && 0 == m_compare_fn(key,(const char*)(m_offsets[index]))))
                    if (r==0)
                    {
                        return true;
                    }
                    collisions++;
                    index += incr;
                    while(index>= m_data->m_size) index -= m_data->m_size;
                }
                while(!(index == index_start || 
                        m_offsets[index] == INVALID_OFFSET));
                
                if(index == index_start)
                {
                    // TODO: hash table full logging
                    full = true;
                }
                return false;
            }
            
            inline bool get_index(KEY key, size_t &index, size_t &collisions) const
            {
                bool full = false;
                return get_index(key, index, collisions, full);
            }
            
            inline bool get_index(KEY key, size_t &index) const
            {
                size_t collisions(0);
                return get_index(key, index, collisions);
            }
            
            bool find(KEY key, VALUE &value, size_t &index) const
            {
                if(! key)
                {
                    return false;
                }
                if(get_index(key, index))
                {
                    value = m_values[index];
                    return true;
                }
                return false;
            }
            
            inline bool find(KEY key, VALUE &value) const
            {
                size_t index;
                return find(key, value, index);
            }
            
            inline bool insert(KEY key, const VALUE &value, size_t &index)
            {
                if(!key)
                {
                    return false;
                }
                bool full = false;
                size_t collisions(0);
                if(get_index(key, index, collisions, full))
                {    //already exists
                    return false;
                }
                if((m_data->m_quantity  *4)>(m_data->m_size  *3))
                {
                    resize();
                    return insert(key, value, index);
                }
                m_data->m_collisions += collisions;
                if(m_data->m_own_keys)
                {
                    m_strings->add(key, m_offsets[index]);
                }
                else
                {
                    m_offsets[index] =(OFFSET)(key);
                }
                m_values[index] = value;
                m_data->m_quantity++;
                
                //update the iterator info
                if(m_data->m_first_index ==(size_t)-1)
                {
                    m_data->m_first_index = index;
                    m_data->m_last_index = index;
                }
                else
                {
                    m_next_index[m_data->m_last_index] = index;
                    m_data->m_last_index = index;
                }
                m_next_index[index] = HASH_TABLE_INVALID_INDEX_;
                
                return true;
            }
            
            inline bool insert(KEY key, const VALUE &value)
            {
                size_t index;
                return insert(key, value, index);
            }
            
            inline bool insert(KEY key, const VALUE &value, iterator &i)
            {
                size_t index;
                bool b = insert(key, value, index);
                i = iterator(index, this);
                return b;
            }
            
            inline bool erase(KEY key)
            {
                size_t index;
                if(!get_index(key, index)) return false;
                if(m_offsets[index] == INVALID_OFFSET) return true; //already erased
                m_offsets[index] = INVALID_OFFSET;
                m_values[index] = VALUE();
                m_data->m_quantity--;
                m_data->m_erased++;
                return true;
            }
            
            inline VALUE &operator[](KEY key)
            {
                size_t index;
                insert(key, VALUE(), index); //will insert if doesnt exist
                return m_values[index];
            }
            
            inline OFFSET get_offset(size_t index) const
            {
                return m_offsets[index];
            }
            
            inline KEY get_key(OFFSET offset) const
            {
                return m_data->m_own_keys ? m_strings->get(offset) : (KEY)(offset);
            }
            
            inline VALUE &get_value(size_t index)
            {
                return m_values[index];
            }
            
            size_t size() const { return m_data->m_quantity; }
            size_t capacity() const { return m_data->m_size; }
            size_t collisions() const { return m_data->m_collisions; }
            size_t erased() const { return m_data->m_erased; }
            const char *name() const { return m_name ? m_name : ""; }
            
            size_t memory_usage() const {
                return m_data->m_size *(sizeof(KEY)+sizeof(VALUE))+get_string_table_size();
            }
            double erased_percentage() const
            {
                return (m_data->m_quantity == 0) ? 0.0 : (double)m_data->m_erased/(double)m_data->m_quantity;
            }
            
            string_collection *get_string_collection() const
            {
                return m_strings;
            }
            
            size_t get_string_table_size() const
            {
                return m_data->m_own_keys ? m_strings->size() : 0;
            }
            
            bool serialize() {
                m_data_pool->save();
                if (m_data->m_own_keys) {
                    if(m_strings) m_strings->serialize();
                }
                if(m_offset_pool) m_offset_pool->save();
                if(m_next_index_pool) m_next_index_pool->save();
                if(m_value_pool) m_value_pool->save();
                return true;
            }
            
            bool deserialize() {
                // TODO: So far we can load existing ht with constructor
                return true;
            }
            
        //private:
            // NOTE: Incomplete, do *not *use it now
            // Do we really need copy constructor?
            hash_table(const this_t &table)
            : m_data_pool(0)
            , m_values(0)
            , m_offsets(0)
            , m_strings(0)
            , m_next_index(0)
            , m_compare_fn(table.m_compare_fn)
            {
                m_name[0]=0;
                if(table.m_name) strcpy(m_name, table.m_name);
                
                if(m_data->m_own_keys)  
                {
                    m_strings = new string_collection(*table.m_strings); //copy constructor
                }
                alloc(false);
                memcpy(m_values, table.m_values, sizeof(VALUE)  *m_data->m_size);
                memcpy(m_offsets, table.m_offsets, sizeof(OFFSET)  *m_data->m_size);
                memcpy(m_next_index, table.m_next_index, sizeof(size_t)  *m_data->m_size);
            }
            
            /*
             Shallow copy, this function is for internal use only,
             this is *not *a real copy constructor
             */
            hash_table(hash_table &table, bool shallow)
            : m_data_pool(0)
            , m_values(0)
            , m_offsets(0)
            , m_strings(0)
            , m_next_index(0)
            , m_compare_fn(table.m_compare_fn)
            {
                m_name[0]=0;
                if(shallow)
                {
                    memcpy(this, &table, sizeof(hash_table));
                }
                else
                {
                    // TODO: Deep copy?
                }
            }
            
            void alloc(bool clear_content=true) 
            {
                char buf[PATH_MAX];
                if(m_data->m_own_keys)
                {
                    if(!m_strings) {
                        m_strings = new string_collection(add_suffix(buf, name(), ".key"));
                    }
                }
                
                m_value_pool=new mem_pool(m_data->m_size*sizeof(VALUE), add_suffix(buf, name(), ".value"));
                m_values=m_value_pool->offptr<VALUE>(0).get();

                m_offset_pool=new mem_pool(m_data->m_size*sizeof(OFFSET), add_suffix(buf, name(), ".offset"));
                m_offsets=m_offset_pool->offptr<OFFSET>(0).get();

                m_next_index_pool=new mem_pool(m_data->m_size*sizeof(size_t), add_suffix(buf, name(), ".next"));
                m_next_index=m_next_index_pool->offptr<size_t>(0).get();
                
                if (clear_content) {
                    memset(m_values, 0, m_data->m_size*sizeof(VALUE));
                    memset(m_offsets, 0xFF, m_data->m_size*sizeof(OFFSET));
                    memset(m_next_index, 0xFF, m_data->m_size*sizeof(size_t));
                }
            }
            
            void clear()
            {
                m_data->m_quantity = 0;

                m_values = NULL;
                if(m_value_pool) delete m_value_pool;
                m_value_pool = NULL;
                
                m_offsets = NULL;
                if(m_offset_pool) delete m_offset_pool;
                m_offset_pool = NULL;

                m_next_index = NULL;
                if(m_next_index_pool) delete m_next_index_pool;
                m_next_index_pool = NULL;

                if(m_strings) delete m_strings;
                m_strings = NULL;
                
                m_data->m_collisions = 0;
                m_data->m_erased = 0;
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
            }
            
            void nuke()
            {
                //reset all entries
                memset(m_values, 0, sizeof(VALUE)*m_data->m_size);
                memset(m_offsets, 0xFF, sizeof(OFFSET)  *m_data->m_size);
                memset(m_next_index, 0xFF, sizeof(size_t)  *m_data->m_size);
                if(m_data->m_own_keys)
                {
                    m_strings->reset();
                }
                
                m_data->m_quantity = 0;
                m_data->m_collisions = 0;
                m_data->m_erased = 0;
                
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
            }
            
            void reset()
            {
                //reset all entries
                memset(m_values, 0, sizeof(VALUE)  *m_data->m_size);
                memset(m_offsets, 0xFF, sizeof(OFFSET)  *m_data->m_size);
                memset(m_next_index, 0xFF, sizeof(size_t)  *m_data->m_size);
                if(m_data->m_own_keys)
                {
                    m_strings->reset();
                }
                
                m_data->m_quantity = 0;
                m_data->m_collisions = 0;
                m_data->m_erased = 0;
                
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
            }
            
            void rename(const char *new_name, bool with_string)
            {
                // Only rename named hash_table
                if (m_name[0] && new_name && new_name[0])
                {
                    strcpy(m_name, new_name);
                    char buf[PATH_MAX];
                    if(m_data_pool) m_data_pool->rename(name());
                    if(m_value_pool) m_value_pool->rename(add_suffix(buf, name(), ".value"));
                    if(m_offset_pool) m_offset_pool->rename(add_suffix(buf, name(), ".offset"));
                    if(m_next_index_pool) m_next_index_pool->rename(add_suffix(buf, name(), ".next"));
                    if (with_string) {
                        if(m_strings) m_strings->rename(add_suffix(buf, name(), ".key"));
                    }
                }
            }
            
            void resize(size_t new_size = 0)
            {
                if((new_size>0) && (new_size <= m_data->m_size)) return;
                
                //shallow copy this hash table to another one
                hash_table table(*this, true/*shallow*/);
                
                size_t old_size=m_data->m_size;
                
                // Rename newly created hash_table
                char buf[PATH_MAX];
                table.rename(add_suffix(buf, name(), ".temp"), false);
                
                m_data_pool=new mem_pool(sizeof(data_t), m_name);
                m_data=m_data_pool->offptr<data_t>(0).get();
                
                //reset your own parameters
                m_data->valid_tag=VALID_TAG;
                m_data->m_quantity = 0; 
                m_data->m_collisions = 0;
                m_data->m_own_keys = table.m_data->m_own_keys;
                m_data->m_size_factor = table.m_data->m_size_factor;
                m_data->m_erased = 0;
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
                m_data->m_caseless = table.m_data->m_caseless;

                if(new_size == 0)
                {
                    m_data->m_size = get_prime_number(old_size +(old_size>> 1));
                }
                else
                {
                    m_data->m_size = get_prime_number(new_size);
                }
                
                // TODO: Log resizing, this may indicate performance impact
                // printf("Resize from %zu to %zu\n", old_size, m_data->m_size);
                       
                //m_size_factor unchanged
                alloc();
                
                //now copy original string collection to urself
                if(m_data->m_own_keys)
                {
                    // Now 2 m_strings point to same string_collection
                    m_strings=table.m_strings;
                }
                
                //now copy everything from other table to urself
                // NOTE: Should *not *use insert() here as it will insert key, which is a string, as well.
                // we operate index arrays directly
                size_t index;
                size_t collisions;
                size_t orig_index;
                bool first = true;
                for(iterator i = table.begin(); i != table.end(); i++)
                {
                    bool b = get_index(i.get_key(), index, collisions);
                    if(b)
                    {
                        // TODO: Error, hash table resizing problem
                    }
                    if(first)
                    {
                        m_data->m_first_index = index;
                        first = false;
                    }
                    else
                    {
                        m_next_index[m_data->m_last_index] = index;
                    }
                    
                    m_data->m_last_index = index;
                    m_next_index[index] = HASH_TABLE_INVALID_INDEX_;
                    
                    table.get_index(i.get_key(), orig_index);
                    m_values[index] = i.get_value();
                    m_offsets[index] = table.m_offsets[orig_index];
                    m_data->m_collisions += collisions;
                    m_data->m_quantity++;
                }
                if(m_data->m_own_keys)
                {
                    // Make sure the only string_collection is *not *destroyed
                    table.m_strings=NULL;
                }
            }
            
            // use after erasing a lot
            void compact() //changes string ptrs/offsets !!
            {
                //shallow copy this hash table to another one
                hash_table table(*this, true/*shallow*/);
                
                // Rename newly created hash_table
                char buf[PATH_MAX];
                table.rename(add_suffix(buf, name(), ".temp"), true);
                
                m_data_pool=new mem_pool(sizeof(data_t), m_name);
                m_data=m_data_pool->offptr<data_t>(0).get();
                
                //reset your own parameters
                m_data->valid_tag=VALID_TAG;
                m_data->m_size=17;
                m_data->m_quantity = 0; 
                m_data->m_collisions = 0;
                m_data->m_own_keys = table.m_data->m_own_keys;
                m_data->m_size_factor = table.m_data->m_size_factor;
                m_data->m_erased = 0;
                m_data->m_first_index = -1;
                m_data->m_last_index = -1;
                m_data->m_caseless = table.m_data->m_caseless;
                //m_own_keys unchanged
                //m_size_factor unchanged
                
                // Allocate memory, create a new string_collection
                m_strings=NULL;
                alloc();
                
                //now copy everything from other table to urself
                
                for(iterator i = table.begin(); i != table.end(); i++)
                {
                    bool b = insert(i.get_key(), i.get_value());
                    if(b)
                    {
                        // TODO: Error, hash table resizing problem
                    }
                }
            }
            
            /**
             * Variant of linear probing, for different key, incremental is different,
             * This does help if we have a large continuous key set
             * The hash table size is a prime number, that guarantees the probing can
             * cover whole table
             */
            inline size_t hash(KEY key, size_t &incr) const
            {
                // NOTE: caseless mode is not really working for UTF-8 strings on most platforms
                // so it's disabled until we find a solution
                // Also we need to deal with Latin accent marks, composition/decomposition, etc.
                // The better way is to pre-process all strings before they get here
                
                //size_t hn = m_caseless ? hash_function_caseless(key) : hash_function(key);
                size_t hn = hash_function(key);
                size_t size = m_data->m_size;
                incr =(hn %(size - 1)) + 1;
                return hn % size;
            }
            
            typedef int(*CompareFn)(const char*, const char*);

            mem_pool *m_data_pool;
            struct data_t {
                uint64_t valid_tag;
                size_t m_size;
                size_t m_quantity;
                size_t m_collisions;
                bool  m_own_keys;
                int m_size_factor;
                size_t m_erased;
                size_t  m_first_index;
                size_t  m_last_index;
                bool    m_caseless;
            };
            data_t *m_data;

            mem_pool *m_value_pool;
            mem_pool *m_offset_pool;
            mem_pool *m_next_index_pool;

            VALUE *m_values;
            OFFSET *m_offsets;
            size_t *m_next_index;
            string_collection *m_strings;

            char m_name[PATH_MAX];
            CompareFn m_compare_fn;
        };
        
        namespace detail {
            template<typename HASH_TABLE>
            inline void hash_iterator<HASH_TABLE>::operator++()
            {
                (void)(*this)++;
            }
            
            template<typename HASH_TABLE>
            inline hash_iterator<HASH_TABLE> hash_iterator<HASH_TABLE>::operator++(int)
            {
                
                m_index = m_table->next(m_index);
                
                return hash_iterator<HASH_TABLE>(m_index, m_table);
            }
            
            template<typename HASH_TABLE>
            inline typename HASH_TABLE::key_type hash_iterator<HASH_TABLE>::get_key() const
            {
                
                return m_table->get_key(m_table->get_offset(m_index));
            }
            
            template<typename HASH_TABLE>
            inline typename HASH_TABLE::mapped_type &hash_iterator<HASH_TABLE>::get_value() const
            {
                return m_table->get_value(m_index);
            }
            
            template<typename HASH_TABLE>
            inline OFFSET hash_iterator<HASH_TABLE>::get_offset() const
            {
                return m_table->get_offset(m_index);
            }
        }   // End of namespace detail
    }   // End of namespace common
}   // End of namespace argos
#endif
