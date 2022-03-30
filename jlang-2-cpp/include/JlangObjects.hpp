#ifndef JLANGOBJECTS_H_
#define JLANGOBJECTS_H_
#include <string>
#include <vector>
#include <map>
#include <concepts>
#include <variant>
#include <sstream>

#include "strhash.hpp"
//#include "Statements.hpp"

#ifndef JLDEF
#define JLDEF
#endif

namespace jlang {

#pragma region Enums

    /// TODO: Strip away unnecessary types and unify them into a generic "word" type
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
        o(CONSTANT)    /* constant statement designator */\
        o(DO)          /* block open */\
        o(IS)          /* assignment operator and block open */\
        o(AS)          /* type designator */\
        o(TO)          /* unused */\
        o(YIELDS)      /* used to designate return type */\
        o(DONE)        /* block end */\
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
        o(RETURN)      /* function return */\
        o(SYSCALL0) \
        o(SYSCALL1) \
        o(SYSCALL2) \
        o(SYSCALL3) \
        o(SYSCALL4) \
        o(SYSCALL5) \
        o(PRINT) \
        o(ADDRESS_OF) \
        o(ALLOCATE)    /* array allocation designator */\
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
        o(FUNCTION)\
    
    #define o(n) n, 
    enum class IdentType { ENUM_IDENTTYPE(o) INVALID};
    #undef o

    #define ENUM_STATEMENT(o) \
    o(STATEMENT)\
    o(EXPRESSION)\
    o(BLOCK)           /* Parameter block */\
    o(LITERAL)          /* Any compile time assignable or allocatable value */\
    o(IDENT)            /* Read, define or set Constant, Variable or Function*/\
    o(FUNCALL)          /* Function call */\
    o(FUNCTION)        /* Function definition */\
    o(INTRINSIC)        /* Compiler integrated functions */\
    o(CONTROL)          /* Control flow statements */\
    o(BINARY)
    
    #define o(n) n,
    enum class StatementType { ENUM_STATEMENT(o) };
    #undef o
    
    #define ENUM_CONTROL(o)\
        o(IF)\
        o(WHILE)
    #define o(n) n,
    enum class ControlType { ENUM_CONTROL(o) };
    #undef o
        

#pragma endregion Enums

    typedef struct TokenValue_s{
        private:
        std::variant<int64_t, std::string, Keyword, Operator, Intrinsic, ExprType> value;

        public:
        TokenValue_s(){}
        TokenValue_s(int64_t i) : value(i){}
        TokenValue_s(std::string s) : value(s){}
        TokenValue_s(Keyword k) : value(k){}
        TokenValue_s(Operator o) : value(o){}
        TokenValue_s(Intrinsic i) : value(i){}

        TokenValue_s(ExprType e) : value(e){}

        ~TokenValue_s() = default;

        const int64_t &as_integer() const {
            return std::get<int64_t>(value);
        }
        const std::string &as_string() const {
            return std::get<std::string>(value);
        }
        const Keyword &as_keyword() const {
            return std::get<Keyword>(value);
        }
        const Operator &as_operator() const {
            return std::get<Operator>(value);
        }
        const Intrinsic &as_intrinsic() const {
            return std::get<Intrinsic>(value);
        }
        const ExprType &as_expr_type() const {
            return std::get<ExprType>(value);
        }


    } TokenValue;
    
#pragma region Implementation

    // These macros use the macros defined above to 
    // create constexpr switch cases for getting names and enum types.

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

    JLDEF constexpr int get_operator_precedence(Operator op){
        switch(op){
            case Operator::PLUS: return 20;
            case Operator::MINUS: return 20;
            case Operator::MULTIPLY: return 30;
            case Operator::DIVIDE: return 30;
            case Operator::MODULO: return 30;
            case Operator::GREATER: return 10;
            case Operator::LESS: return 10;
            case Operator::EQUAL: return 10;
            case Operator::NOT_EQUAL: return 10;
            case Operator::GREATER_EQUAL: return 10;
            case Operator::LESS_EQUAL: return 10;
            default: return -1;
        }
    }

    JLDEF constexpr int get_ident_precedence(IdentType type){
        switch(type){
            case IdentType::FUNCTION: return 30;
            case IdentType::VARIABLE: return 10;
            case IdentType::CONSTANT: return 20;
            default: return -1;
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
                    return std::to_string(value.as_integer());
                case TokenType::STRING_LITERAL:
                case TokenType::IDENTIFIER:
                case TokenType::PAREN_BLOCK_START:
                case TokenType::PAREN_BLOCK_END:
                case TokenType::ARG_DELIMITER:
                    return value.as_string();
                case TokenType::TYPE:
                    return std::string(get_enum_name(value.as_expr_type()));
                case TokenType::KEYWORD:
                    return std::string(get_enum_name(value.as_keyword()));
                case TokenType::OPERATOR:
                    return std::string(get_enum_name(value.as_operator()));
                case TokenType::INTRINSIC:
                    return std::string(get_enum_name(value.as_intrinsic()));
                default:
                    return "";
            }
    }
//#endif // JLANGOBJECTS_IMPLEMENTATION
#pragma endregion Implementation

    
    class NotImplementedException : public std::exception {
        public:
            NotImplementedException(std::string message) : message(message) {}
            const char* what() const noexcept override {
                return message.c_str();
            }
        private:
            std::string message;
    };


