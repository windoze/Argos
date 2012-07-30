//
//  query_handler.h
//  Argos
//
//  Created by Windoze on 12-7-19.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifndef Argos_item_handler_h
#define Argos_item_handler_h

#include "url_handler.hpp"
#include <boost/lexical_cast.hpp>

namespace http {
    namespace server {
        class item_handler : public url_handler {
        public:
            item_handler(argos::index::Index* the_index);
            
            virtual void handle_request(const request& req, reply& rep);
            
            argos::index::Index *the_index_;
            std::string idx_name;
            Logger logger;
            Logger acc;
            Logger err;
        };
    }   // End of namespace server
}   // End of namespace http

#endif
