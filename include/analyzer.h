//
//  analyzer.h
//  Argos
//
//  Created by Windoze on 12-7-12.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <string>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "common/term_list.h"

#ifndef Argos_analyzer_h
#define Argos_analyzer_h

namespace argos {
    namespace analyzer {
        /**
         * Analyzer analyses texts, breaks sentances into terms
         */
        class Analyzer {
        public:
            /**
             * Virtual destructor
             */
            virtual ~Analyzer(){}
            /**
             * Return name of the analyzer
             */
            virtual const char *get_name() const=0;
            /**
             * Generate term list for document string
             */
            virtual bool analyse_doc(const char *prefix, const std::string &src, common::simple_term_list_t &terms)=0;
            /**
             * Generate term list for query string
             */
            virtual bool analyse_query(const char *prefix, const std::string &src, common::simple_term_list_t &terms)=0;
            /**
             * Generate term list for query string
             */
            virtual bool analyse_query_simple(const char *prefix, const std::string &src, common::simple_term_list_t &terms)=0;
            /**
             * Generate term list for query string
             */
            virtual bool is_complex() const=0;
        };
        
        typedef boost::shared_ptr<Analyzer> analyzer_ptr_t;
        
        /**
         * Return default analyzer, which is used to parse query string in default namespace
         */
        analyzer_ptr_t get_default_analyzer();
        
        /**
         * Return analyzer with specific name
         */
        analyzer_ptr_t get_analyzer(const char *name);
    }   // End of namespace analyzer
}   // End of namespace analyzer

#endif
