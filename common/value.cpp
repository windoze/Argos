//
//  value.cpp
//  Argos
//
//  Created by Windoze on 12-6-25.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <limits>
#include "common/util.h"
#include "common/value.h"

namespace argos {
    namespace details {
        inline int safe_strcmp(const char *l, const char *r) {
            if (l==r) {
                return 0;
            }
            if (l==NULL) {
                return -1;
            }
            if (r==NULL) {
                return 1;
            }
            return strcmp(l, r);
        }
    }   // End of namespace details
    
    namespace common {
        Value ArrayValue::get_element(size_t index) const {
            Value ret;
            switch (field_type()) {
                case FT_INT8:
                    ret.type_=VT_INTEGER;
                    ret.number=*((int8_t *)element_base(index));
                    break;
                case FT_INT16:
                    ret.type_=VT_INTEGER;
                    ret.number=*((int16_t *)element_base(index));
                    break;
                case FT_INT32:
                    ret.type_=VT_INTEGER;
                    ret.number=*((int32_t *)element_base(index));
                    break;
                case FT_INT64:
                    ret.type_=VT_INTEGER;
                    ret.number=*((int64_t *)element_base(index));
                    break;
                case FT_FLOAT:
                    ret.type_=VT_DOUBLE;
                    ret.dnumber=*((float *)element_base(index));
                    break;
                case FT_DOUBLE:
                    ret.type_=VT_DOUBLE;
                    ret.dnumber=*((double *)element_base(index));
                    break;
                case FT_GEOLOC:
                    ret.type_=VT_GEOLOCATION;
                    ret.geolocation=*((GeoLocationValue *)element_base(index));
                    break;
                default:
                    break;
            }
            return ret;
        }
        
        inline void convert_value(FIELD_TYPE to_ft, void *to, Value from)
        {
            switch (base_type(to_ft)) {
                case FT_INT8:
                    *((int8_t *)to)=int8_t(from.cast(VT_INTEGER).number);
                    break;
                case FT_INT16:
                    *((int16_t *)to)=int16_t(from.cast(VT_INTEGER).number);
                    break;
                case FT_INT32:
                    *((int32_t *)to)=int32_t(from.cast(VT_INTEGER).number);
                    break;
                case FT_INT64:
                    *((int64_t *)to)=int64_t(from.cast(VT_INTEGER).number);
                    break;
                case FT_FLOAT:
                    *((float *)to)=float(from.cast(VT_DOUBLE).dnumber);
                    break;
                case FT_DOUBLE:
                    *((double *)to)=from.cast(VT_DOUBLE).dnumber;
                    break;
                case FT_GEOLOC:
                    *((GeoLocationValue *)to)=from.cast(VT_GEOLOCATION).geolocation;
                    break;
                default:
                    break;
            }
        }
        
        ArrayValue make_array(void *buf, FIELD_TYPE ft, ArrayValue arr)
        {
            // Assume buf has array_required_length bytes
            ArrayValue ret;
            ret.data=(int8_t *)buf;
            *((uint16_t *)(ret.data+2))=(uint16_t)base_type(ft);
            *((uint16_t *)(ret.data))=(uint16_t)(arr.size());
            for (int i=0; i<arr.size(); i++) {
                convert_value(ft, ret.element_base(i), arr.get_element(i));
            }
            return ret;
        }

        ArrayValue make_array(void *buf, FIELD_TYPE ft, const value_list_t &vl)
        {
            // Assume buf has array_required_length bytes
            ArrayValue ret;
            ret.data=(int8_t *)buf;
            *((uint16_t *)(ret.data+2))=(uint16_t)base_type(ft);
            *((uint16_t *)(ret.data))=(uint16_t)(vl.size());
            for (int i=0; i<vl.size(); i++) {
                convert_value(ft, ret.element_base(i), vl[i]);
            }
            return ret;
        }

        OFFSET make_array(mem_pool *pool, FIELD_TYPE ft, ArrayValue arr)
        {
            size_t sz=array_required_length(ft, arr.size());
            OFFSET off=pool->allocate(sz);
            void *buf=pool->get_addr(off);
            make_array(buf, ft, arr);
            return off;
        }
        
        OFFSET make_array(mem_pool *pool, FIELD_TYPE ft, const value_list_t &vl)
        {
            size_t sz=array_required_length(ft, vl.size());
            OFFSET off=pool->allocate(sz);
            void *buf=pool->get_addr(off);
            make_array(buf, ft, vl);
            return off;
        }
        
