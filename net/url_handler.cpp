//
//  url_handler.cpp
//  Argos
//
//  Created by Windoze on 12-7-8.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <string>
#include <utility>
#include <boost/noncopyable.hpp>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "url_handler.hpp"

namespace http {
    namespace server {
        bool url_handler::url_decode(const std::string& in, std::string& out)
        {
            out.clear();
            out.reserve(in.size());
            for (std::size_t i = 0; i < in.size(); ++i)
            {
                if (in[i] == '%')
                {
                    if (i + 3 <= in.size())
                    {
                        int value = 0;
                        std::istringstream is(in.substr(i + 1, 2));
                        if (is >> std::hex >> value)
                        {
                            out += static_cast<char>(value);
                            i += 2;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        return false;
                    }
                }
                else if (in[i] == '+')
                {
                    out += ' ';
                }
                else
                {
                    out += in[i];
                }
            }
            return true;
        }

        void static_url_handler::handle_request(const request& req, reply& rep)
        {
            timer t;
            mem_counter mc;
            
            // Decode url to path.
            std::string request_path;
            if (!url_decode(req.uri, request_path))
            {
                rep = reply::stock_reply(reply::bad_request);
                LOG4CPLUS_ERROR(err, "CLIENT:" << req.peer << ", CODE:" << rep.status << " - " << req.uri);
                LOG4CPLUS_INFO(acc, "CLIENT:" << req.peer << ", CODE:" << rep.status << ", TIME:" << t*1000 << "ms, MEM:" << mc << ", QID:[], URL:" << req.uri);
                
                return;
            }
            
            // Request path must be absolute and not contain "..".
            if (request_path.empty() || request_path[0] != '/'
                || request_path.find("..") != std::string::npos)
            {
                rep = reply::stock_reply(reply::bad_request);
                LOG4CPLUS_ERROR(err, "CLIENT:" << req.peer << ", CODE:" << rep.status << " - " << req.uri);
                LOG4CPLUS_INFO(acc, "CLIENT:" << req.peer << ", CODE:" << rep.status << ", TIME:" << t*1000 << "ms, MEM:" << mc << ", QID:[], URL:" << req.uri);
                return;
            }
            
            // If path ends in slash (i.e. is a directory) then add "index.html".
            if (request_path[request_path.size() - 1] == '/')
            {
                request_path += "index.html";
            }
            
            // Determine the file extension.
            std::size_t last_slash_pos = request_path.find_last_of("/");
            std::size_t last_dot_pos = request_path.find_last_of(".");
            std::string extension;
            if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
            {
                extension = request_path.substr(last_dot_pos + 1);
            }
            
            // Open the file to send back.
            std::string full_path = doc_root_ + request_path;
            std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
            if (!is)
            {
                rep = reply::stock_reply(reply::not_found);
                LOG4CPLUS_ERROR(err, "CLIENT:" << req.peer << ", CODE:" << rep.status << " - " << req.uri);
                LOG4CPLUS_INFO(acc, "CLIENT:" << req.peer << ", CODE:" << rep.status << ", TIME:" << t*1000 << "ms, MEM:" << mc << ", QID:[], URL:" << req.uri);
                return;
            }
            
            // Fill out the reply to be sent to the client.
            rep.status = reply::ok;
            char buf[512];
            while (is.read(buf, sizeof(buf)).gcount() > 0)
            {
                rep.content.append(buf, is.gcount());
            }
            rep.headers.resize(2);
            rep.headers[0].name = "Content-Length";
            rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
            rep.headers[1].name = "Content-Type";
            rep.headers[1].value = mime_types::extension_to_type(extension);
            LOG4CPLUS_INFO(acc, "CLIENT:" << req.peer << ", CODE:" << rep.status << ", TIME:" << t*1000 << "ms, MEM:" << mc << ", CL:" << rep.content.size() << ", QID:[], URL:" << req.uri);
        }
        
        typedef std::pair<std::string, url_handler *> handler_entry_t;
        typedef std::vector<handler_entry_t> handler_registry_t;
        handler_registry_t handler_registry;
        
        void register_url_handler(const std::string &prefix, url_handler *handler)
        {
            handler_registry.push_back(handler_entry_t(prefix, handler));
        }
        
        url_handler *get_url_handler(const std::string &url)
        {
            // authority part already removed from url
            // Go backward
            for (handler_registry_t::reverse_iterator i=handler_registry.rbegin(); i!=handler_registry.rend(); i++) {
                if (strncasecmp(url.c_str(), i->first.c_str(), i->first.size())==0) {
                    return i->second;
                }
            }
            return NULL;
        }
    }   // End of namespace server
}   // End of namespace http