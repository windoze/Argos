//
//  value.h
//  Argos
//
//  Created by Windoze on 12-6-25.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <utility>
#include <vector>
#include "common/argos_exception.h"
#include "common/field_config.h"
#include "common/heaper.h"
#include "common/mem_pool_allocator.h"

#ifndef Argos_value_h
#define Argos_value_h

namespace argos {
    namespace common {
        typedef enum {
            VT_EMPTY,
            VT_INTEGER,
            VT_DOUBLE,
            VT_GEOLOCATION,
            VT_STRING,
            VT_ARRAY,
        } VALUE_TYPE;
        
        struct Value;
        
        struct ArrayValue {
            int8_t *data;
      
// C++11 only, N2544 Unlimited union
// Union cannot contain any data with non-trivial constructors until C++11
#if __cplusplus>=201103L
            ArrayValue()
            : data(0)
            {}
            
            ArrayValue(const ArrayValue &other)
            : data(other.data)
            {}
            
            ArrayValue(void *p)
            : data((int8_t *)p)
            {}
            
            ArrayValue(mem_pool *pool, OFFSET off)
            : data((int8_t *)(pool->get_addr(off)))
            {}
#endif
            
            size_t size() const {
                if (!data) {
                    return 0;
                }
                // First 2 bytes are number of elements
                return *((uint16_t *)data);
            }
            FIELD_TYPE field_type() const {
                if (!data) {
                    return FT_INVALID;
                }
                // Second 2 bytes store base type
                uint16_t ret=(*((uint16_t *)(data+2)));
                return base_type(FIELD_TYPE(ret));
            }
            size_t element_size() const {
                return field_size(field_type());
            }
            size_t length() const {
                return element_size()*size()+4;
            }
            int8_t *base() const {
                return data+4;
            }
            int8_t *element_base(size_t index) const {
                return base()+index*element_size();
            }
            Value get_element(size_t index) const;
        };
        
        struct GeoLocationValue {
#if __cplusplus>=201103L
            GeoLocationValue()
            : latitude(0)
            , longitude(0)
            {}
            GeoLocationValue(float lat, float lng)
            : latitude(lat)
            , longitude(lng)
            {}
#endif
            float latitude;
            float longitude;
        };
        
        struct Value;
        typedef mem_pool_tl_allocator<Value> mpv_allocator;
        typedef std::vector<Value, mpv_allocator> value_list_t;
        
        /**
         * struct Value is a variant value holder
         *
         * Simple value is stored inside the struct, variant-size value is not owned, Value
         * only stores pointer to them.
         */
        struct Value {
            VALUE_TYPE type_;
            union {
                int64_t number;
                double dnumber;
                GeoLocationValue geolocation;
                const char *string;
                ArrayValue array;
            };
            
            Value()
            : type_(VT_EMPTY)
            , number(0)
            {}
            
            explicit Value(int64_t v)
            : type_(VT_INTEGER)
            , number(v)
            {}
            
            explicit Value(int v)
            : type_(VT_INTEGER)
            , number(v)
            {}
            
            explicit Value(unsigned int v)
            : type_(VT_INTEGER)
            , number(v)
            {}
            
            explicit Value(bool v)
            : type_(VT_INTEGER)
            , number(v)
            {}
            
            explicit Value(double v)
            : type_(VT_DOUBLE)
            , dnumber(v)
            {}
            
            explicit Value(float v)
            : type_(VT_DOUBLE)
            , dnumber(v)
            {}
            
            Value(double v1, double v2)
            : type_(VT_GEOLOCATION)
            {
                geolocation.latitude=v1;
                geolocation.longitude=v2;
            }
            
            Value(GeoLocationValue v)
            : type_(VT_GEOLOCATION)
            , geolocation(v)
            {}
            
            Value(const char *v)
            : type_(VT_STRING)
            , string(v)
            {}
            
            Value(const std::string &v);
            
            Value(ArrayValue v)
            : type_(VT_ARRAY)
            , array(v)
            {}
            
            Value(const value_list_t &vl);

