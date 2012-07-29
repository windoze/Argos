//
//  serialization.cpp
//  Argos
//
//  Created by Windoze on 12-7-8.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "pugixml/pugixml.hpp"
#include "query/enquire.h"
#include "serialization.h"
#include "../json_spirit/json_spirit.h"

namespace argos {
    namespace serialization {
        /*
         const int RS_JSON_ARRAY=0;
         const int RS_JSON_MAP=1;
         const int RS_XML=2;
         const int RS_CSV=3;
         
         // TODO: Protocol Buffer, Avro
         */
        namespace detail {
            json_spirit::Value conv(common::Value v) {
                json_spirit::Value jv;
                switch (v.type_) {
                    case common::VT_INTEGER:
                        jv=v.number;
                        break;
                    case common::VT_DOUBLE:
                        jv=v.dnumber;
                        break;
                    case common::VT_STRING:
                        jv=v.string;
                        break;
                    case common::VT_GEOLOCATION:
                    {
                        json_spirit::Array a;
                        a.push_back(v.geolocation.latitude);
                        a.push_back(v.geolocation.longitude);
                        jv=a;
                    }
                        break;
                    case common::VT_ARRAY:
                    {
                        json_spirit::Array a;
                        for (int i=0; i<v.array.size(); i++) {
                            common::Value av=v.array.get_element(i);
                            a.push_back(conv(av));
                        }
                        jv=a;
                    }
                        break;
                    case common::VT_EMPTY:
                        break;
                    default:
                        break;
                }
                return jv;
            }
            
            const char *value_type_name(common::VALUE_TYPE vt) {
                switch (vt) {
                    case common::VT_ARRAY:
                        return "array";
                        break;
                    case common::VT_INTEGER:
                        return "integer";
                        break;
                    case common::VT_STRING:
                        return "string";
                        break;
                    case common::VT_DOUBLE:
                        return "float";
                        break;
                    default:
                        return "";
                        break;
                }
            }
            common::arstring serialize_field_list(const common::field_list_t &fl)
            {
                common::arstring s;
                for (int i=0; i<fl.size(); i++) {
                    s+=fl[i]->to_string();
                    if (i<fl.size()-1) {
                        s+=',';
                    }
                }
                return s;
            }
            common::arstring serialize_value_list(const common::value_list_t &vl)
            {
                if (vl.size()==1) {
                    return common::Constant(vl[0]).to_string();
                }
                common::arstring s("[");
                for (int i=0; i<vl.size(); i++) {
                    s+=common::Constant(vl[i]).to_string();
                    if (i<vl.size()-1) {
                        s+=',';
                    }
                }
                s+=']';
                return s;
            }

            class rs_impl
            {
            public:
                virtual ~rs_impl(){}
                virtual int rs_id() const=0;
                virtual const char *content_type() const=0;
                virtual std::ostream &serialize_prolog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const=0;
                virtual std::ostream &serialize_histos(std::ostream &os, const std::vector<common::field_list_t> &hschema, const query::histograms_t &histos) const=0;
                virtual std::ostream &serialize_doc(std::ostream &os, const common::field_list_t &schema, const common::value_list_t &vl) const=0;
                virtual std::ostream &serialize_splitter(std::ostream &os, const common::field_list_t &schema, bool last) const=0;
                virtual std::ostream &serialize_epilog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const=0;
            };

