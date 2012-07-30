//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "logging.h"
#include "url_handler.hpp"
#include "query_handler.hpp"
#include "item_handler.hpp"
#include "config_handler.hpp"
#include "insert_handler.hpp"
#include "indices_handler.hpp"
#include "server.hpp"
#include "index.h"

void run_server(const indices_t &indices,
                const char *addr,
                const char *port,
                const char *doc_root,
                int threads)
{
    Logger logger = Logger::getInstance("main");

    http::server::url_handler *h=new http::server::static_url_handler(doc_root);
    http::server::register_url_handler("/", h);
    
    h=new http::server::indices_handler(indices);
    http::server::register_url_handler("/indices.xml", h);

    int n=0;
    for (indices_t::const_iterator i=indices.begin(); i!=indices.end(); ++i) {
        std::string s=i->first;
        s='/'+s;
        
        h=new http::server::query_handler(i->second.get());
        http::server::register_url_handler(s + "/query", h);
        if (n==0) {
            http::server::register_url_handler("/query", h);
        }
        
        h=new http::server::item_handler(i->second.get());
        http::server::register_url_handler(s + "/item", h);
        if (n==0) {
            http::server::register_url_handler("/item", h);
        }
        
        h=new http::server::insert_handler(i->second.get());
        http::server::register_url_handler(s + "/insert", h);
        if (n==0) {
            http::server::register_url_handler("/insert", h);
        }
        
        h=new http::server::config_handler(i->second.get());
        http::server::register_url_handler(s + "/config.xml", h);
        if (n==0) {
            http::server::register_url_handler("/config.xml", h);
        }
        
        n++;
    }
    
    LOG4CPLUS_INFO(logger, "HTTP server starts listening at " << addr << ':' << port);
    http::server::server s(addr, port, doc_root, threads);
    s.run();
    LOG4CPLUS_INFO(logger, "HTTP server stopped");
}

#if 0
int main(int argc, char* argv[])
{
    try
    {
        run_server("0.0.0.0",
                   "8765",
                   "/usr/share/doc/bash",
                   3);
        /*
         // Check command line arguments.
         if (argc != 5)
         {
         std::cerr << "Usage: http_server <address> <port> <threads> <doc_root>\n";
         std::cerr << "  For IPv4, try:\n";
         std::cerr << "    receiver 0.0.0.0 80 1 .\n";
         std::cerr << "  For IPv6, try:\n";
         std::cerr << "    receiver 0::0 80 1 .\n";
         return 1;
         }
         
         // Initialise the server.
         std::size_t num_threads = boost::lexical_cast<std::size_t>(argv[3]);
         http::server::server s(argv[1], argv[2], argv[4], num_threads);
         
         // Run the server until stopped.
         s.run();
         */
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }
    
    return 0;
}
#endif