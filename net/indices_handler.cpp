//
//  indices_handler.cpp
//  Argos
//
//  Created by Xu Chen on 12-7-28.
//  Copyright (c) 2012年 0d0a.com. All rights reserved.
//

#include <iomanip>
#include <boost/interprocess/streams/vectorstream.hpp>
#include "common/mem_pool_allocator.h"
#include "pugixml/pugixml.hpp"
#include "indices_handler.hpp"

namespace http {
    namespace server {
        indices_handler::indices_handler(const indices_t &indices)
        : indices_(indices)
        , xsl_("/indices.xsl")
        {}
        
        indices_handler::indices_handler(const indices_t &indices, const std::string &xsl)
        : indices_(indices)
        , xsl_(xsl)
        {}

        void indices_handler::handle_request(const request& req, reply& rep)
        {
            try {
                pugi::xml_document doc;
                pugi::xml_node xml = doc.append_child(pugi::node_declaration);
                xml.set_name("xml");
                xml.append_attribute("version")="1.0";
                xml.append_attribute("encoding")="UTF-8";
                pugi::xml_node xsl = doc.append_child(pugi::node_declaration);
                xsl.set_name("xml-stylesheet");
                xsl.append_attribute("type")="text/xsl";
                xsl.append_attribute("href")=xsl_.c_str();
                pugi::xml_node indices = doc.append_child("indices");
                indices.append_attribute("count")=int(indices_.size());
                for (indices_t::const_iterator i=indices_.begin(); i!=indices_.end(); ++i) {
                    pugi::xml_node idx=indices.append_child("index");
                    idx.append_attribute("name")=i->first.c_str();
                    idx.append_attribute("path")=i->second->get_name();
                    idx.append_attribute("active")=int(i->second->get_doc_count());
                    idx.append_attribute("erased")=int(i->second->get_forward_index()->erased());
                    // TODO: Add more other info
                }
                boost::interprocess::basic_vectorstream<std::string> sst;
                doc.save(sst);
                sst.swap_vector(rep.content);
                rep.status = reply::ok;
                rep.headers.resize(2);
                rep.headers[0].name = "Content-Length";
                rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
                rep.headers[1].name = "Content-Type";
                rep.headers[1].value = "application/xml";
            }
            catch(...) {
                rep=reply::stock_reply(reply::internal_server_error);
            }
            argos::common::get_tl_mem_pool()->reset();
        }
    }   // End of namespace server
}   // End of namespace http
