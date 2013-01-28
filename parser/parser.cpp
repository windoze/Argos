//
//  parser.cpp
//  Argos
//
//  Created by Windoze on 12-7-8.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <sys/time.h>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "common/argos_exception.h"
#include "common/util.h"
#include "common/token_parser.h"
#include "common/field_config.h"
#include "common/expr_node.h"
#include "index/field_evaluator.h"
#include "query/enquire.h"
#include "parser.h"
#include "value_parser.h"

namespace argos {
    namespace parser {
        using namespace common;
        
        inline double get_time()
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return double(tv.tv_sec)+double(tv.tv_usec)/1000000;
        }
        
        bool parse_value_list(const char *&str, size_t &len, value_list_t &vl, ExecutionContext &context)
        {
            value_list_parser<const char*> parser;
            boost::spirit::ascii::space_type space;
            return boost::spirit::qi::phrase_parse(str, str+len, parser, space, vl);
        }

        template<typename Iterator>
        inline bool parse_value(Iterator first, Iterator last, Value &v)
        {
            value_parser<Iterator> parser;
            boost::spirit::ascii::space_type space;
            return boost::spirit::qi::phrase_parse(first, last, parser, space, v);
        }
        
        // TODO: Uses something like Operator registration
        // @ref argos_ri_backend/doc_ops.cpp
        common::eva_ptr_t create_doc_op(const std::string &name, const std::string &field, const std::string &text, common::ExecutionContext &ctx);
        
        // TODO: Optimize this
        // A set of adaptor classes, work with Boost.Spirit
        struct Composer {
            struct ExprComposer;
            
            typedef std::vector<ExprComposer> ExprListComposer;
            typedef std::vector<ExprComposer> OprandsComposer;
            
            struct FieldComposer {
                FieldComposer() {}
                FieldComposer(const std::string & identifier)
                : identifier_(identifier) {}
                std::string identifier_;
            };
            
            struct ConstantComposer {
                ConstantComposer() {}
                ConstantComposer(const common::Value &v)
                : value(v) {}
                common::Value value;
            };
            
            struct FunctionComposer {
                FunctionComposer() {}
                FunctionComposer(const std::pair<std::string, OprandsComposer> &call)
                : identifier_(call.first)
                , oprands_(call.second)
                {}
                std::string identifier_;
                OprandsComposer oprands_;
            };
            
            struct DocOpComposer {
                DocOpComposer() {}
                
                DocOpComposer(const boost::tuple<std::string, boost::optional<std::string>, boost::optional<std::string> > &doc_op_call)
                : identifier_(doc_op_call.get<0>())
                , field_(doc_op_call.get<1>() ? *doc_op_call.get<1>() : "")
                , text_(doc_op_call.get<2>() ? *doc_op_call.get<2>() : "")
                {}
                std::string identifier_;
                std::string field_;
                std::string text_;
            };
            
            struct ExprComposer {
                ExprComposer() : type(common::ET_NONE) {
                    // Default
                }
                
                ExprComposer(const ConstantComposer &c)
                : type(common::ET_CONST)
                , value(c.value)
                {
                    // Constant
                }
                
                explicit ExprComposer(const FunctionComposer &c)
                : type(common::ET_EXPR)
                , identifier_(c.identifier_)
                , oprands_(c.oprands_)
                {
                    // Function
                }
                
                explicit ExprComposer(const DocOpComposer &c)
                : type(common::ET_DOC)
                , identifier_(c.identifier_)
                , field_(c.field_)
                , text_(c.text_)
                {
                    // DocOp
                }
                
                ExprComposer(const FieldComposer &c)
                : type(common::ET_ATTR)
                , identifier_(c.identifier_)
                {
                    // Field or Named Constants
                }
                
                eva_ptr_t compose_expr(ExecutionContext &context) const {
                    expr_ptr_t ex(new ExprNode(identifier_.c_str()));
                    if(!ex->op)
                        throw argos_bad_operator(identifier_.c_str());;
                    ex->oprands.resize(oprands_.size());
                    for (size_t i=0; i<oprands_.size(); ++i) {
                        ex->oprands[i]=oprands_[i].compose(context);
                    }
                    return ex;
                }
                
