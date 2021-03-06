#ifndef JLANGOBJECTS_H_
#define JLANGOBJECTS_H_
#include <string>
#include <vector>
#include <map>
#include <concepts>
#include <memory>

#include "strhash.hpp"
//#include "Statements.hpp"

#ifndef JLDEF
#define JLDEF
#endif

namespace jlang {

    class NotImplementedException : public std::exception {
        public:
            NotImplementedException(std::string message) : message(message) {}
            const char* what() const noexcept override {
                return message.c_str();
            }
        private:
            std::string message;
    };


#pragma region Enums

    #define ENUM_TOKENTYPE(o) \
        o(KEYWORD)                    /* Keywords are basically all used for statements except for the syscall keyword */\
        o(INTRINSIC) \
        o(IDENTIFIER)                 /* identifiers for variables and functions */\
        o(INT_LITERAL)                /* number literals */\
        o(STRING_LITERAL)             /* string literals */\
        o(OPERATOR)                   /* Things that manipulate values (basically operators but named weird) */\
        /*o(EOE) */                        /* End of expression */\
        o(PAREN_BLOCK_START)          /* Start of parenthesis block */\
        o(PAREN_BLOCK_END)            /* End of parenthesis block */\
        o(ARG_DELIMITER)              /* Argument delimiter */\
        o(TYPE)                       /* Type name */ 

    #define o(n) n, 
    enum class TokenType { ENUM_TOKENTYPE(o) INVALID};
    #undef o

    #define ENUM_KEYWORD(o) \
        o(IF)          /* if conditional designator */\
        o(WHILE)       /* while conditional designator */\
        o(FUNCTION)    /* function definition designator */\
        o(DEFINE)      /* variable definition designator */\
        o(ALLOCATE)    /* array allocation designator */\
        o(CONSTANT)    /* constant statement designator */\
        o(DO)          /* block open */\
        o(IS)          /* assignment operator and block open */\
        o(AS)          /* type designator */\
        o(TO)          /* unused */\
        o(YIELDS)      /* used to designate return type */\
        o(DONE)        /* block end */\
        o(RETURN)      /* function return */\
        o(IMPORT)      /* file import (preprocessor) */\

    #define o(n) n, 
    enum class Keyword { ENUM_KEYWORD(o) INVALID};
    #undef o

    #define ENUM_OPERATOR(o) \
        o(PLUS) \
        o(MINUS) \
        o(MULTIPLY) \
        o(DIVIDE) \
        o(MODULO) \
        o(GREATER) \
        o(LESS) \
        o(EQUAL) \
        o(NOT_EQUAL) \
        o(GREATER_EQUAL) \
        o(LESS_EQUAL) 

    #define o(n) n, 
    enum class Operator { ENUM_OPERATOR(o) INVALID};
    #undef o


    #define ENUM_INTRINSIC(o) \
        o(SYSCALL0) \
        o(SYSCALL1) \
        o(SYSCALL2) \
        o(SYSCALL3) \
        o(SYSCALL4) \
        o(SYSCALL5) \
        o(PRINT) \
        o(ADDRESS_OF) \
        o(DROP) \
        o(LOAD8) \
        o(LOAD16) \
        o(LOAD32) \
        o(LOAD64) \
        o(STORE8) \
        o(STORE16) \
        o(STORE32) \
        o(STORE64) 

    #define o(n) n, 
    enum class Intrinsic { ENUM_INTRINSIC(o) INVALID};
    #undef o

    #define ENUM_EXPRTYPE(o) \
        o(NONE) \
        o(INTEGER) \
        o(POINTER) 

    #define o(n) n, 
    enum class ExprType { ENUM_EXPRTYPE(o) INVALID};
    #undef o

    #define ENUM_EXPRVALTYPE(o) \
        o(EXPRESSION) \
        o(EXPRESSION_LIST) \
        o(INTEGER) \
        o(STRING) 
    
    #define o(n) n,
    enum class ExprValType { ENUM_EXPRVALTYPE(o) INVALID};
    #undef o

    #define ENUM_IDENTTYPE(o) \
        o(VARIABLE) \
        o(GLOBAL_VARIABLE) \
        o(CONSTANT) \
        o(FUNCTION)
    
    #define o(n) n, 
    enum class IdentType { ENUM_IDENTTYPE(o) INVALID};
    #undef o

#pragma endregion Enums

