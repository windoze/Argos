//
//  doc_ops.cpp
//  Argos
//
//  Created by Windoze on 12-7-15.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <iostream>
#include <math.h>
#include "common/evaluatable.h"
#include "common/token_parser.h"
#include "common/term_list.h"
#include "index/forward_index.h"
#include "index/full_index.h"
#include "analyzer.h"
#include "rindex.h"

namespace argos {
    namespace query {
        class DocOpNode : public common::Evaluatable {
        public:
            // This is a doc op node
            virtual int eva_type() const { return common::ET_DOC; }
            
        };

        // term count for current doc
        class TcDocOpNode : public DocOpNode {
        public:
            TcDocOpNode(const std::string &term, const std::string &field, common::ExecutionContext &ctx)
            : term_(term)
            , field_(field)
            , match_info_pos_(-1)
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(field_.c_str());
                    if (!fd) {
                        throw argos_bad_field(field_.c_str());
                    }
                    prefix=fd->get_prefix();
                }
                std::string actual_term=prefix+term_;
                common::match_term_dict_t::const_iterator i=ctx.match_term_dict.find(actual_term);
                if (i!=ctx.match_term_dict.end()) {
                    match_info_pos_=i->second;
                }
            }
            
            // Term count is not same for different doc
            virtual bool is_const() const { return false; }
            // This op requires per-doc match info
            virtual bool uses_match_info() const { return true; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                common::match_info_t *l=context.get_match_info();
                if(match_info_pos_<0 || !l) {
                    return common::Value(int64_t(0));
                }
                return common::Value((*l)[match_info_pos_]);
            }
            
            virtual std::string to_string_impl() const {
                // @TC("term") or @TC(field,"term")
                if (field_.empty()) {
                    std::string s="@TC(\"";
                    s+=term_;
                    s+="\")";
                    return s;
                }
                std::string s="@TC(";
                s+=field_;
                s+=",\"";
                s+=term_;
                s+="\")";
                return s;
            }
            
