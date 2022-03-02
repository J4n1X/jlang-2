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
                Location loc;
                /// TODOOO: Check if it's neccessare to store the type of the statement.
                ExprType type;
            public:
                Location get_location() { return loc; }
                ExprType get_type() { return type; }
                Statement() : loc(), type(ExprType::NONE) {}
                Statement(Location loc, ExprType type) : loc(loc), type(type) {}
                ~Statement() {}

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
        class Expression : public Statement {
            public: 
                Expression(Location loc, ExprType type) : Statement(loc, type) {}
                virtual ~Expression() {}

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

        /// \brief Compile-time resolvable expressions
        class ResolvableExpr : public Expression {
            public:
                ResolvableExpr(Location loc, ExprType type) : Expression(loc, type) {}
                /*std::string display(int depth = 0){
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "ResolvableExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << ref;
                    return ss.str();
                }*/
        };

        /// \brief Represents a nuumber literal.
        class IntLiteralExpr : public ResolvableExpr {
            private:
                uint64_t value;
            public:
                IntLiteralExpr(Location loc, uint64_t value) : ResolvableExpr(loc, ExprType::INTEGER), value(value) {}

                std::string display(int depth) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "IntLiteralExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << std::to_string(value);
                    return ss.str();
                }
        };

        /// \brief Represents a pointer to memory
        class ArrayRefExpr : public ResolvableExpr {
            private:
                std::string ref;
            public:
                ArrayRefExpr(Location loc, std::string ref) : ResolvableExpr(loc, ExprType::POINTER), ref(ref) {}
                std::string display(int depth = 0){
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "ArrayRefExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << ref;
                    return ss.str();
                }

        };

        /// \brief Represents a value inserted at compile time. Therefore this expression should not be visible within the AST.
        class ConstantExpr : public ResolvableExpr {
            private:
                std::string ref;
            public:
                ConstantExpr(Location loc, ExprType type, std::string ref) : ResolvableExpr(loc, type), ref(ref) {}
        };

        class IdentRefExpr : public Expression {
            private:
                std::string ident_name;
            protected:
                IdentType ident_kind;
            public:
                IdentRefExpr(Location loc, ExprType type, std::string ref, IdentType ident_kind) : Expression(loc, type), ident_name(ref), ident_kind(ident_kind) {}
                IdentType get_ident_kind() { return ident_kind; }
                std::string get_ident_name() { return ident_name; }
                std::string display(int depth = 0){
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "IdentRefExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "IdentKind: " << get_enum_name(ident_kind) << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value: " << ident_name;
                    return ss.str();
                }
        };

        /// \brief Load a value from a memory location in a variable
        /// TODO: Add support for loading from a literal
        class LoaderExpr : public Expression {
            private:
                std::unique_ptr<Expression> expr;
            public:
                LoaderExpr(Location loc, std::unique_ptr<Expression> target) : Expression(loc, ExprType::INTEGER), expr(std::move(target)) {
                }
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "LoaderExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value:\n" << expr->display(depth + 8);
                    return ss.str();
                }
                //std::string codegen();
        };
        
        /// \brief get the memory location of an identifier in memory
        class AddressOfExpr : public Expression {
            private:
                std::unique_ptr<IdentRefExpr> expr;
            public:
                AddressOfExpr(Location loc, std::unique_ptr<IdentRefExpr> target) : Expression(loc, ExprType::POINTER), expr(std::move(target)) {}
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "AddressOfExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Target:\n" << expr->display(depth + 8);
                    return ss.str();
                }
        };
        
        class BinaryExpr : public Expression {
            protected:
                Operator op; 
                std::unique_ptr<ResolvableExpr> left, right;
            public:
                BinaryExpr(Location loc, ExprType type, Operator op, std::unique_ptr<ResolvableExpr> lhs, std::unique_ptr<ResolvableExpr> rhs) : Expression(loc, type), op(op), left(std::move(lhs)), right(std::move(rhs)) {
                }
                
                std::string display(int depth = 0) {
                
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "BinaryExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Op: " << get_enum_name(op) << '\n';
                    ss << std::string(depth + 4, ' ') << "LHS:\n" << left->display(depth + 8) << '\n';
                    ss << std::string(depth + 4, ' ') << "RHS:\n" << right->display(depth + 8) << '\n';
                    return ss.str();
                }
        };

        class CallExpr: public Expression {
            protected:
                std::vector<std::unique_ptr<Expression>> args;
            public:
                CallExpr(Location loc, std::vector<std::unique_ptr<Expression>> args) : Expression(loc, ExprType::INTEGER), args(std::move(args)) {}
                /*std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "CallExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Name: " << expr->get_ident_name() << '\n';
                    ss << std::string(depth + 4, ' ') << "Args:\n";
                    for(auto &arg : args) {
                        ss << arg->display(depth + 8) << '\n';
                    }
                    return ss.str();
                }*/
        };

        class SyscallExpr : public CallExpr {
            protected:
                std::unique_ptr<Expression> expr;
            public:
                SyscallExpr(Location loc, std::unique_ptr<Expression> target, std::vector<std::unique_ptr<Expression>> args) : CallExpr(loc, std::move(args)), expr(std::move(target)) {}
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "SyscallExpr\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Call number:\n" << expr->display(depth + 8) << '\n';
                    ss << std::string(depth + 4, ' ') << "Args:\n";
                    for(auto &arg : args) {
                        ss << arg->display(depth + 8) << '\n';
                    }
                    return ss.str();
                }
        };

        class IntrinsicStmt : public Statement {
            protected:
                std::unique_ptr<Expression> expr;
            public:
                IntrinsicStmt(Location loc, std::unique_ptr<Expression> expr) : Statement(loc, ExprType::NONE), expr(std::move(expr)) {}
        };

        class DropStmt : public IntrinsicStmt {
            public:
                DropStmt(Location loc, std::unique_ptr<Expression> expr) : IntrinsicStmt(loc, std::move(expr)) {}
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "DropStmt\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value:\n" << expr->display(depth + 8);
                    return ss.str();
                }
        };
        /// \brief All kinds of variable definitions, from local to global to parameters
        class VarDefStmt : public Statement {
            protected:
                std::unique_ptr<Expression> value;
                std::string name;
                IdentType var_kind;
                size_t size;
            public:
                VarDefStmt(Location loc, std::string name, ExprType var_type, IdentType var_kind, size_t size, std::unique_ptr<IdentRefExpr> value) : Statement(loc, var_type), name(name), value(std::move(value)),var_kind(var_kind), size(size) {}
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "VarDefStmt\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value:\n" << value->display(depth + 8);
                    return ss.str();
                }
        };
        /// \brief Sets a variable to the value given
        class VarSetStmt : public Statement {
            protected:
                std::unique_ptr<IdentRefExpr> var;
                std::unique_ptr<Expression> value;
            public:
                VarSetStmt(Location loc, std::unique_ptr<IdentRefExpr> var, std::unique_ptr<Expression> value) : Statement(loc, var->get_type()), value(std::move(value)), var(std::move(var)) {}
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "VarSetStmt\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Variable: " << var->get_ident_name() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(var->get_type()) << '\n';
                    ss << std::string(depth + 4, ' ') << "Value:\n" << value->display(depth + 8);
                    return ss.str();
                }
        };

        class StorerStmt : public IntrinsicStmt {
            private: 
                std::unique_ptr<Expression> value;
            public:
                StorerStmt(Location loc, std::unique_ptr<IdentRefExpr> target, std::unique_ptr<Expression> value) : IntrinsicStmt(loc, std::move(target)), value(std::move(value)) {}
                std::string display(int depth = 0) {
                    std::stringstream ss;
                    ss << std::string(depth, ' ') << "StorerStmt\n";
                    ss << std::string(depth + 4, ' ') << "Location: " << loc.display() << '\n';
                    ss << std::string(depth + 4, ' ') << "Type: " << get_enum_name(type) << '\n';
                    ss << std::string(depth + 4, ' ') << "Target:\n" << expr->display(depth + 8);
                    ss << std::string(depth + 4, ' ') << "Value:\n" << value->display(depth + 8);
                    return ss.str();
                }
        };
    }
}

#define STATEMENTS_FUNCTIONS
#include "Statements.cpp"

#endif