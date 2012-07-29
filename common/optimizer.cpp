//
//  optimizer.cpp
//  Argos
//
//  Created by Windoze on 12-6-25.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/expr_node.h"

namespace argos {
    namespace common {
        namespace detail {
            eva_ptr_t fold_constants(eva_ptr_t node, ExecutionContext &context)
            {
                if (node->is_const()) {
                    Value v=node->evaluate(0, context);
                    return eva_ptr_t(new Constant(v));
                }
                return node;
            }

            eva_ptr_t optimize_and(expr_ptr_t node, ExecutionContext &context)
            {
                bool reformed=false;
                for (size_t i=0; i<node->oprands.size(); i++) {
                    if (node->oprands[i]->is_const()) {
                        if (bool(node->oprands[i]->evaluate(0, context))==false) {
                            // There is a FALSE constant in AND oprands, make shortcut
                            //delete node;
                            return eva_ptr_t(new Constant(Value(false)));
                        } else {
                            node->oprands[i].reset();
                            reformed=true;
                        }
                    }
                }
                if (reformed) {
                    // Wipe all NULL oprands
                    oprands_t ops;
                    for (size_t i=0; i<node->oprands.size(); i++) {
                        if (node->oprands[i]) {
                            ops.push_back(node->oprands[i]);
                        }
                    }
                    node->oprands=ops;
                }
                if (node->oprands.size()==1) {
                    return node->oprands[0];
                }

                return node;
            }
            
            eva_ptr_t optimize_or(expr_ptr_t node, ExecutionContext &context)
            {
                bool reformed=false;
                for (size_t i=0; i<node->oprands.size(); i++) {
                    if (node->oprands[i]->is_const()) {
                        if (bool(node->oprands[i]->evaluate(0, context))) {
                            return eva_ptr_t(new Constant(Value(true)));
                        } else {
                            // TODO: Remove this oprand as OR(x, y, FALSE)==OR(x, y)
                            node->oprands[i].reset();
                            reformed=true;
                        }
                    }
                }
                if (reformed) {
                    // Wipe all NULL oprands
                    oprands_t ops;
                    for (size_t i=0; i<node->oprands.size(); i++) {
                        if (node->oprands[i]) {
                            ops.push_back(node->oprands[i]);
                        }
                    }
                    node->oprands=ops;
                }
                if (node->oprands.size()==1) {
                    return node->oprands[0];
                }
                return node;
            }
            
            eva_ptr_t optimize_xor(expr_ptr_t node, ExecutionContext &context)
            {
                bool reformed=false;
                for (size_t i=0; i<node->oprands.size(); i++) {
                    if (node->oprands[i]->is_const()) {
                        if (!bool(node->oprands[i]->evaluate(0, context))) {
                            // TODO: Remove this oprand as XOR(x, y, FALSE)==XOR(x, y)
                            node->oprands[i].reset();
                            reformed=true;
                        }
                    }
                }
                if (reformed) {
                    // Wipe all NULL oprands
                    oprands_t ops;
                    for (size_t i=0; i<node->oprands.size(); i++) {
                        if (node->oprands[i]) {
                            ops.push_back(node->oprands[i]);
                        }
                    }
                    node->oprands=ops;
                }
                if (node->oprands.size()==1) {
                    return node->oprands[0];
                }
                return node;
            }
            
            eva_ptr_t optimize_boolean(expr_ptr_t node, ExecutionContext &context)
            {
                if (node->op==get_operator("AND")) {
                    return optimize_and(node, context);
                } else if (node->op==get_operator("OR")) {
                    return optimize_or(node, context);
                } else if (node->op==get_operator("XOR")) {
                    return optimize_xor(node, context);
                }
                return node;
            }

            eva_ptr_t optimize_multi_arith(expr_ptr_t node, const char *op_name, ExecutionContext &context)
            {
                ExprNode cn(op_name);
                oprands_t ops_var;
                for (size_t i=0; i<node->oprands.size(); i++) {
                    if (node->oprands[i]->is_const()) {
                        // Collect all constants
                        cn.oprands.push_back(node->oprands[i]);
                    } else {
                        // Collect all non-constants
                        ops_var.push_back(node->oprands[i]);
                    }
                }
                // Only opt if more than 1 const oprands
                if (cn.op->validate_arity(cn.oprands.size())) {
                    node->oprands=ops_var;
                    node->oprands.push_back(eva_ptr_t(new Constant(cn.evaluate(0, context))));
                } else {
                    // Avoid double deletion
                    cn.oprands.clear();
                }
                return node;
            }
            
            eva_ptr_t optimize_sub(expr_ptr_t node, ExecutionContext &context)
            {
                if (node->oprands[1]->is_const()) {
                    if (node->oprands[1]->evaluate(0, context)==Value(int64_t(0))) {
                        return node->oprands[0];
                    }
                }
                return node;
            }
            
            eva_ptr_t optimize_div(expr_ptr_t node, ExecutionContext &context)
            {
                if (node->oprands[1]->is_const()) {
                    if (node->oprands[1]->evaluate(0, context)==Value(int64_t(1))) {
                        return node->oprands[0];
                    }
                }
                return node;
            }
            
            eva_ptr_t optimize_arith(expr_ptr_t node, ExecutionContext &context)
            {
                if (node->op==get_operator("ADD")) {
                    return optimize_multi_arith(node, "ADD", context);
                } else if (node->op==get_operator("MUL")) {
                    return optimize_multi_arith(node, "MUL", context);
                } else if (node->op==get_operator("XOR")) {
                    return optimize_multi_arith(node, "XOR", context);
                } else if (node->op==get_operator("SUB")) {
                    return optimize_sub(node, context);
                } else if (node->op==get_operator("DIV")) {
                    return optimize_div(node, context);
                }
                return node;
            }
        }   // End of namespace detail

        eva_ptr_t optimize(expr_ptr_t node, ExecutionContext &context)
        {
            eva_ptr_t ret=detail::fold_constants(node, context);
            // Only optimize ExprNode, not others
            if (ret->eva_type()==2) {
                ret=detail::optimize_arith(boost::static_pointer_cast<ExprNode>(ret), context);
            }
            if (ret->eva_type()==2) {
                ret=detail::optimize_boolean(boost::static_pointer_cast<ExprNode>(ret), context);
            }
            return ret;
        }
    }   // End of namespace query
}   // End of namespace argos
