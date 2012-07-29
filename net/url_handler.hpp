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
#include "request.hpp"
#include "reply.hpp"

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
            {}
            
            virtual void handle_request(const request& req, reply& rep);
        private:
            std::string doc_root_;
        };
        
        void register_url_handler(const std::string &prefix, url_handler *handler);
        url_handler *get_url_handler(const std::string &url);

    }   // End of namespace server
}   // End of namespace http

#endif
