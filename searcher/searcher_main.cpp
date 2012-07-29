//
//  searcher_main.cpp
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <map>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
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
    for (vector<string>::const_iterator i=index_dirs.begin(); i!=index_dirs.end(); ++i) {
        if (!i->empty()) {
            argos::index::index_ptr_t idx=argos::index::index_ptr_t(new Index(i->c_str()));
            if(!idx->is_open())
                return false;
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
    indices_t indices;
    if(!init_index(index_dirs, indices)) {
        fprintf(stderr, "Failed to open index\n");
        return;
    }
    try {
        run_server(indices, addr, port, doc_root, thr);
    } catch (...) {
        cerr << "Uncaught exception, exiting...\n";
    }
}

int main(int argc, const char * argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "produce help message")
    ("index-dir,d", po::value< vector<string> >(), "index directory")
    ("listen-addr,a", po::value<string>(), "listening address")
    ("listen-port,p", po::value<string>(), "listening port")
    ("doc-root,r", po::value<string>(), "root directory for static files")
    ("threads,t", po::value<int>(), "number of serving threads")
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
        return 1;
    }
    
    vector<string> index_dirs;
    string addr;
    string port;
    string doc_root;
    int thr;

    if (vm.count("index-dir")) {
        index_dirs=vm["index-dir"].as<vector<string> >();
    }
    
    if (vm.count("listen-addr")) {
        addr=vm["listen-addr"].as<string>();
    } else {
        // addr="0::0"; // For IPv6
        addr="0.0.0.0";
    }
    
    if (vm.count("listen-port")) {
        port=vm["listen-port"].as<string>();
    } else {
        port="8765";
    }
    
    if (vm.count("doc-root")) {
        doc_root=vm["doc-root"].as<string>();
    } else {
        doc_root="./www";
    }
    
    if (vm.count("threads")) {
        thr=vm["threads"].as<int>();
    } else {
        thr=3;
    }
    
    start_server(index_dirs,
                 addr.c_str(),
                 port.c_str(),
                 doc_root.c_str(),
                 thr);
    return 0;
}

