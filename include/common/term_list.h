//
//  term_list.h
//  Argos
//
//  Created by Windoze on 12-7-14.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <string>
#include <vector>
#include <map>
#include "common/value.h"

#ifndef Argos_term_list_h
#define Argos_term_list_h

namespace argos {
    namespace common {
        // TODO: Use mem_pool
        //typedef std::string term_t;
        typedef arstring term_t;
        typedef std::vector<term_t> simple_term_list_t;
        typedef std::map<term_t, uint32_t> term_freq_list_t;
        
        typedef term_freq_list_t term_list_t;

        void merge_term_list(simple_term_list_t &dest, const simple_term_list_t &src);
        void merge_term_list(term_freq_list_t &dest, const term_freq_list_t &src);
        void merge_term_list(term_freq_list_t &dest, const simple_term_list_t &src);
        void add_term(const char *prefix, const term_t &term, simple_term_list_t &terms);
        void add_term(const char *prefix, const term_t &term, term_freq_list_t &terms);
        void string_terms(const char *prefix, const term_t &str, term_list_t &terms);
        void int_terms(const char *prefix, int64_t v, term_list_t &terms);
        void double_terms(const char *prefix, double v, term_list_t &terms);
        void geoloc_terms(const char *prefix, GeoLocationValue v, term_list_t &terms);
        void array_terms(const char *prefix, common::ArrayValue v, term_list_t &terms);
    }   // End of common
}   // End of argos

#endif