            class rs_impl_JSON_array : public rs_impl {
            public:
                rs_impl_JSON_array(){}
                virtual int rs_id() const { return RS_JSON_ARRAY; }
                virtual const char *content_type() const { return "application/json"; }
                virtual std::ostream &serialize_prolog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const{
                    return os;
                }
                virtual std::ostream &serialize_histos(std::ostream &os, const std::vector<common::field_list_t> &hschema, const query::histograms_t &histos) const {
                    for (int i=0; i<hschema.size(); i++) {
                        std::string h=serialize_field_list(hschema[i]);
                        json_spirit::Object histo_obj;
                        for (query::histogram_t::const_iterator j=histos[i].begin(); j!=histos[i].end(); ++j) {
                            std::string k=serialize_value_list(j->first);
                            int v=int(j->second);
                            histo_obj.push_back(json_spirit::Pair(k, v));
                        }
                        if (!histo_obj.empty()) {
                            histos_.push_back(json_spirit::Pair(h, histo_obj));
                        }
                    }
                    return os;
                }
                virtual std::ostream &serialize_doc(std::ostream &os, const common::field_list_t &schema, const common::value_list_t &vl) const {
                    json_spirit::Array doc;
                    for (size_t i=0; i<schema.size(); i++) {
                        std::string k=schema[i]->to_string();
                        json_spirit::Value v=conv(vl[i]);
                        doc.push_back(v);
                    }
                    results_.push_back(doc);
                    return os;
                }
                virtual std::ostream &serialize_splitter(std::ostream &os, const common::field_list_t &schema, bool last) const {
                    return os;
                }
                virtual std::ostream &serialize_epilog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const {
                    json_spirit::Object ret_;
                    if (!histos_.empty()) {
                        ret_.push_back(json_spirit::Pair("histograms", histos_));
                    }
                    ret_.push_back(json_spirit::Pair("total", int(total_hits)));
                    ret_.push_back(json_spirit::Pair("returned", int(nr)));
                    ret_.push_back(json_spirit::Pair("results", results_));
#if defined(NDEBUG) || !defined(DEBUG)
                    json_spirit::write(ret_, os, json_spirit::remove_trailing_zeros);
#else
                    json_spirit::write(ret_, os, json_spirit::remove_trailing_zeros | json_spirit::single_line_arrays);
#endif
                    return os;
                }
                
            private:
                mutable json_spirit::Object histos_;
                mutable json_spirit::Array results_;
            };
            class rs_impl_JSON_map : public rs_impl {
            public:
                rs_impl_JSON_map(){}
                virtual int rs_id() const { return RS_JSON_MAP; }
                virtual const char *content_type() const { return "application/json"; }
                virtual std::ostream &serialize_prolog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const{
                    return os;
                }
                virtual std::ostream &serialize_histos(std::ostream &os, const std::vector<common::field_list_t> &hschema, const query::histograms_t &histos) const {
                    for (int i=0; i<hschema.size(); i++) {
                        std::string h=serialize_field_list(hschema[i]);
                        json_spirit::Object histo_obj;
                        for (query::histogram_t::const_iterator j=histos[i].begin(); j!=histos[i].end(); ++j) {
                            std::string k=serialize_value_list(j->first);
                            int v=int(j->second);
                            histo_obj.push_back(json_spirit::Pair(k, v));
                        }
                        if (!histo_obj.empty()) {
                            histos_.push_back(json_spirit::Pair(h, histo_obj));
                        }
                    }
                    return os;
                }
                virtual std::ostream &serialize_doc(std::ostream &os, const common::field_list_t &schema, const common::value_list_t &vl) const {
                    json_spirit::Object doc;
                    for (size_t i=0; i<schema.size(); i++) {
                        std::string k=schema[i]->to_string();
                        json_spirit::Value v=conv(vl[i]);
                        doc.push_back(json_spirit::Pair(k,v));
                    }
                    results_.push_back(doc);
                    return os;
                }
                virtual std::ostream &serialize_splitter(std::ostream &os, const common::field_list_t &schema, bool last) const {
                    return os;
                }
                virtual std::ostream &serialize_epilog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const {
                    json_spirit::Object ret_;
                    if (!histos_.empty()) {
                        ret_.push_back(json_spirit::Pair("histograms", histos_));
                    }
                    ret_.push_back(json_spirit::Pair("total", int(total_hits)));
                    ret_.push_back(json_spirit::Pair("returned", int(nr)));
                    ret_.push_back(json_spirit::Pair("results", results_));
#if defined(NDEBUG) || !defined(DEBUG)
                    json_spirit::write(ret_, os, json_spirit::remove_trailing_zeros);
#else
                    json_spirit::write(ret_, os, json_spirit::remove_trailing_zeros | json_spirit::single_line_arrays);
#endif
                    return os;
                }
            private:
                mutable json_spirit::Object histos_;
                mutable json_spirit::Array results_;
            };

