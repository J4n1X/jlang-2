#ifndef STATEMENTS_H_
#define STATEMENTS_H_

#include <vector>
#include <map>
#include <string>
#include <typeinfo>
#include <sstream>
#include <concepts>

#include "JlangObjects.hpp"

namespace jlang {
    namespace statements {
        /// A simple statement which specifies the type and the location from which it was parsed. The base of all AST classes.
        class Statement {
            protected:
                Token token;
                ExprType type;
            public:
                Token get_token() { return token; }
                ExprType get_type() { return type; }
                Statement() : token(), type(ExprType::NONE) {}
                Statement(Token token, ExprType type) : token(token), type(type) {}

                // virtual functions all derivatives must implement
                virtual std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "Statement derivative with type ID " << typeid(this).name() << " has not implemented print()";
                    return ss.str();
                }
                virtual std::string codegen() {
                    std::stringstream ss;
                    ss << "Statement derivative with type ID " << typeid(this).name() << " has not implemented codegen()";
                    return ss.str();
                }
        };
        
        /// The Expression is only very loosely based on a Statement, but is derived from it to simplify function parameters later on.
        template<typename T>
        class Expression : public Statement {
            protected: 
                ExprValType value_type;
                T value;


            public:
                ExprValType get_value_type() { return value_type; }
                T get_value() { return value; }
                Expression(Token token, ExprType type, ExprValType value_type, T value) : Statement(token, type), value_type(value_type), value(value) {}

                virtual std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << "Expression derivative with type ID " << typeid(this).name() << " has not implemented display()";
                    return ss.str();
                }
                //std::string display(int depth) requires std::is_convertible<T, uint64_t>::value
                //std::string display(int depth) requires std::is_convertible<T, std::string>::value
                //std::string display(int depth) requires std::is_same<T, Expression>::value
                virtual std::string codegen() {
                    std::stringstream ss;
                    ss << "Expression derivative with type ID " << typeid(this).name() << " has not implemented codegen()";
                    return ss.str();
                }
        };

        template<typename T> requires std::is_convertible<T, uint64_t>::value
        class Expression<T> : public Statement {
            protected: 
                ExprValType value_type;
                uint64_t value;

            public:
                ExprValType get_value_type() { return value_type; }
                Expression(Token token, ExprType type, T value) : Statement(token, type), value_type(ExprValType::INTEGER), value(value) {}
                uint64_t get_value() { return value; }

                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "Expression\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << value;
                    return ss.str();
                }
        };

        template<typename T> requires std::is_convertible<T, std::string>::value
        class Expression<T> : public Statement {
            protected: 
                ExprValType value_type;
                std::string value;

            public:
                ExprValType get_value_type() { return value_type; }
                Expression(Token token, ExprType type, T value) : Statement(token, type), value_type(ExprValType::STRING), value(value) {}
                std::string get_value() { return value; }

                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "Expression\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << value;
                    return ss.str();
                }
        };

        template<class T, class U>
        concept Derived = std::is_base_of<U, T>::value;
        template<class T>
        concept ResolvableExpression = Derived<T, Expression<uint64_t>> || Derived<T, Expression<std::string>>;

        // requires any type that is derived from Expression
        template<ResolvableExpression T>
        class Expression<T> : public Statement {
            protected:
                ExprValType value_type;
                T value;

            public:
                ExprValType get_value_type() { return value_type; }
                Expression(Token token, ExprType type, T value) : Statement(token, type), value_type(ExprValType::EXPRESSION), value(value) {}
                T get_value() { return value; }

                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "Expression\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << value.display();
                    return ss.str();
                }
        };

        template<typename U>
        class Expression<std::vector<Expression<U>>> : public Statement {
            protected:
                ExprValType value_type;
                std::vector<Expression<U>> value;

            public:
                ExprValType get_value_type() { return value_type; }
                Expression(Token token, ExprType type, std::vector<Expression<U>> value) : Statement(token, type), value_type(ExprValType::EXPRESSION_LIST), value(value) {}
                std::vector<Expression<U>> get_value() { return value; }

                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "Expression\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: ";
                    for (auto &e : value) {
                        ss << e.display(depth + 8);
                    }
                    return ss.str();
                }
        };
        
        /// \brief Represents a nuumber literal.
        class IntLiteralExpr : public Expression<uint64_t> {
            public:
                IntLiteralExpr(Token token, uint64_t value) : Expression(token, ExprType::INTEGER, value) {}
        };

        /// \brief Represents a pointer to memory
        class ArrayRefExpr : public Expression<std::string> {
            public:
                ArrayRefExpr(Token token, std::string ref) : Expression(token, ExprType::POINTER, ref) {}
                std::string display(int depth = 0){
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "ArrayRefExpr\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << value;
                    return ss.str();
                }

        };

        class IdentRefExpr : public Expression<std::string> {
            protected:
                IdentType ident_kind;
            public:
                IdentRefExpr(Token token, std::string ref, IdentType ident_kind, ExprType type) : Expression(token, type, ref), ident_kind(ident_kind) {}
                IdentType get_ident_kind() { return ident_kind; }
                std::string display(int depth = 0){
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "IdentRefExpr\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "IdentKind: " << get_enum_name(ident_kind) << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << value;
                    return ss.str();
                }
        };
        /*
        template<class T, typename U>
        concept expr_resolvable = requires(T t){
            {t.value} -> std::same_as<U>;
            {t} -> std::is_same<T, IdentRefExpr>;
        };*/

        /// \brief Load a value from a memory location in a variable
        /// TODO: Add support for loading from a literal
        
        class LoaderExpr : public Expression<IdentRefExpr> {
            public:
                LoaderExpr(Token token, IdentRefExpr target) : Expression(token, ExprType::INTEGER, target) {
                    if(target.get_value_type() != ExprValType::STRING) {
                        throw std::runtime_error("LoaderExpr target must be a string");
                    }
                }
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "LoaderExpr\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value:\n" << value.display(depth + 8);
                    return ss.str();
                }
                //std::string codegen();
        };

        class AddressOfExpr : public Expression<IdentRefExpr> {
            public:
                AddressOfExpr(Token token, IdentRefExpr target) : Expression(token, ExprType::POINTER, target) {}
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "AddressOfExpr\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Target:\n" << value.display(depth + 8);
                    return ss.str();
                }
        };
        /*
        class BinaryExpr : public Expression<> {
            protected:
                Operator op; 
            public:
                BinaryExpr(Token token, ExprType op, expr_resolvable<uint64_t> lhs, expr_resolvable<uint64_t> rhs) : Expression(token, ExprType::BINARY, lhs), op(op) {
                    if(lhs.get_value_type() != ExprValType::EXPRESSION || rhs.get_value_type() != ExprValType::EXPRESSION) {
                        throw std::runtime_error("BinaryExpr operands must be expressions");
                    }
                    if(lhs.get_value().get_value_type() != ExprValType::INTEGER || rhs.get_value().get_value_type() != ExprValType::INTEGER) {
                        throw std::runtime_error("BinaryExpr operands must be integers");
                    }
                }
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "BinaryExpr\n";
                    ss << std::string(depth + 4, ' ') << "Token: " << token.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Op: " << get_enum_name(op) << '\n';
                    ss << std::string(depth + 4, ' ') << "LHS:\n" << value.display(depth + 8) << '\n';
                    ss << std::string(depth + 4, ' ') << "RHS:\n" << value.display(depth + 8) << '\n';
                    return ss.str();
                }
        };
        */
    }
}

#define STATEMENTS_FUNCTIONS
#include "Statements.cpp"

#endif