        Value::Value(const value_list_t &vl)
        {
            VALUE_TYPE vt=common_type(vl);
            if (vl.empty()) {
                vt=VT_INTEGER;
            }
            if (vt==VT_EMPTY) {
                // Clear content
                number=0;
                return;
            }
            FIELD_TYPE ft;
            switch (vt) {
                case VT_INTEGER:
                    ft=FT_INT64;
                    break;
                case VT_DOUBLE:
                    ft=FT_DOUBLE;
                    break;
                case VT_GEOLOCATION:
                    ft=FT_GEOLOC;
                    break;
                default:
                    ft=FT_INT64;
                    break;
            }
            OFFSET off=make_array(get_tl_mem_pool(), ft, vl);
            array.data=(int8_t *)(get_tl_mem_pool()->get_addr(off));
            type_=VT_ARRAY;
        }
        
        Value::Value(const std::string &v)
        {
            type_=VT_EMPTY;
            number=0;
            
            mem_pool *mp=get_tl_mem_pool();
            OFFSET off=mp->add_string(v.c_str(), v.size());  // Trailing '\0'
            if (off==INVALID_OFFSET) {
                return;
            }
            type_=VT_STRING;
            string=(char *)(mp->get_addr(off));
            return;
        }

        Value Value::cast(VALUE_TYPE vt) const
        {
            // No conv
            if (type_==vt) {
                return *this;
            }
            Value ret;
            ret.type_=vt;
            ret.number=0;
            if (vt==VT_EMPTY) {
                return ret;
            }
            switch (vt) {
                case VT_EMPTY:
                    break;
                case VT_INTEGER:
                    switch (type_) {
                        case VT_EMPTY:
                            ret.number=0;
                            break;
                        case VT_INTEGER:
                            ret.number=number;
                            break;
                        case VT_DOUBLE:
                            ret.number=dnumber;
                            break;
                        case VT_STRING:
                            // atoi?
                            ret.number=atoll(string);
                            break;
                        case VT_GEOLOCATION:
                        case VT_ARRAY:
                        default:
                            ret.number=0;
                            break;
                    }
                    break;
                case VT_DOUBLE:
                    switch (type_) {
                        case VT_EMPTY:
                            ret.dnumber=0;
                            break;
                        case VT_INTEGER:
                            ret.dnumber=number;
                            break;
                        case VT_DOUBLE:
                            ret.dnumber=dnumber;
                            break;
                        case VT_STRING:
                            ret.dnumber=::atof(string);
                            break;
                        case VT_GEOLOCATION:
                        case VT_ARRAY:
                        default:
                            ret.dnumber=0;
                            break;
                    }
                    break;
                    // Not support
                case VT_GEOLOCATION:
                case VT_STRING:
                case VT_ARRAY:
                default:
                    break;
            }
            return ret;
        }
        
        VALUE_TYPE Value::coerce(VALUE_TYPE vt2) const
        {
            if (type_==vt2) {
                return type_;
            }
            switch (type_) {
                case VT_EMPTY:
                    return vt2;
                case VT_INTEGER:
                    switch (vt2) {
                        case VT_EMPTY:
                            return VT_INTEGER;
                        case VT_DOUBLE:
                            return VT_DOUBLE;
                        default:
                            return VT_EMPTY;
                    }
                    break;
                case VT_DOUBLE:
                    switch (vt2) {
                        case VT_EMPTY:
                            return VT_DOUBLE;
                        case VT_INTEGER:
                            return VT_DOUBLE;
                        default:
                            return VT_EMPTY;
                    }
                    break;
                default:
                    return VT_EMPTY;
            }
        }
        
        Value operator+(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=v1.coerce(v2.type_);
            if (ret.type_==VT_EMPTY) {
                return ret;
            }
            switch (ret.type_) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number+v2.cast(VT_INTEGER).number;
                    break;
                case VT_DOUBLE:
                    ret.dnumber=v1.cast(VT_DOUBLE).dnumber+v2.cast(VT_DOUBLE).dnumber;
                default:
                    break;
            }
            return ret;
        }
        
