//
//  aton.cpp
//  Argos
//
//  Created by Windoze on 12-6-29.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>

namespace argos {
    namespace common {
        void unescape(char *buf, const char *str, size_t len)
        {
            str++;
            len-=2;
            size_t n=0;
            for (size_t i=0; i<len; i++) {
                if (str[i]=='\\') {
                    // TODO: Unescape '\t', '\r', and '\n'
                    i++;
                }
                buf[n]=str[i];
                n++;
            }
            buf[n]=0;
        }
        
        namespace detail {
            inline bool IS_DIGIT(char c) { return c>='0' && c<='9'; }
        }   // End of namespace detail
        // convert string to integer
        const char *atoi(const char *first, const char *last, int64_t *out)
        {
            int sign = 1;
            if (first != last)
            {
                if (*first == '-')
                {
                    sign = -1;
                    ++first;
                }
                else if (*first == '+')
                {
                    ++first;
                }
            }
            
            int64_t result = 0;
            for (; first != last && detail::IS_DIGIT(*first); ++first)
            {
                result = 10 * result + (*first - '0');
            }
            *out = result * sign;
            
            return first;
        }
        
        // convert hexadecimal string to unsigned integer
        const char *hatoui(const char *first, const char *last, uint64_t *out)
        {
            uint64_t result = 0;
            for (; first != last; ++first)
            {
                int digit;
                if (detail::IS_DIGIT(*first))
                {
                    digit = *first - '0';
                }
                else if (*first >= 'a' && *first <= 'f')
                {
                    digit = *first - 'a' + 10;
                }
                else if (*first >= 'A' && *first <= 'F')
                {
                    digit = *first - 'A' + 10;
                }
                else
                {
                    break;
                }
                result = 16 * result + digit;
            }
            *out = result;
            
            return first;
        }
        
        // convert string to floating point
        const char *atof(const char *first, const char *last, double *out)
        {
            // sign
            double sign = 1;
            if (first != last)
            {
                if (*first == '-')
                {
                    sign = -1;
                    ++first;
                }
                else if (*first == '+')
                {
                    ++first;
                }
            }
            
            // integer part
            float result = 0;
            for (; first != last && detail::IS_DIGIT(*first); ++first)
            {
                result = 10 * result + (*first - '0');
            }
            
            // fraction part
            if (first != last && *first == '.')
            {
                ++first;
                
                double inv_base = 0.1f;
                for (; first != last && detail::IS_DIGIT(*first); ++first)
                {
                    result += (*first - '0') * inv_base;
                    inv_base *= 0.1f;
                }
            }
            
            // result w\o exponent
            result *= sign;
            
            // exponent
            bool exponent_negative = false;
            int exponent = 0;
            if (first != last && (*first == 'e' || *first == 'E'))
            {
                ++first;
                
                if (*first == '-')
                {
                    exponent_negative = true;
                    ++first;
                }
                else if (*first == '+')
                {
                    ++first;
                }
                
                for (; first != last && detail::IS_DIGIT(*first); ++first)
                {
                    exponent = 10 * exponent + (*first - '0');
                }
            }
            
            if (exponent)
            {
                double power_of_ten = 10;
                for (; exponent > 1; exponent--)
                {
                    power_of_ten *= 10;
                }
                
                if (exponent_negative)
                {
                    result /= power_of_ten;
                }
                else
                {
                    result *= power_of_ten;
                }
            }
            
            *out = result;
            
            return first;
        }   // End of namespace detail
        
        int64_t atoi(const char *&str, size_t &len)
        {
            int64_t ret=0;
            const char *p=atoi(str, str+len, &ret);
            len-=(p-str);
            str=p;
            return ret;
        }
        
        uint64_t hatoui(const char *&str, size_t &len)
        {
            uint64_t ret=0;
            const char *p=hatoui(str, str+len, &ret);
            len-=(p-str);
            str=p;
            return ret;
        }
        
        double atof(const char *&str, size_t &len)
        {
            double ret=0;
            const char *p=atof(str, str+len, &ret);
            len-=(p-str);
            str=p;
            return ret;
        }
    }   // End of namespace common
}   // End of namespace argos

