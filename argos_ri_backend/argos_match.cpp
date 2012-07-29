//
//  argos_match.cpp
//  Argos
//
//  Created by Windoze on 12-7-10.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include "common/token_parser.h"
#include "query/doc_iterator.h"
#include "query/match.h"
#include "parser.h"
#include "rindex.h"
#include "analyzer.h"
#include "argos_match.h"

namespace argos {
    namespace query {
        namespace detail {
            class term_doc_iterator_impl : public doc_iterator_impl
            {
            public:
                term_doc_iterator_impl(index::detail::ArgosReverseIndex *rindex, const char *term, int match_info_pos)
                : it_()
                , term_(term)
                , est_size_(0)
                , match_info_pos_(match_info_pos)
                {
                    if(!rindex->has_term(term))
                        return;
                    index::detail::doc_vector_t dv=rindex->term_doc_vector(term);
                    est_size_=dv.size();
                    it_=dv.begin();
                }
                
                virtual bool end() const
                {
                    return it_.ended();
                }
                
                virtual void next()
                {
                    it_++;
                }
                
                virtual docid skip_to(docid did)
                {
                    while (get_docid()<did) {
                        it_++;
                    }
                    return get_docid();
                }
                
                virtual docid get_docid() const
                {
                    if (end()) {
                        return INVALID_DOC;
                    }
                    return *it_;
                }
                
                virtual size_t estimate_size() const
                {
                    return est_size_;
                }
                
                virtual void save_match_info(docid did, common::match_info_t &match_info)
                {
                    if ((match_info_pos_<0) || (did!=get_docid())) {
                        return;
                    }
                    // Record term count
                    // TODO: Record something else such as position
                    match_info[match_info_pos_]=common::Value(int64_t((*it_).freq));
                }
                
                index::detail::doc_vector_t::iterator it_;
                common::term_t term_;
                size_t est_size_;
                int match_info_pos_;
            };
            
            ArgosMatch::ArgosMatch()
            : match_info_pos_(-1)
            {}
            
            ArgosMatch::ArgosMatch(const common::term_t &term)
                : term_(term)
                , match_info_pos_(-1)
                {}
                
            ArgosMatch::ArgosMatch(int op, const children_t &children)
            : op_(op)
            , children_(children)
            , match_info_pos_(-1)
            {}
            
            void ArgosMatch::collect_match_term(int &next_pos, common::match_term_dict_t &dict)
            {
                if (children_.empty()) {
                    if (term_.empty()) {
                        return;
                    }
                    if (dict.find(term_)==dict.end()) {
                        match_info_pos_=next_pos;
                        dict[term_]=match_info_pos_;
                        next_pos++;
                    }
                    return;
                }
                for (children_t::iterator i=children_.begin(); i!=children_.end(); ++i) {
                    i->collect_match_term(next_pos, dict);
                }
            }
            
            doc_iterator_impl_ptr_t ArgosMatch::match(common::ExecutionContext &ctx)
            {
                if (children_.empty()) {
                    if (term_.empty()) {
                        if (op_==OP_IGNORED) {
                            return multi_doc_iterator_impl(OP_IGNORED);
                        } else {
                            return match_none_ptr;
                        }
                    }
                    index::ReverseIndex *ri=ctx.get_reverse_index();
                    index::detail::ArgosReverseIndex *ari=(index::detail::ArgosReverseIndex *)ri;
                    return doc_iterator_impl_ptr_t(new term_doc_iterator_impl(ari, term_.c_str(), match_info_pos_));
                }
                
                //if (children_.size()==1) {
                //    return children_[0].match(ctx);
                //}
                
                std::vector<doc_iterator_impl_ptr_t> sub;
                for (children_t::iterator i=children_.begin(); i!=children_.end(); ++i) {
                    sub.push_back(i->match(ctx));
                }
                return detail::multi_doc_iterator_impl(op_, sub);
            }
        }   // End of namespace detail
    }   // End of namespace query
    namespace parser {
        namespace detail {
            query::detail::ArgosMatch argos_parse_query(const char *&str, size_t &len, common::ExecutionContext &ctx);
            
