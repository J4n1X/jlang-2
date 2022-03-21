#include "ExpressionParser.hpp"
#include "JlangExceptions.hpp"
#include <iostream>

using namespace jlang;
using namespace jlang::statements;
using namespace jlang::parser;

#define UNIQUE(obj) std::unique_ptr<obj>

#define PARSE_SPECIFIC(handler, varname) \
    try { \
        varname = handler(); \
    } catch(ParserException& e) { \
        std::cerr << "An error occurred while parsing the program" << '\n'; \
        std::cerr << "Handler: " << #handler << '\n'; \
        std::cerr << "Error: " << e.what() << '\n'; \
        std::cerr << "Location: " << e.where() << '\n'; \
    }

/// Parse statements until the end of the block is reached
std::vector<UNIQUE(Statement)> ExpressionParser::parse_block(std::function<bool(Token)> start_check, std::function<bool(Token)> end_check){
    if(at_eof){
        throw ParserException("Unexpected end of file", cur_tok.location);
    }
    if(!start_check(cur_tok)){
        throw ParserException("Expected start of block", cur_tok.location);
    }
    next_token();
    std::vector<UNIQUE(Statement)> statements;
    while(!at_eof && !end_check(cur_tok)){
        UNIQUE(Statement) statement;
        PARSE_SPECIFIC(parse_any, statement);
        statements.push_back(std::move(statement));
    }
    if(at_eof){
        throw ParserException("Unexpected end of file", cur_tok.location);
    }
    return statements;
}
/// Parse variable declarations for function parameters
std::vector<Variable> ExpressionParser::parse_proto_params() {
    throw NotImplementedException("Function parameters are not implemented yet");
/*    if(at_eof)
        throw ParserException("Unexpected end of file", cur_tok.location);
    std::vector<Variable> params;
    if(cur_tok.type != TokenType::PAREN_BLOCK_START)
        throw ParserException("Expected '('", cur_tok.location);
    next_token();
    if(cur_tok.type == TokenType::PAREN_BLOCK_END){
        next_token();
        return params;
    }
    Variable param;
    PARSE_SPECIFIC(parse_variable, param);
    params.push_back(param);
    while(cur_tok.type == TokenType::COMMA){
        next_token();
        PARSE_SPECIFIC(parse_variable, param);
        params.push_back(param);
    }
    if(cur_tok.type != TokenType::PAREN_CLOSE)
        throw ParserException("Expected ')'", cur_tok.location);
    next_token();
    return params;*/
}
/// Parse Expressions passed to function call 
std::vector<UNIQUE(Expression)> ExpressionParser::parse_call_args(){
    if(at_eof)
        throw ParserException("Unexpected end of file", cur_tok.location);
    std::vector<UNIQUE(Expression)> args;
    if(cur_tok.type != TokenType::PAREN_BLOCK_START)
        throw ParserException("Expected '('", cur_tok.location);
    next_token();
    if(cur_tok.type == TokenType::PAREN_BLOCK_END){
        next_token();
        return args;
    }
    while(!at_eof && cur_tok.type != TokenType::PAREN_BLOCK_END){
        UNIQUE(Expression) arg;
        PARSE_SPECIFIC(parse_expression,arg);
        args.push_back(std::move(arg));
        if(cur_tok.type == TokenType::ARG_DELIMITER){
            next_token();
        }
    }
    if(at_eof)
        throw ParserException("Unexpected end of file", cur_tok.location);
    if(cur_tok.type != TokenType::PAREN_BLOCK_END)
        throw ParserException("Expected ')'", cur_tok.location);
    return args;
}
/// Create a new identifier reference for use in the AST
/// Yields a list of identifiers that match
/// TODOO: Centralize the documentation of priority
/// Priorities: scope_vars, global_vars, constants, functions
std::vector<UNIQUE(IdentRefExpr)> ExpressionParser::get_ident_ref(std::string_view ident_name){
    std::vector<UNIQUE(IdentRefExpr)> idents;
    if(this->scope_vars.find(ident_name) != this->scope_vars.end()){
        auto var = this->scope_vars[ident_name];
        idents.push_back(std::make_unique<IdentRefExpr>(cur_tok.location, var.get_type(), std::string{var.get_name()}, IdentType::VARIABLE));
    }
    if(this->global_vars.find(ident_name) != this->global_vars.end()){
        auto var = this->global_vars[ident_name];
        idents.push_back(std::make_unique<IdentRefExpr>(cur_tok.location, var.get_type(), std::string{var.get_name()}, IdentType::GLOBAL_VARIABLE));
    }
    if(this->constants.find(ident_name) != this->constants.end()){
        auto const_ = this->constants[ident_name];
        idents.push_back(std::make_unique<IdentRefExpr>(cur_tok.location, const_.get_type(), std::string{const_.get_name()}, IdentType::CONSTANT));
    }
    if(this->prototypes.find(ident_name) != this->prototypes.end()){
        auto proto = this->prototypes[ident_name];
        idents.push_back(std::make_unique<IdentRefExpr>(cur_tok.location, proto.get_return_type(), std::string{proto.get_name()}, IdentType::FUNCTION));
    }    
    if(idents.size() == 0) {
        throw ParserException("Identifier not found", this->cur_tok.location);
    }   
    return idents;
}


std::vector<UNIQUE(Statement)> ExpressionParser::parse_program(){
    std::vector<UNIQUE(Statement)> statements;

    int test;

    return statements;
}


