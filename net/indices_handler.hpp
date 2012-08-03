//
//  indices_handler.h
//  Argos
//
//  Created by Xu Chen on 12-7-28.
//  Copyright (c) 2012年 0d0a.com. All rights reserved.
//

#include <map>
#include <boost/lexical_cast.hpp>
#include "url_handler.hpp"
#include "index.h"

#ifndef Argos_indices_handler_h
#define Argos_indices_handler_h

typedef std::map<std::string, argos::index::index_ptr_t> indices_t;

namespace http {
    namespace server {
        class indices_handler : public url_handler {
        public:
            indices_handler(const indices_t &indices);
            indices_handler(const indices_t &indices, const std::string &xsl);
            
            virtual void handle_request(const request& req, reply& rep);
            
            indices_t indices_;
            std::string xsl_;
            Logger logger;
            Logger acc;
            Logger err;
        };
    }   // End of namespace server
}   // End of namespace http

#endif