            class rs_impl_XML : public rs_impl {
            public:
                rs_impl_XML()
                {
                    pugi::xml_node xml = doc.append_child(pugi::node_declaration);
                    xml.set_name("xml");
                    xml.append_attribute("version")="1.0";
                    xml.append_attribute("encoding")="UTF-8";
                    pugi::xml_node xsl = doc.append_child(pugi::node_declaration);
                    xsl.set_name("xml-stylesheet");
                    xsl.append_attribute("type")="text/xsl";
                    xsl.append_attribute("href")="/results.xsl";
                    r=doc.append_child("result");
                    results = r.append_child("items");
                }
                virtual int rs_id() const { return RS_XML; }
                virtual const char *content_type() const { return "application/xml"; }
                virtual std::ostream &serialize_prolog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const
                {
                    results.append_attribute("total")=int(total_hits);
                    results.append_attribute("returned")=int(nr);
                    return os;
                }
                virtual std::ostream &serialize_histos(std::ostream &os, const std::vector<common::field_list_t> &hschema, const query::histograms_t &histos) const {
                    if (hschema.empty()) {
                        return os;
                    }
                    if (!histograms) {
                        histograms = r.prepend_child("histograms");
                    }
                    for (size_t i=0; i<histos.size(); i++) {
                        // Serialize one histgram
                        pugi::xml_node h=histograms.append_child("histogram");
                        h.append_attribute("key")=serialize_field_list(hschema[i]).c_str();
                        h.append_attribute("count")=int(histos[i].size());
                        for (query::histogram_t::const_iterator j=histos[i].begin(); j!=histos[i].end(); ++j) {
                            pugi::xml_node item=h.append_child("item");
                            item.append_attribute("key")=serialize_value_list(j->first).c_str();
                            item.append_attribute("count")=int(j->second);
                        }
                    }
                    return os;
                }
                virtual std::ostream &serialize_doc(std::ostream &os, const common::field_list_t &schema, const common::value_list_t &vl) const
                {
                    pugi::xml_node r=results.append_child("item");
                    for (size_t i=0; i<schema.size(); i++) {
                        pugi::xml_node field=r.append_child("field");
                        field.append_attribute("name")=schema[i]->to_string().c_str();
                        field.append_attribute("type")=value_type_name(vl[i].type_);
                        if (vl[i].type_==common::VT_STRING) {
                            field.append_attribute("value")=vl[i].string;
                        } else {
                            field.append_attribute("value")=common::Constant(vl[i]).to_string().c_str();
                        }
                    }
                    return os;
                }
                virtual std::ostream &serialize_splitter(std::ostream &os, const common::field_list_t &schema, bool last) const
                {
                    return os;
                }
                virtual std::ostream &serialize_epilog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const
                {
                    doc.save(os);
                    return os;
                }
            private:
                common::arstring serialize_field_list(const common::field_list_t &fl) const
                {
                    common::arstring s;
                    for (int i=0; i<fl.size(); i++) {
                        s+=fl[i]->to_string();
                        if (i<fl.size()-1) {
                            s+=',';
                        }
                    }
                    return s;
                }
                common::arstring serialize_value_list(const common::value_list_t &vl) const
                {
                    if (vl.size()==1) {
                        return common::Constant(vl[0]).to_string();
                    }
                    common::arstring s("[");
                    for (int i=0; i<vl.size(); i++) {
                        s+=common::Constant(vl[i]).to_string();
                        if (i<vl.size()-1) {
                            s+=',';
                        }
                    }
                    s+=']';
                    return s;
                }
                
                mutable pugi::xml_document doc;
                mutable pugi::xml_node results;
                mutable pugi::xml_node r;
                mutable pugi::xml_node histograms;
            };