        private:
            std::string term_;
            std::string field_;
            int match_info_pos_;
        };

        // term freq for current doc
        class TfDocOpNode : public DocOpNode {
        public:
            TfDocOpNode(const std::string &term, const std::string &field, common::ExecutionContext &ctx)
            : term_(term)
            , field_(field)
            , match_info_pos_(-1)
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(field_.c_str());
                    if (!fd) {
                        throw argos_bad_field(field_.c_str());
                    }
                    prefix=fd->get_prefix();
                }
                std::string actual_term=prefix+term_;
                common::match_term_dict_t::const_iterator i=ctx.match_term_dict.find(actual_term);
                if (i!=ctx.match_term_dict.end()) {
                    match_info_pos_=i->second;
                }
                common::FieldDefinition *def=ctx.get_field_config()->get_field_def(field.c_str());
                if (def && def->indexed()) {
                    std::string prefix(def->get_prefix());
                    for (int i=0; i<ctx.get_field_config()->count(); i++) {
                        if (prefix==ctx.get_field_config()->get_field_def(i)->get_prefix()) {
                            included_fields_.push_back(i);
                        }
                    }
                }
            }
            
            // Term freq is not same for different doc
            virtual bool is_const() const { return false; }
            // This op requires per-doc match info
            virtual bool uses_match_info() const { return true; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                common::match_info_t *l=context.get_match_info();
                if((match_info_pos_<0) || !l) {
                    return common::Value(double(0));
                }
                int64_t tc= (*l)[match_info_pos_];
                size_t dl=0;    // TermCount/DocLen
                for (std::vector<int>::const_iterator i=included_fields_.begin(); i!=included_fields_.end(); ++i) {
                    dl+=context.get_forward_index()->get_field_length(did, *i);
                }
                if (dl==0) {
                    // Shouldn't happen, the doc has this term at least
                    return common::Value(double(0));
                }
                double tf=double(tc)/double(dl);
                return common::Value(tf);
            }
            
            virtual std::string to_string_impl() const {
                // @TF("term") or @TF(field,"term")
                if (field_.empty()) {
                    std::string s="@TF(\"";
                    s+=term_;
                    s+="\")";
                    return s;
                }
                std::string s="@TF(";
                s+=field_;
                s+=",\"";
                s+=term_;
                s+="\")";
                return s;
            }
            
        private:
            std::string term_;
            std::string field_;
            std::vector<int> included_fields_;
            int match_info_pos_;
        };
        
        // current doc length (term count)
        class DlDocOpNode : public DocOpNode {
        public:
            DlDocOpNode(const std::string &field, common::ExecutionContext &ctx)
            : field_(field)
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(field_.c_str());
                    if (!fd) {
                        throw argos_bad_field(field_.c_str());
                    }
                    prefix=fd->get_prefix();
                }
                // find all fields with same namespace as field
                for (int i=0; i<ctx.get_field_config()->count(); i++) {
                    if (prefix==ctx.get_field_config()->get_field_def(i)->get_prefix()) {
                        included_fields_.push_back(i);
                    }
                }
            }
            
            // Doc length is not same for different doc
            virtual bool is_const() const { return false; }
            
            // This op requires per-doc match info
            virtual bool uses_match_info() const { return false; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                size_t ret=0;
                for (std::vector<int>::const_iterator i=included_fields_.begin(); i!=included_fields_.end(); ++i) {
                    ret+=context.get_forward_index()->get_field_length(did, *i);
                }
                return common::Value(int64_t(ret));
            }
            
            virtual std::string to_string_impl() const {
                // @DL or @DL(field)
                if (field_.empty()) {
                    return "@DL";
                }
                std::string s="@DL(";
                s+=field_;
                s+=")";
                return s;
            }

        private:
            std::string field_;
            std::vector<int> included_fields_;
        };
        
        // doc count for given term
        class DcDocOpNode : public DocOpNode {
        public:
            DcDocOpNode(const std::string &term, const std::string &field, common::ExecutionContext &ctx)
            : term_(term)
            , field_(field)
            , dc_(int64_t(0))
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(field_.c_str());
                    if (!fd) {
                        throw argos_bad_field(field_.c_str());
                    }
                    prefix=fd->get_prefix();
                }
                std::string actual_term_=prefix+term_;
                index::ReverseIndex *ri=ctx.get_reverse_index();
                index::detail::ArgosReverseIndex *ari=(index::detail::ArgosReverseIndex *)ri;
                if (!ari->has_term(actual_term_.c_str())) {
                    return;
                }
                index::detail::doc_vector_t dv=ari->term_doc_vector(actual_term_.c_str());
                dc_=common::Value(int64_t(dv.size()));
            }
            
            // Term doc count for same term is alway same
            virtual bool is_const() const { return true; }

            // This op doesn't require per-doc match info
            virtual bool uses_match_info() const { return false; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                return dc_;
            }
            
            virtual std::string to_string_impl() const {
                // @DC("term") or @DC(field,"term")
                if (field_.empty()) {
                    std::string s="@DC(\"";
                    s+=term_;
                    s+="\")";
                    return s;
                }
                std::string s="@DC(";
                s+=field_;
                s+=",\"";
                s+=term_;
                s+="\")";
                return s;
            }
            
        private:
            std::string term_;
            std::string field_;
            common::Value dc_;
        };

        // IDF for given term
        class IdfDocOpNode : public DocOpNode {
        public:
            IdfDocOpNode(const std::string &term, const std::string &field, common::ExecutionContext &ctx)
            : term_(term)
            , field_(field)
            , idf_(double(0))
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(field_.c_str());
                    if (!fd) {
                        throw argos_bad_field(field_.c_str());
                    }
                    prefix=fd->get_prefix();
                }
                std::string actual_term_=prefix+term_;
                index::ReverseIndex *ri=ctx.get_reverse_index();
                index::detail::ArgosReverseIndex *ari=(index::detail::ArgosReverseIndex *)ri;
                if (!ari->has_term(actual_term_.c_str())) {
                    return;
                }
                index::detail::doc_vector_t dv=ari->term_doc_vector(actual_term_.c_str());
                size_t dc=dv.size();
                size_t doccount=ctx.get_index()->get_doc_count();
                double idf=log(double(doccount)/double(dc));
                idf_=common::Value(idf);
            }
            
            // Term doc count for same term is alway same
            virtual bool is_const() const { return true; }
            
            // This op doesn't require per-doc match info
            virtual bool uses_match_info() const { return false; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                return idf_;
            }
            
            virtual std::string to_string_impl() const {
                // @IDF("term") or @IDF(field,"term")
                if (field_.empty()) {
                    std::string s="@IDF(\"";
                    s+=term_;
                    s+="\")";
                    return s;
                }
                std::string s="@IDF(";
                s+=field_;
                s+=",\"";
                s+=term_;
                s+="\")";
                return s;
            }
            
        private:
            std::string term_;
            std::string field_;
            common::Value idf_;
        };
        
        // TFIDF score for current doc
        // TF=TC/DL
        // IDF=log(totaldoccount/doccount)
        // TFIDF= SUM( TF * IDF ) for each term analysed from "term"
        class TfidfDocOpNode : public DocOpNode {
        public:
            TfidfDocOpNode(const std::string &term, const std::string &field, common::ExecutionContext &ctx)
            : term_(term)
            , field_(field)
            , match_info_poses_()
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(field_.c_str());
                    if (!fd) {
                        throw argos_bad_field(field_.c_str());
                    }
                    prefix=fd->get_prefix();
                }
                // Parse all match terms
                common::simple_term_list_t tl;
                analyzer::analyzer_ptr_t a;
                if (field_.empty()) {
                    a=analyzer::get_default_analyzer();
                } else {
                    a=analyzer::get_analyzer(ctx.get_field_config()->get_field_def(field_.c_str())->get_analyzer_name());
                }
                a->analyse_query(prefix.c_str(), term_, tl);
                index::ReverseIndex *ri=ctx.get_reverse_index();
                index::detail::ArgosReverseIndex *ari=(index::detail::ArgosReverseIndex *)ri;
                for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                    common::match_term_dict_t::const_iterator ti=ctx.match_term_dict.find(*i);
                    if (ti!=ctx.match_term_dict.end()) {
                        match_info_poses_.push_back(ti->second);
                        // Calculate idf for each term
                        if (!ari->has_term(i->c_str())) {
                            idfs_.push_back(0);
                            continue;
                        }
                        index::detail::doc_vector_t dv=ari->term_doc_vector(i->c_str());
                        size_t dc=dv.size();
                        size_t doccount=ctx.get_index()->get_doc_count();
                        double idf=log(double(doccount)/double(dc));
                        idfs_.push_back(idf);
                    }
                }
                // Record all needed fields to calculate doc-length
                for (int i=0; i<ctx.get_field_config()->count(); i++) {
                    if (prefix==ctx.get_field_config()->get_field_def(i)->get_prefix()) {
                        included_fields_.push_back(i);
                    }
                }
            }
            
            // TFIDF is not same for different doc
            virtual bool is_const() const { return false; }
            // This op requires per-doc match info
            virtual bool uses_match_info() const { return true; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                common::match_info_t *l=context.get_match_info();
                if(match_info_poses_.empty() || !l) {
                    return common::Value(double(0));
                }
                double ret=0;
                for (int i=0; i<match_info_poses_.size(); i++) {
                    int64_t tc= (*l)[match_info_poses_[i]];
                    size_t dl=0;    // TermCount/DocLen
                    for (std::vector<int>::const_iterator j=included_fields_.begin(); j!=included_fields_.end(); ++j) {
                        dl+=context.get_forward_index()->get_field_length(did, *j);
                    }
                    if (dl==0) {
                        // Shouldn't happen, the doc has this term at least
                        return common::Value(double(0));
                    }
                    double tf=double(tc)/double(dl);
                    double tfidf=tf*idfs_[i];
                    ret+=tfidf;
                }
                return common::Value(ret);
            }
            
            virtual std::string to_string_impl() const {
                // @TFIDF("text") or @TFIDF(field,"text")
                if (field_.empty()) {
                    std::string s="@TFIDF(\"";
                    s+=term_;
                    s+="\")";
                    return s;
                }
                std::string s="@TFIDF(";
                s+=field_;
                s+=",\"";
                s+=term_;
                s+="\")";
                return s;
            }
            
        private:
            std::string term_;
            std::string field_;
            std::vector<int> included_fields_;
            std::vector<int> match_info_poses_;
            std::vector<double> idfs_;
        };

        // NTFIDF normalized TFIDF score for current doc
        // TF=TC/DL
        // IDF=log(totaldoccount/doccount)
        // TFIDF= SUM( TF * IDF ) for each term analysed from "term"
        // NTFIDF=TFIDF/(log(totaldoccount) * (term count in text"))
        class NtfidfDocOpNode : public DocOpNode {
        public:
            NtfidfDocOpNode(const std::string &term, const std::string &field, common::ExecutionContext &ctx)
            : term_(term)
            , field_(field)
            , match_info_poses_()
            , norm_factor_(0)
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(field_.c_str());
                    if (!fd) {
                        throw argos_bad_field(field_.c_str());
                    }
                    prefix=fd->get_prefix();
                }
                // Parse all match terms
                common::simple_term_list_t tl;
                analyzer::analyzer_ptr_t a;
                if (field_.empty()) {
                    a=analyzer::get_default_analyzer();
                } else {
                    a=analyzer::get_analyzer(ctx.get_field_config()->get_field_def(field_.c_str())->get_analyzer_name());
                }
                a->analyse_query(prefix.c_str(), term_, tl);
                index::ReverseIndex *ri=ctx.get_reverse_index();
                index::detail::ArgosReverseIndex *ari=(index::detail::ArgosReverseIndex *)ri;
                for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                    common::match_term_dict_t::const_iterator ti=ctx.match_term_dict.find(*i);
                    if (ti!=ctx.match_term_dict.end()) {
                        match_info_poses_.push_back(ti->second);
                        // Calculate idf for each term
                        if (!ari->has_term(i->c_str())) {
                            idfs_.push_back(0);
                            continue;
                        }
                        index::detail::doc_vector_t dv=ari->term_doc_vector(i->c_str());
                        size_t dc=dv.size();
                        size_t doccount=ctx.get_index()->get_doc_count();
                        double idf=log(double(doccount)/double(dc));
                        idfs_.push_back(idf);
                    }
                }
                norm_factor_=log(double(ctx.get_index()->get_doc_count()))*tl.size();
                if (norm_factor_==0) {
                    // TODO: Error
                    norm_factor_=1;
                }
                // Record all needed fields to calculate doc-length
                for (int i=0; i<ctx.get_field_config()->count(); i++) {
                    if (prefix==ctx.get_field_config()->get_field_def(i)->get_prefix()) {
                        included_fields_.push_back(i);
                    }
                }
            }
            
            // TFIDF is not same for different doc
            virtual bool is_const() const { return false; }
            // This op requires per-doc match info
            virtual bool uses_match_info() const { return true; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                common::match_info_t *l=context.get_match_info();
                if(match_info_poses_.empty() || !l) {
                    return common::Value(double(0));
                }
                double ret=0;
                for (int i=0; i<match_info_poses_.size(); i++) {
                    int64_t tc= (*l)[match_info_poses_[i]];
                    size_t dl=0;    // TermCount/DocLen
                    for (std::vector<int>::const_iterator j=included_fields_.begin(); j!=included_fields_.end(); ++j) {
                        dl+=context.get_forward_index()->get_field_length(did, *j);
                    }
                    if (dl==0) {
                        // Shouldn't happen, the doc has this term at least
                        return common::Value(double(0));
                    }
                    double tf=double(tc)/double(dl);
                    double tfidf=tf*idfs_[i];
                    ret+=tfidf;
                }
                return common::Value(ret/norm_factor_);
            }
            
            virtual std::string to_string_impl() const {
                // @NTFIDF("text") or @NTFIDF(field,"text")
                if (field_.empty()) {
                    std::string s="@NTFIDF(\"";
                    s+=term_;
                    s+="\")";
                    return s;
                }
                std::string s="@NTFIDF(";
                s+=field_;
                s+=",\"";
                s+=term_;
                s+="\")";
                return s;
            }
            
        private:
            std::string term_;
            std::string field_;
            std::vector<int> included_fields_;
            std::vector<int> match_info_poses_;
            std::vector<double> idfs_;
            double norm_factor_;
        };

        // Check if a phrase exists in field
        class HasDocOpNode : public DocOpNode {
        public:
            HasDocOpNode(const std::string &term, const std::string &field, common::ExecutionContext &ctx)
            : term_(term)
            , field_(field)
            , match_info_poses_()
            {
                std::string prefix("=");
                if (!field_.empty()) {
                    prefix=ctx.get_field_config()->get_field_def(field_.c_str())->get_prefix();
                }
                // Parse all match terms
                common::simple_term_list_t tl;
                analyzer::analyzer_ptr_t a=analyzer::get_analyzer(ctx.get_field_config()->get_field_def(field_.c_str())->get_analyzer_name());
                a->analyse_query(prefix.c_str(), term_, tl);
                index::ReverseIndex *ri=ctx.get_reverse_index();
                index::detail::ArgosReverseIndex *ari=(index::detail::ArgosReverseIndex *)ri;
                for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                    common::match_term_dict_t::const_iterator ti=ctx.match_term_dict.find(*i);
                    if (ti!=ctx.match_term_dict.end()) {
                        match_info_poses_.push_back(ti->second);
                        {
                            // Calculate idf for each term
                            if (!ari->has_term(i->c_str())) {
                                continue;
                            }
                            index::detail::doc_vector_t dv=ari->term_doc_vector(i->c_str());
                            dcs_.push_back(uint32_t(dv.size()));
                        }
                    }
                }
                // Record all needed fields to calculate doc-length
                for (int i=0; i<ctx.get_field_config()->count(); i++) {
                    if (prefix==ctx.get_field_config()->get_field_def(i)->get_prefix()) {
                        included_fields_.push_back(i);
                    }
                }
            }
            
            // HAS returned value is not same for different doc
            virtual bool is_const() const { return false; }
            // This op requires per-doc match info
            virtual bool uses_match_info() const { return true; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                common::match_info_t *l=context.get_match_info();
                if(match_info_poses_.empty() || !l) {
                    return common::Value(int64_t(0));
                }
                // Return 1 only if all terms exist
                for (int i=0; i<match_info_poses_.size(); i++) {
                    int64_t tc= (*l)[match_info_poses_[i]];
                    if (tc==0) {
                        return common::Value(int64_t(0));
                    }
                }
                return common::Value(int64_t(1));
            }
            
            virtual std::string to_string_impl() const {
                // @HAS("text") or @HAS(field,"text")
                if (field_.empty()) {
                    std::string s="@HAS(\"";
                    s+=term_;
                    s+="\")";
                    return s;
                }
                std::string s="@HAS(";
                s+=field_;
                s+=",\"";
                s+=term_;
                s+="\")";
                return s;
            }
            
        private:
            std::string term_;
            std::string field_;
            std::vector<int> included_fields_;
            std::vector<int> match_info_poses_;
            std::vector<uint32_t> dcs_;
        };

        // total number of docs in the index
        class DoccountDocOpNode : public DocOpNode {
        public:
            DoccountDocOpNode(common::ExecutionContext &ctx)
            : doccount_(int64_t(ctx.get_index()->get_doc_count()))
            {}
            
            // Doc count for is alway same
            virtual bool is_const() const { return true; }

            // This op doesn't require per-doc match info
            virtual bool uses_match_info() const { return false; }
            
            virtual common::Value evaluate(docid did, common::ExecutionContext &context) const {
                return doccount_;
            }
            
            virtual std::string to_string_impl() const {
                // @DOCCOUNT
                return "@DOCCOUNT";
            }
            
            virtual std::ostream &serialize(std::ostream &os) const {
                // @DOCCOUNT
                os << "@DOCCOUNT";
                return os;
            }
            
        private:
            common::Value doccount_;
        };
    }   // End of namespace query
    namespace parser {
        common::eva_ptr_t parse_doc_op(const char *&str, size_t &len, common::ExecutionContext &ctx)
        {
            // Leading '@' is already parsed
            using namespace common;
            token t=next_token(str, len);
            if (t.type!=TT_ID) {
                return eva_ptr_t();
            }
            std::string op(t.p, t.sz);
            if (op=="TC") {
                // Parse TC("term")
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                std::string term;
                std::string field;
                if (t.type==TT_ID) {
                    field=std::string(t.p, t.sz);
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA) {
                        throw argos_syntax_error("Syntax error in match");
                    }
                    t=next_token(str, len);
                }
                if (t.type==TT_STR) {
                    term=std::string(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=std::string(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::TcDocOpNode(term, field, ctx));
            } else if (op=="TF") {
                // Parse TF("term")
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                std::string term;
                std::string field;
                if (t.type==TT_ID) {
                    field=std::string(t.p, t.sz);
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA) {
                        throw argos_syntax_error("Syntax error in match");
                    }
                    t=next_token(str, len);
                }
                if (t.type==TT_STR) {
                    term=std::string(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=std::string(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::TfDocOpNode(term,field,ctx));
            } else if (op=="TFIDF") {
                // Parse TFIDF("term")
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                std::string term;
                std::string field;
                if (t.type==TT_ID) {
                    field=std::string(t.p, t.sz);
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA) {
                        throw argos_syntax_error("Syntax error in match");
                    }
                    t=next_token(str, len);
                }
                if (t.type==TT_STR) {
                    term=std::string(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=std::string(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::TfidfDocOpNode(term,field,ctx));
            } else if (op=="NTFIDF") {
                // Parse NTFIDF("term")
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                std::string term;
                std::string field;
                if (t.type==TT_ID) {
                    field=std::string(t.p, t.sz);
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA) {
                        throw argos_syntax_error("Syntax error in match");
                    }
                    t=next_token(str, len);
                }
                if (t.type==TT_STR) {
                    term=std::string(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=std::string(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::NtfidfDocOpNode(term,field,ctx));
            } else if (op=="HAS") {
                // Parse HAS(field,"term")
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_ID) {
                    throw argos_syntax_error("Syntax error in match");
                }
                std::string field(t.p, t.sz);
                t=next_token(str, len);
                if (t.type!=TT_COMMA) {
                    throw argos_syntax_error("Syntax error in match");
                }
                std::string term;
                t=next_token(str, len);
                if (t.type==TT_STR) {
                    term=std::string(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=std::string(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::HasDocOpNode(term, field,ctx));
            } else if (op=="DC") {
                // Parse DC("term")
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                std::string term;
                std::string field;
                if (t.type==TT_ID) {
                    field=std::string(t.p, t.sz);
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA) {
                        throw argos_syntax_error("Syntax error in match");
                    }
                    t=next_token(str, len);
                }
                if (t.type==TT_STR) {
                    term=std::string(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=std::string(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::DcDocOpNode(term,field,ctx));
            } else if (op=="IDF") {
                // Parse IDF("term")
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                std::string term;
                std::string field;
                if (t.type==TT_ID) {
                    field=std::string(t.p, t.sz);
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA) {
                        throw argos_syntax_error("Syntax error in match");
                    }
                    t=next_token(str, len);
                }
                if (t.type==TT_STR) {
                    term=std::string(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=std::string(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::IdfDocOpNode(term,field,ctx));
            } else if (op=="DL") {
                // Parse DL or DL(field)
                t=peek_next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    return eva_ptr_t(new query::DlDocOpNode("", ctx));
                }
                // Consume next '('
                t=next_token(str, len);
                // Get field name
                t=next_token(str, len);
                std::string field;
                if (t.type==TT_ID) {
                    field=std::string(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                return eva_ptr_t(new query::DlDocOpNode(field,ctx));
            } else if (op=="DOCCOUNT") {
                return eva_ptr_t(new query::DoccountDocOpNode(ctx));
            } else {
                //
            }
            throw argos_syntax_error("Syntax error in match");
        }
    }
}   // End of namespace argos