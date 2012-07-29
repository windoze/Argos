//
//  token_parser.cpp
//  Argos
//
//  Created by Windoze on 12-7-2.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stdint.h>
#include "common/util.h"
#include "common/token_parser.h"

namespace argos{
    namespace common {
        inline bool is_id(char c)
        {
            return c=='_' || (c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9');
        }
        
        inline bool is_num(char c)
        {
            return (c>='0' && c<='9') || c=='.';
        }
        
        size_t parse_identifier(const char *&str, size_t &len)
        {
            size_t ret=0;
            while (*str && len>0 && is_id(*str)) {
                str++;
                ret++;
                len--;
            }
            return ret;
        }
        
        size_t parse_number(const char *&str, size_t &len, int &token_type)
        {
            const char *s=str;
            size_t l=len;
            atof(s, l);
            size_t consumed=len-l;
            token_type=TT_INT;
            for (int i=0; i<consumed; i++) {
                if (str[i]=='.' || str[i]=='e' || str[i]=='E') {
                    // This is a float/double
                    token_type=TT_FLOAT;
                    break;
                }
            }
            // This is a integer
            str=s;
            len=l;
            return consumed;
        }
        
        size_t parse_string(const char *&str, size_t &len)
        {
            size_t ret=0;
            // Skip first '"'
            str++;
            len--;
            // TODO: Escape seq
            while (*str && len>0 && *str!='"') {
                if (*str=='\\') {
                    // Start of escape seq
                    if (len>1) {
                        // Check next char
                        if (*(str+1)=='\\' || *(str+1)=='"' || *(str+1)=='\'') {
                            str++;
                            ret++;
                            len--;
                        } else {
                            // ERROR: Wrong escape seq
                            return size_t(-1);
                        }
                    } else {
                        return size_t(-1);
                    }
                }
                str++;
                ret++;
                len--;
            }
            // There is another '"'
            ret+=2;
            str++;
            len--;
            return ret;
        }
        
        size_t parse_array(const char *&str, size_t &len)
        {
            // TODO: This is wrong!!! i.e. [1,2,"we have a ']' here"]
            size_t ret=0;
            // Skip first '['
            str++;
            len--;
            while (*str && len>0 && *str!=']') {
                str++;
                ret++;
                len--;
            }
            // There is another ']'
            ret+=2;
            str++;
            len--;
            return ret;
        }
        
        // float pair is like '<f1,f2>', which is used to represent geolocation
        size_t parse_fpair(const char *&str, size_t &len, int &token_type)
        {
            size_t ret=0;
            
            const char *p=str;
            size_t l=len;
            
            // Skip first '<'
            p++;
            l--;
            
            token t=next_token(p, l);
            if ((t.type!=TT_FLOAT) && (t.type!=TT_INT)) {
                token_type=TT_INVALID;
                return 0;
            }
            t=next_token(p, l);
            if (t.type!=TT_COMMA) {
                token_type=TT_INVALID;
                return 0;
            }
            t=next_token(p, l);
            if ((t.type!=TT_FLOAT) && (t.type!=TT_INT)) {
                token_type=TT_INVALID;
                return 0;
            }
            if (l==0) {
                token_type=TT_INVALID;
                return 0;
            }
            if (*p!='>') {
                token_type=TT_INVALID;
                return 0;
            }
            // Skip last '>'
            p++;
            l--;
            ret=len-l;
            str=p;
            len=l;
            token_type=TT_FPAIR;
            return ret;
        }
        
        token next_token(const char *&str, size_t &len)
        {
            token tok;
            tok.type=TT_INVALID;
            tok.p=str;
            tok.sz=0;
            if (len==0 || !str || !str[0]) {
                return tok;
            }
            
            // Skip leading whitespace
            while (*str==' ' || *str=='\t' || *str=='\n' || *str=='\r') {
                str++;
                len--;
            }
            tok.p=str;
            
            char c=*str;
            if (c=='(') {
                tok.sz=1;
                tok.type=TT_LPAREN;
                str++;
                len--;
                return tok;
            } else if (c==')') {
                tok.sz=1;
                tok.type=TT_RPAREN;
                str++;
                len--;
                return tok;
            } else if (c==',') {
                tok.sz=1;
                tok.type=TT_COMMA;
                str++;
                len--;
                return tok;
            } else if (c==';') {
                tok.sz=1;
                tok.type=TT_SEMI;
                str++;
                len--;
                return tok;
            } else if (c=='<') {
                tok.sz=parse_fpair(str, len, tok.type);
            } else if ((c>='0' && c<='9') || c=='-' || c=='.') {
                // Parse number, integer or float
                tok.sz=parse_number(str, len, tok.type);
            } else if (c=='"') {
                tok.sz=parse_string(str, len);
                tok.type=TT_STR;
            } else if (c=='[') {
                tok.sz=parse_array(str, len);
                tok.type=TT_ARRAY;
            } else if (is_id(c)) {
                // Parse Identifier
                tok.sz=parse_identifier(str, len);
                tok.type=TT_ID;
            } else if (c=='@') {
                tok.sz=1;
                tok.type=TT_AT;
                str++;
                len--;
            }
            //len-=tok.sz;
            return tok;
        }
        
        token peek_next_token(const char *str, size_t len)
        {
            // LL(1), Look ahead 1 token
            token t=next_token(str, len);
            return t;
        }
    }   // End of namespace common
}   // End of namespace argos