        Value operator-(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=v1.coerce(v2.type_);
            if (ret.type_==VT_EMPTY) {
                return ret;
            }
            switch (ret.type_) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number-v2.cast(VT_INTEGER).number;
                    break;
                case VT_DOUBLE:
                    ret.dnumber=v1.cast(VT_DOUBLE).dnumber-v2.cast(VT_DOUBLE).dnumber;
                default:
                    break;
            }
            return ret;
        }
        
        Value operator*(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=v1.coerce(v2.type_);
            if (ret.type_==VT_EMPTY) {
                return ret;
            }
            switch (ret.type_) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number*v2.cast(VT_INTEGER).number;
                    break;
                case VT_DOUBLE:
                    ret.dnumber=v1.cast(VT_DOUBLE).dnumber*v2.cast(VT_DOUBLE).dnumber;
                default:
                    break;
            }
            return ret;
        }
        
        // Devide always returns double
        Value operator/(const Value &v1, const Value &v2)
        {
            Value rv2=v2.cast(VT_DOUBLE);
            if (rv2.type_==VT_EMPTY || rv2.dnumber<=std::numeric_limits<double>::epsilon()) {
                return Value();
            }
            return  Value(v1.cast(VT_DOUBLE).dnumber / rv2.dnumber);
        }
        
        // MOD always returns integer
        Value operator%(const Value &v1, const Value &v2)
        {
            Value rv2=v2.cast(VT_INTEGER);
            if (rv2.number==0) {
                return Value();
            }
            return  Value(v1.cast(VT_INTEGER).number / rv2.number);
        }
        
        Value operator<(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.type_=VT_INTEGER;
            ret.number=0;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number<v2.cast(VT_INTEGER).number ? 1 : 0;
                    break;
                case VT_DOUBLE:
                    ret.number=v1.cast(VT_DOUBLE).dnumber<v2.cast(VT_DOUBLE).dnumber ? 1 : 0;
                    break;
                case VT_STRING:
                    ret.number=details::safe_strcmp(v1.string, v2.string)<0;
                    break;
                default:
                    break;
            }
            return ret;
        }
        
        Value operator<=(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number<=v2.cast(VT_INTEGER).number ? 1 : 0;
                    break;
                case VT_DOUBLE:
                    ret.number=v1.cast(VT_DOUBLE).dnumber<=v2.cast(VT_DOUBLE).dnumber ? 1 : 0;
                    break;
                case VT_STRING:
                    ret.number=details::safe_strcmp(v1.string, v2.string)<=0;
                    break;
                default:
                    break;
            }
            return ret;
        }

        Value operator>(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number>v2.cast(VT_INTEGER).number ? 1 : 0;
                    break;
                case VT_DOUBLE:
                    ret.number=v1.cast(VT_DOUBLE).dnumber>v2.cast(VT_DOUBLE).dnumber ? 1 : 0;
                    break;
                case VT_STRING:
                    ret.number=details::safe_strcmp(v1.string, v2.string)>0;
                    break;
                default:
                    break;
            }
            return ret;
        }
        
        Value operator>=(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number>=v2.cast(VT_INTEGER).number ? 1 : 0;
                    break;
                case VT_DOUBLE:
                    ret.number=v1.cast(VT_DOUBLE).dnumber>=v2.cast(VT_DOUBLE).dnumber ? 1 : 0;
                    break;
                case VT_STRING:
                    ret.number=details::safe_strcmp(v1.string, v2.string)>=0;
                    break;
                default:
                    break;
            }
            return ret;
        }
        
        Value operator==(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number==v2.cast(VT_INTEGER).number ? 1 : 0;
                    break;
                case VT_DOUBLE:
                    ret.number=v1.cast(VT_DOUBLE).dnumber==v2.cast(VT_DOUBLE).dnumber ? 1 : 0;
                    break;
                case VT_STRING:
                    ret.number=details::safe_strcmp(v1.string, v2.string)==0;
                    break;
                default:
                    break;
            }
            return ret;
        }

        Value operator!=(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    ret.number=v1.cast(VT_INTEGER).number!=v2.cast(VT_INTEGER).number ? 1 : 0;
                    break;
                case VT_DOUBLE:
                    ret.number=v1.cast(VT_DOUBLE).dnumber!=v2.cast(VT_DOUBLE).dnumber ? 1 : 0;
                    break;
                case VT_STRING:
                    ret.number=details::safe_strcmp(v1.string, v2.string)!=0;
                    break;
                default:
                    break;
            }
            return ret;
        }
        
        Value operator!(const Value &v1)
        {
            return Value(bool(v1));
        }

        Value operator&(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    // Bit op for integers
                    ret.number=v1.cast(VT_INTEGER).number&v2.cast(VT_INTEGER).number;
                    break;
                default:
                    ret.number=bool(v1) && bool(v2);
                    break;
            }
            return ret;
        }

        Value operator|(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    // Bit op for integers
                    ret.number=v1.cast(VT_INTEGER).number|v2.cast(VT_INTEGER).number;
                    break;
                default:
                    ret.number=bool(v1) || bool(v2);
                    break;
            }
            return ret;
        }

        Value operator^(const Value &v1, const Value &v2)
        {
            Value ret;
            ret.number=0;
            ret.type_=VT_INTEGER;
            VALUE_TYPE cvt=v1.coerce(v2.type_);
            if (cvt==VT_EMPTY) {
                return ret;
            }
            switch (cvt) {
                case VT_INTEGER:
                    // Bit op for integers
                    ret.number=v1.cast(VT_INTEGER).number^v2.cast(VT_INTEGER).number;
                    break;
                default:
                    ret.number=bool(v1) ^ bool(v2);
                    break;
            }
            return ret;
        }
    }   // End of namespace common
}   // End of namespace argos