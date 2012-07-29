//
//  field_config.cpp
//  Argos
//
//  Created by Windoze on 12-6-27.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <sstream>
#include "pugixml/pugixml.hpp"
#include "common/field_config.h"

namespace argos {
    namespace common {
        const uint8_t field_sizes[]={0, 1, 2, 4, 8, sizeof(float), sizeof(double), sizeof(float)*2, 8};
        const char *field_type_name[]={"",
            "int8",
            "int16",
            "int32",
            "int64",
            "float",
            "double",
            "geo", /*TODO*/
            "text"
        };
        
        const FieldDefinition PRIMART_KEY_FIELD("_id", FT_INT64, FF_STORE);
        
        FieldDefinition::FieldDefinition(const char *name, FIELD_TYPE type, FIELD_FLAG flags, const char *ns, const char *ana)
        : type_(type)
        , flags_(flags)
        , field_id_(0)
        {
            name_[0]=0;
            if (name) {
                strcpy(name_, name);
            }
            ns_[0]=0;
            if (ns) {
                if (ns[0]) {
                    strcpy(ns_, ns);
                }
            } else {
                strcpy(ns_, name);
            }
            strcat(ns_, "=");
            
            if (ana && ana[0] && (type_==FT_STRING)) {
                // Only apply analyzer to string field
                strcpy(analyzer_, ana);
            } else {
                strcpy(analyzer_, "token");
            }
        }
        
        FieldConfig::FieldConfig()
        : field_pool_(sizeof(FieldDefinition)*100)
        , field_count_(0)
        , checksum_(0)
        {
            add_field(&PRIMART_KEY_FIELD);
        }
        
        int FieldConfig::get_field_id(const char *name) const
        {
            int fid=-1;
            if(!field_map_.find(name, fid))
                return -1;
            return fid;
        }
        
        int FieldConfig::get_field_id(const char *name, size_t sz) const
        {
            char buf[256];
            if (sz>255) {
                sz=255;
            }
            memcpy(buf, name, sz);
            buf[sz]=0;
            return get_field_id(buf);
        }
        
        FieldInfo FieldConfig::get_field(const char *name) const
        {
            int fid=get_field_id(name);
            if (fid<0) {
                return INVALID_FIELD;
            }
            return get_field(fid);
        }
        
        FieldInfo FieldConfig::get_field(int fid) const
        {
            if (fid<0 || fid>=field_count_) {
                return INVALID_FIELD;
            }
            field_ptr_t p(&field_pool_, 0);
            return p[fid].get_field_info();
        }
        
        FieldDefinition *FieldConfig::get_field_def(int fid) const
        {
            if (fid<0 || fid>=field_count_) {
                return NULL;
            }
            field_ptr_t p(&field_pool_, 0);
            return &(p[fid]);
        }
        
        FieldDefinition *FieldConfig::get_field_def(const char *name) const
        {
            int fid=get_field_id(name);
            if (fid<0 || fid>=field_count_) {
                return NULL;
            }
            return get_field_def(fid);
        }
        
        int FieldConfig::add_field(const FieldDefinition *fd)
        {
            int fid=get_field_id(fd->get_name());
            if (fid>=0) {
                // Field Exists, do nothing
                return fid;
            }
            fid=int(field_count_);
            field_ptr_t p(&field_pool_, field_pool_.add_chunk(fd, sizeof(FieldDefinition)));
            field_map_.insert(fd->get_name(), fid);
            field_count_++;
            return fid;
        }

        std::string FieldConfig::serialize() const
        {
            std::stringstream sst;
            pugi::xml_document doc;
            
            pugi::xml_node xml = doc.append_child(pugi::node_declaration);
            xml.set_name("xml");
            xml.append_attribute("version")="1.0";
            xml.append_attribute("encoding")="UTF-8";
            pugi::xml_node xsl = doc.append_child(pugi::node_declaration);
            xsl.set_name("xml-stylesheet");
            xsl.append_attribute("type")="text/xsl";
            xsl.append_attribute("href")="/config.xsl";
            pugi::xml_node fields = doc.append_child("config").append_child("fields");
            for (int i=1; i<count(); i++) {
                pugi::xml_node field=fields.append_child("field");
                FieldDefinition *fd=get_field_def(i);
                field.append_attribute("name")=fd->name_;
                switch (base_type(get_field_def(i)->type_)) {
                    case FT_INT8:
                        field.append_attribute("type")="integer";
                        field.append_attribute("byte")=1;
                        break;
                    case FT_INT16:
                        field.append_attribute("type")="integer";
                        field.append_attribute("byte")=2;
                        break;
                    case FT_INT32:
                        field.append_attribute("type")="integer";
                        field.append_attribute("byte")=4;
                        break;
                    case FT_INT64:
                        field.append_attribute("type")="integer";
                        field.append_attribute("byte")=8;
                        break;
                    case FT_FLOAT:
                        field.append_attribute("type")="float";
                        break;
                    case FT_DOUBLE:
                        field.append_attribute("type")="double";
                        break;
                    case FT_GEOLOC:
                        field.append_attribute("type")="geolocation";
                        break;
                    case FT_STRING:
                        field.append_attribute("type")="string";
                        break;
                    default:
                        // Skip this one
                        continue;
                        break;
                }
                if (fd->type_ & FT_MULTI) {
                    field.append_attribute("multi")="true";
                }
                if (fd->flags_ & FF_STORE) {
                    field.append_attribute("store")="true";
                } else {
                    field.append_attribute("store")="false";
                }
                if (fd->flags_ & FF_INDEX) {
                    field.append_attribute("index")="true";
                    size_t nl=strlen(fd->name_);
                    if (strncmp(fd->ns_, fd->name_, nl)==0 && fd->ns_[nl]=='=' && fd->ns_[nl+1]=='\0') {
                        // Default, ns is field name
                    } else {
                        size_t nl=strlen(fd->ns_);
                        if (nl==1 && fd->ns_[0]=='=') {
                            field.append_attribute("namespace")="-";
                        } else {
                            char buf[256];
                            // no trailing '='
                            strncpy(buf, fd->ns_, nl-1);
                            buf[nl-1]=0;
                            field.append_attribute("namespace")=buf;
                        }
                    }
                    if (fd->type_==FT_STRING) {
                        // Only apply analyzer to string fields
                        field.append_attribute("analyzer")=fd->analyzer_;
                    }
                } else {
                    field.append_attribute("index")="false";
                }
            }
            doc.save(sst);
            return sst.str();
        }
        