                eva_ptr_t compose_field(ExecutionContext &context) const {
                    // Named Constants
                    if (identifier_=="PI") {
                        eva_ptr_t(new common::Constant(common::Value(3.141592653589793)));
                    }
                    if (identifier_=="E") {
                        eva_ptr_t(new common::Constant(common::Value(2.718281828459045)));
                    }
                    if (identifier_=="SOFA") {
                        eva_ptr_t(new common::Constant(common::Value(2.207416099162478)));
                    }
                    if (identifier_=="NOW") {
                        eva_ptr_t(new common::Constant(common::Value(get_time())));
                    }
                    
                    // Or Fields
                    int fid=context.get_field_config()->get_field_id(identifier_.c_str());
                    if (fid<0) {
                        throw argos_bad_field(std::string(identifier_.c_str()).c_str());
                    }
                    field_eva_cache_t::const_iterator i=context.field_eva_cache.find(fid);
                    if(i==context.field_eva_cache.end())
                    {
                        eva_ptr_t e=eva_ptr_t(new index::FieldEvaluator(context.get_field_config(), fid));
                        context.field_eva_cache[fid]=e;
                        return e;
                    }
                    return i->second;
                }
                
                eva_ptr_t compose_docop(ExecutionContext &context) const {
                    return create_doc_op(identifier_, field_, text_, context);
                }
                
                eva_ptr_t compose(ExecutionContext &context) const {
                    switch(type) {
                        case common::ET_NONE:
                            break;
                        case common::ET_CONST:
                            return eva_ptr_t(new common::Constant(value));
                            break;
                        case common::ET_EXPR:
                            return compose_expr(context);
                            break;
                        case common::ET_DOC:
                            return compose_docop(context);
                            break;
                        case common::ET_ATTR:
                            return compose_field(context);
                            break;
                    }
                    throw argos_syntax_error("Syntax error in expression");
                    return eva_ptr_t();
                }
                
                common::EVA_TYPE type;
                common::Value value;
                std::string identifier_;
                OprandsComposer oprands_;
                boost::optional<std::pair<boost::optional<std::string>, boost::optional<std::string> > > doc_oprands_;
                std::string field_;
                std::string text_;
            };
            
            typedef std::pair<ExprComposer, bool> sort_criterion;
            typedef std::vector<sort_criterion> sort_criteria;
            typedef std::vector<ExprListComposer> histograms;
        };
        
        std::ostream &operator<<(std::ostream &os, const Composer::ExprComposer &comp) {
            switch(comp.type) {
                case common::ET_NONE:
                    os << "<NULL>";
                    break;
                case common::ET_CONST:
                    common::Constant(comp.value).serialize(os);
                    break;
                case common::ET_EXPR:
                    os << comp.identifier_ << "(...)";
                    break;
                case common::ET_DOC:
                    os << '@' << comp.identifier_ << "(...)";
                    break;
                case common::ET_ATTR:
                    os << comp.identifier_;
                    break;
            }
            return os;
        }
        
        eva_ptr_t parse_expr(const char *&str, size_t &len, ExecutionContext &context)
        {
            expr_parser<const char *, Composer> parser;
            boost::spirit::ascii::space_type space;
            Composer::ExprComposer comp;
            if(!boost::spirit::qi::phrase_parse(str, str+len, parser, space, comp)) {
                throw argos_syntax_error("Syntax error in expression");
            }
            eva_ptr_t e=comp.compose(context);
            // return optimize(e, context)
            return e;
        }

        bool parse_expr_list(const char *&str, size_t &len, oprands_t &oprands, ExecutionContext &context)
        {
            expr_list_parser<const char *, Composer> parser;
            boost::spirit::ascii::space_type space;
            Composer::ExprListComposer comp;
            if(!boost::spirit::qi::phrase_parse(str, str+len, parser, space, comp)) {
                throw argos_syntax_error("Syntax error in expression");
            }
            oprands.resize(comp.size());
            for(size_t i=0; i<comp.size(); i++) {
                oprands[i]=comp[i].compose(context);
                //std::cout << oprands[i] << std::endl;
            }
            return true;
        }
        
