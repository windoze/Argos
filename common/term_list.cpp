//
//  term_list.cpp
//  Argos
//
//  Created by Windoze on 12-7-14.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <set>
#include "common/term_list.h"

namespace argos {
    namespace common {
        typedef std::set<term_t> term_set_t;
        
        void merge_term_list(simple_term_list_t &dest, const simple_term_list_t &src)
        {
            term_set_t s;
            for (simple_term_list_t::const_iterator i=dest.begin(); i!=dest.end(); ++i) {
                s.insert(*i);
            }
            for (simple_term_list_t::const_iterator i=src.begin(); i!=src.end(); ++i) {
                if (s.find(*i)==s.end()) {
                    dest.push_back(*i);
                };
            }
        }
        
        void merge_term_list(term_freq_list_t &dest, const term_freq_list_t &src)
        {
            for (term_freq_list_t::const_iterator i=src.begin(); i!=src.end(); ++i) {
                term_freq_list_t::iterator di=dest.find(i->first);
                if (di==dest.end()) {
                    dest[i->first]=i->second;
                } else {
                    di->second+=i->second;
                }
            }
        }
        
        void merge_term_list(term_freq_list_t &dest, const simple_term_list_t &src)
        {
            for (simple_term_list_t::const_iterator i=src.begin(); i!=src.end(); ++i) {
                term_freq_list_t::iterator di=dest.find(*i);
                if (di==dest.end()) {
                    dest[*i]=1;
                } else {
                    di->second++;
                }
            }
        }
        
        void add_term(const char *prefix, const term_t &term, simple_term_list_t &terms)
        {
            term_t ret(prefix);
            if (term.length()==0) {
                return;
            }
            terms.push_back(ret+term);
        }
        
        void add_term(const char *prefix, const term_t &term, term_freq_list_t &terms)
        {
            if (term.length()==0) {
                return;
            }
            term_t ret(prefix);
            ret+=term;
            term_list_t::iterator mi=terms.find(ret);
            if (mi==terms.end()) {
                terms[ret]=1;
            } else {
                mi->second++;
            }
        }
        
        void string_terms(const char *prefix, const term_t &str, term_list_t &terms)
        {
            term_t ret(prefix);
            size_t pstart=0;
            while (pstart!=str.npos) {
                size_t pend=str.find_first_of(" \t\n\r", pstart);
                if (pend==str.npos) {
                    add_term(prefix, term_t(str.begin()+pstart, str.end()), terms);
                    break;
                }
                add_term(prefix, term_t(str.begin()+pstart, str.begin()+pend), terms);
                pstart=pend+1;
            }
        }
        
        void int_terms(const char *prefix, int64_t v, term_list_t &terms)
        {
            char buf[100];
            sprintf(buf, "%lld", v);
            add_term(prefix, buf, terms);
        }
        
        void double_terms(const char *prefix, double v, term_list_t &terms)
        {
            char buf[100];
            sprintf(buf, "%f", v);
            add_term(prefix, buf, terms);
        }
        
        void geoloc_terms(const char *prefix, GeoLocationValue v, term_list_t &terms)
        {
            // TODO: GeoHash/GeoLayer terms
        }
        
        void array_terms(const char *prefix, common::ArrayValue v, term_list_t &terms)
        {
            switch (v.field_type()) {
                case common::FT_INT8:
                case common::FT_INT16:
                case common::FT_INT32:
                case common::FT_INT64:
                    for (int i=0; i<v.size(); i++) {
                        int_terms(prefix, v.get_element(i).number, terms);
                    }
                    break;
                case common::FT_FLOAT:
                case common::FT_DOUBLE:
                    for (int i=0; i<v.size(); i++) {
                        double_terms(prefix, v.get_element(i).dnumber, terms);
                    }
                    break;
                default:
                    break;
            }
        }
    }   // End of namespace common
}   // End of namespace argos