UNIQUE(Statement) ExpressionParser::parse_primary(){
    UNIQUE(Statement) ret_expr;
    if(this->at_eof){
        throw ParserException("Unexpected end of file", this->cur_tok.location);
    }
    switch(this->cur_tok.type){
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
            throw ParserException(err_string.c_str(), this->cur_tok.location);
    }
    return ret_expr;
}

UNIQUE(Statement) ExpressionParser::parse_any(){
    if(this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok.location);

    auto stmt = parse_primary();
    
    // if the type is invalid, an error has occurred
    if(stmt->get_type() == ExprType::INVALID)
        throw ParserException("Invalid statement", this->cur_tok.location);

    // else start parsing a binary expression
    // cast the statement into an expression
    /// TODO: Check if this causes a memory leak
    auto expr = recast_base_pointer<Statement, Expression>(std::move(stmt));
    return parse_binary_expr(0, std::make_unique<Expression>(*expr));
}

UNIQUE(Statement) ExpressionParser::parse_statement(){
    if(this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok.location);
    auto stmt = parse_primary();
    // if the type is none then it's a valid statement
    if(stmt->get_type() != ExprType::NONE)
        throw ParserException("Tried to parse statement got but Exception", this->cur_tok.location); 
    else
        return stmt;
}

UNIQUE(Expression) ExpressionParser::parse_expression(){
    // a simple check for a valid type should be enough
    if(this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok.location);
    auto stmt = parse_any();
    if(stmt->get_type() == ExprType::INVALID && stmt->get_type() != ExprType::NONE){
        auto err_string = "Unexpected token" + this->cur_tok.display();
        throw ParserException("Invalid expression", this->cur_tok.location);
    }
    return recast_base_pointer<Statement, Expression>(std::move(stmt));
}

/// Parse an expression keyword
UNIQUE(Statement) ExpressionParser::parse_keyword(){
    throw NotImplementedException("Keyword parsing not implemented");
}

/// Parse an integer literal
UNIQUE(statements::Statement) ExpressionParser::parse_int_literal_expr(){
    throw NotImplementedException("Integer literal parsing not implemented");
}
/// Parse a string literal
UNIQUE(statements::Statement) ExpressionParser::parse_string_literal_expr(){
    throw NotImplementedException("String literal parsing not implemented");
}
/// Parse an intrinsic function call
UNIQUE(statements::Statement) ExpressionParser::parse_intrinsic(){
    throw NotImplementedException("Intrinsic parsing not implemented");
}
/// Parse a type cast
UNIQUE(statements::Statement) ExpressionParser::parse_type_cast(){
    throw NotImplementedException("Type cast parsing not implemented");
}


UNIQUE(IdentRefExpr) ExpressionParser::parse_ident(){
    throw NotImplementedException("ExpressionParser::parse_ident is not implemented yet");
}

UNIQUE(statements::BinaryExpr) ExpressionParser::parse_binary_expr(int precedence, UNIQUE(statements::Expression) left){
    if(this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok.location);
        //return recast_base_pointer<Expression, BinaryExpr>(std::move(left));
    while(this->cur_tok.type == TokenType::OPERATOR){
        int tok_prec = get_precedence();
        if(tok_prec < precedence)
            return recast_base_pointer<Expression, BinaryExpr>(std::move(left));
        Token op_tok = this->cur_tok;
        this->next_token();
        if(this->at_eof)
            throw ParserException("Unexpected end of file", this->cur_tok.location);
        
        /// TODO: Better error handling here
        auto right = parse_expression();
        int next_prec = get_precedence();
        if(tok_prec < next_prec){
            auto prev_tok = this->cur_tok;
            right = parse_binary_expr(tok_prec + 1, std::move(right));
        }
        left = std::make_unique<BinaryExpr>(op_tok.location, left->get_type(), op_tok.value.as.operator_, std::move(left), std::move(right));
    }
    return recast_base_pointer<Expression, BinaryExpr>(std::move(left));
}

FunProto ExpressionParser::parse_fun_proto(){
    if(this->at_eof)
        throw ParserException("Unexpected end of file", this->cur_tok.location);
    if(this->cur_tok.type != TokenType::KEYWORD)
        throw ParserException("Expected keyword", this->cur_tok.location);
    if(this->cur_tok.value.as.keyword != Keyword::FUNCTION)
        throw ParserException("Expected keyword 'function'", this->cur_tok.location);
    auto prev_tok = this->cur_tok;
    this->next_token();
    if(this->cur_tok.type != TokenType::IDENTIFIER)
        throw ParserException("Expected identifier for function prototype", this->cur_tok.location);
    auto ident = parse_ident();
    if(ident->get_ident_kind() != IdentType::INVALID){
        auto err_string = "Redefinition of identifier of type " 
                          + std::string(get_enum_name(ident->get_ident_kind()));
                          + " first defined at " + ident->get_location().display();
        throw ParserException(err_string.c_str(), this->cur_tok.location);
    }
    std::string name = this->cur_tok.text;
    this->next_token();
    auto params = parse_proto_params();
    this->next_token();
    /// TODO: Check if this is safe
    if(this->cur_tok.type != TokenType::KEYWORD || this->cur_tok.value.as.keyword != Keyword::YIELDS)
        throw ParserException("Expected keyword 'yields' for function prototype", this->cur_tok.location);
    this->next_token();
    if(this->cur_tok.type != TokenType::TYPE)
        throw ParserException("Expected type for function prototype", this->cur_tok.location);
    auto ret_type = this->cur_tok.value.as.expr_type;
    this->next_token();
    return FunProto(name, params, ret_type);   
}