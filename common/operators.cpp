//
//  operators.cpp
//  Argos
//
//  Created by Windoze on 12-6-21.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <math.h>
#include <map>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include "common/expr_node.h"

namespace argos {
    namespace common {
        namespace operators {
            class OperatorFactory {
            public:
                ~OperatorFactory() {
                    for (int i=0; i<operators.size(); i++) {
                        if (operators[i]) {
                            delete operators[i];
                        }
                    }
                }
                
                static void init_factory() {
                    if (the_instance==0) {
                        the_instance=new OperatorFactory;
                    }
                }
                
                static void close_factory() {
                    if (the_instance) {
                        delete the_instance;
                        the_instance=0;
                    }
                }
                
                static inline Operator *get_operator(int op) {
                    return the_instance->get_op(op);
                }
                
                static inline Operator *get_operator(const char *op_name) {
                    return the_instance->get_op(op_name);
                }
                
                static inline int add_operator(Operator *op) {
                    return the_instance->add_op(op);
                }
                
            private:
                Operator *get_op(int op) const {
                    return operators[op];
                }
                
                Operator *get_op(const char *op_name) const {
                    name_map_t::const_iterator i=name_map.find(op_name);
                    if (i==name_map.end()) {
                        return NULL;
                    }
                    return get_op(i->second);
                }
                
                int add_op(Operator *op) {
                    if (get_op(op->get_name())!=NULL) {
                        return -1;
                    }
                    operators.push_back(op);
                    name_map[op->get_name()]=int(operators.size()-1);
                    return int(operators.size()-1);
                }
                
                typedef std::map<std::string, int> name_map_t;
                name_map_t name_map;
                std::vector<Operator *> operators;
                static OperatorFactory *the_instance;
            };
            OperatorFactory *OperatorFactory::the_instance=0;

