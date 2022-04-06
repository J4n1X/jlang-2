#include "ExpressionParser.hpp"
#include "JlangExceptions.hpp"
#include <algorithm>
#include <iostream>
#include <ranges>

using namespace jlang;
using namespace jlang::statements;
using namespace jlang::parser;

#define UNIQUE(obj) std::unique_ptr<obj>

#define PARSE_SPECIFIC(handler, varname)                                       \
    try {                                                                      \
        varname = handler();                                                   \
    } catch (ParserException & e) {                                            \
        std::cerr << "An error occurred while parsing the program" << '\n';    \
        std::cerr << "Handler: " << #handler << '\n';                          \
        std::cerr << "Error: " << e.what() << '\n';                            \
        std::cerr << "Location: " << e.where() << '\n';                        \
    }

/// Parse statements until the end of the block is reached
BlockStmt *
ExpressionParser::parse_block(std::function<bool(Token)> start_check,
                              std::function<bool(Token)> end_check,
                              std::function<bool(Token)> delim_check) {
    if (at_eof)
        throw ParserException("Unexpected end of file", cur_tok);
    if (!start_check(cur_tok))
        throw ParserException("Expected start of block", cur_tok);
    next_token();
    auto prev_tok = cur_tok;
    std::vector<Statement *> statements;
    while (true) {
        statements::Statement *statement = parse_any();
        statements.push_back(statement);

        if (end_check(cur_tok))
            break;

        if (delim_check(cur_tok)) {
            next_token();
        } else {
            throw ParserException("Expected delimiter", cur_tok);
        }
    }

    return new BlockStmt(prev_tok.location, ExprType::NONE, this->state,
                         statements);
}

BlockStmt *ExpressionParser::parse_function_body() {
    auto block = parse_block(
        [](Token tok) {
            if (tok.type == TokenType::KEYWORD)
                return tok.value.as_keyword() == Keyword::IS;
            return false;
        },
        [](Token tok) {
            if (tok.type == TokenType::KEYWORD)
                return tok.value.as_keyword() == Keyword::DONE;
            return false;
        },
        [](Token tok) { return true; });

    ExprType ret_type = ExprType::NONE;
    auto statements = block->get_statements();

    for (auto &stmt : statements) {
        if (stmt->get_kind() == StatementType::INTRINSIC) {
            if (((IntrinsicStmt *)stmt)->get_intrinsic_kind() ==
                Intrinsic::RETURN) {
                std::cout << "Found return statement" << '\n';
                ret_type = stmt->get_type();
                break;
            }
        }
    }

    block->set_type(ret_type);
    this->next_token();
    return block;
}

BlockStmt *ExpressionParser::parse_call_args() {
    return parse_block(
        [](Token tok) { return tok.type == TokenType::PAREN_BLOCK_START; },
        [](Token tok) { return tok.type == TokenType::PAREN_BLOCK_END; },
        [](Token tok) { return tok.type == TokenType::ARG_DELIMITER; });
}

/// Parse variable declarations for function parameters
std::vector<Variable> ExpressionParser::parse_proto_params() {
    std::vector<Variable> params;
    if (cur_tok.type != TokenType::PAREN_BLOCK_START) {
        throw ParserException("Expected start of parameter list, but got " +
                                  cur_tok.text,
                              cur_tok);
    } else {
        next_token();
        if (cur_tok.type != TokenType::PAREN_BLOCK_END) {
            while (true) {
                if (cur_tok.type != TokenType::IDENTIFIER) {
                    throw ParserException("Expected identifier", cur_tok);
                }
                params.push_back(parse_variable());
                if (cur_tok.type == TokenType::PAREN_BLOCK_END)
                    break;
                if (cur_tok.type != TokenType::ARG_DELIMITER) {
                    throw ParserException("Expected colon", cur_tok);
                }
                next_token();
            }
        }
    }
    next_token();
    return params;
}

std::vector<IdentLookup>
ExpressionParser::get_ident_list(std::string ident_name) {
    std::vector<IdentLookup> idents;
    if (this->state->scope_vars.find(ident_name) !=
        this->state->scope_vars.end()) {
        auto var = this->state->scope_vars[ident_name];
        idents.push_back(IdentLookup(var, IdentType::VARIABLE));
    }
    if (this->state->global_vars.find(ident_name) !=
        this->state->global_vars.end()) {
        auto var = this->state->global_vars[ident_name];
        idents.push_back(IdentLookup(var, IdentType::GLOBAL_VARIABLE));
    }
    if (this->state->constants.find(ident_name) !=
        this->state->constants.end()) {
        auto const_ = this->state->constants[ident_name];
        idents.push_back(IdentLookup(const_));
    }
    if (this->state->prototypes.find(ident_name) !=
        this->state->prototypes.end()) {
        auto proto = this->state->prototypes[ident_name];
        idents.push_back(IdentLookup(proto));
    }
    return idents;
}

