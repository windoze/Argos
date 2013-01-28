//
//  insert_handler.cpp
//  Argos
//
//  Created by Windoze on 12-7-23.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <boost/thread/thread.hpp>
#include "common/concurrent_queue.h"
#include "parser.h"
#include "index.h"
#include "insert_handler.hpp"

namespace http {
    namespace server {
        namespace detail {
            const size_t update_threshold=100;
            
            class index_updater {
            public:
                index_updater(argos::index::Index *index)
                : index_(index)
                , ctx(*index_->create_context())
                , update_count_(0)
                , err(Logger::getInstance("error"))
                {}
                
                bool insert_document(const background_command_t &doc)
                {
                    if (update_count_>update_threshold) {
                        ctx.temp_pool->reset();
                        update_count_=0;
                    }
                    update_count_++;
                    
                    argos::common::value_list_t vl;
                    const char *s=doc.second.c_str();
                    size_t len=doc.second.size();
                    if(!argos::parser::parse_value_list(s, len, vl, ctx)) {
                        LOG4CPLUS_ERROR(err, "Document parse failed :\"" << doc.second << "\"");
                        return false;
                    }
                    for (argos::common::value_list_t::const_iterator i=vl.begin(); i!=vl.end(); ++i) {
                        if (i->type_==argos::common::VT_EMPTY) {
                            LOG4CPLUS_ERROR(err, "Document contains empty field :\"" << doc.second << "\"");
                            return false;
                        }
                    }
                    bool ret=argos::is_valid(index_->add_document(vl, ctx));
                    if (!ret) {
                        LOG4CPLUS_ERROR(err, "Insert document failed :\"" << doc.second << "\"");
                    }
                    return ret;
                }
                
                bool update_document(const background_command_t &doc)
                {
                    if (update_count_>update_threshold) {
                        ctx.temp_pool->reset();
                        update_count_=0;
                    }
                    update_count_++;
                    
                    argos::common::value_list_t vl;
                    const char *s=doc.second.c_str();
                    size_t len=doc.second.size();
                    if(!argos::parser::parse_value_list(s, len, vl, ctx)) {
                        LOG4CPLUS_ERROR(err, "Document parse failed :\"" << doc.second << "\"");
                        return false;
                    }
                    // Check value list length, make sure it has correct number of fields
                    if (vl.size()!=index_->get_field_config()->count()) {
                        LOG4CPLUS_ERROR(err, "Document has wrong number of fields :\"" << doc.second << "\"");
                        return false;
                    }
                    for (size_t i=0; i<vl.size(); i++) {
                        // Check each field, make sure all indexed field are null
                        argos::common::FieldDefinition *fd=index_->get_field_config()->get_field_def(i);
                        if ((fd->indexed()) && (vl[i].type_!=argos::common::VT_EMPTY)) {
                            LOG4CPLUS_ERROR(err, "Cannot update indexed field[" << i << ",\"" <<fd->get_name() << "\"] :\"" << doc.second << "\"");
                            return false;
                        } else {
                            // Try to convert field to target type, make sure it's convertable
                            if (vl[i].type_==argos::common::VT_EMPTY) {
                                continue;
                            }
                            if (vl[i].coerce(to_value_type(fd->get_field_info().type_))==argos::common::VT_EMPTY) {
                                LOG4CPLUS_ERROR(err, "Incompatible type at field[" << i << ",\"" <<fd->get_name() << "\"] :\"" << doc.second << "\"");
                                return false;
                            }
                        }
                    }
                    
                    // Everything is OK, now update the document
                    argos::primary_key id=vl[0].cast(argos::common::VT_INTEGER).number;
                    
                    // Start from 2nd field, 1st is the primary key
                    for (size_t i=1; i<vl.size(); i++) {
                        if (vl[i].type_!=argos::common::VT_EMPTY) {
                            index_->set_doc_field(id, i, vl[i]);
                        }
                    }
                    return false;
                }
                
            private:
                argos::index::Index *index_;
                argos::common::ExecutionContext &ctx;
                size_t update_count_;
                Logger err;
            };
            
            struct update_thread_proc {
                update_thread_proc(update_queue_t &q, argos::index::Index *index)
                : upd_queue_(q)
                , updater_(index)
                {}
                
                void operator()() {
                    background_command_t c;
                    while (upd_queue_.pop(c)) {
                        switch (c.first) {
                            case INSERT_COMMAND:
                                updater_.insert_document(c);
                                break;
                            case UPDATE_COMMAND:
                                updater_.update_document(c);
                                break;
                            default:
                                // TODO: Error, Unknown command
                                break;
                        }
                    }
                }
                
                void insert_or_update_document(const background_command_t &doc)
                {
                    upd_queue_.push(doc);
                }
                
                update_queue_t &upd_queue_;
                index_updater updater_;
            };
        }   // End of namespace detail
        
        insert_handler::insert_handler(argos::index::Index* the_index)
        : the_index_(the_index)
        , idx_name(get_index_name(the_index))
        , the_update_queue_()
        , the_update_thread_proc_(new detail::update_thread_proc(the_update_queue_, the_index))
        , the_update_thread_(*the_update_thread_proc_)
        , logger(Logger::getInstance("main"))
        , acc(Logger::getInstance(std::string("access.")+idx_name))
        , err(Logger::getInstance(std::string("error.")+idx_name))
        {}
        
        insert_handler::~insert_handler()
        {
            the_update_queue_.close();
            the_update_thread_.join();
            delete the_update_thread_proc_;
        }
        
        void insert_handler::handle_request(const request& req, reply& rep) {
            timer t;
            mem_counter mc;
            
            try {
                size_t qm=req.uri.find('?');
                if (qm==req.uri.npos) {
                    rep=reply::stock_reply(reply::bad_request);
                    throw argos::argos_logic_error("");
                }
                std::string data(req.uri.c_str()+qm+1, req.uri.size()-qm-1);
                std::string doc;
                url_decode(data, doc);
                if (req.uri[qm-6]=='i') {
                    // insert
                    the_update_thread_proc_->insert_or_update_document(std::make_pair(INSERT_COMMAND, doc));
                } else if (req.uri[qm-6]=='u') {
                    // update
                    the_update_thread_proc_->insert_or_update_document(std::make_pair(UPDATE_COMMAND, doc));
                } else {
                    // TODO: Error
                }
                rep=reply::stock_reply(reply::ok);
            }
            catch(argos::argos_logic_error &e) {
                rep=reply::stock_reply(reply::bad_request);
                LOG4CPLUS_ERROR(err, "CLIENT:" << req.peer << ", CODE:" << rep.status << " - " << req.uri);
            }
            catch(...) {
                rep=reply::stock_reply(reply::internal_server_error);
                LOG4CPLUS_ERROR(err, "CLIENT:" << req.peer << ", CODE:" << rep.status << " - " << req.uri);
            }
            LOG4CPLUS_INFO(acc, "CLIENT:" << req.peer << ", CODE:" << rep.status << ", TIME:" << t*1000 << "ms, MEM:" << mc << ", CL:" << rep.content.size() << ", QID:[], URL:" << req.uri);
        }
    }   // End of namespace server
}   // End of namespace http
