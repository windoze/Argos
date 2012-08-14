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
        
        common::eva_ptr_t parse_doc_op(const char *&str, size_t &len, common::ExecutionContext &ctx);
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
        
        bool parse_expr_list(const char *&str, size_t &len, oprands_t &oprands, ExecutionContext &context)
        {
            if (len==0 || !str || !str[0]) { return false;}
            token t=peek_next_token(str, len);
            // Empty oprand list
            if (t.type==TT_RPAREN) {
                return true;
            }
            eva_ptr_t node=parse_expr(str, len, context);
            if(!node)
                return false;
            oprands.push_back(node);
            while (len>0) {
                token t=peek_next_token(str, len);
                switch (t.type) {
                    case TT_SEMI:
                        // Segment complete
                        return true;
                    case TT_COMMA:
                    {
                        // Consume next ','
                        next_token(str, len);
                        eva_ptr_t node=parse_expr(str, len, context);
                        if(!node)
                            return false;
                        oprands.push_back(node);
                    }
                        break;
                    case TT_RPAREN:
                        // Parse completed
                        return true;
                    default:
                        // Oprands list must be seperated by ','
                        return false;
                }
            }
            return true;
        }
        
        inline double get_time()
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return double(tv.tv_sec)+double(tv.tv_usec)/1000000;
        }
        
        eva_ptr_t parse_expr(const char *&str, size_t &len, ExecutionContext &context)
        {
            // TODO: Error report
            token t=next_token(str, len);
            switch (t.type) {
                case TT_INT:
                case TT_FLOAT:
                case TT_FPAIR:
                case TT_STR:
                case TT_ARRAY:
                    // Return const
                    // TODO: ensure t.sz size constraints
                    // TODO: Use mem_pool_allocator
                {
                    Value v;
                    if(parse_value(t.p, t.p+t.sz, v)) {
                        return eva_ptr_t(new Constant(v));
                    }
                    throw argos_syntax_error("Syntax error in expression");
                }
                case TT_AT:
                {
                    return parse_doc_op(str, len, context);
                }
                case TT_ID:
                {
                    token tn=peek_next_token(str, len);
                    if (tn.type==TT_COLON) {
                        // Consume colon
                        next_token(str, len);
                        eva_ptr_t expr=parse_expr(str, len, context);
                        if (expr) {
                            expr->set_name(std::string(t.p, t.sz));
                        }
                        return expr;
                    } else {
                        if (t.sz==1 && t.p[0]=='E') {
                            return eva_ptr_t(new common::Constant(common::Value(2.718281828459045)));
                        } else if (t.sz==2 && t.p[0]=='P' && t.p[1]=='I') {
                            return eva_ptr_t(new common::Constant(common::Value(3.141592653589793)));
                        } else if (t.sz==3 && t.p[0]=='N' && t.p[1]=='O' && t.p[2]=='W') {
                            return eva_ptr_t(new common::Constant(common::Value(get_time())));
                        } else if (t.sz==4 && t.p[0]=='S' && t.p[1]=='O' && t.p[2]=='F' && t.p[3]=='A') {
                            // Moving sofa constant :)
                            return eva_ptr_t(new common::Constant(common::Value(2.207416099162478)));
                        }
                        token tn=peek_next_token(str, len);
                        if (tn.type!=TT_LPAREN) {
                            // TODO: Use mem_pool_allocator
                            // Generate FieldEvaluator
#if 0
                            return eva_ptr_t(new index::FieldEvaluator(context.get_field_config(), t.p, t.sz));
#else
                            int fid=context.get_field_config()->get_field_id(t.p, t.sz);
                            if (fid<0) {
                                throw argos_bad_field(std::string(t.p, t.sz).c_str());
                            }
                            field_eva_cache_t::const_iterator i=context.field_eva_cache.find(fid);
                            if(i==context.field_eva_cache.end())
                            {
                                eva_ptr_t e=eva_ptr_t(new index::FieldEvaluator(context.get_field_config(), fid));
                                context.field_eva_cache[fid]=e;
                                return e;
                            }
                            return i->second;
#endif
                        }
                        
                        // TODO: Use mem_pool_allocator
                        expr_ptr_t node=expr_ptr_t(new ExprNode(t.p, t.sz));
                        if (!node->op) {
                            // Undefined operator
                            throw argos_bad_operator(std::string(t.p, t.sz).c_str());
                            return eva_ptr_t();
                        }
                        // Consume '('
                        token tp=next_token(str, len);
                        if (tp.type!=TT_LPAREN) {
                            // Operator must be followed by '('
                            throw argos_syntax_error("Operator must be followed by '('");
                            return eva_ptr_t();
                        }
                        if (!parse_expr_list(str, len, node->oprands, context)) {
                            return eva_ptr_t();
                        }
                        // Consume ')'
                        tp=next_token(str, len);
                        if (tp.type!=TT_RPAREN) {
                            // Missing ')'
                            throw argos_syntax_error("Missing ')'");
                            return eva_ptr_t();
                        }
                        // Validate arity
                        if (!node->op->validate_arity(node->oprands.size())) {
                            // Wrong number of oprands
                            throw argos_syntax_error("Wrong number of arguments");
                            return eva_ptr_t();
                        }
                        return optimize(node, context);
                        //return node;
                    }
                }
                    break;
            }
            // Expression can only be constants or ID(...)
            throw argos_syntax_error("Syntax error in expression");
        }
        
        bool parse_sort_crit(const char *&str, size_t &len, query::sort_criteria &sort_crit, ExecutionContext &context)
        {
            query::sort_criterion sc;
            while (len>0) {
                sc.sort_key=parse_expr(str, len, context);
                if (!sc.sort_key) {
                    return false;
                }
                if (len==0) {
                    // Default is ascending sort
                    sc.desc=false;
                    sort_crit.push_back(sc);
                    return true;
                }
                token t=next_token(str, len);
                if (t.type!=TT_COMMA) {
                    return false;
                }
                t=next_token(str, len);
                if (t.type!=TT_ID) {
                    return false;
                }
                if ((t.p[0]=='a') || (t.p[0]=='A')) {
                    sc.desc=false;
                } else if ((t.p[0]=='d') || (t.p[0]=='D')) {
                    sc.desc=true;
                } else {
                    return false;
                }
                sort_crit.push_back(sc);
                if (len>0) {
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA && len>0) {
                        return false;
                    }
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
        
        // expr,expr;expr,expr,expr;...;expr,expr
        bool parse_groups(const char *&str, size_t &len, std::vector<common::field_list_t> &groups, common::ExecutionContext &context)
        {
            while (len>0) {
                field_list_t fl;
                if(!parse_field_list(str, len, fl, context)) {
                    return false;
                }
                groups.push_back(fl);
                if (len>0) {
                    token t=next_token(str, len);
                    if (t.type!=TT_SEMI) {
                        return false;
                    }
                }
            }
            return true;
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
                               q,
                               ctx);
        }
    }   // End of namespace parser
}   // End of namespace argos
