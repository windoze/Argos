//
//  insert_handler.cpp
//  Argos
//
//  Created by Windoze on 12-7-23.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

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
                {}
                
                bool insert_document(const std::string &doc)
                {
                    if (update_count_>update_threshold) {
                        ctx.temp_pool->reset();
                        update_count_=0;
                    }
                    update_count_++;
                    
                    argos::common::value_list_t vl;
                    const char *s=doc.c_str();
                    size_t len=doc.size();
                    if(!argos::parser::parse_value_list(s, len, vl, ctx))
                        return false;
                    for (argos::common::value_list_t::const_iterator i=vl.begin(); i!=vl.end(); ++i) {
                        if (i->type_==argos::common::VT_EMPTY) {
                            return false;
                        }
                    }
                    return argos::is_valid(index_->add_document(vl, ctx));
                }
                
            private:
                argos::index::Index *index_;
                argos::common::ExecutionContext &ctx;
                size_t update_count_;
            };
            
            struct update_thread_proc {
                update_thread_proc(update_queue_t &q, argos::index::Index *index)
                : upd_queue_(q)
                , updater_(index)
                {}
                
                void operator()() {
                    std::string s;
                    while (upd_queue_.pop(s)) {
                        updater_.insert_document(s);
                    }
                }
                
                void insert_document(const std::string &doc)
                {
                    upd_queue_.push(doc);
                }
                
                update_queue_t &upd_queue_;
                index_updater updater_;
            };
        }   // End of namespace detail
        
        insert_handler::insert_handler(argos::index::Index* the_index)
        : the_index_(the_index)
        , the_update_queue_()
        , the_update_thread_proc_(new detail::update_thread_proc(the_update_queue_, the_index))
        , the_update_thread_(*the_update_thread_proc_)
        , idx_name(get_index_name(the_index))
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
                the_update_thread_proc_->insert_document(doc);
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