            Value(const Value &v)
            : type_(v.type_)
            , number(0)
            {
                switch (type_) {
                    case VT_INTEGER:
                        number=v.number;
                        break;
                    case VT_DOUBLE:
                        dnumber=v.dnumber;
                        break;
                    case VT_GEOLOCATION:
                        geolocation=v.geolocation;
                        break;
                    case VT_STRING:
                        string=v.string;
                        break;
                    case VT_ARRAY:
                        array=v.array;
                        break;
                    default:
                        break;
                }
            }
            
            bool empty() const {
                return type_==VT_EMPTY;
            }
            
            void clear() {
                type_=VT_EMPTY;
                number=0;
            }

            Value &operator=(int64_t v)
            {
                type_=VT_INTEGER;
                number=v;
                return *this;
            }

            Value &operator=(double v)
            {
                type_=VT_DOUBLE;
                dnumber=v;
                return *this;
            }
            
            Value &operator=(const Value &v)
            {
                type_=v.type_;
                switch (type_) {
                    case VT_INTEGER:
                        number=v.number;
                        break;
                    case VT_DOUBLE:
                        dnumber=v.dnumber;
                        break;
                    case VT_GEOLOCATION:
                        geolocation=v.geolocation;
                        break;
                    case VT_STRING:
                        string=v.string;
                        break;
                    case VT_ARRAY:
                        array=v.array;
                        break;
                    default:
                        break;
                }
                return *this;
            }
            
            operator bool() const {
                switch (type_) {
                    case VT_INTEGER:
                        return number!=0;
                        break;
                    case VT_DOUBLE:
                        return dnumber!=0;
                    case VT_GEOLOCATION:
                        return (geolocation.latitude!=0) && (geolocation.longitude!=0);
                    case VT_STRING:
                        return string!=NULL;
                    case VT_ARRAY:
                        return array.data!=NULL;
                    default:
                        break;
                }
                return false;
            }
            
            /**
             * Value type cast
             */
            Value cast(VALUE_TYPE vt) const;
            
            /**
             * Find appropriate type
             */
            VALUE_TYPE coerce(VALUE_TYPE vt) const;
        };
        
        Value operator+(const Value &v1, const Value &v2);
        Value operator-(const Value &v1, const Value &v2);
        Value operator*(const Value &v1, const Value &v2);
        Value operator/(const Value &v1, const Value &v2);
        Value operator%(const Value &v1, const Value &v2);
        Value operator<(const Value &v1, const Value &v2);
        Value operator<=(const Value &v1, const Value &v2);
        Value operator>(const Value &v1, const Value &v2);
        Value operator>=(const Value &v1, const Value &v2);
        Value operator==(const Value &v1, const Value &v2);
        Value operator!=(const Value &v1, const Value &v2);
        Value operator!(const Value &v1);
        Value operator&(const Value &v1, const Value &v2);
        Value operator|(const Value &v1, const Value &v2);
        Value operator^(const Value &v1, const Value &v2);
        
        inline VALUE_TYPE common_type(const value_list_t &vl)
        {
            if (vl.empty()) {
                return VT_EMPTY;
            }
            VALUE_TYPE vt=vl[0].type_;
            for (int i=1; (i<vl.size() && vt!=VT_EMPTY); i++) {
                vt=vl[i].coerce(vt);
            }
            return vt;
        }
        
        inline size_t array_required_length(FIELD_TYPE ft, size_t count)
        {
            return field_size(base_type(ft))*count+4;
        }
        ArrayValue make_array(void *buf, FIELD_TYPE ft, ArrayValue arr);
        ArrayValue make_array(void *buf, FIELD_TYPE ft, const value_list_t &vl);
        OFFSET make_array(mem_pool *pool, FIELD_TYPE ft, ArrayValue arr);
        OFFSET make_array(mem_pool *pool, FIELD_TYPE ft, const value_list_t &vl);
        
        inline uint64_t hash_function(Value v)
        {
            switch (v.type_) {
                case VT_INTEGER:
                    return v.number;
                case VT_STRING:
                    return hash_function(v.string);
                case VT_GEOLOCATION:
                case VT_DOUBLE:
                case VT_ARRAY:
                default:
                    break;
            }
            throw argos_logic_error();
        }
    }   // End of namespace common
}   // End of namespace argos

namespace std {
    template<>
    inline void swap<argos::common::Value>(argos::common::Value &a, argos::common::Value &b)
    {
        argos::common::Value temp(a);
        a=b;
        b=temp;
    }
}

#endif
