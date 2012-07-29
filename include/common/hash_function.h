//
//  hash_function.h
//  Argos
//
//  Created by Windoze on 12-6-13.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stdint.h>

#ifndef Argos_hash_function_h
#define Argos_hash_function_h

/**
 * hash_function generate 64bits hash value for strings and integrals
 */

namespace argos {
    namespace common {
        namespace detail {
            /*
             Different string hash functions
             Defauls to DJB hash, it's practically better than others.
             NOTE: Cityhash from google ( http://code.google.com/p/cityhash/ ) could be another choice,
             */

            // SDBM Hash Function
            inline uint64_t SDBMHash(const char *str)
            {
                uint64_t hash = 0;
                
                while (*str)
                {
                    // equivalent to: hash = 65599*hash + (*str++);
                    hash = (*str++) + (hash << 6) + (hash << 16) - hash;
                }
                
                return hash;
            }
            
            // RS Hash Function
            inline uint64_t RSHash(const char *str)
            {
                uint64_t b = 378551;
                uint64_t a = 63689;
                uint64_t hash = 0;
                
                while (*str)
                {
                    hash = hash * a + (*str++);
                    a *= b;
                }
                
                return hash;
            }
            
            // JS Hash Function
            inline uint64_t JSHash(const char *str)
            {
                uint64_t hash = 1315423911;
                
                while (*str)
                {
                    hash ^= ((hash << 5) + (*str++) + (hash >> 2));
                }
                
                return hash;
            }
            
            // P. J. Weinberger Hash Function
            inline uint64_t PJWHash(const char *str)
            {
                uint64_t BitsInUnignedInt = (uint64_t)(sizeof(uint64_t) * 8);
                uint64_t ThreeQuarters	= (uint64_t)((BitsInUnignedInt  * 3) / 4);
                uint64_t OneEighth		= (uint64_t)(BitsInUnignedInt / 8);
                uint64_t HighBits		 = (uint64_t)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
                uint64_t hash			 = 0;
                uint64_t test			 = 0;
                
                while (*str)
                {
                    hash = (hash << OneEighth) + (*str++);
                    if ((test = hash & HighBits) != 0)
                    {
                        hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
                    }
                }
                
                return hash;
            }
            
            // ELF Hash Function
            inline uint64_t ELFHash(const char *str)
            {
                uint64_t hash = 0;
                uint64_t x	= 0;
                
                while (*str)
                {
                    hash = (hash << 4) + (*str++);
                    if ((x = hash & 0xF0000000L) != 0)
                    {
                        hash ^= (x >> 24);
                        hash &= ~x;
                    }
                }
                
                return hash;
            }
            
            // BKDR Hash Function
            inline uint64_t BKDRHash(const char *str)
            {
                uint64_t seed = 131; // 31 131 1313 13131 131313 etc..
                uint64_t hash = 0;
                
                while (*str)
                {
                    hash = hash * seed + (*str++);
                }
                
                return hash;
            }
            
            // DJB Hash Function
            inline uint64_t DJBHash(const char *str)
            {
                uint64_t hash = 5381;
                
                while (*str)
                {
                    hash += (hash << 5) + (*str++);
                }
                
                return hash;
            }
            
            // AP Hash Function
            inline uint64_t APHash(const char *str)
            {
                uint64_t hash = 0;
                int i;
                
                for (i=0; *str; i++)
                {
                    if ((i & 1) == 0)
                    {
                        hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
                    }
                    else
                    {
                        hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
                    }
                }
                
                return hash;
            }
        }   // end of namespace detail

        /**
         * Generate 64bit hash for strings
         */
        inline uint64_t hash_function(const char *str)
        {
            return detail::DJBHash(str);
        }

        /**
         * Generate 64bit hash for integers
         *
         * NOTE: key set is likely to be continuous, by rotating LSBits and MSBits, continuous key set is scatterred
         * number of effective bits is *not* reduced, key space remains
         */
        inline uint64_t hash_function( unsigned long long key)
        {
            uint64_t hash = (key<<7)+key + ((key >> 28 ) & 0xffff);
            return uint64_t(hash);
        }
    }   // End of namespace common
}   // End of namespace argos

#endif