/// Create a new identifier reference for use in the AST
/// Yields a list of identifiers that match
/// TODOO: Centralize the documentation of priority
/// Priorities: scope_vars, global_vars, constants, functions
std::vector<IdentStmt *>
ExpressionParser::get_ident_ref(std::string ident_name) {
    auto idents = get_ident_list(ident_name);
    std::vector<IdentStmt *> ret_idents;
    for (auto &ident : idents) {
        switch (ident.get_kind()) {
        case IdentType::VARIABLE:
        case IdentType::GLOBAL_VARIABLE: {
            auto var = ident.get_variable();
            ret_idents.push_back(new IdentStmt(cur_tok.location, var.get_type(),
                                               state, ident.get_kind(), var));
        }
        case IdentType::CONSTANT: {
            auto const_ = ident.get_constant();
            ret_idents.push_back(new IdentStmt(cur_tok.location,
                                               const_.get_type(), state,
                                               ident.get_kind(), const_));
        }
        case IdentType::FUNCTION: {
            auto proto = ident.get_function();
            ret_idents.push_back(new IdentStmt(cur_tok.location,
                                               proto.get_return_type(), state,
                                               ident.get_kind(), proto));
        }
        default:
            throw ParserException("Unhandled identifier type", cur_tok);
        }
    }
    next_token();
    return ret_idents;
}

std::vector<statements::Statement *> ExpressionParser::parse_program() {
    std::vector<statements::Statement *> statements;
    while (!this->at_eof) {
        statements.push_back(parse_any());
    }
    std::cout << this->state->dump() << std::endl;
    return statements;
}

statements::Statement *ExpressionParser::parse_primary() {
    std::cout << this->cur_tok.display() << std::endl;
    statements::Statement *ret_expr;
    if (this->at_eof) {
        throw ParserException("Unexpected end of file", this->cur_tok);
    }
    switch (this->cur_tok.type) {
    case TokenType::KEYWORD:
        ret_expr = parse_keyword();
        break;
    case TokenType::IDENTIFIER:
        ret_expr = parse_ident();
        break;
    case TokenType::INT_LITERAL:
        ret_expr = parse_int_literal_expr();
        break;
    case TokenType::STRING_LITERAL:
        ret_expr = parse_string_literal_expr();
        break;
    case TokenType::INTRINSIC:
        ret_expr = parse_intrinsic();
        break;
    case TokenType::TYPE:
        ret_expr = parse_type_cast();
        break;
    default:
        auto err_string = "Unexpected token" + this->cur_tok.display();
        throw ParserException(err_string.c_str(), this->cur_tok);
    }
    return ret_expr;
}

statements::Statement *ExpressionParser::parse_any() {
    if (this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok);

    auto stmt = parse_primary();

    // if the type is invalid, an error has occurred
    if (stmt->get_type() == ExprType::INVALID)
        throw ParserException("Invalid statement", this->cur_tok);

    // else start parsing a binary expression
    // cast the statement into an expression
    /// TODO: Check if this causes a memory leak
    if (this->peek_token().type == TokenType::OPERATOR)
        return parse_binary_expr(0, stmt);
    else
        return stmt;
}

statements::Statement *ExpressionParser::parse_statement() {
    if (this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok);
    auto stmt = parse_primary();
    // if the type is none then it's a valid statement
    if (stmt->get_type() != ExprType::NONE)
        throw ParserException("Tried to parse statement got but Exception",
                              this->cur_tok);
    else
        return stmt;
}

statements::Statement *ExpressionParser::parse_expression() {
    // a simple check for a valid type should be enough
    if (this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok);
    auto stmt = parse_any();

    if (stmt->get_type() == ExprType::INVALID &&
        stmt->get_type() != ExprType::NONE) {
        auto err_string = "Unexpected token " + this->cur_tok.display();
        throw ParserException("Invalid expression", this->cur_tok);
    }
    return stmt;
}

