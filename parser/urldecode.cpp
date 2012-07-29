//
//  urldecode.cpp
//  Argos
//
//  Created by Windoze on 12-7-6.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>
#include <string.h>
#include <ctype.h>

namespace argos {
    namespace common {
        inline unsigned char hex_to_char(char c)
        {
            if (c>='0' && c<='9') {
                return c-'0';
            } else if(c>='A' && c<='F') {
                return c-'A'+0xA;
            } else if(c>='a' && c<='f') {
                return c-'a'+0xA;
            }
            return 0;
        }

        inline char hex_to_char(char c1, char c2)
        {
            return (char)((hex_to_char(c1)<<4) | hex_to_char(c2));
        }
        
        size_t urldecode_size(const char *source, size_t src_len)
        {
            if (src_len==0) {
                src_len=strlen(source);
            }
            size_t ret=0;
            for(size_t i = 0; i<src_len; i++, ret++)
            {
                if (source[i]=='%' && isxdigit(source[i + 1]) && isxdigit(source[i + 2])) {
                    i += 2;
                }
            }
            return ret;
        }

        size_t urldecode(char *desc, const char *source, size_t src_len)
        {
            if (src_len==0) {
                src_len=strlen(source);
            }
            if (!desc) {
                return urldecode_size(source, src_len);
            }
            size_t ret=0;
            for(size_t i = 0; i<src_len; i++)
            {
                switch(source[i])
                {
                    case '+':
                        desc[ret++] = ' ';
                        break;
                    case '%':
                        if(isxdigit(source[i + 1]) && isxdigit(source[i + 2]))
                        {
                            desc[ret++] = hex_to_char(source[i+1], source[i+2]);
                            i += 2;
                        }else {
                            desc[ret++] = '%';
                        }
                        break;
                    default:
                        desc[ret++] = source[i];
                        break;
                }
            }
            desc[ret]=0;
            return ret;
        }
    }   // End of namespace common
}   // End of namespace argos
