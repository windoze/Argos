//
//  query_handler.cpp
//  Argos
//
//  Created by Windoze on 12-7-19.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <iomanip>
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include "index/full_index.h"
#include "query/enquire.h"
#include "parser.h"
#include "serialization.h"
#include "item_handler.hpp"

namespace http {
    namespace server {
        item_handler::item_handler(argos::index::Index* the_index)
        : the_index_(the_index)
        {}
        
        void item_handler::handle_request(const request& req, reply& rep)
        {
            double start_time=argos::common::get_time();
            boost::scoped_ptr<argos::common::ExecutionContext> pctx(the_index_->create_context());
            argos::common::ExecutionContext &ctx=*(pctx.get());
            argos::query::Query q;
            size_t orig_size=ctx.temp_pool->get_used_size();
            try {
                {
                    size_t orig_size=ctx.temp_pool->get_used_size();
                    std::stringstream sst;
                    size_t qm=req.uri.find('?');
                    if (qm==req.uri.npos) {
                        rep=reply::stock_reply(reply::bad_request);
                        return;
                    }
                    bool ret=argos::parser::parse_query(req.uri.c_str()+qm+1,
                                                        req.uri.size()-qm-1,
                                                        q,
                                                        ctx);
                    if (!ret) {
                        rep=reply::stock_reply(reply::bad_request);
                        return;
                    }
                    argos::query::Enquire enq;
                    argos::query::results_t results;
                    size_t total_hits=enq.execute(q.pks_, results, ctx);
                    // TODO: Use mem_pool to store output content
                    argos::serialization::results_serializer ser(q.fmt.c_str(), q.get_field_list(), total_hits, results.size());
                    argos::common::value_list_t vl;
                    ser.serialize_prolog(sst);
                    for (int i=0; i<results.size(); i++) {
                        ctx.set_match_info(&results[i].match_info);
                        ctx.get_index()->get_value_list(results[i].did, q.get_field_list(), vl, ctx);
                        ser.serialize_doc(sst, vl, i);
                    }
                    ser.serialize_epilog(sst);
                    rep.content=sst.str();
                    rep.status = reply::ok;
                    rep.headers.resize(2);
                    rep.headers[0].name = "Content-Length";
                    rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
                    rep.headers[1].name = "Content-Type";
                    rep.headers[1].value = ser.content_type();
                    
                    std::cout << "Memory used in this query :" << argos::common::get_tl_mem_pool()->get_used_size()-orig_size << std::endl;
                }
            }
            catch(argos::argos_logic_error &e) {
                rep=reply::stock_reply(reply::bad_request);
            }
            catch(...) {
                rep=reply::stock_reply(reply::internal_server_error);
            }
            double end_time=argos::common::get_time();
            // TODO: Log
            std::cout << '[' << std::setiosflags(std::ios::fixed) << start_time << "][QID:" << q.query_id_ <<"][CODE:" << rep.status << "] Time: " << (end_time-start_time)*1000 << "ms, Memory: " << argos::common::get_tl_mem_pool()->get_used_size()-orig_size << std::endl;
            // TODO: Set a meaningful threshold
            if (ctx.temp_pool->get_used_size()>3*1024*1024) {
                ctx.temp_pool->reset();
            }
        }
    }   // End of namespace server
}   // End of namespace http