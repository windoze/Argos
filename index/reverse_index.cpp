//
//  reverse_index.cpp
//  Argos
//
//  Created by Windoze on 12-7-4.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <map>
#include <set>
#include "index/reverse_index.h"
#include "index/forward_index.h"
#include "index/doc_storage.h"
#include "analyzer.h"

namespace argos {
    namespace index {
        bool ReverseIndex::add_document(docid did, const common::value_list_t &vl, common::ExecutionContext &ctx)
        {
            // TODO: Prepare terms and postings
            const common::FieldConfig *fc=ctx.get_field_config();
            common::term_list_t all_terms;
            for (int i=0; i<fc->count(); i++) {
                common::term_list_t terms;
                if (fc->get_field_def(i)->indexed()) {
                    switch (vl[i].type_) {
                        case common::VT_STRING:
                        {
                            analyzer::analyzer_ptr_t a=analyzer::get_analyzer(fc->get_field_def(i)->get_analyzer_name());
                            if (a) {
                                common::simple_term_list_t tl;
                                a->analyse_doc(fc->get_field_def(i)->get_prefix(), vl[i].string, tl);
                                common::merge_term_list(terms, tl);
                            } else {
                                // Fall back
                                common::string_terms(fc->get_field_def(i)->get_prefix(), vl[i].string, terms);
                            }
                            break;
                        }
                        case common::VT_INTEGER:
                            common::int_terms(fc->get_field_def(i)->get_prefix(), vl[i].number, terms);
                            break;
                        case common::VT_DOUBLE:
                            common::double_terms(fc->get_field_def(i)->get_prefix(), vl[i].dnumber, terms);
                            break;
                        case common::VT_ARRAY:
                            array_terms(fc->get_field_def(i)->get_prefix(), vl[i].array, terms);
                            break;
                        default:
                            break;
                    }
                    // Record doc-length/term-count for this field
                    size_t dl=0;
                    for (common::term_list_t::const_iterator j=terms.begin(); j!=terms.end(); ++j) {
//                        std::cout << "Term: " << j->first << std::endl;
                        dl+=j->second;
                    }
                    ctx.get_forward_index()->get_storage()->set_field_len(i, did, dl);
                    common::merge_term_list(all_terms, terms);
                }
            }
            return add_doc(did, all_terms);
        }
    }   // End of namespace index
}   // End of namespace argos