    typedef struct TokenValue_s{
        union {
            int64_t integer;
            std::string *string;
            Keyword keyword;
            Operator operator_;
            Intrinsic intrinsic;
            ExprType expr_type;
        } as;
        bool needs_free;

        TokenValue_s(){
            needs_free = false;
        }
        TokenValue_s(int64_t i){
            as.integer = i;
            needs_free = false;
        }
        TokenValue_s(std::string s){
            as.string = new std::string(s);
            needs_free = true;
        }
        TokenValue_s(Keyword k){
            as.keyword = k;
            needs_free = false;
        }
        TokenValue_s(Operator o){
            as.operator_ = o;
            needs_free = false;
        }
        TokenValue_s(Intrinsic i){
            as.intrinsic = i;
            needs_free = false;
        }
        TokenValue_s(ExprType e){
            as.expr_type = e;
            needs_free = false;
        }

        ~TokenValue_s(){
            /*if(needs_free){
                delete as.string;
            }*/
        }	
    } TokenValue;
    
#pragma region Implementation
//#define JLANGOBJECTS_IMPLEMENTATION
//#ifdef JLANGOBJECTS_IMPLEMENTATION
    #define o(n) case strhash::hash(#n): return TokenType::n;
    JLDEF static constexpr TokenType get_token_type_by_name(const char *name){
        switch(strhash::hash(name)){
            ENUM_TOKENTYPE(o)
            default: return TokenType::INVALID;
        }
    }
    #undef o

    #define o(n) case TokenType::n: return "TOKENTYPE_"#n;
    JLDEF constexpr const char *get_enum_name(TokenType type){
        switch(type){
            ENUM_TOKENTYPE(o)
            default: return "INVALID";
        }
    }
    #undef o


    #define o(n) case strhash::hash(#n): return Keyword::n;  
    JLDEF constexpr Keyword get_keyword_by_name(const char *name){
        switch(strhash::hash(name)){
            ENUM_KEYWORD(o)
            default:
                return Keyword::INVALID;
        }
    }
    #undef o

    #define o(n) case Keyword::n: return "KEYWORD_"#n;
    JLDEF constexpr const char *get_enum_name(Keyword type){
        switch(type){
            ENUM_KEYWORD(o)
            default: 
                return "INVALID";
        }
    }
    #undef o


    JLDEF constexpr Operator get_operator_by_name(const char *name){
        switch(strhash::hash(name)){
            case strhash::hash("plus"): return Operator::PLUS;
            case strhash::hash("minus"): return Operator::MINUS;
            case strhash::hash("multiply"): return Operator::MULTIPLY;
            case strhash::hash("divide"): return Operator::DIVIDE;
            case strhash::hash("modulo"): return Operator::MODULO;
            case strhash::hash("greater"): return Operator::GREATER;
            case strhash::hash("less"): return Operator::LESS;
            case strhash::hash("equal"): return Operator::EQUAL;
            case strhash::hash("not-equal"): return Operator::NOT_EQUAL;
            case strhash::hash("greater-equal"): return Operator::GREATER_EQUAL;
            case strhash::hash("less-equal"): return Operator::LESS_EQUAL;
            default: return Operator::INVALID;
        }
    }

    JLDEF constexpr const char *get_enum_name(Operator type){
        switch(type){
            case Operator::PLUS: return "OPERATOR_PLUS";
            case Operator::MINUS: return "OPERATOR_MINUS"; 
            case Operator::MULTIPLY: return "OPERATOR_MULTIPLY";
            case Operator::DIVIDE: return "OPERATOR_DIVIDE";
            case Operator::MODULO: return "OPERATOR_MODULO";
            case Operator::GREATER: return "OPERATOR_GREATER";
            case Operator::LESS: return "OPERATOR_LESS";
            case Operator::EQUAL: return "OPERATOR_EQUAL";
            case Operator::NOT_EQUAL: return "OPERATOR_NOT_EQUAL";
            case Operator::GREATER_EQUAL: return "OPERATOR_GREATER_EQUAL";
            case Operator::LESS_EQUAL: return "OPERATOR_LESS_EQUAL";
            default: return "INVALID";
        }
    }

    #define o(n) case strhash::hash(#n): return Intrinsic::n;  
    JLDEF constexpr Intrinsic get_intrinsic_by_name(const char *name){
        switch(strhash::hash(name)){
            ENUM_INTRINSIC(o)
            default:
                return Intrinsic::INVALID;
        }
    }
    #undef o

    #define o(n) case Intrinsic::n: return "INTRINSIC_"#n;
    JLDEF constexpr const char *get_enum_name(Intrinsic type){
        switch(type){
            ENUM_INTRINSIC(o)
            default: 
                return "INVALID";
        }
    }
    #undef o


    #define o(n) case strhash::hash(#n): return ExprType::n;  
    JLDEF constexpr ExprType get_expr_type_by_name(const char *name){
        switch(strhash::hash(name)){
            ENUM_EXPRTYPE(o)
            default:
                return ExprType::INVALID;
        }
    }
    #undef o

    #define o(n) case ExprType::n: return "EXPRTYPE_"#n;
    JLDEF constexpr const char *get_enum_name(ExprType type){
        switch(type){
            ENUM_EXPRTYPE(o)
            default: 
                return "INVALID";
        }
    }
    #undef o

    #define o(n) case strhash::hash(#n): return ExprValType::n;
    JLDEF constexpr ExprValType get_expr_val_type_by_name(const char *name){
        switch(strhash::hash(name)){
            ENUM_EXPRVALTYPE(o)
            default:
                return ExprValType::INVALID;
        }
    }
    #undef o

