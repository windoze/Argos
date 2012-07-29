//
//  config_handler.h
//  Argos
//
//  Created by Windoze on 12-7-11.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifndef Argos_config_handler_h
#define Argos_config_handler_h

#include "url_handler.hpp"
#include <boost/lexical_cast.hpp>

namespace http {
    namespace server {
        class config_handler : public url_handler {
        public:
            config_handler(argos::index::Index* the_index);
            
            virtual void handle_request(const request& req, reply& rep);
            
            argos::index::Index *the_index_;
        };
    }   // End of namespace server
}   // End of namespace http

#endif
