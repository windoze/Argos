//
//  value_parser.h
//  Argos
//
//  Created by Xu Chen on 12-8-10.
//  Copyright (c) 2012年 0d0a.com. All rights reserved.
//

//#define BOOST_SPIRIT_DEBUG

#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
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
                             | empty_value
                ;
                
                empty_value = boost::spirit::lit("null")
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
            boost::spirit::qi::rule<Iterator, common::NullValue(), boost::spirit::ascii::space_type> empty_value;
            boost::spirit::qi::rule<Iterator, common::Value(), boost::spirit::ascii::space_type> value;
            boost::spirit::qi::rule<Iterator, common::Value(), boost::spirit::ascii::space_type> simple_value;
            boost::spirit::qi::rule<Iterator, std::pair<double, double>(), boost::spirit::ascii::space_type> geo_location;
            boost::spirit::qi::rule<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> array;
            boost::spirit::qi::rule<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> empty_array;
            esc_string_parser<Iterator> string_;
            boost::spirit::qi::rule<Iterator, std::string()> content;
            boost::spirit::qi::real_parser<double, boost::spirit::qi::strict_real_policies<double> > real;
        };
        
        template <typename Iterator>
        struct value_list_parser : boost::spirit::qi::grammar<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> {
            value_list_parser() : value_list_parser::base_type(value_list) {
                value_list %= value % ','
                ;
            }
            value_parser<Iterator> value;
            boost::spirit::qi::rule<Iterator, common::value_list_t(), boost::spirit::ascii::space_type> value_list;
        };
        
        template<typename Iterator, typename Composer>
        struct expr_parser : boost::spirit::qi::grammar<Iterator, typename Composer::ExprComposer(), boost::spirit::ascii::space_type> {
            typedef typename Composer::ExprComposer ExprComposer;
            typedef typename Composer::FieldComposer FieldComposer;
            typedef typename Composer::ConstantComposer ConstantComposer;
            typedef typename Composer::DocOpComposer DocOpComposer;
            typedef typename Composer::FunctionComposer FunctionComposer;
            typedef typename Composer::OprandsComposer OprandsComposer;
            typedef std::pair<std::string, OprandsComposer> function_signature;
            typedef boost::tuple<std::string, boost::optional<std::string>, boost::optional<std::string> > docop_signature;
            
            expr_parser() : expr_parser::base_type(expression) {

                expression = constant
                           | function
                           | docop
                           | field
                ;
                
                constant = value
                ;
                
                function = boost::spirit::qi::as<function_signature>()[identifier >> '(' >> oprands >> ')']
                ;
                
                oprands %= expression % ','
                ;
                
                docop = boost::spirit::qi::as<docop_signature>()[docopname >> '(' >> -(identifier >> ',') >> -string_ >> ')']
                ;
                
                field = identifier
                ;
                
                identifier = boost::spirit::qi::char_("_a-zA-Z") >> *boost::spirit::qi::char_("_a-zA-Z0-9")
                ;

                docopname = '@' >> identifier
                ;
                
                //BOOST_SPIRIT_DEBUG_NODE(expression);
                //BOOST_SPIRIT_DEBUG_NODE(constant);
                //BOOST_SPIRIT_DEBUG_NODE(function);
            }
            boost::spirit::qi::rule<Iterator, ExprComposer(), boost::spirit::ascii::space_type> expression;
            boost::spirit::qi::rule<Iterator, ConstantComposer(), boost::spirit::ascii::space_type> constant;
            value_parser<Iterator> value;
            boost::spirit::qi::rule<Iterator, FunctionComposer(), boost::spirit::ascii::space_type> function;
            boost::spirit::qi::rule<Iterator, OprandsComposer(), boost::spirit::ascii::space_type> oprands;
            boost::spirit::qi::rule<Iterator, DocOpComposer(), boost::spirit::ascii::space_type> docop;
            boost::spirit::qi::rule<Iterator, FieldComposer(), boost::spirit::ascii::space_type> field;
            boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> identifier;
            boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> docopname;
            esc_string_parser<Iterator> string_;
        };
        
        template<typename Iterator, typename Composer>
        struct expr_list_parser : boost::spirit::qi::grammar<Iterator, typename Composer::ExprListComposer(), boost::spirit::ascii::space_type> {
            typedef typename Composer::ExprComposer ExprComposer;
            typedef typename Composer::ExprListComposer ExprListComposer;

            expr_list_parser() : expr_list_parser::base_type(expr_list) {
                expr_list %= expression % ','
                ;
            }
            
            boost::spirit::qi::rule<Iterator, ExprListComposer(), boost::spirit::ascii::space_type> expr_list;
            expr_parser<Iterator, Composer> expression;
        };

        template<typename Iterator, typename Composer>
        struct sort_criterion_parser : boost::spirit::qi::grammar<Iterator, typename Composer::sort_criterion(), boost::spirit::ascii::space_type> {
            typedef typename Composer::sort_criterion sort_criterion;
            
            struct sort_order_symbols : boost::spirit::qi::symbols<char, bool> {
                sort_order_symbols() {
                    add
                    ("ASC",false)
                    ("DESC",true)
                    ;
                }
            };
            
            sort_criterion_parser(): sort_criterion_parser::base_type(criterion) {
                criterion = expression >> ',' >> order
                ;
            }
            
            boost::spirit::qi::rule<Iterator, sort_criterion(), boost::spirit::ascii::space_type> criterion;
            expr_parser<Iterator, Composer> expression;
            sort_order_symbols order;
        };

        template<typename Iterator, typename Composer>
        struct sort_criteria_parser : boost::spirit::qi::grammar<Iterator, typename Composer::sort_criteria(), boost::spirit::ascii::space_type> {
            typedef typename Composer::sort_criteria sort_criteria;
            
            sort_criteria_parser(): sort_criteria_parser::base_type(criteria) {
                criteria %= criterion % ','
                ;
            }

            boost::spirit::qi::rule<Iterator, sort_criteria(), boost::spirit::ascii::space_type> criteria;
            sort_criterion_parser<Iterator, Composer> criterion;
        };

        template<typename Iterator, typename Composer>
        struct histo_parser : boost::spirit::qi::grammar<Iterator, typename Composer::histograms(), boost::spirit::ascii::space_type> {
            typedef typename Composer::ExprListComposer ExprListComposer;
            typedef typename Composer::histograms histograms;
            
            histo_parser() : histo_parser::base_type(histo) {
                histo %= expr_list % ';'
                ;
            }
            
            boost::spirit::qi::rule<Iterator, histograms(), boost::spirit::ascii::space_type> histo;
            expr_list_parser<Iterator, Composer> expr_list;
        };
    }   // End of namespace parser
}   // End of namespace argos

#endif