        bool parse_sort_crit(const char *&str, size_t &len, query::sort_criteria &sort_crit, ExecutionContext &context)
        {
            sort_criteria_parser<const char *, Composer> parser;
            boost::spirit::ascii::space_type space;
            Composer::sort_criteria crit;
            if(!boost::spirit::qi::phrase_parse(str, str+len, parser, space, crit)) {
                throw argos_syntax_error("Syntax error");
            }
            sort_crit.resize(crit.size());
            for(size_t i=0; i<crit.size(); i++) {
                sort_crit[i].sort_key=crit[i].first.compose(context);
                sort_crit[i].desc=crit[i].second;
            }
            return true;
        }
        
        // expr,expr;expr,expr,expr;...;expr,expr
        bool parse_groups(const char *&str, size_t &len, std::vector<common::field_list_t> &groups, common::ExecutionContext &context)
        {
            histo_parser<const char *, Composer> parser;
            boost::spirit::ascii::space_type space;
            Composer::histograms comp;
            if(!boost::spirit::qi::phrase_parse(str, str+len, parser, space, comp)) {
                throw argos_syntax_error("Syntax error");
            }
            groups.resize(comp.size());
            for(size_t i=0; i<comp.size(); ++i) {
                groups[i].resize(comp[i].size());
                for(size_t j=0; j<comp[i].size(); ++j) {
                    groups[i][j]=comp[i][j].compose(context);
                }
            }
            return true;
        }

        bool parse_field_list(const char *&str, size_t &len, field_list_t &fl, ExecutionContext &context)
        {
            return parse_expr_list(str, len, fl, context);
        }

        MatchParser::MatchParser()
        : impl_(detail::create_match_parser_impl())
        {}
        
        MatchParser::~MatchParser()
        {
            if (impl_) {
                delete impl_;
            }
        }
        
        query::match_ptr_t MatchParser::parse(const char *&str, size_t &len, common::ExecutionContext &context)
        {
            return impl_->parse(str, len, context);
        }
        
        bool parse_query(const char *fmt, size_t fmt_len,
                         const char *match, size_t match_len,
                         const char *filter, size_t filter_len,
                         const char *sort_crit, size_t sort_crit_len,
                         const char *histos, size_t histos_len,
                         const char *nr, size_t nr_len,
                         const char *sk, size_t sk_len,
                         const char *field_list, size_t field_list_len,
                         const char *id_list, size_t id_list_len,
                         const char *qid, size_t qid_len,
                         const char *c, size_t c_len,
                         query::Query &q,
                         common::ExecutionContext &context)
        {
            if (qid_len>0 && qid[0]) {
                // Use query id passed from caller, in any format
                q.query_id_.assign(qid, qid+qid_len);
            } else {
                // Generate a UUID as the query id
                // NOTE: Generate UUID takes about 5ms?
                //q.query_id_=boost::uuids::to_string(boost::uuids::random_generator()());
            }
            
            if (fmt_len>0 && fmt[0]) {
                q.fmt=std::string(fmt, fmt_len);
            } else {
                q.fmt="XML";
            }

            if (c_len>0 && (c[0]=='1' || c[0]=='y' || c[0]=='Y')) {
                q.comp_=true;
            } else {
                q.comp_=false;
            }

            if (match_len>0 && match[0]) {
                if (match_len==1 && match[0]=='*') {
                    // '*' to do full table scan
                    q.match_=query::match_ptr_t(new query::MatchAll);
                } else {
                    q.match_=MatchParser().parse(match, match_len, context);
                    if (!q.match_) {
                        return false;
                    }
                }
                // We record origin match param here as the match is parsed and analysed via
                // specified analyzers and it's not easy to be assembled.
                // This param is only recorded *after* a successful parse so errors are detected
                q.match_param_.assign(match, match_len);
            } else {
                // No match
                q.match_=query::match_ptr_t(new query::MatchNone);
            }
            
            // Collect all terms used in this query, we'll record term counts for every matched doc
            q.match_->collect_match_term(context.match_term_dict);
            
            if (!(filter_len>0 && filter[0])) {
                // No filter
                filter="1";
                filter_len=1;
            }
            q.filter_=parse_expr(filter, filter_len, context);
            if (!q.filter_) {
                return false;
            }
            
            if (!(sort_crit_len>0 && sort_crit[0])) {
                // Default order by _id
                sort_crit="_id,ASC";
                sort_crit_len=7;
            }
            if(!parse_sort_crit(sort_crit, sort_crit_len, q.sort_crit_, context)) {
                return false;
            }

            if (histos_len>0 && histos[0]) {
                if(!parse_groups(histos, histos_len, q.histos_, context)) {
                    return false;
                }
            }
            
            if (nr_len>0 && nr[0]) {
                q.nr_=::atoi(nr);
            } else {
                // Default result set size is 15
                q.nr_=15;
            }
            if (sk_len>0 && sk[0]) {
                q.sk_=::atoi(sk);
            } else {
                // Default skip 0
                q.sk_=0;
            }
            
            if (!(field_list_len>0 && field_list[0])) {
                // Default returns primary key
                field_list="_id";
                field_list_len=3;
            }
            if (field_list_len==1 && field_list[0]=='*') {
                for (int i=0; i<context.get_field_config()->count(); i++) {
                    if (context.get_field_config()->get_field_def(i)->stored()) {
                        eva_ptr_t e=eva_ptr_t(new index::FieldEvaluator(context.get_field_config(), i));
                        q.fl_.push_back(e);
                    }
                }
            } else {
                if (!parse_field_list(field_list, field_list_len, q.fl_, context)) {
                    return false;
                }
            }

            if (id_list_len>0 && id_list[0]) {
                // Parse id_list
                common::value_list_t vl;
                if(!parse_value_list(id_list, id_list_len, vl, context))
                    return false;
                q.pks_.resize(vl.size());
                for (size_t i=0; i<vl.size(); i++) {
                    if (vl[i].type_!=VT_INTEGER) {
                        return false;
                    }
                    q.pks_[i]=primary_key(vl[i].cast(VT_INTEGER).number);
                }
            }
            return true;
        }
        
