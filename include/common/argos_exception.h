//
//  argos_exception.h
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <string>
#include <sstream>
#include <exception>

#ifndef Argos_argos_exception_h
#define Argos_argos_exception_h

namespace argos {
    // Exception classes
    
    /**
     * Base class of all argos specific exceptions
     */
    class argos_exception : public std::exception
    {
    public:
        argos_exception() throw(){}
        virtual ~argos_exception() throw() {}
        virtual const char *what() const throw() { return msg.c_str(); }
    protected:
        std::string msg;
    };
    
    /**
     * Failed to allocate memory
     */
    class argos_bad_alloc : public argos_exception
    {};

    /**
     * Base class of all argos runtime errors
     */
    class argos_runtime_error : public argos_exception
    {};
    
    /**
     * mem_pool failed to complete specific operations
     */
    class argos_mem_pool_error : public argos_runtime_error
    {
    public:
        argos_mem_pool_error(bool loading, const char *name) throw()
        {
            if (name && name[0]) {
                if (loading) {
                    msg="Fail to load mem_pool ";
                    msg+=name;
                } else {
                    msg="Fail to create mem_pool ";
                    msg+=name;
                }
            } else {
                msg="Fail to create anonymous mem_pool";
                msg+=name;
            }
        }
    };
    
    /**
     * Base class of all argos logical errors
     */
    class argos_logic_error : public argos_exception
    {
    public:
        argos_logic_error() throw(){}
        argos_logic_error(const char *err) throw()
        {
            std::stringstream sst;
            sst << "Logical error: " << err;
            msg =sst.str();
        }
    };
    
    /**
     * Syntax error in expressions
     */
    class argos_syntax_error : public argos_logic_error
    {
    public:
        argos_syntax_error(const char *err) throw()
        {
            std::stringstream sst;
            sst << "Syntax error: " << err;
            msg =sst.str();
        }
    };
    
    /**
     * Field doesn't exist
     */
    class argos_bad_field : public argos_logic_error
    {
    public:
        argos_bad_field(const char *name) throw()
        {
            std::stringstream sst;
            sst << "Bad field name :" << name;
            msg =sst.str();
        }
        argos_bad_field(int fid) throw()
        {
            std::stringstream sst;
            sst << "Bad field id :" << fid;
            msg =sst.str();
        }
    };
    
    /**
     * Operator doesn't exist
     */
    class argos_bad_operator : public argos_logic_error
    {
    public:
        argos_bad_operator(const char *name) throw()
        {
            std::stringstream sst;
            sst << "Bad operator :" << name;
            msg =sst.str();
        }
    };
}   // End of namespace argos

#endif
