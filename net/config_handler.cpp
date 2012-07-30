//
//  config_handler.cpp
//  Argos
//
//  Created by Windoze on 12-7-11.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/field_config.h"
#include "index/full_index.h"
#include "config_handler.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

namespace http {
    namespace server {
        config_handler::config_handler(argos::index::Index* the_index)
        : the_index_(the_index)
        , idx_name(get_index_name(the_index))
        , logger(Logger::getInstance("main"))
        , acc(Logger::getInstance(std::string("access.")+idx_name))
        , err(Logger::getInstance(std::string("error.")+idx_name))
        {}
        
        void config_handler::handle_request(const request& req, reply& rep)
        {
            timer t;
            mem_counter mc;
            
            boost::scoped_ptr<argos::common::ExecutionContext> pctx(the_index_->create_context());
            argos::common::ExecutionContext &ctx=*(pctx.get());
            try {
                rep.content=the_index_->get_field_config()->serialize();
                rep.status = reply::ok;
                rep.headers.resize(2);
                rep.headers[0].name = "Content-Length";
                rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
                rep.headers[1].name = "Content-Type";
                rep.headers[1].value = "application/xml";
            }
            catch(...) {
                rep=reply::stock_reply(reply::internal_server_error);
                LOG4CPLUS_ERROR(err, rep.status << " - " << req.uri);
            }
            LOG4CPLUS_INFO(acc, "CODE:" << rep.status << ", TIME:" << t*1000 << "ms, MEM:" << mc << ", CL:" << rep.content.size() << ", QID:[], URL:" << req.uri);
            if (ctx.temp_pool->get_used_size()>3*1024*1024) {
                ctx.temp_pool->reset();
            }
        }
    }   // End of namespace server
}   // End of namespace http