            /**
             * ADD(v1, v2, ... vn) = v1+v2+...+vn
             */
            class AddOp : public Operator {
            public:
                virtual const char *get_name() const { return "ADD"; }
                virtual bool validate_arity(size_t arity) const { return arity>=2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value ret(int64_t(0));
                    for (oprands_t::const_iterator i=oprands.begin(); i!=oprands.end(); ++i) {
                        ret=ret+(*i)->evaluate(did, context);
                    }
                    return ret;
                }
            };
            /**
             * SUB(v1, v2) = v1-v2
             */
            class SubOp : public Operator {
            public:
                virtual const char *get_name() const { return "SUB"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context)-oprands[1]->evaluate(did, context);
                }
            };
            /**
             * MUL(v1, v2, ... vn) = v1*v2*...*vn
             */
            class MulOp : public Operator {
            public:
                virtual const char *get_name() const { return "MUL"; }
                virtual bool validate_arity(size_t arity) const { return arity>=2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value ret(int64_t(1));
                    for (oprands_t::const_iterator i=oprands.begin(); i!=oprands.end(); ++i) {
                        ret=ret*(*i)->evaluate(did, context);
                    }
                    return ret;
                }
            };
            /**
             * DIV(v1, v2) = v1/v2
             */
            class DivOp : public Operator {
            public:
                virtual const char *get_name() const { return "DIV"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context)/oprands[1]->evaluate(did, context);
                }
            };
            /**
             * MOD(v1, v2) = v1 mod v2
             */
            class ModOp : public Operator {
            public:
                virtual const char *get_name() const { return "MOD"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context)%oprands[1]->evaluate(did, context);
                }
            };
            /**
             * NEG(v) = -v
             */
            class NegOp : public Operator {
            public:
                virtual const char *get_name() const { return "NEG"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(int64_t(0))-oprands[0]->evaluate(did, context);
                }
            };
            /**
             * LT(v1, v2) = v1<v2
             */
            class LtOp : public Operator {
            public:
                virtual const char *get_name() const { return "LT"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context) < oprands[1]->evaluate(did, context);
                }
            };
            /**
             * LE(v1, v2) = v1<=v2
             */
            class LeOp : public Operator {
            public:
                virtual const char *get_name() const { return "LE"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context) <= oprands[1]->evaluate(did, context);
                }
            };
            /**
             * GT(v1, v2) = v1>v2
             */
            class GtOp : public Operator {
            public:
                virtual const char *get_name() const { return "GT"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context) > oprands[1]->evaluate(did, context);
                }
            };
            /**
             * GE(v1, v2) = v1>=v2
             */
            class GeOp : public Operator {
            public:
                virtual const char *get_name() const { return "GE"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context) >= oprands[1]->evaluate(did, context);
                }
            };
            /**
             * EQ(v1, v2) = v1==v2
             */
            class EqOp : public Operator {
            public:
                virtual const char *get_name() const { return "EQ"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context) == oprands[1]->evaluate(did, context);
                }
            };
            /**
             * NE(v1, v2) = v1!=v2
             */
            class NeOp : public Operator {
            public:
                virtual const char *get_name() const { return "NE"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return oprands[0]->evaluate(did, context) != oprands[1]->evaluate(did, context);
                }
            };
            /**
             * AND(v1, v2, ... ,vn) = v1 & v2 & ... & vn
             */
            class AndOp : public Operator {
            public:
                virtual const char *get_name() const { return "AND"; }
                virtual bool validate_arity(size_t arity) const { return arity>=2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value ret(int64_t(1));
                    for (oprands_t::const_iterator i=oprands.begin(); i!=oprands.end(); ++i) {
                        ret=ret & (*i)->evaluate(did, context);
                        // Shortcut AND
                        if (ret.number==0) {
                            return ret;
                        }
                    }
                    return ret;
                }
            };
            /**
             * OR(v1, v2, ... ,vn) = v1 | v2 | ... | vn
             */
            class OrOp : public Operator {
            public:
                virtual const char *get_name() const { return "OR"; }
                virtual bool validate_arity(size_t arity) const { return arity>=2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value ret(int64_t(0));
                    for (oprands_t::const_iterator i=oprands.begin(); i!=oprands.end(); ++i) {
                        ret=ret | (*i)->evaluate(did, context);
                        // Shortcut OR
                        if (ret.number!=0) {
                            return ret;
                        }
                    }
                    return ret;
                }
            };
            /**
             * XOR(v1, v2, ... ,vn) = v1 ^ v2 ^ ... ^ vn
             */
            class XorOp : public Operator {
            public:
                virtual const char *get_name() const { return "XOR"; }
                virtual bool validate_arity(size_t arity) const { return arity>=2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value ret(int64_t(0));
                    for (oprands_t::const_iterator i=oprands.begin(); i!=oprands.end(); ++i) {
                        ret=ret ^ (*i)->evaluate(did, context);
                    }
                    return ret;
                }
            };
            /**
             * NOT(v) = !v1
             */
            class NotOp : public Operator {
            public:
                virtual const char *get_name() const { return "NOT"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(int64_t(!(oprands[0]->evaluate(did, context))));
                }
            };
            /**
             * LOG(v) = log(e,v)
             */
            class LogOp : public Operator {
            public:
                virtual const char *get_name() const { return "LOG"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(log(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * LOG10(v) = log(10,v)
             */
            class Log10Op : public Operator {
            public:
                virtual const char *get_name() const { return "LOG10"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(log10(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * LOG(v) = log(2,v)
             */
            class Log2Op : public Operator {
            public:
                virtual const char *get_name() const { return "LOG2"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(log2(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * EXP(v) = e ^ v
             */
            class ExpOp : public Operator {
            public:
                virtual const char *get_name() const { return "EXP"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(exp(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * POW2(v) = v*v
             */
            class Pow2Op : public Operator {
            public:
                virtual const char *get_name() const { return "POW2"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value((oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber)
                                 * (oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * POW(x, y) = x ^ y
             */
            class PowOp : public Operator {
            public:
                virtual const char *get_name() const { return "POW"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(pow(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber,
                               oprands[1]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * SQRT(v) = sqrt(v)
             */
            class SqrtOp : public Operator {
            public:
                virtual const char *get_name() const { return "SQRT"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(sqrt(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * SIN(v) = sin(v)
             */
            class SinOp : public Operator {
            public:
                virtual const char *get_name() const { return "SIN"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(sin(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * COS(v) = cos(v)
             */
            class CosOp : public Operator {
            public:
                virtual const char *get_name() const { return "COS"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(cos(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * TAN(v) = tan(v)
             */
            class TanOp : public Operator {
            public:
                virtual const char *get_name() const { return "TAN"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(tan(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * ASIN(v) = asin(v)
             */
            class AsinOp : public Operator {
            public:
                virtual const char *get_name() const { return "ASIN"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(asin(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * ACOS(v) = acos(v)
             */
            class AcosOp : public Operator {
            public:
                virtual const char *get_name() const { return "ACOS"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(acos(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * ATAN(v) = atan(v)
             */
            class AtanOp : public Operator {
            public:
                virtual const char *get_name() const { return "ATAN"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(atan(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber));
                }
            };
            /**
             * IF(cond, true_clause, false_clause) = cond ? true_clause : false_clause
             */
            class IfOp : public Operator {
            public:
                virtual const char *get_name() const { return "IF"; }
                virtual bool validate_arity(size_t arity) const { return arity==3; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    if (bool(oprands[0]->evaluate(did, context))) {
                        return oprands[1]->evaluate(did, context);
                    }
                    return oprands[2]->evaluate(did, context);
                }
            };
            /**
             * CASE(cond, cond0, cond1,... ,condn) = cond0 if cond==0, cond1 if cond==1...
             */
            class CaseOp : public Operator {
            public:
                virtual const char *get_name() const { return "CASE"; }
                virtual bool validate_arity(size_t arity) const { return arity>=3; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    int64_t n=(oprands[0]->evaluate(did, context)).cast(VT_INTEGER).number;
                    if (n>oprands.size()+1 || n<0) {
                        // TODO: Error report, Out of range
                        return Value();
                    }
                    return oprands[n+1]->evaluate(did, context);
                }
            };
            /**
             * RANGE(v, seg1, seg2,... ,segn) = 0 if v<seg1, 1 if seg1<=v<seg2, ... n if segn<=v
             */
            class RangeOp : public Operator {
            public:
                virtual const char *get_name() const { return "RANGE"; }
                virtual bool validate_arity(size_t arity) const { return arity>=2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value v=oprands[0]->evaluate(did, context);
                    for (int64_t i=1; i<oprands.size(); i++) {
                        if (v<oprands[i]->evaluate(did, context)) {
                            return Value(i-1);
                        }
                    }
                    return Value(int64_t(oprands.size()-1));
                }
            };
            
            /**
             * LEN(v) = strlen(v) if v is string, number of element if v is array, otherwise 0
             */
            class LenOp : public Operator {
            public:
                virtual const char *get_name() const { return "LEN"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value v=oprands[0]->evaluate(did, context);
                    if (v.type_==VT_STRING) {
                        return Value(int64_t(v.string ? strlen(v.string) : 0));
                    } else if (v.type_==VT_ARRAY) {
                        return Value(int64_t(v.array.size()));
                    }
                    return Value(int64_t(0));
                }
            };
            
            /**
             * AT(arr, i) = arr[i]
             */
            class AtOp : public Operator {
            public:
                virtual const char *get_name() const { return "AT"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value arr=oprands[0]->evaluate(did, context);
                    if (arr.type_==VT_ARRAY) {
                        int64_t index=oprands[1]->evaluate(did, context).cast(VT_INTEGER).number;
                        if (index>=arr.array.size()) {
                            return Value();
                        }
                        return arr.array.get_element(index);
                    }
                    return Value();
                }
            };
            
            /**
             * FIND(arr, v) = index of v in arr, -1 if not found
             */
            class FindOp : public Operator {
            public:
                virtual const char *get_name() const { return "FIND"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value arr=oprands[0]->evaluate(did, context);
                    if (arr.type_==VT_ARRAY) {
                        Value v=oprands[1]->evaluate(did, context);
                        for (int i=0; i<arr.array.size(); i++) {
                            if (v==arr.array.get_element(i)) {
                                return Value(int64_t(i));
                            }
                        }
                        return Value(int64_t(-1));
                    }
                    return Value();
                }
            };
            
            inline double rad(double d)
            {
                const float PI = float(3.1415926);//pi
                return d * PI / 180.0;
            }
            
            inline double dist(double lat1, double long1, double lat2, double long2)
            {
                const double EARTH_RADIUS = 6378.137;
                double radLat1 = rad(lat1);
                double radLat2 = rad(lat2);
                double a = radLat1 - radLat2;
                double b = rad(long1) - rad(long2);
                double s = 2 * asin(sqrt(pow(sin(a/2),2) + cos(radLat1)*cos(radLat2)*pow(sin(b/2),2)));
                s = s * EARTH_RADIUS * 1000;
                return s;
            }
            
            inline double get_lat(Value v) {
                if (v.type_==VT_ARRAY) {
                    return v.array.get_element(0).cast(VT_DOUBLE).dnumber;
                } else if (v.type_==VT_GEOLOCATION) {
                    return v.geolocation.latitude;
                }
                return v.cast(VT_DOUBLE).dnumber;
            }

            inline double get_long(Value v) {
                if (v.type_==VT_ARRAY) {
                    return v.array.get_element(1).cast(VT_DOUBLE).dnumber;
                } else if (v.type_==VT_GEOLOCATION) {
                    return v.geolocation.longitude;
                }
                return v.cast(VT_DOUBLE).dnumber;
            }
            
            class LatOp : public Operator {
            public:
                virtual const char *get_name() const { return "LAT"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(get_lat(oprands[0]->evaluate(did, context)));
                }
            };
            
            class LngOp : public Operator {
            public:
                virtual const char *get_name() const { return "LNG"; }
                virtual bool validate_arity(size_t arity) const { return arity==1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(get_long(oprands[0]->evaluate(did, context)));
                }
            };

            class GeoLocOp : public Operator {
            public:
                virtual const char *get_name() const { return "GEOLOC"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    return Value(oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber,
                                 oprands[1]->evaluate(did, context).cast(VT_DOUBLE).dnumber);
                }
            };
            
            /**
             * DIST(lat1, long1, lat2, long2) or DIST(<lat1,long1>, <lat2,long2>) or DIST([lat1,long1...], [lat2,long2...])
             *
             * spherical distance between (lat1, long1) and (lat2, long1) on the planet Earth (Earth radius is used)
             * latitudes and longitudes are *in degree*
             * returned value is in meter
             */
            class DistOp : public Operator {
            public:
                virtual const char *get_name() const { return "DIST"; }
                virtual bool validate_arity(size_t arity) const { return (arity==2) || (arity==4); }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    double lat1,long1,lat2,long2;
                    // We have either 4 oprands or 2 oprands
                    if (oprands.size()==4) {
                        // oprands are lat1,long1,lat2,long2
                        lat1=oprands[0]->evaluate(did, context).cast(VT_DOUBLE).dnumber;
                        long1=oprands[1]->evaluate(did, context).cast(VT_DOUBLE).dnumber;
                        lat2=oprands[2]->evaluate(did, context).cast(VT_DOUBLE).dnumber;
                        long2=oprands[3]->evaluate(did, context).cast(VT_DOUBLE).dnumber;
                    } else {
                        // oprands are [lat1,long1],[lat2,long2] or <lat1,long1>,<lat2,long2>
                        Value v1=oprands[0]->evaluate(did, context);
                        Value v2=oprands[1]->evaluate(did, context);
                        if ((v1.type_==VT_ARRAY || v1.type_==VT_GEOLOCATION) && (v2.type_==VT_ARRAY || v2.type_==VT_GEOLOCATION)) {
                            lat1=get_lat(v1);
                            long1=get_long(v1);
                            lat2=get_lat(v2);
                            long2=get_long(v2);
                        } else {
                            return Value();
                        }
                    }
                    return Value(dist(lat1, long1, lat2, long2));
                }
            };
            /**
             * MINDIST(<lat1, long1>, [<lat2, long2>, ... ])
             *
             * Return min distance between a point and an array of points
             */
            class MinDistOp : public Operator {
            public:
                virtual const char *get_name() const { return "MINDIST"; }
                virtual bool validate_arity(size_t arity) const { return (arity==2); }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    // We have at exact 2 oprands
                    // First one must be convertible to geolocation
                    Value v1=oprands[0]->evaluate(did, context).cast(VT_GEOLOCATION);
                    if (v1.empty()) {
                        return Value();
                    }
                    // Second one must be a non-empty array of geolocations
                    Value v2=oprands[1]->evaluate(did, context);
                    if (v2.type_!=VT_ARRAY) {
                        return Value();
                    }
                    if (v2.array.size()<=0) {
                        return Value();
                    }
                    if (v2.array.field_type()!=FT_GEOLOC) {
                        return Value();
                    }
                    // 400000km is far enough, on the planet Earth.
                    double min_dist=40000*1000*10;
                    for (size_t i=0; i<v2.array.size(); i++) {
                        double d=dist(v1.geolocation.latitude,
                                      v1.geolocation.longitude,
                                      v2.array.get_element(i).geolocation.latitude,
                                      v2.array.get_element(i).geolocation.longitude);
                        if (d<min_dist) {
                            min_dist=d;
                        }
                    }
                    return Value(min_dist);
                }
            };
            /**
             * MIN(v1, v2...vn) returns the min oprand
             * MIN([array]) returns the min element in array
             */
            class MinOp : public Operator {
            public:
                virtual const char *get_name() const { return "MIN"; }
                virtual bool validate_arity(size_t arity) const { return arity>=1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value v=oprands[0]->evaluate(did, context);
                    if (v.type_==common::VT_ARRAY) {
                        if (v.array.size()<=0) {
                            return Value();
                        }
                        Value vr=v.array.get_element(0);
                        for (size_t i=1; i<v.array.size(); i++) {
                            Value v1=v.array.get_element(i);
                            if (vr > v1) {
                                vr=v1;
                            }
                        }
                        return vr;
                    } else {
                        for (int64_t i=1; i<oprands.size(); i++) {
                            Value v1=oprands[i]->evaluate(did, context);
                            if (v > v1) {
                                v=v1;
                            }
                        }
                        return v;
                    }
                }
            };
            /**
             * MAX(v1, v2...vn) returns the max oprand
             * MAX([array]) returns the max element in array
             */
            class MaxOp : public Operator {
            public:
                virtual const char *get_name() const { return "MAX"; }
                virtual bool validate_arity(size_t arity) const { return arity>=1; }
                virtual bool const_foldable() const { return true; }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value v=oprands[0]->evaluate(did, context);
                    if (v.type_==common::VT_ARRAY) {
                        if (v.array.size()<=0) {
                            return Value();
                        }
                        Value vr=v.array.get_element(0);
                        for (size_t i=1; i<v.array.size(); i++) {
                            Value v1=v.array.get_element(i);
                            if (vr < v1) {
                                vr=v1;
                            }
                        }
                        return vr;
                    } else {
                        for (int64_t i=1; i<oprands.size(); i++) {
                            Value v1=oprands[i]->evaluate(did, context);
                            if (v < v1) {
                                v=v1;
                            }
                        }
                        return v;
                    }
                }
            };
            /**
             * REGEX(patter, v) returns 1 if v matches the pattern, otherwise 0
             */
            class RegexOp : public Operator {
            public:
                virtual const char *get_name() const { return "REGEX"; }
                virtual bool validate_arity(size_t arity) const { return arity==2; }
                virtual bool const_foldable() const { return true; }
                virtual bool stateful() const { return true; }
                virtual Operator *clone() {
                    return new RegexOp;
                }
                virtual Value evaluate(docid did, ExecutionContext &context, const oprands_t &oprands) const {
                    Value p=oprands[0]->evaluate(did, context);
                    Value s=oprands[1]->evaluate(did, context);
                    if (p.type_!=common::VT_STRING || s.type_!=common::VT_STRING) {
                        return Value(int64_t(0));
                    }
                    if (re.empty()) {
                        re=boost::make_u32regex(p.string);
                        
                    }
                    if (boost::u32regex_match(s.string, re)) {
                        return Value(int64_t(1));
                    }
                    return Value(int64_t(0));
                }
                
                mutable boost::u32regex re;
            };
        }   // End of namespace operators
        
        Operator *get_operator(const char *op_name)
        {
            return operators::OperatorFactory::get_operator(op_name);
        }
        
        Operator *get_operator(const char *op_name, size_t sz)
        {
            std::string s(op_name, sz);
            return operators::OperatorFactory::get_operator(s.c_str());
        }
        
        void add_operator(Operator *op)
        {
            operators::OperatorFactory::add_operator(op);
        }

        Operator *get_operator(int op_id)
        {
            return operators::OperatorFactory::get_operator(op_id);
        }
        
        void init_operators() {
            operators::OperatorFactory::init_factory();
            add_operator(new operators::AddOp);
            add_operator(new operators::SubOp);
            add_operator(new operators::MulOp);
            add_operator(new operators::DivOp);
            add_operator(new operators::ModOp);
            add_operator(new operators::NegOp);
            add_operator(new operators::LtOp);
            add_operator(new operators::LeOp);
            add_operator(new operators::GtOp);
            add_operator(new operators::GeOp);
            add_operator(new operators::EqOp);
            add_operator(new operators::NeOp);
            add_operator(new operators::AndOp);
            add_operator(new operators::OrOp);
            add_operator(new operators::XorOp);
            add_operator(new operators::NotOp);
            add_operator(new operators::LogOp);
            add_operator(new operators::Log10Op);
            add_operator(new operators::Log2Op);
            add_operator(new operators::ExpOp);
            add_operator(new operators::PowOp);
            add_operator(new operators::Pow2Op);
            add_operator(new operators::SqrtOp);
            add_operator(new operators::SinOp);
            add_operator(new operators::CosOp);
            add_operator(new operators::TanOp);
            add_operator(new operators::AsinOp);
            add_operator(new operators::AcosOp);
            add_operator(new operators::AtanOp);
            add_operator(new operators::IfOp);
            add_operator(new operators::LenOp);
            add_operator(new operators::AtOp);
            add_operator(new operators::FindOp);
            add_operator(new operators::CaseOp);
            add_operator(new operators::RangeOp);
            add_operator(new operators::LatOp);
            add_operator(new operators::LngOp);
            add_operator(new operators::GeoLocOp);
            add_operator(new operators::DistOp);
            add_operator(new operators::MinDistOp);
            add_operator(new operators::MinOp);
            add_operator(new operators::MaxOp);
            add_operator(new operators::RegexOp);
        }

        namespace detail {
            struct operator_loader {
                operator_loader() {
                    init_operators();
                }
                ~operator_loader() {
                    operators::OperatorFactory::close_factory();
                }
            };
            operator_loader the_loader;
        }   // End of namespace detail
    }   // End of namespace common
}   // End of namespace argos
