#ifndef EXPRESSIONPARSER_H_
#define EXPRESSIONPARSER_H_
//#include <deque>
#include "JlangObjects.hpp"
#include "Statements.hpp"
#include <functional>
#include <variant>

#define UNIQUE(obj) std::unique_ptr<obj>

namespace jlang {

// TODO: Maybe use this instead of multiple vectors for the state management
struct IdentLookup {
  private:
    std::variant<Variable, Constant, FunProto> ref;
    IdentType kind;

  public:
    IdentLookup(Variable var, IdentType kind) : ref(var), kind(kind) {}
    IdentLookup(Constant con) : ref(con), kind(IdentType::CONSTANT) {}
    IdentLookup(FunProto proto) : ref(proto), kind(IdentType::FUNCTION) {}
    const IdentType get_kind() const { return kind; }
    const Variable &get_variable() const { return std::get<Variable>(ref); }
    const Constant &get_constant() const { return std::get<Constant>(ref); }
    const FunProto &get_function() const { return std::get<FunProto>(ref); }
};
namespace parser {
class ExpressionParser {
  private:
    /// TODOOOOOO: Profile whether deques are faster than vectors
    std::shared_ptr<CompilerState> state;

    size_t index;
    Token cur_tok;
    bool in_scope;
    bool at_eof;

  public:
    ExpressionParser(std::vector<Token> tokens) {
        this->state = std::make_shared<CompilerState>(CompilerState());
        this->state->tokens = tokens;
        this->index = 0;
        this->cur_tok = this->state->tokens[this->index];
        this->in_scope = false;
        this->at_eof = false;
    }

    inline void prepend_tokens(std::vector<Token> new_tokens) {
        if (new_tokens.size() == 0)
            return;
        this->state->tokens.insert(this->state->tokens.begin(),
                                   new_tokens.begin(), new_tokens.end());
    }
    inline Token &next_token() {
        if (this->index + 1 > this->state->tokens.size()) {
            this->at_eof = true;
            return this->cur_tok;
        }
        this->index++;
        this->cur_tok = this->state->tokens[this->index];
        return this->state->tokens[this->index];
    }
    inline Token &peek_token() {
        if (this->index >= this->state->tokens.size()) {
            this->at_eof = true;
            return this->cur_tok;
        }
        return this->cur_tok;
    }
    inline int get_precedence() {
        if (this->cur_tok.type == TokenType::OPERATOR) {
            return get_operator_precedence(this->cur_tok.value.as_operator());
        }
        return -1;
    }

    /// Cast the unique pointer of a base class to a unique pointer of a derived
    /// class
    template <typename Base, typename Spec>
    inline std::unique_ptr<Spec>
    recast_base_pointer(std::unique_ptr<Base> ptr) {
        return std::unique_ptr<Spec>(static_cast<Spec *>(ptr.release()));
    }

    /// Parse statements until the end of the block is reached
    statements::BlockStmt *parse_block(std::function<bool(Token)> start_check,
                                       std::function<bool(Token)> end_check,
                                       std::function<bool(Token)> delim_check);
    /// Parse a function body
    statements::BlockStmt *parse_function_body();
    /// Parse variable declarations for function parameters
    std::vector<Variable> parse_proto_params();
    /// Parse Expressions passed to function call
    statements::BlockStmt *parse_call_args();
    // std::vector<UNIQUE(statements::Expression)> parse_call_args();
    /// Create a list of identifier lookups for the given identifier name
    std::vector<IdentLookup> get_ident_list(std::string ident_name);
    /// Create a new identifier reference for use in the AST
    std::vector<statements::IdentStmt *> get_ident_ref(std::string ident_name);

    /// Parse all tokens passed
    std::vector<statements::Statement *> parse_program();
    /// Parse either Statement or Expression (but do not resolve to AST tree)
    statements::Statement *parse_primary();
    /// Parse either Statement or Expression
    statements::Statement *parse_any();
    /// Parse only statements, if the next next IR Object isn't a Statement, it
    /// will throw an exception
    statements::Statement *parse_statement();
    /// Parse only expressions, if the next next IR Object isn't an Expression,
    /// it will throw an exception
    statements::Statement *parse_expression();

    //
    // SPECIALIZED PARSER FUNCTIONS
    //

    /// Parse an expression keyword
    statements::Statement *parse_keyword();
    /// Parse an intrinsic function call
    statements::Statement *parse_intrinsic();
    /// Parse an identifier
    statements::IdentStmt *parse_ident();
    /// Parse an integer literal
    statements::Statement *parse_int_literal_expr();
    /// Parse a string literal
    statements::Statement *parse_string_literal_expr();
    /// Parse a type cast
    statements::Statement *parse_type_cast();
    /// Parse a binary expression
    statements::Statement *parse_binary_expr(int precedence,
                                             statements::Statement *left);

    //
    // SPECIAL KEYWORD PARSER FUNCTIONS
    //

    void parse_constant_def();
    void parse_param_def();
    void parse_import();
    Variable parse_variable();
    statements::IdentStmt *parse_variable_def();
    statements::FunStmt *parse_function_def();
    statements::ControlStmt *parse_control_stmt(Keyword kind);

    /// Parse a function prototype
    FunProto &parse_fun_proto();
};
} // namespace parser
} // namespace jlang

#undef UNIQUE
#endif