    #define o(n) case ExprValType::n: return "EXPRVALTYPE_"#n;
    JLDEF constexpr const char *get_enum_name(ExprValType type){
        switch(type){
            ENUM_EXPRVALTYPE(o)
            default: 
                return "INVALID";
        }
    }
    #undef o

    #define o(n) case strhash::hash(#n): return IdentType::n;
    JLDEF constexpr IdentType get_ident_type_by_name(const char *name){
        switch(strhash::hash(name)){
            ENUM_IDENTTYPE(o)
            default:
                return IdentType::INVALID;
        }
    }
    #undef o

    #define o(n) case IdentType::n: return "IDENTTYPE_"#n;
    JLDEF constexpr const char *get_enum_name(IdentType type){
        switch(type){
            ENUM_IDENTTYPE(o)
            default: 
                return "INVALID";
        }
    }
    #undef o


    JLDEF static std::string get_token_type_value_string(TokenType type, TokenValue value) {
            switch(type) {
                case TokenType::INT_LITERAL:
                    return std::to_string(value.as.integer);
                case TokenType::STRING_LITERAL:
                case TokenType::IDENTIFIER:
                case TokenType::PAREN_BLOCK_START:
                case TokenType::PAREN_BLOCK_END:
                case TokenType::ARG_DELIMITER:
                    return *value.as.string;
                case TokenType::TYPE:
                    return std::string(get_enum_name(value.as.expr_type));
                case TokenType::KEYWORD:
                    return std::string(get_enum_name(value.as.keyword));
                case TokenType::OPERATOR:
                    return std::string(get_enum_name(value.as.operator_));
                case TokenType::INTRINSIC:
                    return std::string(get_enum_name(value.as.intrinsic));
                default:
                    return "";
            }
    }
//#endif // JLANGOBJECTS_IMPLEMENTATION
#pragma endregion Implementation


    typedef struct Location_s {
        std::string file;
        std::size_t line;
        std::size_t column;

        std::string display() {
            return file + ":" + std::to_string(line) + ":" + std::to_string(column);
        }

        Location_s(){
            file = "";
            line = 0;
            column = 0;
        }
        Location_s(std::string file, std::size_t line, std::size_t column){
            this->file = file;
            this->line = line;
            this->column = column;
        }
    } Location;

    typedef struct Token_s {
        TokenType type;
        std::string text;
        Location location;
        TokenValue value;

        Token_s(){
            type = TokenType::INVALID;
            text = "";
            location = Location();
            value = TokenValue();
        }

        Token_s(TokenType type, std::string text, Location location, TokenValue value) {
            this->type = type;
            this->text = text;
            this->location = location;
            this->value = value;
        }

        std::string display() {
            return location.display() + ": " + get_enum_name(type) + "; " + get_token_type_value_string(type, value) +  "; \"" + text +'"';
        }
    } Token;
/*
    class ExprValue {
    private:
        ExprValType type;
        ExprValueField_u as;

    public:
        ExprValue() {
            type = ExprValType::INVALID;
            as.integer = 0;
        }
        ExprValue(uint64_t integer) {
            this->type = ExprValType::INTEGER;
            this->as.integer = integer;
        }
        ExprValue(std::string string) {
            this->type = ExprValType::STRING;
            this->as.string = new std::string(string);
        }
        ExprValue(statements::Expression expr) {
            this->type = ExprValType::EXPRESSION;
            this->as.expr = new statements::Expression(expr);
        }
        ExprValue(std::vector<statements::Expression> expr_list) {
            this->type = ExprValType::EXPRESSION_LIST;
            this->as.expr_list = new std::vector<statements::Expression>(expr_list);
        }
        ExprValType get_type() { return type;}

        uint64_t get_integer() {
            if(type == ExprValType::INTEGER) {
                return as.integer;
            }
            return 0;
        }

        std::string get_string() {
            if(type == ExprValType::STRING) {
                return *as.string;
            }
            return "";
        }

        statements::Expression get_expression() {
            if(type == ExprValType::EXPRESSION) {
                return *as.expr;
            }
            return statements::Expression();
        }

        std::vector<statements::Expression> get_expression_list() {
            if(type == ExprValType::EXPRESSION_LIST) {
                return *as.expr_list;
            }
            return std::vector<statements::Expression>();
        }

        std::string display(int depth = 0) {
            switch(type) {
                case ExprValType::INTEGER:
                    return std::to_string(as.integer);
                case ExprValType::STRING:
                    return *as.string;
                case ExprValType::EXPRESSION:
                    return as.expr->display();
                case ExprValType::EXPRESSION_LIST:
                    {
                        std::stringstream ss;
                        ss << "[";
                        for(auto &expr : *as.expr_list) {
                            ss << std::string(depth + 4, ' ') << expr.display(depth + 4) << ",\n";
                        }
                        ss << std::string(depth, ' ') << "]";
                        return ss.str();
                    }
                default:
                    return "";
            }
        }
    };
    */
}
#endif // JLANGOBJECTS_H_