//
//  expr_node.h
//  Argos
//
//  Created by Windoze on 12-6-19.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/evaluatable.h"

#ifndef Argos_expr_node_h
#define Argos_expr_node_h

namespace argos {
    namespace common {
        // TODO: use mem_pool_allocator
        typedef field_list_t oprands_t;

        /**
         * class Operator is abstract base class of all operators
         */
        class Operator {
        public:
            /**
             * Virtual destructor
             */
            virtual ~Operator(){}

            /**
             * Return name of the operator
             */
            virtual const char *get_name() const=0;

            /**
             * Return true if this operator always returns same value with same oprants
             *
             * This function is used during optimization, if the operator can be folded, optimizer
             * will use pre-calculated value to replace corresponding Evaluatable
             */
            virtual bool const_foldable() const=0;

            /**
             * Syntax check if the operator is given correct number of oprands
             */
            virtual bool validate_arity(size_t arity) const=0;
            
            /**
             * Calculate result with given oprands and context
             */
            virtual common::Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const=0;
            
            /**
             * Return true if the operator needs state and need to be cloned
             */
            virtual bool stateful() const { return false; }

            /**
             * Return a clone of this operator
             */
            virtual Operator *clone() { return NULL; }
        };
        
        /**
         * class ExprNode represents an expression
         */
        class ExprNode : public Evaluatable {
        public:
            
            /**
             * Default constructor for convenience, constructed instance is unusable
             */
            ExprNode()
            : op(NULL)
            {}
            
            /**
             * Constructor, create instance with given operator
             */
            ExprNode(const char *op_name);
            ExprNode(const char *op_name, size_t sz);
            ExprNode(Operator *o);
            
            virtual ~ExprNode();

            /**
             * This is an expression
             */
            virtual int eva_type() const { return ET_EXPR; }

            /**
             * This evaluatable is const-foldable only if the operator is const-foldable
             */
            virtual bool is_const() const;
            virtual bool uses_match_info() const;
            virtual common::Value evaluate(docid did, ExecutionContext &context) const;
            virtual std::string to_string_impl() const;

            Operator *op;
            oprands_t oprands;
        private:
            ExprNode(const ExprNode &n);
        };
        
        typedef boost::shared_ptr<ExprNode> expr_ptr_t;

        /**
         * Optimize ExprNode, so far we're doing const-folding and some other simple optimizations
         */
        eva_ptr_t optimize(expr_ptr_t node, ExecutionContext &context);

        /**
         * Get the specific operator singleton instance
         */
        Operator *get_operator(const char *op_name);
        Operator *get_operator(const char *op_name, size_t sz);
        Operator *get_operator(int op_id);
    }   // End of namespace name
}   // End of namespace argos

#endif
