#ifndef EXPRESSIONPARSER_H_
#define EXPRESSIONPARSER_H_
#include <string>
#include <vector>
//#include <deque>
#include "Statements.hpp"
#include "JlangObjects.hpp"

namespace jlang {
    namespace parser {
        class ExpressionParser {
        private:
            /// TODOOOOOO: Profile whether deques are faster than vectors
            std::vector<Token> tokens;
            std::vector<Constant> constants;
            std::vector<FunProto> prototypes;
            std::vector<Variable> global_vars;
            std::vector<Variable> scope_vars;
            std::vector<Variable> anon_scope_vars;

            size_t index;
            Token cur_tok;
            bool in_scope;
            bool at_eof;

            Token &next_token() {
                if(index < tokens.size()) {
                    return tokens[index++];
                }
                at_eof = true;
                return cur_tok;
            }

            inline int get_precedence() {
                if(cur_tok.type == TokenType::OPERATOR) {
                    return get_operator_precedence(cur_tok.value.as.operator_);
                }
                return -1;
            }
        public:
            ExpressionParser(std::vector<Token> tokens){
                this->tokens = tokens;
                this->index = 0;
                this->cur_tok = this->tokens[this->index];
                this->in_scope = false;
                this->at_eof = false;
            }

        };
    }
}

#endif