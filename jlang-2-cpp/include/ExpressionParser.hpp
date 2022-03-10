#ifndef EXPRESSIONPARSER_H_
#define EXPRESSIONPARSER_H_
#include <string>
#include <vector>
//#include <deque>
#include <map>
#include <functional>
#include "Statements.hpp"
#include "JlangObjects.hpp"


namespace jlang {
    /// Unused.
    typedef struct IdentLookup_s {
        IdentType type;
        size_t index;
    } IdentLookup;
    namespace parser {
        class ExpressionParser {
        private:
            /// TODOOOOOO: Profile whether deques are faster than vectors
            std::vector<Token> tokens;
            std::map<std::string_view, Constant> constants;
            std::map<std::string_view, FunProto> prototypes;
            std::map<std::string_view, Variable> global_vars;
            std::map<std::string_view, Variable> scope_vars;
            std::vector<Variable> anon_scope_vars;

            size_t index;
            Token cur_tok;
            bool in_scope;
            bool at_eof;
        public:
            ExpressionParser(std::vector<Token> tokens){
                this->tokens = tokens;
                this->index = 0;
                this->cur_tok = this->tokens[this->index];
                this->in_scope = false;
                this->at_eof = false;
            }

            inline void prepend_tokens(std::vector<Token> new_tokens) {
                if(new_tokens.size() == 0) return;
                this->tokens.insert(this->tokens.begin(), new_tokens.begin(), new_tokens.end());
            }
            inline Token &next_token() {
                if(this->index >= this->tokens.size()){
                    this->at_eof = true;
                    return this->cur_tok;
                }
                return this->tokens[this->index++];
            }
            inline Token &peek_token() {
                if(this->index >= this->tokens.size()){
                    this->at_eof = true;
                    return this->cur_tok;
                }
                return this->tokens[this->index];
            }
            inline int get_precedence() {
                if(this->cur_tok.type == TokenType::OPERATOR){
                    return get_operator_precedence(this->cur_tok.value.as.operator_);
                }
                return -1;
            }

            /// Parse statements until the end of the block is reached
            std::vector<statements::Statement> parse_block(std::function<bool(Token)> start_check, std::function<bool(Token)> end_check);
            /// Parse variable declarations for function parameters
            std::vector<Variable> parse_proto_params(); 
            /// Parse Expressions passed to function call 
            std::vector<statements::Expression> parse_call_args();
            /// Create a new identifier reference for use in the AST
            std::vector<statements::IdentRefExpr> get_ident_ref(std::string_view ident_name);


            /// Parse all tokens passed
            std::vector<statements::Statement> parse_program();
            /// Parse either Statement or Expression (but do not resolve to AST tree)
            statements::Statement parse_primary();      
            /// Parse either Statement or Expression
            statements::Statement parse_any();          
            /// Parse only statements, if the next next IR Object isn't a Statement, it will throw an exception
            statements::Statement parse_statement();    
            /// Parse only expressions, if the next next IR Object isn't an Expression, it will throw an exception
            statements::Expression parse_expression();  
        };
    }
}

#endif