        inline size_t find_eq(const char *str, size_t len)
        {
            for (size_t i=0; i<len; i++) {
                if (str[i]=='=') {
                    return i;
                }
            }
            return len+1;
        }
        
        inline bool split_kv(const char *str, size_t len, size_t &key_len, const char *&value, size_t &value_len)
        {
            size_t pos=find_eq(str, len);
            if (pos==0) {
                // No key
                return false;
            }
            if (pos>len) {
                // '=' not found
                return false;
            }
            key_len=pos;
            if (key_len+1==len) {
                // '=' is the last char, no value
                value=NULL;
                value_len=0;
                return true;
            }
            value=str+key_len+1;
            value_len=len-key_len-1;
            return true;
        }
        
        inline bool next_param(const char *&str, size_t &len, const char *&param, size_t &param_len)
        {
            if (!str || !str[0] || len==0) {
                return false;
            }
            size_t i=0;
            for (; i<len; i++) {
                if(str[i]=='&') {
                    break;
                }
            }
            param=str;
            param_len=i;
            if (len<=i) {
                str+=len;
                len=0;
                return true;
            }
            str+=(i+1);
            len-=(i+1);
            return true;
        }
        
        inline char *alloc_string(mem_pool *mp, size_t sz)
        {
            OFFSET off=mp->allocate(sz+1);
            char *ret=(char *)(mp->get_addr(off));
            ret[sz-1]=0;
            return ret;
        }
        