            query::detail::ArgosMatch argos_parse_term_query(const char *&str, size_t &len, common::ExecutionContext &ctx)
            {
                using namespace common;
                token t=next_token(str, len);
                if (t.type==TT_STR) {
                    common::term_t term(t.p+1, t.sz-2);
                    common::simple_term_list_t tl;
                    analyzer::analyzer_ptr_t a=analyzer::get_default_analyzer();
                    if (!a) {
                        a=analyzer::get_analyzer("token");
                    }
                    if (/*a->is_complex()*/0) {
                        std::vector<query::detail::ArgosMatch> or_q;
                        a->analyse_query("=", term, tl);
                        if (tl.size()==1) {
                            or_q.push_back(query::detail::ArgosMatch(tl[0]));
                        } else {
                            std::vector<query::detail::ArgosMatch> ql1;
                            for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                                ql1.push_back(query::detail::ArgosMatch(*i));
                            }
                            or_q.push_back(query::detail::ArgosMatch(query::OP_AND, ql1));
                        }
                        
                        tl.clear();
                        a->analyse_query_simple("=", term, tl);
                        if (tl.size()==1) {
                            or_q.push_back(query::detail::ArgosMatch(tl[0]));
                        } else {
                            std::vector<query::detail::ArgosMatch> ql2;
                            for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                                ql2.push_back(query::detail::ArgosMatch(*i));
                            }
                            or_q.push_back(query::detail::ArgosMatch(query::OP_AND, ql2));
                        }
                        return query::detail::ArgosMatch(query::OP_OR, or_q);
                    } else {
                        a->analyse_query("=", term, tl);
                        if (tl.size()==1) {
                            return query::detail::ArgosMatch(tl[0]);
                        }
                        std::vector<query::detail::ArgosMatch> ql;
                        for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                            ql.push_back(query::detail::ArgosMatch(*i));
                        }
                        return query::detail::ArgosMatch(query::OP_AND, ql);
                    }
                }
                throw argos_syntax_error("Syntax error in match");
            }
            
            query::detail::ArgosMatch argos_parse_and_or_query(const char *&str, size_t &len, common::ExecutionContext &ctx)
            {
                using namespace common;
                token t=next_token(str, len);
                if (t.type==TT_ID) {
                    token tn=next_token(str, len);
                    if (tn.type!=TT_LPAREN) {
                        // Error, ID must be followed by '('
                        throw argos_syntax_error("Syntax error in match");
                    }
                    arstring s(t.p, t.sz);
                    int o;
                    if (s=="AND") {
                        o=query::OP_AND;
                    } else if (s=="OR") {
                        o=query::OP_OR;
                    } else {
                        // Error, only AND and OR are supported
                        throw argos_syntax_error("Syntax error in match");
                    }
                    std::vector<query::detail::ArgosMatch> ql;
                    query::detail::ArgosMatch q=argos_parse_query(str, len, ctx);
                    ql.push_back(q);
                    while (len>0) {
                        t=peek_next_token(str, len);
                        if (t.type==TT_RPAREN) {
                            // Consume next ')'
                            t=next_token(str, len);
                            break;
                        }
                        if (t.type!=TT_COMMA) {
                            // op(Q1 ','...)
                            throw argos_syntax_error("Syntax error in match");
                        }
                        // Consume next ','
                        t=next_token(str, len);
                        q=argos_parse_query(str, len, ctx);
                        ql.push_back(q);
                    }
                    if (ql.size()>=1) {
                        // Allow not only AND(term,...) but also AND(term)
                        return query::detail::ArgosMatch(o, ql);
                    }
                }
                throw argos_syntax_error("Syntax error in match");
            }
            
            query::detail::ArgosMatch argos_parse_has_query(const char *&str, size_t &len, common::ExecutionContext &ctx)
            {
                using namespace common;
                token t=next_token(str, len);
                if (t.type!=TT_ID) {
                    throw argos_syntax_error("Syntax error in match");
                }
                if (!(t.sz==3 && t.p[0]=='H' && t.p[1]=='A' && t.p[2]=='S')) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_ID) {
                    throw argos_syntax_error("Syntax error in match");
                }
                int fid=ctx.get_field_config()->get_field_id(t.p, t.sz);
                common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(fid);
                if (!fd) {
                    throw argos_bad_field(std::string(t.p, t.sz).c_str());
                }
                t=next_token(str, len);
                if (t.type!=TT_COMMA) {
                    throw argos_syntax_error("Syntax error in match");
                }
                common::term_t term;
                t=next_token(str, len);
                if (t.type==TT_STR) {
                    term=common::term_t(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=common::term_t(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=common::term_t(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                int term_type=t.type;
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                if (term_type==TT_STR) {
                    common::simple_term_list_t tl;
                    analyzer::analyzer_ptr_t a=analyzer::get_analyzer(fd->get_analyzer_name());
                    if (!a) {
                        a=analyzer::get_analyzer("token");
                    }
                    if (/*a->is_complex()*/0) {
                        std::vector<query::detail::ArgosMatch> or_q;
                        a->analyse_query(fd->get_prefix(), term, tl);
                        if (tl.size()==1) {
                            or_q.push_back(query::detail::ArgosMatch(tl[0]));
                        } else {
                            std::vector<query::detail::ArgosMatch> ql1;
                            for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                                ql1.push_back(query::detail::ArgosMatch(*i));
                            }
                            or_q.push_back(query::detail::ArgosMatch(query::OP_AND, ql1));
                        }
                        
                        tl.clear();
                        a->analyse_query_simple(fd->get_prefix(), term, tl);
                        if (tl.size()==1) {
                            or_q.push_back(query::detail::ArgosMatch(tl[0]));
                        } else {
                            std::vector<query::detail::ArgosMatch> ql2;
                            for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                                ql2.push_back(query::detail::ArgosMatch(*i));
                            }
                            or_q.push_back(query::detail::ArgosMatch(query::OP_AND, ql2));
                        }
                        return query::detail::ArgosMatch(query::OP_OR, or_q);
                    } else {
                        a->analyse_query(fd->get_prefix(), term, tl);
                        if (tl.size()==1) {
                            return query::detail::ArgosMatch(tl[0]);
                        }
                        std::vector<query::detail::ArgosMatch> ql;
                        for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                            ql.push_back(query::detail::ArgosMatch(*i));
                        }
                        return query::detail::ArgosMatch(query::OP_AND, ql);
                    }
                }
                term=fd->get_prefix()+term;
                return query::detail::ArgosMatch(term);
            }
            
            query::detail::ArgosMatch argos_parse_info_query(const char *&str, size_t &len, common::ExecutionContext &ctx)
            {
                using namespace common;
                token t=next_token(str, len);
                if (t.type!=TT_ID) {
                    throw argos_syntax_error("Syntax error in match");
                }
                if (!(t.sz==4 && t.p[0]=='I' && t.p[1]=='N' && t.p[2]=='F' && t.p[3]=='O')) {
                    throw argos_syntax_error("Syntax error in match");
                }
                t=next_token(str, len);
                if (t.type!=TT_LPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                std::vector<common::FieldDefinition *> fds;
                while (len>0) {
                    t=next_token(str, len);
                    if (t.type!=TT_ID) {
                        break;
                    }
                    int fid=ctx.get_field_config()->get_field_id(t.p, t.sz);
                    common::FieldDefinition *fd=ctx.get_field_config()->get_field_def(fid);
                    if (!fd) {
                        throw argos_bad_field(std::string(t.p, t.sz).c_str());
                    }
                    fds.push_back(fd);
                    t=next_token(str, len);
                    if (t.type!=TT_COMMA) {
                        throw argos_syntax_error("Syntax error in match");
                    }
                }
                //if (fds.empty()) {
                //    throw argos_syntax_error("Syntax error in match");
                //}

                common::term_t term;
                if (t.type==TT_STR) {
                    term=common::term_t(t.p+1, t.sz-2);
                } else if (t.type==TT_INT) {
                    term=common::term_t(t.p, t.sz);
                } else if (t.type==TT_FLOAT) {
                    term=common::term_t(t.p, t.sz);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                int term_type=t.type;
                t=next_token(str, len);
                if (t.type!=TT_RPAREN) {
                    throw argos_syntax_error("Syntax error in match");
                }
                std::vector<query::detail::ArgosMatch> ql;
                if (term_type==TT_STR) {
                    common::simple_term_list_t tl;
                    if (fds.empty()) {
                        analyzer::analyzer_ptr_t a=analyzer::get_default_analyzer();
                        if (!a) {
                            a=analyzer::get_analyzer("token");
                        }
                        a->analyse_query("=", term, tl);
                        for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                            ql.push_back(query::detail::ArgosMatch(*i));
                        }
                    } else {
                        for (int i=0; i<fds.size(); i++) {
                            common::FieldDefinition *fd=fds[i];
                            analyzer::analyzer_ptr_t a=analyzer::get_analyzer(fd->get_analyzer_name());
                            if (!a) {
                                a=analyzer::get_analyzer("token");
                            }
                            a->analyse_query(fd->get_prefix(), term, tl);
                            for (common::simple_term_list_t::const_iterator i=tl.begin(); i!=tl.end(); ++i) {
                                ql.push_back(query::detail::ArgosMatch(*i));
                            }
                        }
                    }
                } else {
                    if (fds.empty()) {
                        term="="+term;
                        ql.push_back(query::detail::ArgosMatch(term)) ;
                    } else {
                        for (int i=0; i<fds.size(); i++) {
                            common::FieldDefinition *fd=fds[i];
                            term=fd->get_prefix()+term;
                            ql.push_back(query::detail::ArgosMatch(term)) ;
                        }
                    }
                }
                // TODO: Do we need to check this?
//                if (ql.empty()) {
//                    return query::detail::ArgosMatch();
//                }
                return query::detail::ArgosMatch(query::OP_IGNORED, ql);
            }

            query::detail::ArgosMatch argos_parse_query(const char *&str, size_t &len, common::ExecutionContext &ctx)
            {
                using namespace common;
                token t=peek_next_token(str, len);
                query::detail::ArgosMatch q;
                if (t.type==TT_ID) {
                    if (t.sz==3 && t.p[0]=='A' && t.p[1]=='N' && t.p[2]=='D') {
                        q=argos_parse_and_or_query(str, len, ctx);
                    } else if (t.sz==2 && t.p[0]=='O' && t.p[1]=='R') {
                        q=argos_parse_and_or_query(str, len, ctx);
                    } else if (t.sz==3 && t.p[0]=='H' && t.p[1]=='A' && t.p[2]=='S') {
                        q=argos_parse_has_query(str, len, ctx);
#if 1
                    } else if (t.sz==4 && t.p[0]=='I' && t.p[1]=='N' && t.p[2]=='F' && t.p[3]=='O') {
                        q=argos_parse_info_query(str, len, ctx);
#endif
                    } else {
                        throw argos_syntax_error("Syntax error in match");
                    }
                } else if(t.type==TT_STR) {
                    q=argos_parse_term_query(str, len, ctx);
                } else if(t.type==TT_INT) {
                    arstring term(t.p, t.sz);
                    term="="+term;
                    q=query::detail::ArgosMatch(term);
                } else if(t.type==TT_FLOAT) {
                    arstring term(t.p, t.sz);
                    term="="+term;
                    q=query::detail::ArgosMatch(term);
                } else {
                    throw argos_syntax_error("Syntax error in match");
                }
                return q;
            }
            
            class ArgosMatchParser_impl : public MatchParser_impl {
            public:
                query::match_ptr_t parse(const char *&str, size_t &len, common::ExecutionContext &context)
                {
                    query::detail::ArgosMatch *ret=new query::detail::ArgosMatch(argos_parse_query(str, len, context));
                    return query::match_ptr_t(ret);
                }
            };
            
            MatchParser_impl *create_match_parser_impl()
            {
                return new ArgosMatchParser_impl;
            }
        }
    }
}   // End of namespace argos