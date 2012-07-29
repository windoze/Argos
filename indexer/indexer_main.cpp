//
//  indexer_main.cpp
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <fstream>
#include <boost/program_options.hpp>
#include "index.h"
#include "parser.h"

using namespace std;
using namespace argos;
using namespace argos::common;
using namespace argos::index;
using namespace argos::parser;

namespace po = boost::program_options;

inline void mark(size_t sz)
{
    if (sz % 100==0) {
        if (sz % 1000==0) {
            if (sz % 10000==0) {
                cout << sz;
            } else {
                cout << (sz/1000 % 10);
            }
        }
        cout << '.';
        cout.flush();
    }
}

Index *the_index=0;

void close_index();

bool init_index(const char *conf, const char *index_dir)
{
    if (conf && conf[0] && index_dir && index_dir[0]) {
        the_index=new Index(conf, index_dir, INDEX_CREATE);
        if(!the_index->is_open())
        {
            close_index();
            the_index=0;
        }
    }
    return the_index;
}

bool add_document(const std::string &line, ExecutionContext &context)
{
    if (!the_index) {
        return false;
    }
    
    value_list_t vl;
    const char *s=line.c_str();
    size_t len=line.size();
    if(!parse_value_list(s, len, vl, context))
        return false;
    for (value_list_t::const_iterator i=vl.begin(); i!=vl.end(); ++i) {
        if (i->type_==VT_EMPTY) {
            return false;
        }
    }
    return is_valid(the_index->add_document(vl, context));
}

void close_index()
{
    if (the_index) {
        delete the_index;
    }
}

bool add_documents(istream &is, size_t &line_count, size_t &error_count)
{
    string line;
    ExecutionContext *context=the_index->create_context();
    //context->temp_pool=get_tl_mem_pool();
    line_count=0;
    error_count=0;
    bool ret=true;
    while (is)
    {
        line_count++;
        if ((line_count-1)%1000==0) {
            context->temp_pool->reset();
        }
        if (!context) {
            cerr << "ERROR: Fail to create context when processing line:" << line_count << endl;
            return false;
        }
        getline(is, line);
        if (line.empty()) {
            error_count++;
            continue;
        }
        if(!add_document(line, *context))
        {
            error_count++;
            cerr << "WARNING: Fail to add document at line:" << line_count+1 << endl;
            continue;
        }
        mark(line_count-1);
    }
    if (context) {
        delete context;
    }
    return ret;
}

bool build_index(const string &conf, const string &index_dir, const string &inp, size_t &line_count, size_t &error_count)
{
    if (inp=="-") {
        if (!init_index(conf.c_str(), index_dir.c_str())) {
            return false;
        }
        bool ret=add_documents(cin, line_count, error_count);
        close_index();
        return ret;
    } else {
        ifstream is(inp.c_str());
        if (!is) {
            return false;
        }
        if (!init_index(conf.c_str(), index_dir.c_str())) {
            return false;
        }
        bool ret=add_documents(is, line_count, error_count);
        close_index();
        return ret;
    }
}

int main(int argc, const char * argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "produce help message")
    ("config,c", po::value<string>(), "index config XML file")
    ("index-dir,d", po::value<string>(), "index directory")
    ("input-file,i", po::value<string>(), "input file")
    ;
    
    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
    } catch (...) {
        cout << "Invalid command line options\n";
        cout << desc << "\n";
        return 1;
    }
    po::notify(vm);
    
    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }
    
    string conf;
    string index_dir;
    string inp;
    if (vm.count("config")) {
        conf=vm["config"].as<string>();
    } else {
        conf="./config.xml";
    }
    
    int fd=open(conf.c_str(), O_RDONLY);
    if (fd<0) {
        cerr << "Cannot access config file.\n";
        return 100;
    }
    close(fd);
    
    if (vm.count("index-dir")) {
        index_dir=vm["index-dir"].as<string>();
    } else {
        index_dir="./index.db";
    }
    
    if (vm.count("input-file")) {
        inp=vm["input-file"].as<string>();
    } else {
        inp="-";
    }
    
    cout << "Building index at \"" << index_dir << "\"...\n";
    
    size_t line_count=0;
    size_t error_count=0;
    try {
        if(!build_index(conf, index_dir, inp, line_count, error_count))
        {
            cerr << "\nFailed\n";
            return 100;
        }
        cout << "\nDone.\n";
        cout << "Processed " << line_count << " lines, " << error_count << " lines with error\n";
        return 0;
    }
    catch(...) {
        cerr << "\nERROR: Uncaught exception.\n";
    }
    return 100;
}
