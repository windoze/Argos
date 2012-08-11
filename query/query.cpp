//
//  query.cpp
//  Argos
//
//  Created by Windoze on 12-7-2.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "query/argos_query.h"

namespace argos {
    namespace query {
        Query::Query()
        : nr_(0)
        , sk_(0)
        {}
        
        bool Query::uses_match_info() const {
            bool ret=filter_->uses_match_info();
            for (sort_criteria::const_iterator i=sort_crit_.begin(); i!=sort_crit_.end(); ++i) {
                ret = ret || i->sort_key->uses_match_info();
                if (ret) {
                    return true;
                }
            }
            for (common::field_list_t::const_iterator i=fl_.begin(); i!=fl_.end(); ++i) {
                ret = ret || (*i)->uses_match_info();
                if (ret) {
                    return true;
                }
            }
            return ret;
        }
        
        std::string Query::to_string() const {
            std::stringstream sst;
            if (pks_.empty()) {
                // This is a normal query
                bool first=true;
                sst << "/query?";
                if (!match_param_.empty()) {
                    sst << "m=" << match_param_;
                    first=false;
                }
                if (first) {
                    first=false;
                } else {
                    sst << "&";
                }
                sst << "f=" << filter_->to_string();
                sst << "&s=";
                for (sort_criteria::const_iterator i=sort_crit_.begin(); i!=sort_crit_.end(); ++i) {
                    sst << i->sort_key->to_string() << ',';
                    if (i->desc) {
                        sst << "DESC";
                    } else {
                        sst << "ASC";
                    }
                }
                sst << "&fl=";
                bool first_field=true;
                for (common::field_list_t::const_iterator i=fl_.begin(); i!=fl_.end(); ++i) {
                    if (first_field) {
                        first_field=false;
                    } else {
                        sst << ',';
                    }
                    sst << (*i)->to_string();
                }
                // Omit histo part if there is no histogram spec
                if (!histos_.empty()) {
                    sst << "&h=";
                    bool first_histo=true;
                    for (histos_t::const_iterator i=histos_.begin(); i!=histos_.end(); ++i) {
                        if (first_histo) {
                            first_histo=false;
                        } else {
                            sst << ';';
                        }
                        bool first_exp=true;
                        for (common::field_list_t::const_iterator j=i->begin(); j!=i->end(); ++j) {
                            if (first_exp) {
                                first_exp=false;
                            } else {
                                sst << ',';
                            }
                            sst << (*j)->to_string();
                        }
                    }
                }
                if (nr_>0) {
                    sst << "&nr=" << nr_;
                }
                if (sk_>0) {
                    sst << "&sk=" << sk_;
                }
            } else {
                // This is a item query
                bool first=true;
                for (pk_list_t::const_iterator i=pks_.begin(); i<pks_.end(); ++i) {
                    if (first) {
                        first=false;
                    } else {
                        sst << ',';
                    }
                    sst << *i;
                }
                sst << "&fl=";
                bool first_field=true;
                for (common::field_list_t::const_iterator i=fl_.begin(); i!=fl_.end(); ++i) {
                    if (first_field) {
                        first_field=false;
                    } else {
                        sst << ',';
                    }
                    sst << (*i)->to_string();
                }
            }
            sst << "&fmt=" << fmt;
            if (!query_id_.empty()) {
                sst << "&queryid=" << query_id_;
            }
            return sst.str();
        }
    }   // End of namespace query
}   // End of namespace argos
