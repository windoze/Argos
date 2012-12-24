//
//  insert_handler.h
//  Argos
//
//  Created by Windoze on 12-7-23.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifndef Argos_insert_handler_h
#define Argos_insert_handler_h

#include <utility>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include "common/concurrent_queue.h"
#include "url_handler.hpp"

namespace http {
    namespace server {
        namespace detail {
            struct update_thread_proc;
        }   // End of namespace detail

        const int INSERT_COMMAND=1;
        const int UPDATE_COMMAND=2;
        
        typedef std::pair<int, std::string> background_command_t;
        typedef argos::common::concurrent_queue< background_command_t > update_queue_t;
        
        class insert_handler : public url_handler {
        public:
            insert_handler(argos::index::Index* the_index);
            ~insert_handler();
            
            virtual void handle_request(const request& req, reply& rep);
            
            argos::index::Index *the_index_;
            std::string idx_name;
            update_queue_t the_update_queue_;
            detail::update_thread_proc *the_update_thread_proc_;
            boost::thread the_update_thread_;
            Logger logger;
            Logger acc;
            Logger err;
        };
    }   // End of namespace server
}   // End of namespace http


#endif
