//
//  analyser.cpp
//  Argos
//
//  Created by Windoze on 12-7-12.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <map>
#include "analyzer.h"

namespace argos {
    namespace analyzer {
        
        void break_text(const std::string &str, std::vector<std::string> &phrases, size_t gram_len, bool multi);        

        namespace detail {
            inline void add_term(const char *prefix, const std::string &term, std::vector<std::string> &terms)
            {
                std::string ret(prefix);
                if (term.length()==0) {
                    return;
                }
                terms.push_back(ret+term);
            }

            class TokenAnalyzer : public Analyzer
            {
            public:
                TokenAnalyzer(){}
                
                virtual const char *get_name() const
                {
                    return "token";
                }
                
                virtual bool analyse_doc(const char *prefix, const std::string &str, common::simple_term_list_t &terms)
                {
                    std::string ret(prefix);
                    size_t pstart=0;
                    while (pstart!=str.npos) {
                        size_t pend=str.find_first_of(" \t\n\r", pstart);
                        if (pend==str.npos) {
                            common::add_term(prefix, std::string(str.begin()+pstart, str.end()), terms);
                            break;
                        }
                        common::add_term(prefix, std::string(str.begin()+pstart, str.begin()+pend), terms);
                        pstart=pend+1;
                    }
                    return true;
                }
                virtual bool analyse_query(const char *prefix, const std::string &str, common::simple_term_list_t &terms)
                {
                    return analyse_doc(prefix, str, terms);
                }
                virtual bool analyse_query_simple(const char *prefix, const std::string &src, common::simple_term_list_t &terms){ return true; }
                virtual bool is_complex() const { return false; }
            };

            class TextAnalyzer : public Analyzer
            {
            public:
                TextAnalyzer(){}
                
                virtual const char *get_name() const
                {
                    return "text";
                }
                
                virtual bool analyse_doc(const char *prefix, const std::string &str, common::simple_term_list_t &terms)
                {
                    common::simple_term_list_t tl;
                    break_text(str, tl, 1, true);
                    for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                        std::string s(prefix);
                        s+=*i;
                        terms.push_back(s);
                    }
                    return true;
                }
                virtual bool analyse_query(const char *prefix, const std::string &str, common::simple_term_list_t &terms)
                {
                    return analyse_doc(prefix, str, terms);
                }
                virtual bool analyse_query_simple(const char *prefix, const std::string &src, common::simple_term_list_t &terms){ return true; }
                virtual bool is_complex() const { return false; }
            };
            
            class BiGramAnalyzer : public Analyzer
            {
            public:
                BiGramAnalyzer(){}
                
                virtual const char *get_name() const
                {
                    return "bigram";
                }
                
                virtual bool analyse_doc(const char *prefix, const std::string &str, common::simple_term_list_t &terms)
                {
                    common::simple_term_list_t tl;
                    break_text(str, tl, 2, true);
                    for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                        std::string s(prefix);
                        s+=*i;
                        terms.push_back(s);
                    }
                    return true;
                }
                virtual bool analyse_query(const char *prefix, const std::string &str, common::simple_term_list_t &terms)
                {
                    common::simple_term_list_t tl;
                    break_text(str, tl, 2, false);
                    for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                        std::string s(prefix);
                        s+=*i;
                        terms.push_back(s);
                    }
                    return true;
                }
                virtual bool analyse_query_simple(const char *prefix, const std::string &str, common::simple_term_list_t &terms){
                    common::simple_term_list_t tl;
                    break_text(str, tl, 1, false);
                    for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                        std::string s(prefix);
                        s+=*i;
                        terms.push_back(s);
                    }
                    return true;
                }
                virtual bool is_complex() const { return true; }
            };
        }   // End of namespace detail
        
        typedef std::map<std::string, analyzer_ptr_t> analyzer_map_t;
        analyzer_map_t analyzer_map;
        struct AnalyzerLoader {
            AnalyzerLoader() {
                // Initialize TokenAnalyzer
                analyzer_ptr_t a=analyzer_ptr_t(new detail::TokenAnalyzer);
                analyzer_map[a->get_name()]=a;
                a=analyzer_ptr_t(new detail::TextAnalyzer);
                analyzer_map[a->get_name()]=a;
                a=analyzer_ptr_t(new detail::BiGramAnalyzer);
                analyzer_map[a->get_name()]=a;
            }
        } the_loader;
        
        analyzer_ptr_t get_analyzer(const char *name)
        {
            analyzer_map_t::const_iterator i=analyzer_map.find(name);
            if (i==analyzer_map.end()) {
                return analyzer_ptr_t();
            }
            return i->second;
        }

        analyzer_ptr_t get_default_analyzer()
        {
            return get_analyzer("text");
        }
    }   // End of namespace analyzer
}   // End of namespace argos