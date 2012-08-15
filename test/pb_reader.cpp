/*
 * =====================================================================================
 *
 *       Filename:  pb_reader.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012-08-10 19:08:12
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xinli.chen (search engine group), xinli.chen@dianping.com
 *        Company:  www.dianping.com
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string>
#include "serialization/search_result.pb.h"

using namespace pb;
//using namespace argos::common;

#define SIZE    (4*1024*1024)
char buffer[SIZE];

char *serialize_value(const Value &value, char *buf, int size)
{
    int ret = 0;
    switch(value.type()) {
    case Value::INT:
        ret = snprintf(buf, size, "%ld", value.ivalue());
        break;
    case Value::INTARR:
        ret = snprintf(buf+ret, size - ret, "[");
        for(int k=0; k<value.ivalues_size(); k++)
        ret = snprintf(buf+ret, size - ret, "%ld", value.ivalues(k));
        ret = snprintf(buf+ret, size - ret, "]");
        break;
    case Value::DOUBLE:
        ret = snprintf(buf+ret, size - ret, "%f", value.dvalue());
        break;
    case Value::DOUBLEARR:
        ret = snprintf(buf+ret, size - ret, "[");
        for(int k=0; k<value.dvalues_size(); k++)
        ret = snprintf(buf+ret, size - ret, "%f,", value.dvalues(k));
        ret = snprintf(buf+ret, size - ret, "]");
        break;
    case Value::GEOLOC:
        ret = snprintf(buf+ret, size - ret, "[%f:%f]", value.geoloc().lng(), value.geoloc().lat());
        break;
    case Value::GEOLOCARR:
        ret = snprintf(buf+ret, size - ret, "[");
        for(int k=0; k<value.geolocs_size(); k++)
        ret = snprintf(buf+ret, size - ret, "%f:%f", value.geolocs(k).lng(), value.geolocs(k).lat());
        ret = snprintf(buf+ret, size - ret, "]");
        break;
    case Value::STRING:
        ret = snprintf(buf+ret, size - ret, "%s", value.svalue().c_str());
        break;
    default:
        break;
    }
    return buf;
}

void print_histo(const Histo &histo) 
{
    fprintf(stdout, "field:%s, count:%d\n\t", histo.field().c_str(), histo.count());
    size_t size = histo.pairs_size();
    char tmp[1024];
    for(int i=0; i<size; i++)
    {
        fprintf(stdout, "%s:%d,", serialize_value(histo.pairs(i).key(), tmp, 1024), histo.pairs(i).count());
    }
    fprintf(stdout, "\n");
}

void print_document(const Document &doc)
{
    int value_size = doc.values_size();
    for (size_t i=0; i<value_size; i++) {
        const Value &value = doc.values(i);
        switch(value.type()) {
            case Value::INT:
                fprintf(stdout, "%ld\t", value.ivalue());
                break;
            case Value::INTARR:
                fprintf(stdout, "[");
                for(int k=0; k<value.ivalues_size(); k++)
                fprintf(stdout, "%ld,", value.ivalues(k));
                fprintf(stdout, "]");
                break;
            case Value::DOUBLE:
                fprintf(stdout, "%f\t", value.dvalue());
                break;
            case Value::DOUBLEARR:
                fprintf(stdout, "[");
                for(int k=0; k<value.dvalues_size(); k++)
                fprintf(stdout, "%f,", value.dvalues(k));
                fprintf(stdout, "]");
                break;
            case Value::GEOLOC:
                fprintf(stdout, "[%f:%f]\t", value.geoloc().lng(), value.geoloc().lat());
                break;
            case Value::GEOLOCARR:
                fprintf(stdout, "[");
                for(int k=0; k<value.geolocs_size(); k++)
                fprintf(stdout, "%f:%f ", value.geolocs(k).lng(), value.geolocs(k).lat());
                fprintf(stdout, "]");
                break;
            case Value::STRING:
                fprintf(stdout, "%s\t", value.svalue().c_str());
                break;
            default:
                break;
        }
    }
    fprintf(stdout, "\n");
}

int main(int argc, char *argv[])
{
    SearchResult result;
    size_t bytes = fread(buffer, 1, SIZE, stdin);
    bool ret = result.ParseFromArray(buffer, bytes);
    if(ret) {
        fprintf(stdout, "total:\t%d\n", result.total());
        fprintf(stdout, "returned:\t%d\n", result.returned());
      
        size_t size = result.histos_size();
        for(int i=0; i<size; i++) 
        {
            print_histo(result.histos(i));
        }

        size = result.fields_size();
        for(size_t i=0; i<size; i++)
        {
            fprintf(stdout, "%s\t", result.fields(i).c_str());
        }
        fprintf(stdout, "\n");

        size = result.documents_size();
        for(int j=0; j<size; j++)
        {
            print_document(result.documents(j));
        }
    } 
    else {
        fprintf(stdout, "parse error, with size: %d\n", bytes);
    }
    return 0;
}