/// Parse an expression keyword
statements::Statement *ExpressionParser::parse_keyword() {
    auto keyword = this->cur_tok.value.as_keyword();
    switch (keyword) {
    /// TODO: make all definitions return a identref instead of parsing the
    /// next statement (Will require automatic discard)
    case Keyword::CONSTANT:
        parse_constant_def();
        return parse_any();
    case Keyword::DEFINE:
        return parse_variable_def();
    case Keyword::FUNCTION:
        return parse_function_def();
    case Keyword::IF:
    case Keyword::WHILE:
        return parse_control_stmt(keyword);
    case Keyword::IMPORT:
        parse_import();
        return parse_any();
    default:
        throw ParserException("Invalid keyword", this->cur_tok);
    }
}

/// Parse an integer literal
statements::Statement *ExpressionParser::parse_int_literal_expr() {
    // no need to check anything, just return it
    auto literal =
        new LiteralStmt(cur_tok.location, ExprType::INTEGER, this->state,
                        this->cur_tok.value.as_integer());
    this->next_token();
    return literal;
}
/// Parse a string literal
statements::Statement *ExpressionParser::parse_string_literal_expr() {
    if (this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok);
    if (this->cur_tok.type != TokenType::STRING_LITERAL)
        throw ParserException("Expected string literal", this->cur_tok);
    auto strval = this->cur_tok.value.as_string();
    auto strid = this->state->anon_global_vars.size();
    auto strref = "_anon_str_" + std::to_string(strid);
    this->state->anon_global_vars.push_back(
        Constant(strref, ExprType::POINTER, strval.size(), strval));
    auto retval = new LiteralStmt(this->cur_tok.location, ExprType::POINTER,
                                  this->state, strref);
    this->next_token();
    return retval;
}

/// Parse an intrinsic function call
statements::Statement *ExpressionParser::parse_intrinsic() {
    std::cout << "Parsing intrinsic" << std::endl;
    auto intrinsic = this->cur_tok.value.as_intrinsic();
    ExprType ret_type = ExprType::NONE;
    auto prev_tok = this->cur_tok;
    this->next_token();
    // collect args
    switch (intrinsic) {
    case Intrinsic::SYSCALL0:
    case Intrinsic::SYSCALL1:
    case Intrinsic::SYSCALL2:
    case Intrinsic::SYSCALL3:
    case Intrinsic::SYSCALL4:
    case Intrinsic::SYSCALL5: {
        std::cout << "TODO: ARG COUNT CHECK FOR SYSCALL" << std::endl;
        auto args = parse_call_args();
        this->next_token();
        if (args->get_size() < 1)
            throw ParserException("Expected at least one argument", prev_tok);
        if (args->get_size() > 6)
            throw ParserException("Expected at most 6 arguments", prev_tok);
        return new IntrinsicStmt(prev_tok.location, ExprType::INTEGER,
                                 this->state, intrinsic, args);
    }

    case Intrinsic::PRINT:
    case Intrinsic::ADDRESS_OF:
    case Intrinsic::LOAD8:
    case Intrinsic::LOAD16:
    case Intrinsic::LOAD32:
    case Intrinsic::LOAD64:
    case Intrinsic::ALLOCATE: {
        auto args = parse_call_args();
        this->next_token();
        if (args->get_size() != 1)
            throw ParserException("Expected 1 argument for print", prev_tok);
        break;
    }
    case Intrinsic::STORE8:
    case Intrinsic::STORE16:
    case Intrinsic::STORE32:
    case Intrinsic::STORE64: {
        auto args = parse_call_args();
        this->next_token();
        if (args->get_size() != 2)
            throw ParserException("Expected 2 arguments for store", prev_tok);
        return new IntrinsicStmt(prev_tok.location, ExprType::NONE, this->state,
                                 intrinsic, args);
    }
    case Intrinsic::RETURN: {
        if (this->cur_tok.type == TokenType::TYPE) {
            if (this->cur_tok.value.as_expr_type() == ExprType::NONE)
                return new IntrinsicStmt(prev_tok.location, ExprType::NONE,
                                         this->state, intrinsic, nullptr);
        }
        auto expr = parse_expression();
        return new IntrinsicStmt(prev_tok.location, expr->get_type(),
                                 this->state, intrinsic, expr);
    }
    case Intrinsic::DROP: {
        auto expr = parse_any();
        return new IntrinsicStmt(prev_tok.location, ExprType::NONE, this->state,
                                 intrinsic, expr);
    }
    case Intrinsic::INVALID:
        break;
    }
    throw ParserException("Invalid intrinsic", this->cur_tok);
}
/// Parse a type cast
statements::Statement *ExpressionParser::parse_type_cast() {
    if (this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok);
    if (this->cur_tok.type != TokenType::TYPE)
        throw ParserException("Expected type for type cast", this->cur_tok);
    auto type = this->cur_tok.value.as_expr_type();
    this->next_token();
    auto paren_content = parse_call_args()->get_statements();
    if (paren_content.size() != 1)
        throw ParserException("Expected one argument for type cast",
                              this->cur_tok);
    auto ret_item = new Statement(*paren_content[0]);
    ret_item->set_type(type);
    return ret_item;
}

