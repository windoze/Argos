//
//  TextAnalyzer.cpp
//  Argos
//
//  Created by Windoze on 12-7-12.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <unicode/utypes.h>
#include <unicode/normalizer2.h>
#include "utf8.h"
#include "analyzer.h"
#include "char_type.h"

namespace argos {
    namespace analyzer {
        namespace detail {
            UErrorCode errorCode;
            const Normalizer2 *norm2=Normalizer2::getInstance(NULL, "nfkc", UNORM2_DECOMPOSE, errorCode);
            
            typedef std::vector<std::string> phrases_t;
            typedef utf8::unchecked::iterator<std::string::const_iterator> u32iterator;
            
            inline void push_back_u32(std::string &str, u32iterator i)
            {
                u32iterator i1=i;
                i1++;
                utf8::unchecked::utf32to8(i, i1, back_inserter(str));
            }
            
            inline void one_gram(u32iterator it, const std::string &str, size_t n, std::vector<uint32_t> &ret)
            {
                ret.clear();
                for (size_t i=0; (it.base()!=str.end()) && (i<n); i++, it++) {
                    ret.push_back(*it);
                }
            }
            
            inline void push_back_gram(phrases_t &phrases, const std::vector<uint32_t> &gram)
            {
                std::string s;
                utf8::unchecked::utf32to8(gram.begin(), gram.end(), back_inserter(s));
                phrases.push_back(s);
            }
            
            void ngram(const std::string &str, phrases_t &phrases, size_t gram_len)
            {
                std::string::const_iterator it=str.begin();
                std::vector<uint32_t> gram;
                size_t generated_grams=0;
                for(u32iterator i(it); i.base()!=str.end(); ++i, ++generated_grams) {
                    one_gram(i, str, gram_len, gram);
                    if (gram.size()<gram_len) {
                        if (generated_grams==0) {
                            push_back_gram(phrases, gram);
                            return;
                        }
                        return;
                    }
                    push_back_gram(phrases, gram);
                }
            }
            
            inline size_t utf8strlen(const std::string &str)
            {
                size_t ret=0;
                for(u32iterator i(str.begin()); i.base()!=str.end(); ++i) {
                    ret++;
                }
                return ret;
            }
            
            void break_CJK(const std::string &str, phrases_t &phrases, size_t gram_len)
            {
                for (int i=1; i<=std::min(gram_len,utf8strlen(str)); i++) {
                    ngram(str, phrases, i);
                }
            }
            
        }   // End of namespace detail
        
        // Break text into phrases, each phrase has same character set
        void break_text(const std::string &str, detail::phrases_t &phrases, size_t gram_len, bool multi)
        {
            std::string ph;
            detail::CharType last_ct=detail::CT_UNKNOWN;
            UnicodeString decomp;
            std::string::const_iterator it=str.begin();
            for(detail::u32iterator i(it); i.base()!=str.end(); ++i) {
                uint32_t c=*i;
                detail::CharType ct=detail::get_char_type(c);
                if (ct==detail::CT_MARKER) {
                    // Ignore markers
                    continue;
                }
                if (ct!=last_ct)
                {
                    // We have a new phrase, push into phrases
                    if(!ph.empty()) {
                        if (last_ct==detail::CT_CJK) {
                            if (multi) {
                                detail::break_CJK(ph, phrases, gram_len);
                            } else {
                                detail::ngram(ph, phrases, gram_len);
                            }
                        } else {
                            phrases.push_back(ph);
                        }
                    }
                    last_ct=ct;
                    ph.clear();
                }
                // Add char into ph if it's a letter or number
                if (ct==detail::CT_CJK) {
                    detail::push_back_u32(ph, i);
                } else if (ct==detail::CT_LATIN || ct==detail::CT_NUMBER) {
                    // process latin letters, to NFKD, ignore markers, the to lower
                    if (detail::norm2->getDecomposition(c, decomp))
                    {
                        // c has a decomposition form
                        for(int32_t i=0; i<decomp.length(); ++i)
                        {
                            UChar dc=decomp[i];
                            if (detail::is_latin(dc) || detail::is_number(dc)) {
                                ph.push_back(u_tolower(dc));
                            }
                            // FIXME: handle something like "3/4"(u00BE), which should be broken into 2 phrases "3" and "4" instead of "34"
                        }
                    } else {
                        // FIXME: shouldn't be here, for anything unknown
                        ph.push_back(u_tolower(c));
                    }
                }
            }

            // last phrase
            if(!ph.empty()) {
                if (last_ct==detail::CT_CJK) {
                    if (multi) {
                        detail::break_CJK(ph, phrases, gram_len);
                    } else {
                        detail::ngram(ph, phrases, gram_len);
                    }
                } else {
                    phrases.push_back(ph);
                }
            }
        }
    }   // End of namespace analyzer
}   // End of namespace common