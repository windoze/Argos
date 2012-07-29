//
//  util.h
//  Argos
//
//  Created by Windoze on 12-7-12.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>

#ifndef Argos_util_h
#define Argos_util_h

namespace argos {
    namespace common {
        class Operator;

        /**
         * Parse input string to find an integer, adjust string pointer and length
         */
        int64_t atoi(const char *&str, size_t &len);

        /**
         * Parse input string to find an unsigned integer in hex, adjust string pointer and length
         */
        uint64_t hatoui(const char *&str, size_t &len);

        /**
         * Parse input string to find a float/double number, adjust string pointer and length
         */
        double atof(const char *&str, size_t &len);

        /**
         * URLDecoding
         *
         * @param desc destination buffer to store decoded string, function returns decodec size without actually decoding if this param is NULL
         * @param source source URL encoded string
         * @param src_len length in bytes of the source string, 0 for whole string with ending '\0'
         */
        size_t urldecode(char *desc, const char *source, size_t src_len=0);

        /**
         * Unescape the string into buf
         */
        void unescape(char *buf, const char *str, size_t len);
        
        /**
         * Return current time, return value is the number seconds since the epoch(1970-1-1)
         */
        volatile double get_time();
        
        /**
         * Register a new operator
         */
        void add_operator(Operator *op);
    }   // End of namespace common
}   // End of namespace argos

#endif