IdentStmt *ExpressionParser::parse_ident() {
    auto prev_tok = cur_tok;
    auto idents = get_ident_list(this->cur_tok.value.as_string());
    if (idents.size() < 1) {
        throw ParserException("Invalid identifier", this->cur_tok);
    }
    auto paren_check_tok = this->next_token();
    if (paren_check_tok.type == TokenType::PAREN_BLOCK_START) {
        std::cout << "Parsing function call" << std::endl;
        // filter out all but the first one of function type
        idents.erase(std::remove_if(idents.begin(), idents.end(),
                                    [](const auto &ident) {
                                        return ident.get_kind() !=
                                               IdentType::FUNCTION;
                                    }),
                     idents.end());
        if (idents.size() < 1) {
            throw ParserException("Invalid identifier", this->cur_tok);
        }
        auto proto = idents[0].get_function();
        auto args = parse_call_args();
        if (proto.get_args().size() != args->get_size()) {
            throw ParserException("Invalid number of arguments for function " +
                                      proto.get_name(),
                                  this->cur_tok);
        }
        return new IdentStmt(prev_tok.location, proto.get_return_type(),
                             this->state, IdentType::FUNCTION, proto, args);
    }
    auto ident = idents[0];
    switch (ident.get_kind()) {
    case IdentType::VARIABLE:
    case IdentType::GLOBAL_VARIABLE: {
        auto var = ident.get_variable();
        return new IdentStmt(prev_tok.location, var.get_type(), this->state,
                             ident.get_kind(), var);
    }
    case IdentType::CONSTANT: {
        auto const_ = ident.get_constant();
        return new IdentStmt(prev_tok.location, const_.get_type(), this->state,
                             IdentType::CONSTANT, const_);
    }
    default:
        throw ParserException("Unreachable", this->cur_tok);
    }
}

Statement *ExpressionParser::parse_binary_expr(int precedence,
                                               Statement *left) {
    std::cout << "Parsing binary expr" << std::endl;
    if (this->at_eof)
        return left;
    // return recast_base_pointer<Expression, BinaryExpr>(std::move(left));
    while (this->cur_tok.type == TokenType::OPERATOR) {
        int tok_prec = get_precedence();
        if (tok_prec < precedence)
            return left;
        Token op_tok = this->cur_tok;
        this->next_token();
        if (this->at_eof)
            throw ParserException("Unexpected end of file", this->cur_tok);

        /// TODO: Better error handling here
        auto right = parse_expression();
        int next_prec = get_precedence();
        if (tok_prec < next_prec) {
            auto prev_tok = this->cur_tok;
            right = parse_binary_expr(tok_prec + 1, right);
        }
        left = new BinaryStmt(op_tok.location, left->get_type(), this->state,
                              op_tok.value.as_operator(), left, right);
    }
    return left;
}

FunProto &ExpressionParser::parse_fun_proto() {
    if (this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok);
    if (this->cur_tok.type != TokenType::KEYWORD)
        throw ParserException(std::string("Expected keyword, but got ") +
                                  get_enum_name(this->cur_tok.type),
                              this->cur_tok);
    if (this->cur_tok.value.as_keyword() != Keyword::FUNCTION)
        throw ParserException("Expected keyword 'function'", this->cur_tok);
    auto prev_tok = this->cur_tok;
    this->next_token();
    if (this->cur_tok.type != TokenType::IDENTIFIER)
        throw ParserException("Expected identifier for function prototype",
                              this->cur_tok);
    std::string name = this->cur_tok.text;
    std::cout << "Parsing function prototype: " << name << std::endl;

    auto references = get_ident_ref(name);
    for (auto &ref : references) {
        if (get_ident_precedence(ref->get_ident_kind()) ==
            get_ident_precedence(IdentType::FUNCTION))
            throw ParserException("Function name already in use",
                                  this->cur_tok);
    }

    auto params = parse_proto_params();
    /// TODO: Check if this is safe
    if (this->cur_tok.type != TokenType::KEYWORD ||
        this->cur_tok.value.as_keyword() != Keyword::YIELDS)
        throw ParserException(
            "Expected keyword 'yields' for function prototype " + name,
            this->cur_tok);
    this->next_token();
    if (this->cur_tok.type != TokenType::TYPE)
        throw ParserException("Expected type for function prototype" + name,
                              this->cur_tok);
    auto ret_type = this->cur_tok.value.as_expr_type();
    this->next_token();
    auto ret_val = FunProto(name, params, ret_type);
    this->state->prototypes.emplace(name, ret_val);
    return this->state->prototypes.at(name);
}

