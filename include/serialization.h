//
//  serialization.h
//  Argos
//
//  Created by Windoze on 12-7-7.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <boost/shared_ptr.hpp>
#include "common/evaluatable.h"

#ifndef Argos_serialization_h
#define Argos_serialization_h

namespace argos {
    namespace serialization {
        const int RS_JSON_ARRAY=0;
        const int RS_JSON_MAP=1;
        const int RS_XML=2;
        const int RS_CSV=3;
        
        namespace detail {
            class rs_impl;
        }
        typedef boost::shared_ptr<detail::rs_impl> rs_ptr;
        
        class results_serializer {
        public:
            /**
             * Constructor, create serializer with given format
             */
            results_serializer(const char *fmtname, const common::field_list_t &schema, size_t total_hits, size_t nr);
            /**
             * Constructor, create serializer with given format
             */
            results_serializer(int format, const common::field_list_t &schema, size_t total_hits, size_t nr);
            /**
             * Destructor
             */
            ~results_serializer();

            /**
             * Return content-type of serialized text
             */
            const char *content_type() const;

            std::ostream &serialize_prolog(std::ostream &os) const;
            std::ostream &serialize_histos(std::ostream &os, const std::vector<common::field_list_t> &hschema, const query::histograms_t &histos) const;
            std::ostream &serialize_doc(std::ostream &os, const common::value_list_t &vl, size_t n) const;
            std::ostream &serialize_epilog(std::ostream &os) const;
        private:
            rs_ptr impl_;
            const common::field_list_t &schema_;
            size_t total_hits_;
            size_t nr_;
        };
    }   // End of namespace serialization
}   // End of namespace argos

#endif
