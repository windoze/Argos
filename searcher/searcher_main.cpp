//
//  searcher_main.cpp
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <map>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <log4cplus/configurator.h>
#include "logging.h"
#include "index.h"

using namespace std;
using argos::index::Index;
using argos::index::INDEX_CREATE_OR_OPEN;

namespace po = boost::program_options;

typedef std::map<std::string, argos::index::index_ptr_t> indices_t;

std::string get_index_name(argos::index::index_ptr_t idx)
{
    boost::filesystem::path p(idx->get_name());
    boost::filesystem::path fn=p.filename();
    return fn.string<std::string>();
}

bool init_index(const vector<string> &index_dirs, indices_t &indices)
{
    Logger logger = Logger::getInstance("main");
    
    for (vector<string>::const_iterator i=index_dirs.begin(); i!=index_dirs.end(); ++i) {
        if (!i->empty()) {
            LOG4CPLUS_DEBUG(logger, "Opening index at " << *i);
            argos::index::index_ptr_t idx=argos::index::index_ptr_t(new Index(i->c_str()));
            if(!idx->is_open())
            {
                LOG4CPLUS_FATAL(logger, "Failed to open index " << *i);
                return false;
            }
            indices.insert(indices_t::value_type(get_index_name(idx), idx));
        }
    }
    return true;
}

void run_server(const indices_t &indices,
                const char *addr,
                const char *port,
                const char *doc_root,
                int threads);

void start_server(const vector<string> &index_dirs, const char *addr, const char *port, const char *doc_root, int thr)
{
    Logger logger = Logger::getInstance("main");
    LOG4CPLUS_INFO(logger, "searcher started");

    indices_t indices;
    if(!init_index(index_dirs, indices)) {
        LOG4CPLUS_FATAL(logger, "Failed to open index");
        LOG4CPLUS_INFO(logger, "searcher stopped");
        return;
    }
    try {
        run_server(indices, addr, port, doc_root, thr);
    } catch (...) {
        LOG4CPLUS_FATAL(logger, "Uncaught exception");
    }
    LOG4CPLUS_INFO(logger, "searcher stopped");
}

int main(int argc, const char * argv[])
{
    vector<string> index_dirs;
    string addr;
    string port;
    string doc_root;
    string log_config;
    int thr;
    
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "produce help message")
    ("index-dir,d", po::value< vector<string> >(&index_dirs), "index directory")
    ("listen-addr,a", po::value<string>(&addr)->default_value("0.0.0.0"), "listening address")
    ("listen-port,p", po::value<string>(&port)->default_value("8765"), "listening port")
    ("doc-root,r", po::value<string>(&doc_root)->default_value("./www"), "root directory for static files")
    ("threads,t", po::value<int>(&thr)->default_value(3), "number of serving threads")
    ("log-config,l", po::value<string>(&log_config), "log configuration file")
    ;
    
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    } catch (...) {
        cerr << "Invalid command line options\n";
        cerr << desc << "\n";
        return 1;
    }
    po::notify(vm);
    
    if (vm.count("help")) {
        cout << desc << "\n";
        return 0;
    }
    
    Logger logger = Logger::getInstance("main");
    if (log_config.empty()) {
        BasicConfigurator config;
        config.configure();
    } else {
        PropertyConfigurator config(log_config);
        config.configure();
    }
    char cwd[1024];
    LOG4CPLUS_INFO(logger, "Searcher running at " << getcwd(cwd, 1024));
    for (int i=0; i<argc; i++) {
        LOG4CPLUS_DEBUG(logger, "Command line option: " << argv[i]);
    }
    if (log_config.empty()) {
        LOG4CPLUS_DEBUG(logger, "Using empty log config");
    } else {
        LOG4CPLUS_DEBUG(logger, "Using log config " << log_config);
    }
    for(size_t i=0; i<index_dirs.size(); i++) {
        LOG4CPLUS_DEBUG(logger, "Using index at " << index_dirs[i]);
    }
    LOG4CPLUS_DEBUG(logger, "Listening at " << addr << ':' << port);
    LOG4CPLUS_DEBUG(logger, "Document root at " << doc_root);
    
    start_server(index_dirs,
                 addr.c_str(),
                 port.c_str(),
                 doc_root.c_str(),
                 thr);
    
    return 0;
}

