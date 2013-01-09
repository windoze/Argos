//
//  evaluatable.h
//  Argos
//
//  Created by Windoze on 12-6-25.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <vector>
#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include "common/value.h"

#ifndef Argos_evaluatable_h
#define Argos_evaluatable_h

namespace argos {
    namespace common {
        typedef enum {
            ET_NONE=0,
            ET_CONST=1,
            ET_EXPR=2,
            ET_ATTR=3,
            ET_DOC=4
        } EVA_TYPE;
        
        class ExecutionContext;
        
        /**
         * Abstract base class of all evaluatables
         */
        class Evaluatable {
        public:
            
            /**
             * Virtual destructor
             */
            virtual ~Evaluatable(){}

            /**
             * Returns type of this evaluatable
             */
            virtual int eva_type() const=0;

            /**
             * Returns true if this evaluatable is constant
             */
            virtual bool is_const() const=0;

            /**
             * Returns true if this evaluatable needs per-doc match info
             */
            virtual bool uses_match_info() const=0;
            
            /**
             * Evaluate this evaluatable, returns result value
             *
             * @param did the doc used during the evaluation
             * @param context the context contains necessary info
             */
            virtual Value evaluate(docid did, ExecutionContext &context) const=0;

            /**
             * Serialize this evaluatable, returnd value must can be re-parsed into same evaluatable
             */
            inline std::string to_string() const
            {
                if (name_.empty()) {
                    return to_string_impl();
                }
                return name_;
            }

            /**
             * Serialize this evaluatable into a std::ostream
             */
            inline std::ostream &serialize(std::ostream &os) const {
                os << to_string();
                return os;
            }
            
            inline const std::string &get_name() const {
                return name_;
            }
            
            inline void set_name(const std::string &name) {
                name_=name;
            }
            
            virtual std::string to_string_impl() const=0;
            
        private:
            std::string name_;
        };
        
        typedef boost::shared_ptr<Evaluatable> eva_ptr_t;
        typedef std::vector<eva_ptr_t>  field_list_t;

        inline std::ostream &operator<<(std::ostream &os, eva_ptr_t e) {
            e->serialize(os);
            return os;
        }
        
        /**
         * class Constant
         */
        class Constant : public Evaluatable {
        public:
            /**
             * Constructor
             *
             * @param v the value used to construct this constant
             */
            Constant(const common::Value &v)
            : value(v)
            {}
            
            /**
             * This is a constant
             */
            virtual int eva_type() const { return ET_CONST; }

            /**
             * This is a constant
             */
            virtual bool is_const() const { return true; }
            virtual bool uses_match_info() const { return false; }
            virtual common::Value evaluate(docid did, ExecutionContext &context) const { return value; }
            virtual std::string to_string_impl() const;
        private:
            common::Value value;
        };
    }   // End of common
}   // End of argos


#endif
