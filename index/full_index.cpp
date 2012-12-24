//
//  full_index.cpp
//  Argos
//
//  Created by Windoze on 12-7-2.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include "index/forward_index.h"
#include "index/reverse_index.h"
#include "index/full_index.h"

namespace argos {
    namespace index {
        const int AUTO_COMMIT_THRESHOLD=100000;
        
        /**
         * Constructor, load an index
         */
        Index::Index(const char *path)
        : opened_(false)
        , fc_(0)
        , ri_(0)
        , fi_(0)
        , auto_commit_count_(0)
        {
            char buf[PATH_MAX];
            memset(buf, 0, PATH_MAX);
            strcpy(buf, path);
            strcat(buf, "/config.xml");
            std::ifstream is(buf);
            if (is) {
                opened_=init(is, path, INDEX_OPEN);
            }
        }
        
        Index::Index(const char *conf_file, const char *path, int open_flag)
        : opened_(false)
        , fc_(0)
        , ri_(0)
        , fi_(0)
        , auto_commit_count_(0)
        {
            std::ifstream is(conf_file);
            opened_=init(is, path, open_flag);
        }
        
        Index::Index(std::istream &is, const char *path, int open_flag)
        : opened_(false)
        , fc_(0)
        , ri_(0)
        , fi_(0)
        , auto_commit_count_(0)
        {
            opened_=init(is, path, open_flag);
        }
        
        Index::~Index()
        {
            flush();
            if (ri_) {
                delete ri_;
            }
            if (fi_) {
                delete fi_;
            }
            if (fc_) {
                delete fc_;
            }
        }
        
        void Index::flush()
        {
            if (fi_) {
                fi_->flush();
            }
            if (ri_) {
                ri_->flush();
            }
        }
        
        bool Index::init(std::istream &is, const char *path, int open_flag)
        {
            name_[0]=0;
            if (path && path[0]) {
                strcpy(name_, path);
            }
            if (name_[0]) {
                // Prepare directory
                switch (open_flag) {
                    case INDEX_CREATE_OR_OPEN:
                    {
                        DIR *dir=opendir(name_);
                        if (!dir) {
                            // Create dir
                            if(mkdir(name_, 0755)!=0) {
                                return false;
                            }
                        } else {
                            closedir(dir);
                        }
                    }
                        break;
                    case INDEX_CREATE:
                    {
                        DIR *dir=opendir(name_);
                        if (dir) {
                            // Directory exists, failed
                            closedir(dir);
                            return false;
                        }
                        // Create dir
                        if(mkdir(name_, 0755)!=0) {
                            return false;
                        }
                    }
                        break;
                    case INDEX_OPEN:
                    {
                        DIR *dir=opendir(name_);
                        if (!dir) {
                            return false;
                        }
                        closedir(dir);
                    }
                        break;
                    default:
                        break;
                }
            }
            
            fc_=common::FieldConfig::load(is);
            if (!fc_) {
                return false;
            }
            
            char buf[PATH_MAX];
            ri_=create_reverse_index(common::add_suffix(buf, name_, "/rindex"));
            if (!ri_) {
                delete fc_;
                return false;
            }
            
            fi_=new index::ForwardIndex(common::add_suffix(buf, name_, "/findex"), fc_);
            if (!fi_) {
                delete ri_;
                delete fc_;
                return false;
            }
            
            if (open_flag==INDEX_CREATE || open_flag==INDEX_CREATE_OR_OPEN) {
                if (path && path[0]) {
                    // Save config.xml file under the index directory if this index has a name
                    char buf[PATH_MAX];
                    memset(buf, 0, PATH_MAX);
                    strcpy(buf, path);
                    strcat(buf, "/config.xml");
                    std::ofstream os(buf);
                    os << fc_->serialize();
                }
            }
            return true;
        }
        
        common::ExecutionContext *Index::create_context()
        {
            common::ExecutionContext *ret=new common::ExecutionContext();
            ret->index=this;
            ret->temp_pool=argos::common::get_tl_mem_pool();
            return ret;
        }
        
        size_t Index::get_doc_count() const
        {
            if (!opened_) {
                return 0;
            }
            return fi_->get_last_doc()-fi_->erased();
        }
        
        primary_key Index::get_primary_key(docid did) const
        {
            if (!opened_) {
                return INVALID_PK;
            }
            return fi_->get_primary_key(did);
        }
        
        docid Index::get_docid(primary_key pk) const
        {
            if (!opened_) {
                return INVALID_DOC;
            }
            return fi_->get_docid(pk);
        }
        