FunStmt *ExpressionParser::parse_function_def() {
    std::cout << "Parsing function" << std::endl;
    if (this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok);
    auto prev_tok = this->cur_tok;
    auto proto = parse_fun_proto();

    // cultivate scope variables with parameters
    /// TODO: There has got to be a more elegant way to do this
    this->state->scope_vars.clear();
    for (auto &arg : proto.get_args()) {
        this->state->scope_vars.emplace(arg.get_name(), arg);
    }

    auto body = parse_function_body();
    if (proto.get_return_type() != body->get_type()) {
        throw ParserException(
            std::string("Function body does not match return type. Body is ") +
                get_enum_name(body->get_type()) + " but prototype is " +
                get_enum_name(proto.get_return_type()),
            this->cur_tok);
    }
    std::cout << "Function parsed, name is " << proto.get_name() << std::endl;
    return new FunStmt(prev_tok.location, this->state, proto, body);
}

Variable ExpressionParser::parse_variable() {
    auto prev_tok = this->cur_tok;
    auto name = this->cur_tok.text;
    auto references = get_ident_ref(cur_tok.text);
    IdentType sel_type = IdentType::INVALID;
    if (this->state->in_scope) {
        for (auto &ref : references) {
            if (get_ident_precedence(ref->get_ident_kind()) >=
                get_ident_precedence(IdentType::VARIABLE)) {
                throw ParserException(
                    (new std::string("Redefinition of identifier '" +
                                     cur_tok.text + "', first defined at " +
                                     ref->get_loc().display()))
                        ->c_str(),
                    this->cur_tok);
            }
        }
        sel_type = IdentType::VARIABLE;
    } else {
        for (auto &ref : references) {
            if (get_ident_precedence(ref->get_ident_kind()) >=
                get_ident_precedence(IdentType::VARIABLE)) {
                throw ParserException(
                    (new std::string("Redefinition of identifier '" +
                                     cur_tok.text + "', first defined at " +
                                     ref->get_loc().display()))
                        ->c_str(),
                    this->cur_tok);
            }
        }
        sel_type = IdentType::GLOBAL_VARIABLE;
    }
    /// TODO: Remove 'as' keyword
    if (this->cur_tok.text != "as") {
        throw ParserException("Expected 'as' keyword at variable definition",
                              this->cur_tok);
    }
    next_token();
    if (this->cur_tok.type != TokenType::TYPE)
        throw ParserException("Expected type for variable definition",
                              this->cur_tok);
    auto type = this->cur_tok.value.as_expr_type();
    next_token();
    return Variable(name, type, 8);
}

void ExpressionParser::parse_constant_def() {

    throw NotImplementedException(
        "ExpressionParser::parse_constant_def is not implemented yet");
}

IdentStmt *ExpressionParser::parse_variable_def() {
    std::cout << "Parsing variable definition" << std::endl;
    auto prev_tok = this->cur_tok;
    next_token();
    auto result = parse_variable();
    auto name = result.get_name();
    Statement *param = nullptr;
    /// TODO: Remove 'is' keyword
    if (this->cur_tok.text == "is") {
        this->next_token();
        param = parse_expression();
    }
    if (this->state->in_scope) {
        /// TODO: More flexible size
        this->state->global_vars.emplace(name, result);
        this->next_token();
        std::cout << "Variable parsed, name is " << name << std::endl;
        return new IdentStmt(prev_tok.location, ExprType::NONE, this->state,
                             IdentType::VARIABLE,
                             this->state->global_vars.at(name), param);
    } else {
        this->state->scope_vars.emplace(name, result);
        this->next_token();
        std::cout << "Variable parsed, name is " << name << std::endl;
        return new IdentStmt(prev_tok.location, ExprType::NONE, this->state,
                             IdentType::GLOBAL_VARIABLE,
                             this->state->scope_vars.at(name), param);
    }
}

void ExpressionParser::parse_import() {
    throw NotImplementedException(
        "ExpressionParser::parse_import is not implemented yet");
}

statements::ControlStmt *ExpressionParser::parse_control_stmt(Keyword kind) {
    throw NotImplementedException(
        "ExpressionParser::parse_control_stmt is not implemented yet");
}