    /// \brief Contains the location of a token in a file.
    typedef struct  Location_s {
        std::string file;
        std::size_t line;
        std::size_t column;

        std::string display() const {
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

    /// \brief Contains the type, location, text and value of a Token from a parse file.
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

    /// \brief Contains information about a defined variable
    class Variable {
        protected:
        std::string name;
        ExprType type;
        size_t size; // in bytes
        public:
        Variable(std::string name, ExprType type, size_t size) : name(name), type(type), size(size) {}
        Variable() : name(""), type(ExprType::INVALID), size(0) {}
        const std::string get_name() const { return name; }
        const ExprType get_type() const { return type; }
        const size_t get_size() const { return size; }
        auto operator==(const Variable &other) const { return name == other.name && type == other.type; }
        auto operator==(const std::string &name) const {
            return this->name == name;
        }
   };

    /// \brief Contains information and the value of a constant.
    class Constant : public Variable {
        private:
        std::variant<uint64_t, std::string> value;
        public:
        Constant(std::string name, ExprType type, size_t size, uint64_t value) : Variable(name, type, size) {
            this->value = value;
        }
        Constant(std::string name, ExprType type, size_t size, std::string value) : Variable(name, type, size) {
            this->value = value;
        }
        Constant() : Variable() {}
        const uint64_t &get_int_value() const { return std::get<uint64_t>(value); }
        const std::string &get_string_value() const { return std::get<std::string>(value); }
        auto operator==(const Constant &other) const {
            bool cmp_trivial = this->name == other.name && this->type == other.type && this->size == other.size;
            if(this->type == ExprType::POINTER) {
                return cmp_trivial && std::get<std::string>(this->value) == std::get<std::string>(other.value);
            } else {
                return cmp_trivial && std::get<uint64_t>(this->value) == std::get<uint64_t>(other.value);
            }
        }
   };

    /// \brief Contains information about a function.
    class FunProto {
        private: 
        std::string name;
        std::vector<Variable> args;
        ExprType return_type;
        public:
        FunProto(std::string name, std::vector<Variable> args, ExprType return_type) : name(name), args(args), return_type(return_type) {}
        FunProto() : name(""), args(std::vector<Variable>()), return_type(ExprType::INVALID) {}
        const std::string &get_name() const { return name; }
        constexpr std::vector<Variable> &get_args() { return args; }
        constexpr ExprType get_return_type() const { return return_type; }
        bool operator==(const FunProto& fun) const {
            return name == fun.get_name() && return_type == fun.get_return_type();
        }
   };

    typedef struct CompilerState_s {
        std::vector<Token> tokens;
        std::map<std::string, Constant> constants;
        std::map<std::string, FunProto> prototypes;
        std::map<std::string, Variable> global_vars;
        std::map<std::string, Variable> scope_vars;
        std::vector<Variable> anon_global_vars;
        std::vector<Variable> anon_scope_vars;
        bool in_scope;

        const std::string dump(){
            std::stringstream ss;
            ss << "Constants: " << std::endl;
            for(auto &[name, constant] : constants) {
                ss << "bruh" << std::endl;
                ss << constant.get_name() << " : " << get_enum_name(constant.get_type()) << " : " << constant.get_size() << std::endl;
            }
            ss << "Prototypes: " << std::endl;
            for(auto &[name, proto] : prototypes) {
                ss << name << " : " << get_enum_name(proto.get_return_type()) << std::endl;
                for(auto &arg : proto.get_args()) {
                    ss << "    " << arg.get_name() << " : " << get_enum_name(arg.get_type()) << " : " << arg.get_size() << std::endl;
                }
            }
            ss << "Global Variables: " << std::endl;
            for(auto &[name, var] : global_vars) {
                ss << var.get_name() << " : " << get_enum_name(var.get_type()) << " : " << var.get_size() << std::endl;
            }
            ss << "Scope Variables: " << std::endl;
            for(auto &[name, var] : scope_vars) {
                ss << var.get_name() << " : " << get_enum_name(var.get_type()) << " : " << var.get_size() << std::endl;
            }
            return ss.str();
        }
    } CompilerState;

}

#undef ENUM_TOKENTYPE
#undef ENUM_KEYWORD
#undef ENUM_OPERATOR
#undef ENUM_INTRINSIC
#undef ENUM_STATEMENT
#undef ENUM_CONTROL
#endif // JLANGOBJECTS_H_