        docid Index::add_document(const common::value_list_t &vl, common::ExecutionContext &ctx)
        {
            if (auto_commit_count_>=AUTO_COMMIT_THRESHOLD) {
                flush();
                auto_commit_count_=0;
            }
            auto_commit_count_++;
            if (!opened_) {
                return INVALID_DOC;
            }
            docid did=fi_->add_document(vl);
            if (!is_valid(did)) {
                return did;
            }
            if(!ri_->add_document(did, vl, ctx))
            {
                fi_->delete_document(did);
                return INVALID_DOC;
            }
            return did;
        }
        
        bool Index::delete_document(primary_key pk)
        {
            if (!opened_) {
                return false;
            }
            docid did=fi_->get_docid(pk);
            if (!is_valid(did)) {
                return false;
            }
            fi_->delete_document(did);
            return true;
        }
        
        bool Index::is_deleted(docid did) const
        {
            if (!opened_) {
                return true;
            }
            return fi_->is_deleted(did);
        }
        
        bool Index::is_deleted(primary_key pk) const
        {
            if (!opened_) {
                return true;
            }
            return fi_->is_deleted(fi_->get_docid(pk));
        }

        common::Value Index::get_doc_field(primary_key pk, int fid) const
        {
            if (!opened_) {
                return common::Value();
            }
            docid did=fi_->get_docid(pk);
            return get_doc_field(did, fid);
        }
        
        common::Value Index::get_doc_field(docid did, int fid) const
        {
            if (!opened_) {
                return common::Value();
            }
            if (!is_valid(did) || did>fi_->get_last_doc()) {
                return common::Value();
            }
            if (fid<0 || fid>=fi_->get_field_config()->count()) {
                return common::Value();
            }
            common::FieldInfo fi=get_field_config()->get_field(fid);
            uint8_t sz=fi.size();
            void *p=fi_->get_storage()->get_field_pool(fid)->get_addr(sz*did);
            common:: Value ret;
            if (fi.type_ & common:: FT_MULTI) {
                ret.type_=common:: VT_ARRAY;
                common::ArrayValue arr;
                arr.data=(int8_t *)(fi_->get_storage()->get_field_data_pool(fid)->get_addr(*(common::OFFSET *)p));
                ret.array=arr;
                return ret;
            }
            switch (fi.type_) {
                case common::FT_INT8:
                    ret.type_=common::VT_INTEGER;
                    ret.number=*((int8_t *)p);
                    break;
                case common::FT_INT16:
                    ret.type_=common::VT_INTEGER;
                    ret.number=*((int16_t *)p);
                    break;
                case common::FT_INT32:
                    ret.type_=common::VT_INTEGER;
                    ret.number=*((int32_t *)p);
                    break;
                case common::FT_INT64:
                    ret.type_=common::VT_INTEGER;
                    ret.number=*((int64_t *)p);
                    break;
                case common::FT_FLOAT:
                    ret.type_=common::VT_DOUBLE;
                    ret.dnumber=*((float *)p);
                    break;
                case common::FT_DOUBLE:
                    ret.type_=common::VT_DOUBLE;
                    ret.dnumber=*((double *)p);
                    break;
                case common::FT_GEOLOC:
                    ret.type_=common::VT_GEOLOCATION;
                    ret.geolocation=*((common::GeoLocationValue *)p);
                    break;
                case common::FT_STRING:
                    ret.type_=common::VT_STRING;
                    ret.string=(const char *)(fi_->get_storage()->get_field_data_pool(fid)->get_addr(*(common::OFFSET *)p));
                    break;
                case common::FT_INVALID:
                default:
                    break;
            }
            return ret;
        }
        
        bool Index::get_value_list(docid did, const common::field_list_t &fl, common::value_list_t &vl, common::ExecutionContext &ctx) const
        {
            if (!opened_) {
                return false;
            }
            vl.resize(fl.size());
            for (int i=0; i<fl.size(); ++i) {
                vl[i]=fl[i]->evaluate(did, ctx);
            }
            return true;
        }
        
        bool Index::get_value_list(primary_key pk, const common::field_list_t &fl, common::value_list_t &vl, common::ExecutionContext &ctx) const
        {
            if (!opened_) {
                return false;
            }
            return get_value_list(fi_->get_docid(pk), fl, vl, ctx);
        }
        
        bool Index::set_doc_field(primary_key pk, int fid, common::Value v)
        {
            docid did=get_docid(pk);
            if (!is_valid(did)) {
                return false;
            }
            return fi_->set_field_value(fid, did, v);
        }
        
        bool Index::set_doc_field(docid did, int fid, common::Value v)
        {
            if (!is_valid(did)) {
                return false;
            }
            return fi_->set_field_value(fid, did, v);
        }
    }   // End of namespace index
}   // End of namespace argos