            class rs_impl_CSV : public rs_impl {
            public:
                rs_impl_CSV(){}
                virtual int rs_id() const { return RS_JSON_ARRAY; }
                virtual const char *content_type() const { return "text/csv"; }
                virtual std::ostream &serialize_prolog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const
                {
                    return os;
                }
                virtual std::ostream &serialize_histos(std::ostream &os, const std::vector<common::field_list_t> &hschema, const query::histograms_t &histos) const {
                    return os;
                }
                virtual std::ostream &serialize_doc(std::ostream &os, const common::field_list_t &schema, const common::value_list_t &vl) const
                {
                    for (int i=0; i<vl.size(); i++) {
                        os << common::Constant(vl[i]).to_string();
                        if (i<vl.size()-1) {
                            os << ',';
                        }
                    }
                    return os;
                }
                virtual std::ostream &serialize_splitter(std::ostream &os, const common::field_list_t &schema, bool last) const
                {
                    os << '\n';
                    return os;
                }
                virtual std::ostream &serialize_epilog(std::ostream &os, const common::field_list_t &schema, size_t total_hits, size_t nr) const
                {
                    return os;
                }
            };
            
            rs_ptr create_rs(const char *fmtname)
            {
                if (strcasecmp(fmtname, "CSV")==0) {
                    return rs_ptr(new detail::rs_impl_CSV);
                } else if (strcasecmp(fmtname, "JSONA")==0) {
                    return rs_ptr(new detail::rs_impl_JSON_array);
                } else if (strcasecmp(fmtname, "JSONM")==0) {
                    return rs_ptr(new detail::rs_impl_JSON_map);
                } else if (strcasecmp(fmtname, "XML")==0) {
                    return rs_ptr(new detail::rs_impl_XML);
                }
                // default to CSV
                return rs_ptr(new detail::rs_impl_CSV);
            }
            rs_ptr create_rs(int format)
            {
                switch (format) {
                    case RS_JSON_ARRAY:
                        return rs_ptr(new detail::rs_impl_JSON_array);
                    case RS_JSON_MAP:
                        return rs_ptr(new detail::rs_impl_JSON_map);
                    case RS_XML:
                        return rs_ptr(new detail::rs_impl_XML);
                    case RS_CSV:
                        return rs_ptr(new detail::rs_impl_CSV);
                    default:
                        // TODO: Error log
                        break;
                }
                return rs_ptr();
            }
        }   // End of namespace detail

        results_serializer::results_serializer(const char *fmtname, const common::field_list_t &schema, size_t total_hits, size_t nr)
        : impl_(detail::create_rs(fmtname))
        , schema_(schema)
        , total_hits_(total_hits)
        , nr_(nr)
        {}
        
        results_serializer::results_serializer(int format, const common::field_list_t &schema, size_t total_hits, size_t nr)
        : impl_(detail::create_rs(format))
        , schema_(schema)
        , total_hits_(total_hits)
        , nr_(nr)
        {}
        
        results_serializer::~results_serializer()
        {}
        
        const char *results_serializer::content_type() const
        {
            return impl_->content_type();
        }

        std::ostream &results_serializer::serialize_prolog(std::ostream &os) const
        {
            impl_->serialize_prolog(os, schema_, total_hits_, nr_);
            return os;
        }
        
        std::ostream &results_serializer::serialize_histos(std::ostream &os, const std::vector<common::field_list_t> &hschema, const query::histograms_t &histos) const
        {
            return impl_->serialize_histos(os, hschema, histos);
        }
        
        std::ostream &results_serializer::serialize_doc(std::ostream &os, const common::value_list_t &vl, size_t n) const
        {
            impl_->serialize_doc(os, schema_, vl);
            impl_->serialize_splitter(os, schema_, (n==nr_-1));
            return os;
        }
                
        std::ostream &results_serializer::serialize_epilog(std::ostream &os) const
        {
            impl_->serialize_epilog(os, schema_, total_hits_, nr_);
            return os;
        }
    }   // End of namespace serialization
}   // End of namespace argos