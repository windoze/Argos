//
//  CharType.h
//  wordseg
//
//  Created by Windoze on 12-2-9.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifndef wordseg_CharType_h
#define wordseg_CharType_h

#include <unicode/uchar.h>

namespace argos {
    namespace analyzer {
        namespace detail {
            typedef enum _CharType {
                CT_SEPERATOR,   // Seperators, whitespaces, punctuations, anything consider as a word-break.
                CT_MARKER,      // something like accent marks
                CT_NUMBER,
                CT_LATIN,
                CT_CJK,
                CT_UNKNOWN      // Max
            } CharType;
            
            inline bool is_CJK(UChar32 c)
            {
                UBlockCode bc=ublock_getCode(c);
                return 
                (u_charType(c)==U_OTHER_LETTER)
                && (
                    (bc==UBLOCK_CJK_UNIFIED_IDEOGRAPHS)
                    || (bc==UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A)
                    || (bc==UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS)
                    || (bc==UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B)
                    || (bc==UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C)
                    || (bc==UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D)
                    || (bc==UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT)
                    );
            }
            
            inline bool is_seperator(UChar32 c)
            {
                int8_t ct=u_charType(c);
                return (ct==U_SPACE_SEPARATOR)
                || (ct==U_LINE_SEPARATOR)
                || (ct==U_PARAGRAPH_SEPARATOR)
                || (ct==U_CONTROL_CHAR)
                || (ct==U_FORMAT_CHAR)
                || (ct==U_PRIVATE_USE_CHAR)
                || (ct==U_SURROGATE)
                || (ct==U_DASH_PUNCTUATION)
                || (ct==U_START_PUNCTUATION)
                || (ct==U_END_PUNCTUATION)
                || (ct==U_CONNECTOR_PUNCTUATION)
                || (ct==U_OTHER_PUNCTUATION)
                || (ct==U_MATH_SYMBOL)
                || (ct==U_CURRENCY_SYMBOL)
                || (ct==U_MODIFIER_SYMBOL)
                || (ct==U_OTHER_SYMBOL)
                || (ct==U_INITIAL_PUNCTUATION)
                || (ct==U_FINAL_PUNCTUATION);
            }
            
            inline bool is_number(UChar32 c)
            {
                int8_t ct=u_charType(c);
                return (ct==U_DECIMAL_DIGIT_NUMBER)
                || (ct==U_LETTER_NUMBER)
                || (ct==U_OTHER_NUMBER);
            }
            
            inline bool is_latin(UChar32 c)
            {
                UBlockCode bc=ublock_getCode(c);
                return (
                        (u_charType(c)==U_UPPERCASE_LETTER)
                        || (u_charType(c)==U_LOWERCASE_LETTER)
                        || (u_charType(c)==U_OTHER_LETTER)
                        )
                && (
                    (bc==UBLOCK_BASIC_LATIN)
                    || (bc==UBLOCK_LATIN_1_SUPPLEMENT)
                    || (bc==UBLOCK_LATIN_EXTENDED_A)
                    || (bc==UBLOCK_LATIN_EXTENDED_B)
                    );
            }
            
            inline bool is_marker(UChar32 c)
            {
                int8_t ct=u_charType(c);
                return (ct==U_NON_SPACING_MARK)
                || (ct==U_ENCLOSING_MARK)
                || (ct==U_COMBINING_SPACING_MARK);
            }
            
            inline CharType get_char_type(UChar32 c)
            {
                if(is_latin(c))
                    return CT_LATIN;
                if(is_number(c))
                    return CT_NUMBER;
                if(is_seperator(c))
                    return CT_SEPERATOR;
                if(is_marker(c))
                    return CT_MARKER;
                if(is_CJK(c))
                    return CT_CJK;
                // Anything unknown consider as a seperator
                return CT_SEPERATOR;
            }
        }   // End of namespace detail
    }   // End of namespace analyzer
}   // End of namespace argos

#endif