        bool parse_query(const char *req, size_t len, query::Query &q, ExecutionContext &ctx)
        {
            const char *param=NULL;
            size_t param_len=0;
            
            const char *fmt=NULL;
            size_t fmt_len=0;
            const char *match=NULL;
            size_t match_len=0;
            const char *filter=NULL;
            size_t filter_len=0;
            const char *sort_crit=NULL;
            size_t sort_crit_len=0;
            const char *histos=NULL;
            size_t histos_len=0;
            const char *nr=NULL;
            size_t nr_len=0;
            const char *sk=NULL;
            size_t sk_len=0;
            const char *field_list=NULL;
            size_t field_list_len=0;
            const char *id_list=NULL;
            size_t id_list_len=0;
            const char *qid=NULL;
            size_t qid_len=0;
            const char *c=NULL;
            size_t c_len=0;
            
            while (next_param(req, len, param, param_len)) {
                const char *key=param;
                size_t key_len=0;
                const char *value=NULL;
                size_t value_len=0;
                if (split_kv(param, param_len, key_len, value, value_len)) {
                    if (key_len==0) {
                        // Empty key is not allowed
                        return false;
                    }
                    if (value_len==0) {
                        // Empty value is ignored
                        continue;
                    }
                    if (strncasecmp(key, "fmt", 3)==0) {
                        fmt=value;
                        fmt_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, fmt_len);
                        size_t buf_len=urldecode(buf, fmt, fmt_len);
                        buf[buf_len]=0;
                        fmt=buf;
                        fmt_len=buf_len;
                        //std::cout << "fmt:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "m", key_len)==0) {
                        match=value;
                        match_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, match_len);
                        size_t buf_len=urldecode(buf, match, match_len);
                        buf[buf_len]=0;
                        match=buf;
                        match_len=buf_len;
                        //std::cout << "match:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "f", key_len)==0) {
                        filter=value;
                        filter_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, filter_len);
                        size_t buf_len=urldecode(buf, filter, filter_len);
                        buf[buf_len]=0;
                        filter=buf;
                        filter_len=buf_len;
                        //std::cout << "filter:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "s", key_len)==0) {
                        sort_crit=value;
                        sort_crit_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, sort_crit_len);
                        size_t buf_len=urldecode(buf, sort_crit, sort_crit_len);
                        buf[buf_len]=0;
                        sort_crit=buf;
                        sort_crit_len=buf_len;
                        //std::cout << "sort_crit:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "h", key_len)==0) {
                        histos=value;
                        histos_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, histos_len);
                        size_t buf_len=urldecode(buf, histos, histos_len);
                        buf[buf_len]=0;
                        histos=buf;
                        histos_len=buf_len;
                        //std::cout << "histos:" << std::string(histos, histos_len) << std::endl;
                    } else if (strncasecmp(key, "nr", key_len)==0) {
                        nr=value;
                        nr_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, nr_len);
                        size_t buf_len=urldecode(buf, nr, nr_len);
                        buf[buf_len]=0;
                        nr=buf;
                        nr_len=buf_len;
                        //std::cout << "nr:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "sk", key_len)==0) {
                        sk=value;
                        sk_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, sk_len);
                        size_t buf_len=urldecode(buf, sk, sk_len);
                        buf[buf_len]=0;
                        sk=buf;
                        sk_len=buf_len;
                        //std::cout << "sk:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "fl", key_len)==0) {
                        field_list=value;
                        field_list_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, field_list_len);
                        size_t buf_len=urldecode(buf, field_list, field_list_len);
                        buf[buf_len]=0;
                        field_list=buf;
                        field_list_len=buf_len;
                        //std::cout << "field_list:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "id", key_len)==0) {
                        id_list=value;
                        id_list_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, id_list_len);
                        size_t buf_len=urldecode(buf, id_list, id_list_len);
                        buf[buf_len]=0;
                        id_list=buf;
                        id_list_len=buf_len;
                        //std::cout << "id_list:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "queryid", key_len)==0) {
                        qid=value;
                        qid_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, qid_len);
                        size_t buf_len=urldecode(buf, qid, qid_len);
                        buf[buf_len]=0;
                        qid=buf;
                        qid_len=buf_len;
                        //std::cout << "queryid:" << std::string(buf, buf_len) << std::endl;
                    } else if (strncasecmp(key, "c", key_len)==0) {
                        c=value;
                        c_len=value_len;
                        char *buf=alloc_string(ctx.temp_pool, c_len);
                        size_t buf_len=urldecode(buf, c, c_len);
                        buf[buf_len]=0;
                        c=buf;
                        c_len=buf_len;
                        //std::cout << "compress:" << std::string(buf, buf_len) << std::endl;
                    } else {
                        // TODO: Unknown param
                    }
                } else {
                    // Param format error
                    return false;
                }
            }
            return parse_query(fmt, fmt_len,
                               match, match_len,
                               filter, filter_len,
                               sort_crit, sort_crit_len,
                               histos, histos_len,
                               nr, nr_len,
                               sk, sk_len,
                               field_list, field_list_len,
                               id_list, id_list_len,
                               qid, qid_len,
                               c, c_len,
                               q,
                               ctx);
        }
    }   // End of namespace parser
}   // End of namespace argos
