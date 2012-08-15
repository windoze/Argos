//
//  value_parser.h
//  Argos
//
//  Created by Xu Chen on 12-8-10.
//  Copyright (c) 2012年 0d0a.com. All rights reserved.
//

//#define BOOST_SPIRIT_DEBUG

#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/qi.hpp>
#include "common/value.h"

#ifndef Argos_value_parser_h
#define Argos_value_parser_h

namespace argos {
    namespace parser {
        template <typename Iterator>
        struct esc_string_parser : boost::spirit::qi::grammar<Iterator,std::string() > {
            struct esc_parser : boost::spirit::qi::symbols<char, char> {
                esc_parser() {
                    add
                        ("\\\"",'"')
                        ("\\\\",'\\')
                        ("\\a",'\a')
                        ("\\b",'\b')
                        ("\\f",'\f')
                        ("\\n",'\n')
                        ("\\r",'\r')
                        ("\\t",'\t')
                        ("\\v",'\v')
                    ;
                }
            };
            
            esc_string_parser() : esc_string_parser::base_type(str) {
                str %=  '"' >> *chr >> '"';
                chr %=  escaped | (boost::spirit::qi::char_ - '"');
            }
            esc_parser escaped;
            boost::spirit::qi::rule<Iterator,std::string()> str;
            boost::spirit::qi::rule<Iterator,char()> chr;
        };

        template <typename Iterator>
        struct value_parser : boost::spirit::qi::grammar<Iterator, common::Value(), boost::spirit::ascii::space_type> {
            template<typename T>
            struct strict_real_policies : boost::spirit::qi::real_policies<T> {
                static bool const expect_dot = true;
            };
            
            value_parser() : value_parser::base_type(value) {
                value = simple_value
                      | string_
                      | array
                ;
                
                simple_value = real
                             | i64
                             | geo_location
                ;
                
                geo_location = '<' >> boost::spirit::double_ >> ',' >> boost::spirit::double_ >> '>'
                ;
                
                // We don't allow embedded array/string
                array %= '[' >> simple_value % ',' >> ']'
                       | boost::spirit::lit('[') >> boost::spirit::lit(']');
                ;
                
                //BOOST_SPIRIT_DEBUG_NODE(array);
            }
            
            // NOTE: Clang and GCC have different int64_t definitions, GCC uses `long`, Clang uses `long long`
            // Uses boost::spirit::long_long will cause GCC fail to compile due to ambiguous conversion
            // And uses boost::spirit::long_ will mess Clang, WTF
            boost::spirit::qi::int_parser<int64_t> i64;
            boost::spirit::qi::rule<Iterator, common::Value(), boost::spirit::ascii::space_type> value;
            boost::spirit::qi::rule<Iterator, common::Value(), boost::spirit::ascii::space_type> simple_value;
            boost::spirit::qi::rule<Iterator, std::pair<double, double>(), boost::spirit::ascii::space_type> geo_location;
            boost::spirit::qi::rule<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> array;
            boost::spirit::qi::rule<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> empty_array;
            esc_string_parser<Iterator> string_;
            boost::spirit::qi::rule<Iterator, std::string()> content;
            boost::spirit::qi::real_parser<double, strict_real_policies<double> > real;
        };
        
        template <typename Iterator>
        struct value_list_parser : boost::spirit::qi::grammar<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> {
            value_list_parser() : value_list_parser::base_type(value_list) {
                value_list %= value >> *(',' >> value)
                ;
            }
            value_parser<Iterator> value;
            boost::spirit::qi::rule<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> value_list;
        };
    }   // End of namespace parser
}   // End of namespace argos

#endif
