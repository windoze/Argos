//
//  token_parser.h
//  Argos
//
//  Created by Windoze on 12-7-2.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>

#ifndef Argos_token_parser_h
#define Argos_token_parser_h

namespace argos {
    namespace common {
        const int TT_INVALID=0;
        const int TT_ID=1;
        const int TT_INT=2;
        const int TT_FLOAT=3;
        const int TT_STR=4;
        const int TT_LPAREN=5;
        const int TT_RPAREN=6;
        const int TT_COMMA=7;
        const int TT_ARRAY=8;
        const int TT_LBRACKET=9;
        const int TT_RBRACKET=10;
        const int TT_FPAIR=11;
        const int TT_AT=12;
        const int TT_SEMI=13;   // Semicolon
        
        struct token {
            const char *p;
            size_t sz;
            int type;
        };
        
        /**
         * Parse input str, returns next token, advance string pointer, decrease remaining length
         */
        token next_token(const char *&str, size_t &len);
        
        /**
         * Peek next token, doesn't change string pointer and remaining length, for LL(1) parser we're using
         */
        token peek_next_token(const char *str, size_t len);
    }   // End of namespace common
}   // End of namespace argos

#endif