        FieldConfig *FieldConfig::load(std::istream &is)
        {
            FieldConfig *fc=new FieldConfig;
            
            pugi::xml_document doc;
            doc.load(is);
            pugi::xml_node fields=doc.child("config").child("fields");
            if (fields) {
                for (pugi::xml_node fn = fields.child("field"); fn; fn = fn.next_sibling("field"))
                {
                    FieldDefinition fd;
                    pugi::xml_attribute name=fn.attribute("name");
                    //std::cout << name.as_string() << "\n";
                    strcpy(fd.name_, name.as_string());
                    
                    pugi::xml_attribute type=fn.attribute("type");
                    if (strcasecmp(type.as_string(), "string")==0) {
                        fd.type_=FT_STRING;
                    } else if (strcasecmp(type.as_string(), "integer")==0) {
                        pugi::xml_attribute byte=fn.attribute("byte");
                        int n=8;
                        if (byte) {
                            n=byte.as_int();
                        }
                        switch (n) {
                            case 1:
                                fd.type_=FT_INT8;
                                break;
                            case 2:
                                fd.type_=FT_INT16;
                                break;
                            case 4:
                                fd.type_=FT_INT32;
                                break;
                            case 8:
                            default:
                                fd.type_=FT_INT64;
                                break;
                        }
                    } else if (strcasecmp(type.as_string(), "float")==0) {
                        fd.type_=FT_FLOAT;
                    } else if (strcasecmp(type.as_string(), "double")==0) {
                        fd.type_=FT_DOUBLE;
                    } else if (strcasecmp(type.as_string(), "geolocation")==0) {
                        fd.type_=FT_GEOLOC;
                    }
                    
                    pugi::xml_attribute multi=fn.attribute("multi");
                    if (strcasecmp(multi.as_string(), "true")==0
                        || strcasecmp(multi.as_string(), "yes")==0) {
                        if (fd.type_!=FT_STRING) {
                            fd.type_=array_type(fd.type_);
                        }
                    }

                    fd.flags_=FF_NONE;
                    pugi::xml_attribute store=fn.attribute("store");
                    if (strcasecmp(store.as_string(), "true")==0
                        || strcasecmp(store.as_string(), "yes")==0) {
                        fd.flags_ = (FIELD_FLAG)(fd.flags_ | FF_STORE);
                    }

                    pugi::xml_attribute index=fn.attribute("index");
                    if (strcasecmp(index.as_string(), "true")==0
                        || strcasecmp(index.as_string(), "yes")==0) {
                        fd.flags_ = (FIELD_FLAG)(fd.flags_ | FF_INDEX);
                        
                        pugi::xml_attribute ns=fn.attribute("namespace");
                        if (ns) {
                            if (strcasecmp(ns.as_string(), "-")==0) {
                                // No namespace
                                fd.ns_[0]='=';
                                fd.ns_[1]='\0';
                            } else {
                                strcpy(fd.ns_, ns.as_string());
                                strcat(fd.ns_, "=");
                            }
                        } else {
                            // Default, use fieldname
                            strcpy(fd.ns_, fd.name_);
                            strcat(fd.ns_, "=");
                        }
                        
                        strcpy(fd.analyzer_, "token");
                        if (fd.type_==FT_STRING) {
                            pugi::xml_attribute ana=fn.attribute("analyzer");
                            if (ana) {
                                strcpy(fd.analyzer_, ana.as_string());
                            }
                        }
                    } else {
                        // For non-index field
                        strcpy(fd.ns_, fd.get_name());
                        strcat(fd.ns_, "=");
                    }
                    
                    fc->add_field(&fd);
                }
            }
            fc->checksum_=common::hash_function(fc->serialize().c_str());
            return fc;
        }
    }   // End of namespace common
}   // End of namespace argos