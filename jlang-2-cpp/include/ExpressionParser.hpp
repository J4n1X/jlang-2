#ifndef EXPRESSIONPARSER_H_
#define EXPRESSIONPARSER_H_
#include <string>
#include <vector>
//#include <deque>
#include <map>
#include <functional>
#include "Statements.hpp"
#include "JlangObjects.hpp"

#define UNIQUE(obj) std::unique_ptr<obj>


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

            /// Cast the unique pointer of a base class to a unique pointer of a derived class
            template<typename Base, typename Spec>
            inline std::unique_ptr<Spec> recast_base_pointer(std::unique_ptr<Base> ptr) {
                return std::unique_ptr<Spec>(static_cast<Spec*>(ptr.release()));
            }

            /// Parse statements until the end of the block is reached
            std::vector<UNIQUE(statements::Statement)> parse_block(std::function<bool(Token)> start_check, std::function<bool(Token)> end_check);
            /// Parse variable declarations for function parameters
            std::vector<Variable> parse_proto_params(); 
            /// Parse Expressions passed to function call 
            std::vector<UNIQUE(statements::Expression)> parse_call_args();
            /// Create a new identifier reference for use in the AST
            std::vector<UNIQUE(statements::IdentRefExpr)> get_ident_ref(std::string_view ident_name);


            /// Parse all tokens passed
            std::vector<UNIQUE(statements::Statement)> parse_program();
            /// Parse either Statement or Expression (but do not resolve to AST tree)
            UNIQUE(statements::Statement) parse_primary();      
            /// Parse either Statement or Expression
            UNIQUE(statements::Statement) parse_any();          
            /// Parse only statements, if the next next IR Object isn't a Statement, it will throw an exception
            UNIQUE(statements::Statement) parse_statement();    
            /// Parse only expressions, if the next next IR Object isn't an Expression, it will throw an exception
            std::unique_ptr<statements::Expression> parse_expression(); 
            /// Parse an expression keyword
            UNIQUE(statements::Statement) parse_keyword();
            /// Parse an identifier
            UNIQUE(statements::IdentRefExpr) parse_ident();
            /// Parse an integer literal
            UNIQUE(statements::Statement) parse_int_literal_expr();
            /// Parse a string literal
            UNIQUE(statements::Statement) parse_string_literal_expr();
            /// Parse an intrinsic function call
            UNIQUE(statements::Statement) parse_intrinsic();
            /// Parse a type cast
            UNIQUE(statements::Statement) parse_type_cast();
            /// Parse a binary expression
            UNIQUE(statements::BinaryExpr) parse_binary_expr(int precedence, UNIQUE(statements::Expression) left);

            /// Parse a function prototype
            FunProto parse_fun_proto();

        };
    }
}

#undef UNIQUE
#endif