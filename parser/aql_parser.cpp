//
//  parser.cpp
//  Argos
//
//  Created by Windoze on 12-7-20.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

/**
 start ::= statement
 
 statement ::= 'SELECT' field_list|'*' from_clause [match_clause] [where_clause] [sort_clause] [histo_clause]
 
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

