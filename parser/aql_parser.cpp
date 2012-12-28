//
//  parser.cpp
//  Argos
//
//  Created by Windoze on 12-7-20.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#if 0

#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/qi.hpp>
#include "query/argos_query.h"

namespace argos {
    namespace aql {
        
        struct match_composer {
            struct str {
                
            };
            
            struct identifier {
                
            };
            
            struct term {
                
            };
            
            struct factor {
                
            };
            
            struct match_all {
                
            };
        };
        
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
        struct match_parser : boost::spirit::qi::grammar<Iterator, match_composer(), boost::spirit::ascii::space_type> {
            match_parser() : match_parser::base_type(match) {
                match = "MATCH" >> (match_expr | match_all)
                ;
                
                match_all = '*'
                ;
                
                match_expr %= match_term % "OR"
                ;
                
                match_term %= match_factor % "AND"
                ;
                
                match_factor = ('(' >> match_expr >> ')' )
                             | string_
                             | (identifier >> "HAS" >> string_)
                ;
                
                match_all = '*'
                ;
                
                identifier = boost::spirit::qi::char_("_A-Za-z") >> *(boost::spirit::qi::char_("_A-Za-z0-9"))
                ;
                
                //BOOST_SPIRIT_DEBUG_NODE(match);
            }
            
            boost::spirit::qi::rule<Iterator, match_composer(), boost::spirit::ascii::space_type> match;
            boost::spirit::qi::rule<Iterator, match_composer(), boost::spirit::ascii::space_type> match_expr;
            boost::spirit::qi::rule<Iterator, match_composer::term(), boost::spirit::ascii::space_type> match_term;
            boost::spirit::qi::rule<Iterator, match_composer::factor(), boost::spirit::ascii::space_type> match_factor;
            boost::spirit::qi::rule<Iterator, match_composer::match_all(), boost::spirit::ascii::space_type> match_all;
            esc_string_parser<Iterator> string_;
            boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> identifier;
        };
    }   // End of namespace aql
}   // End of namespace argos

#endif

/**
 start ::= statement
 
 sel_statement ::= 'SELECT' field_list|'*' from_clause [match_clause] [where_clause] [sort_clause] [histo_clause]
 
 field_list ::= named_expression_list
 
 named_expression_list ::= named_expression[',' named_expression]*
 
 named_expression ::= expression ['AS' identifier]
 
 match_clause ::= 'MATCH' (match_expr | '*')
 
 from_clause ::= 'FROM' index_name
 
 index_name ::= identifier
 
 match_expr ::= match_term ['OR' match_term]*
 
 match_term ::= match_factor ['AND' match_factor]*
 
 match_factor ::= '(' match_expr ')'
 | string
 | field_name 'HAS' string
 
 where_clause ::= 'WHERE' expression
 
 sort_clause ::= sort_crit [',' sort_crit]*
 
 sort_crit ::= expression ['ASC'|'DESC']
 
 histo_clause ::= 'HISTO' expression_list
 
 expression_list ::= expression [',' expression]*
 
 expression ::= cond
 
 cond ::= summand '?' summand ':' summand
 | summand
 
 summand ::= term '+'|'-'|'OR'|'XOR' term
 | term
 
 term ::= factor '*'|'/'|'%'|'AND' factor
 | factor
 
 factor ::= '(' expression ')'
 | function
 | doc_op
 | field_name
 | constant
 
 function ::= identifier '(' parameter_list ')'
 
 doc_op ::= '@'identifier '(' parameter_list ')'
 
 parameter_list ::= expression_list
 
 constant ::= number
 | string
 | array
 
 number ::= integer | float
 
 string ::= '"'.*'"'
 
 array ::= '[' ']'
 | '[' number_list ']'
 
 number_list ::= number [',' number]*
 
 integer ::= ['+'|'-']'[0-9]'+
 
 float ::= ...
 
 identifier ::= ['@']'[_a-zA-Z][_a-zA-Z0-9]'
 **/
