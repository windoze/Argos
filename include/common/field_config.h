//
//  field_config.h
//  Argos
//
//  Created by Windoze on 12-6-20.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <iostream>
#include "common/argos_consts.h"
#include "common/hash_table.h"

#ifndef Argos_field_config_h
#define Argos_field_config_h

namespace argos {
    namespace common {
        typedef enum {
            FT_INVALID  =0,
            FT_INT8     =1,
            FT_INT16    =2,
            FT_INT32    =3,
            FT_INT64    =4,
            FT_FLOAT    =5,
            FT_DOUBLE   =6,
            FT_GEOLOC   =7,     // 2 floats
            FT_STRING   =8,
            FT_MULTI    =0x80
        } FIELD_TYPE;

        extern const uint8_t field_sizes[];
        extern const char *field_type_name[];
        
        /**
         * Base type for given type, if given type is an array, it returns the element type
         */
        inline FIELD_TYPE base_type(FIELD_TYPE ft)
        {
            return (FIELD_TYPE)(ft & ~FT_MULTI);
        }
        
        /**
         * Array type of given element type, does nothing if an array type is given
         */
        inline FIELD_TYPE array_type(FIELD_TYPE ft)
        {
            return (FIELD_TYPE)(ft | FT_MULTI);
        }
        
        /**
         * Return element size in bytes, it always returns 8 for variant-size types such as string and array
         */
        inline uint8_t field_size(FIELD_TYPE ft)
        {
            return ((ft & FT_MULTI)==FT_MULTI) ? 8 : field_sizes[ft];
        }    

        typedef enum {
            // Placeholder
            FF_NONE =0x00,
            // The field should be indexed in reverse index
            FF_INDEX=0x01,
            // The field should be stored in forward index
            FF_STORE=0x02
        } FIELD_FLAG;

        /**
         * Simplfied field definition, used during querying for speed
         */
        struct FieldInfo {
            FIELD_TYPE type_;
            inline uint8_t size() const { return field_size(type_); }
            inline bool has_data() const { return ((type_ & FT_STRING)==FT_STRING) || ((type_ & FT_MULTI)==FT_MULTI); }
        };
        
        static const struct FieldInfo INVALID_FIELD={FT_INVALID};
        
        /**
         * Defined a field, includes name, type, and other info
         */
        class FieldDefinition {
        public:
            /**
             * Constructor
             */
            FieldDefinition(const char *name, FIELD_TYPE type, FIELD_FLAG flags, const char *ns=NULL, const char *ana=NULL);
            
            /**
             * Returns the prefix need to be added for each term
             */
            const char *get_prefix() const { return ns_; }
            
            /**
             * Returns the name of the field
             */
            const char *get_name() const { return name_; }

            /**
             * Returns the name of analyzer
             */
            const char *get_analyzer_name() const { return analyzer_; }
            
            /**
             * Returns the size of the field
             */
            uint8_t size() const { return field_size(type_); }

            /**
             * Returns true if the field need to be indexed
             */
            bool indexed() const { return flags_ & FF_INDEX; }

            /**
             * Returns true if the field need to be indexed
             */
            bool stored() const { return flags_ & FF_STORE; }

            /**
             * Returns simplified field info
             */
            inline FieldInfo get_field_info() const 
            {
                FieldInfo fi;
                fi.type_=FT_INVALID;
                if ((flags_ & FF_STORE)==FF_STORE) {
                    fi.type_=type_;
                }
                return fi;
            }

        private:
            FieldDefinition(){}
            char name_[32];
            FIELD_TYPE type_;
            FIELD_FLAG flags_;
            char ns_[33];
            char analyzer_[33];
            int field_id_;
            friend class FieldConfig;
        };
        
        extern const FieldDefinition PRIMART_KEY_FIELD;
        
        /**
         * class FieldConfig contains all fields in an index
         */
        class FieldConfig {
        public:
            /**
             * Constructor
             */
            FieldConfig();            
            /**
             * Return the id of field
             */
            int get_field_id(const char *name) const;
            /**
             * Return the id of field
             */
            int get_field_id(const char *name, size_t sz) const;
            /**
             * Return the FieldInfo of field
             */
            FieldInfo get_field(const char *name) const;
            /**
             * Return the FieldInfo of field
             */
            FieldInfo get_field(int fid) const;
            /**
             * Return the FieldDefinition of field
             */
            FieldDefinition *get_field_def(int fid) const;
            /**
             * Return the FieldDefinition of field
             */
            FieldDefinition *get_field_def(const char *name) const;
            /**
             * Add a new field
             */
            int add_field(const FieldDefinition *fd);            
            /**
             * TODO: Checksum, the function is unfinished
             */
            primary_key checksum() const { return checksum_; }
            
            /**
             * Return the count of fields
             */
            inline uint32_t count() const { return uint32_t(field_count_); }
            
            /**
             * Serialize the field config into string
             */
            std::string serialize() const;

            /**
             * Load the field config from std::istream
             */
            static FieldConfig *load(std::istream &is);
        private:
            typedef common::hash_table<const char *, int> field_map_t;
            typedef common::offptr_t<FieldDefinition> field_ptr_t;
            field_map_t field_map_;
            common::mem_pool field_pool_;
            size_t field_count_;
            primary_key checksum_;
        };
    }   // End of namespace index
}   // End of namespace argos

#endif
