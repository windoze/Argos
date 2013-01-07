//
//  expr_node.cpp
//  Argos
//
//  Created by Windoze on 12-6-21.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//
#include "common/expr_node.h"

namespace argos {
    namespace common {
        std::string Constant::to_string_impl() const
        {
            std::stringstream sst;
            sst << std::fixed;
            switch (value.type_) {
                case VT_INTEGER:
                    sst << value.number;
                    break;
                case VT_DOUBLE:
                    sst << value.dnumber;
                    break;
                case VT_GEOLOCATION:
                    sst << '[' << value.geolocation.latitude << ',' << value.geolocation.longitude << ']';
                    break;
                case VT_STRING:
                    sst << '"' <<  value.string << '"';
                    break;
                case VT_ARRAY:
                    sst << '[';
                    for (int i=0; i<value.array.size(); i++) {
                        sst << Constant(value.array.get_element(i)).to_string();
                        if (i!=value.array.size()-1) {
                            sst << ",";
                        }
                    }
                    sst << ']';
                    break;
                default:
                    break;
            }
            return sst.str();
        }
        
        ExprNode::ExprNode(const char *op_name)
        : op(get_operator(op_name))
        {
            if (op && op->stateful()) {
                op=op->clone();
            }
        }

        ExprNode::ExprNode(const char *op_name, size_t sz)
        : op(get_operator(op_name, sz))
        {
            if (op && op->stateful()) {
                op=op->clone();
            }
        }
        
        ExprNode::ExprNode(Operator *o)
        : op(o)
        {
            if (op && op->stateful()) {
                op=op->clone();
            }
        }
        
        ExprNode::~ExprNode()
        {
            if (op && op->stateful()) {
                delete op;
            }
        }
        
        bool ExprNode::is_const() const
        {
            // The expr is non-const if the operator is not const-foldable,
            // even all oprands are const
            if (!op->const_foldable()) {
                return false;
            }

            // The expr is const if operator is const-foldable and all oprands are const
            bool ret=true;
            for (oprands_t::const_iterator i=oprands.begin(); i!=oprands.end(); ++i) {
                ret = ret && (*i)->is_const();
                if (!ret) {
                    return false;
                }
            }
            return true;
        }

        VALUE_TYPE ExprNode::type() const
        {
            type_list_t tl(oprands.size(), VT_EMPTY);
            for (size_t i=0; i<oprands.size(); i++) {
                tl[i]=oprands[i]->type();
            }
            return op->type_inference(tl);
        }
        
        Value ExprNode::evaluate(docid did, ExecutionContext &context) const
        {
            return op->evaluate(did, context, oprands);
        }

        bool ExprNode::uses_match_info() const {
            bool ret=false;
            for (size_t i=0; i<oprands.size(); i++) {
                ret = ret || oprands[i]->uses_match_info();
            }
            return ret;
        }
        
        std::string ExprNode::to_string_impl() const
        {
            std::stringstream sst;
            sst << op->get_name();
            sst << '(';
            for (size_t i=0; i<oprands.size(); i++) {
                sst << oprands[i]->to_string();
                if (i<oprands.size()-1) {
                    sst << ',';
                }
            }
            sst << ')';
            return sst.str();
        }
    }   // End of namespace query
}   // End of namespace argos
