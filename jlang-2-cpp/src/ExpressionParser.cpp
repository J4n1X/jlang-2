#include "ExpressionParser.hpp"
#include "JlangExceptions.hpp"
#include <iostream>

using namespace jlang;
using namespace jlang::statements;
using namespace jlang::parser;

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
std::vector<statements::Statement> ExpressionParser::parse_block(std::function<bool(Token)> start_check, std::function<bool(Token)> end_check){
    if(at_eof){
        throw ParserException("Unexpected end of file", cur_tok.location);
    }
    if(!start_check(cur_tok)){
        throw ParserException("Expected start of block", cur_tok.location);
    }
    next_token();
    std::vector<statements::Statement> statements;
    while(!at_eof && !end_check(cur_tok)){
        statements::Statement statement;
        PARSE_SPECIFIC(parse_any, statement);
        statements.push_back(statement);
    }
    if(at_eof){
        throw ParserException("Unexpected end of file", cur_tok.location);
    }
    return statements;
}
/// Parse variable declarations for function parameters
/*std::vector<Variable> ExpressionParser::parse_proto_params() {
    if(at_eof)
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
    return params;
}*/
/// Parse Expressions passed to function call 
std::vector<Expression> ExpressionParser::parse_call_args(){
    if(at_eof)
        throw ParserException("Unexpected end of file", cur_tok.location);
    std::vector<Expression> args;
    if(cur_tok.type != TokenType::PAREN_BLOCK_START)
        throw ParserException("Expected '('", cur_tok.location);
    next_token();
    if(cur_tok.type == TokenType::PAREN_BLOCK_END){
        next_token();
        return args;
    }
    while(!at_eof && cur_tok.type != TokenType::PAREN_BLOCK_END){
        Expression arg;
        PARSE_SPECIFIC(parse_expression, arg);
        args.push_back(arg);
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
std::vector<IdentRefExpr> ExpressionParser::get_ident_ref(std::string_view ident_name){
    std::vector<IdentRefExpr> idents;
    if(this->scope_vars.find(ident_name) != this->scope_vars.end()){
        auto var = this->scope_vars[ident_name];
        idents.push_back(IdentRefExpr(cur_tok.location, var.get_type(), std::string{var.get_name()}, IdentType::VARIABLE));
    }
    if(this->global_vars.find(ident_name) != this->global_vars.end()){
        auto var = this->global_vars[ident_name];
        idents.push_back(IdentRefExpr(cur_tok.location, var.get_type(), std::string{var.get_name()}, IdentType::GLOBAL_VARIABLE));
    }
    if(this->constants.find(ident_name) != this->constants.end()){
        auto const_ = this->constants[ident_name];
        idents.push_back(IdentRefExpr(cur_tok.location, const_.get_type(), std::string{const_.get_name()}, IdentType::CONSTANT));
    }
    if(this->prototypes.find(ident_name) != this->prototypes.end()){
        auto proto = this->prototypes[ident_name];
        idents.push_back(IdentRefExpr(cur_tok.location, proto.get_return_type(), std::string{proto.get_name()}, IdentType::FUNCTION));
    }    
    if(idents.size() == 0) {
        throw ParserException("Identifier not found", this->cur_tok.location);
    }   
    return idents;
}


std::vector<Statement> ExpressionParser::parse_program(){
    std::vector<Statement> statements;

    int test;

    return statements;
}

Statement ExpressionParser::parse_primary(){
    throw NotImplementedException("ExpressionParser::parse_primary is not implemented yet");
}

Statement ExpressionParser::parse_any(){
    throw NotImplementedException("ExpressionParser::parse_any is not implemented yet");
}

Statement ExpressionParser::parse_statement(){
    throw NotImplementedException("ExpressionParser::parse_statement is not implemented yet");
}

Expression ExpressionParser::parse_expression(){
    throw NotImplementedException("ExpressionParser::parse_expression is not implemented yet");
}