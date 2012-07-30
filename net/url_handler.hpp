//
//  url_handler.h
//  Argos
//
//  Created by Windoze on 12-7-7.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifndef net_url_handler_h
#define net_url_handler_h

#include <string>
#include <boost/filesystem.hpp>
#include "request.hpp"
#include "reply.hpp"
#include "common/mem_pool_allocator.h"
#include "common/util.h"
#include "logging.h"
#include "index.h"

namespace http {
    namespace server {
        class url_handler {
        public:
            virtual void handle_request(const request& req, reply& rep)=0;
            bool url_decode(const std::string& in, std::string& out);
        };
        
        class static_url_handler : public url_handler {
        public:
            static_url_handler(const std::string &doc_root)
            : doc_root_(doc_root)
            , logger(Logger::getInstance("main"))
            , acc(Logger::getInstance("access"))
            , err(Logger::getInstance("error"))
            {}
            
            virtual void handle_request(const request& req, reply& rep);
        private:
            std::string doc_root_;
            Logger logger;
            Logger acc;
            Logger err;
        };
        
        struct timer {
            inline timer() : start_time_(argos::common::get_time())
            {}
            
            inline operator double() const { return argos::common::get_time()-start_time_; }
            
            double start_time_;
        };
        
        struct mem_counter {
            inline mem_counter()
            : mem_usage_(argos::common::get_tl_mem_pool()->get_used_size())
            {}
            
            inline operator size_t() const {
                return argos::common::get_tl_mem_pool()->get_used_size()-mem_usage_;
            }
            
            size_t mem_usage_;
        };
        
        inline std::string get_index_name(argos::index::Index *idx)
        {
            boost::filesystem::path p(idx->get_name());
            boost::filesystem::path fn=p.filename();
            return fn.string<std::string>();
        }
        
        void register_url_handler(const std::string &prefix, url_handler *handler);
        url_handler *get_url_handler(const std::string &url);

    }   // End of namespace server
}   // End of namespace http

#endif
