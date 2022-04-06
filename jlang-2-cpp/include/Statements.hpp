#ifndef STATEMENTS_H_
#define STATEMENTS_H_

#include <concepts>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include "JlangObjects.hpp"

namespace jlang {
namespace statements {
struct Statement {
  protected:
    Location loc;
    ExprType type;
    StatementType kind;
    std::shared_ptr<jlang::CompilerState> parser;

  public:
    Statement() = default;
    Statement(Location loc, ExprType type, StatementType kind,
              std::shared_ptr<jlang::CompilerState> parser)
        : loc(loc), type(type), kind(kind), parser(parser) {}
    const auto get_loc() const { return this->loc; }
    const auto get_type() const { return this->type; }
    void set_type(ExprType type) { this->type = type; }
    const auto get_kind() const { return this->kind; }
    virtual ~Statement() = default;
    virtual std::string to_string() const {
        throw std::runtime_error(
            "String conversion not implemented for this statement");
    }
    virtual std::string codegen() const {
        throw std::runtime_error("Codegen not implemented for this statement");
    }
};

struct BlockStmt : public Statement {
  protected:
    std::vector<Statement *> statements;

  public:
    BlockStmt(Location loc, ExprType type,
              std::shared_ptr<jlang::CompilerState> parser,
              std::vector<Statement *> statements)
        : Statement(loc, type, StatementType::BLOCK, parser),
          statements(statements) {}
    BlockStmt(Location loc, ExprType type, StatementType kind,
              std::shared_ptr<jlang::CompilerState> parser,
              std::vector<Statement *> statements)
        : Statement(loc, type, kind, parser), statements(statements) {}
    BlockStmt() = default;

    const auto &get_statements() const { return this->statements; }
    auto &get_statements() { return this->statements; }
    const std::size_t get_size() const { return this->statements.size(); }

    ~BlockStmt() override {
        for (auto stmt : this->statements) {
            delete stmt;
        }
    }
};

struct LiteralStmt : public Statement {
  private:
    std::variant<uint64_t, std::string> value;

  public:
    LiteralStmt(Location loc, ExprType type,
                std::shared_ptr<jlang::CompilerState> parser, std::string value)
        : Statement(loc, type, StatementType::LITERAL, parser), value(value) {}
    LiteralStmt(Location loc, ExprType type,
                std::shared_ptr<jlang::CompilerState> parser,
                std::uint64_t value)
        : Statement(loc, type, StatementType::LITERAL, parser), value(value) {}
    LiteralStmt() = default;
};

struct IdentStmt : public Statement {
  private:
    IdentType ident_kind;
    std::variant<Variable, Constant, FunProto> target;
    Statement *param; // can hold function call args or a variable value
  public:
    IdentStmt(Location loc, ExprType type,
              std::shared_ptr<jlang::CompilerState> parser,
              IdentType ident_kind,
              std::variant<Variable, Constant, FunProto> target,
              Statement *param = nullptr)
        : Statement(loc, type, StatementType::IDENT, parser),
          ident_kind(ident_kind), target(target), param(param) {}

    const auto get_ident_kind() const { return this->ident_kind; }
};

struct FunStmt : public Statement {
  private:
    FunProto &proto;
    BlockStmt *body;

  public:
    FunStmt(Location loc, std::shared_ptr<jlang::CompilerState> parser,
            FunProto &proto, BlockStmt *body)
        : Statement(loc, proto.get_return_type(), StatementType::FUNCTION,
                    parser),
          proto(proto), body(body) {}
    ~FunStmt() override { delete this->body; }
};

struct FunCallStmt : public Statement {
  private:
    FunProto proto;
    Statement *args;

  public:
    FunCallStmt(Location loc, ExprType type,
                std::shared_ptr<jlang::CompilerState> parser, FunProto &proto,
                Statement *args)
        : Statement(loc, type, StatementType::FUNCALL, parser), proto(proto),
          args(args) {}
};

struct IntrinsicStmt : public Statement {
  private:
    Intrinsic intrinsic_kind;
    Statement *args;

  public:
    IntrinsicStmt(Location loc, ExprType type,
                  std::shared_ptr<jlang::CompilerState> parser,
                  Intrinsic intrinsic_kind, Statement *args)
        : Statement(loc, type, StatementType::INTRINSIC, parser),
          intrinsic_kind(intrinsic_kind), args(args) {}
    const Intrinsic get_intrinsic_kind() const { return this->intrinsic_kind; }
    ~IntrinsicStmt() override { delete this->args; }
};

struct ControlStmt : public Statement {
  private:
    ControlType control_kind;
    Statement *condition;
    std::vector<Statement *> statements;

  public:
    ControlStmt(Location loc, ExprType type,
                std::shared_ptr<jlang::CompilerState> parser,
                ControlType control_kind, Statement *condition,
                std::vector<Statement *> statements)
        : Statement(loc, type, StatementType::CONTROL, parser),
          control_kind(control_kind), condition(condition),
          statements(statements) {}
    ~ControlStmt() override {
        delete this->condition;
        for (auto stmt : this->statements) {
            delete stmt;
        }
    }
};

struct BinaryStmt : public Statement {
  private:
    Operator op;
    Statement *left;
    Statement *right;

  public:
    BinaryStmt(Location loc, ExprType type,
               std::shared_ptr<jlang::CompilerState> parser, Operator op,
               Statement *left, Statement *right)
        : Statement(loc, type, StatementType::BINARY, parser), op(op),
          left(left), right(right) {}
    ~BinaryStmt() override {
        delete this->left;
        delete this->right;
    }
};

/// A simple statement which specifies the type and the location from which it
/// was parsed. The base of all AST classes.
//        class Statement {
//            protected:
//                Location loc;
//                /// TODOOO: Check if it's neccessare to store the type of the
//                statement. ExprType type;
//            public:
//                const auto get_location() const { return loc; }
//                const auto get_type() const { return type; }
//                void set_type(ExprType type) { this->type = type; }
//                Statement() : loc(), type(ExprType::NONE) {}
//                Statement(auto loc, auto type) : loc(loc), type(type) {}
//                ~Statement() {}
//
//                // virtual functions all derivatives must implement
//                virtual std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "Statement derivative
//                    with type ID " << typeid(this).name() << " has not
//                    implemented print()"; return ss.str();
//                }
//                virtual std::string codegen() {
//                    std::stringstream ss;
//                    ss << "Statement derivative with type ID " <<
//                    typeid(this).name() << " has not implemented codegen()";
//                    return ss.str();
//                }
//                virtual std::vector<std::byte> to_ir() {
//                    throw NotImplementedException("Generating IR is not
//                    supported yet");
//                }
//        };
//
//        /// The Expression is only very loosely based on a Statement, but is
//        derived from it to simplify function parameters later on. class
//        Expression : public Statement {
//            public:
//                Expression() : Statement() {}
//                Expression(Location loc, ExprType type) : Statement(loc,
//                type)
//                {} virtual ~Expression() {}
//
//                virtual std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << "Expression derivative with type ID " <<
//                    typeid(this).name() << " has not implemented display()";
//                    return ss.str();
//                }
//                //std::string display(int depth) requires
//                std::is_convertible<T, uint64_t>::value
//                //std::string display(int depth) requires
//                std::is_convertible<T, std::string>::value
//                //std::string display(int depth) requires std::is_same<T,
//                Expression>::value virtual std::string codegen() {
//                    std::stringstream ss;
//                    ss << "Expression derivative with type ID " <<
//                    typeid(this).name() << " has not implemented codegen()";
//                    return ss.str();
//                }
//        };
//
//        /// \brief Compile-time resolvable expressions
//        /// This replaces a lengthy check in the Python version to
//        /// determine if you could resolve the expression at compile time.
//        class ResolvableExpr : public Expression {
//            public:
//                ResolvableExpr(Location loc, ExprType type) : Expression(loc,
//                type) {}
//                /*std::string display(int depth = 0){
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "ResolvableExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value: " << ref; return
//                    ss.str();
//                }*/
//        };
//
//        /// \brief Represents a nuumber literal.
//        class IntLiteralExpr : public ResolvableExpr {
//            private:
//                uint64_t value;
//            public:
//                IntLiteralExpr(Location loc, uint64_t value) :
//                ResolvableExpr(loc, ExprType::INTEGER), value(value) {}
//
//                std::string display(int depth) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "IntLiteralExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Value: " << std::to_string(value); return ss.str();
//                }
//        };
//
//        /// \brief Represents a pointer to memory
//        class ArrayRefExpr : public ResolvableExpr {
//            private:
//                std::string ref;
//            public:
//                ArrayRefExpr(Location loc, std::string ref) :
//                ResolvableExpr(loc, ExprType::POINTER), ref(ref) {}
//                std::string display(int depth = 0){
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "ArrayRefExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value: " << ref; return
//                    ss.str();
//                }
//
//        };
//
//        /// \brief Represents a value inserted at compile time. Therefore
//        this expression should not be visible within the AST. class
//        ConstantExpr : public ResolvableExpr {
//            private:
//                std::string ref;
//            public:
//                ConstantExpr(Location loc, ExprType type, std::string ref) :
//                ResolvableExpr(loc, type), ref(ref) {}
//        };
//
//        class IdentRefExpr : public Expression {
//            private:
//                std::string ident_name;
//            protected:
//                IdentType ident_kind;
//            public:
//                IdentRefExpr(Location loc, ExprType type, std::string ref,
//                IdentType ident_kind) : Expression(loc, type),
//                ident_name(ref), ident_kind(ident_kind) {} IdentType
//                get_ident_kind() { return ident_kind; } std::string
//                get_ident_name() { return ident_name; } std::string
//                display(int depth = 0){
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "IdentRefExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "IdentKind: " << get_enum_name(ident_kind) << '\n'; ss
//                    << std::string(depth + 4, ' ') << "Type: " <<
//                    get_enum_name(type) << '\n'; ss << std::string(depth + 4,
//                    ' ') << "Value: " << ident_name; return ss.str();
//                }
//        };
//
//        /// \brief Load a value from a memory location in a variable
//        /// TODO: Add support for loading from a literal
//        class LoaderExpr : public Expression {
//            private:
//                std::unique_ptr<Expression> expr;
//            public:
//                LoaderExpr(Location loc, std::unique_ptr<Expression> target)
//                : Expression(loc, ExprType::INTEGER), expr(std::move(target))
//                {
//                }
//                std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "LoaderExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value:\n" <<
//                    expr->display(depth + 8); return ss.str();
//                }
//                //std::string codegen();
//        };
//
//        /// \brief get the memory location of an identifier in memory
//        class AddressOfExpr : public Expression {
//            private:
//                std::unique_ptr<IdentRefExpr> expr;
//            public:
//                AddressOfExpr(Location loc, std::unique_ptr<IdentRefExpr>
//                target) : Expression(loc, ExprType::POINTER),
//                expr(std::move(target)) {} std::string display(int depth = 0)
//                {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "AddressOfExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Target:\n" <<
//                    expr->display(depth + 8); return ss.str();
//                }
//        };
//
//        class BinaryExpr : public Expression {
//            protected:
//                Operator op;
//                std::unique_ptr<Expression> left, right;
//            public:
//                BinaryExpr(Location loc, ExprType type, Operator op,
//                std::unique_ptr<Expression> lhs, std::unique_ptr<Expression>
//                rhs) : Expression(loc, type), op(op), left(std::move(lhs)),
//                right(std::move(rhs)) {
//                }
//
//                std::string display(int depth = 0) {
//
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "BinaryExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Op: " <<
//                    get_enum_name(op)
//                    << '\n'; ss << std::string(depth + 4, ' ') << "LHS:\n" <<
//                    left->display(depth + 8) << '\n'; ss << std::string(depth
//                    + 4, ' ') << "RHS:\n" << right->display(depth + 8) <<
//                    '\n'; return ss.str();
//                }
//        };
//
//        class CallExpr: public Expression {
//            protected:
//                std::vector<std::unique_ptr<Expression>> args;
//            public:
//                CallExpr(Location loc,
//                std::vector<std::unique_ptr<Expression>> args) :
//                Expression(loc, ExprType::INTEGER), args(std::move(args)) {}
//                /*std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "CallExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Name: " <<
//                    expr->get_ident_name() << '\n'; ss << std::string(depth +
//                    4, ' ') << "Args:\n"; for(auto &arg : args) {
//                        ss << arg->display(depth + 8) << '\n';
//                    }
//                    return ss.str();
//                }*/
//        };
//
//        class SyscallExpr : public CallExpr {
//            protected:
//                std::unique_ptr<Expression> expr;
//            public:
//                SyscallExpr(Location loc, std::unique_ptr<Expression> target,
//                std::vector<std::unique_ptr<Expression>> args) :
//                CallExpr(loc, std::move(args)), expr(std::move(target)) {}
//                std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "SyscallExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Call number:\n" <<
//                    expr->display(depth + 8) << '\n'; ss << std::string(depth
//                    + 4, ' ') << "Args:\n"; for(auto &arg : args) {
//                        ss << arg->display(depth + 8) << '\n';
//                    }
//                    return ss.str();
//                }
//        };
//
//        class FunCallExpr : public CallExpr {
//            protected:
//                std::unique_ptr<IdentRefExpr> expr;
//            public:
//                /// TODO: Lookup of function data from context
//                FunCallExpr(Location loc, std::unique_ptr<IdentRefExpr>
//                target, std::vector<std::unique_ptr<Expression>> args) :
//                CallExpr(loc, std::move(args)), expr(std::move(target)) {}
//                std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "FunCallExpr\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Name: " <<
//                    expr->get_ident_name() << '\n'; ss << std::string(depth +
//                    4, ' ') << "Args:\n"; for(auto &arg : args) {
//                        ss << arg->display(depth + 8) << '\n';
//                    }
//                    return ss.str();
//                }
//        };
//
//        class IntrinsicStmt : public Statement {
//            protected:
//                std::unique_ptr<Expression> expr;
//            public:
//                IntrinsicStmt(Location loc, std::unique_ptr<Expression> expr)
//                : Statement(loc, ExprType::NONE), expr(std::move(expr)) {}
//        };
//
//        class DropStmt : public IntrinsicStmt {
//            public:
//                DropStmt(Location loc, std::unique_ptr<Expression> expr) :
//                IntrinsicStmt(loc, std::move(expr)) {} std::string
//                display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "DropStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value:\n" <<
//                    expr->display(depth + 8); return ss.str();
//                }
//        };
//        /// \brief All kinds of variable definitions, from local to global to
//        parameters class VarDefStmt : public Statement {
//            protected:
//                std::unique_ptr<Expression> value;
//                std::string name;
//                IdentType var_kind;
//                size_t size;
//            public:
//                VarDefStmt(Location loc, std::string name, ExprType var_type,
//                IdentType var_kind, size_t size,
//                std::unique_ptr<IdentRefExpr> value) : Statement(loc,
//                var_type), name(name),
//                value(std::move(value)),var_kind(var_kind), size(size) {}
//                std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "VarDefStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value:\n" <<
//                    value->display(depth + 8); return ss.str();
//                }
//        };
//        /// \brief Sets a variable to the value given
//        class VarSetStmt : public Statement {
//            protected:
//                std::unique_ptr<IdentRefExpr> var;
//                std::unique_ptr<Expression> value;
//            public:
//                VarSetStmt(Location loc, std::unique_ptr<IdentRefExpr> var,
//                std::unique_ptr<Expression> value) : Statement(loc,
//                var->get_type()), value(std::move(value)),
//                var(std::move(var))
//                {} std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "VarSetStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Variable: " << var->get_ident_name() << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Type: " <<
//                    get_enum_name(var->get_type()) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value:\n" <<
//                    value->display(depth + 8); return ss.str();
//                }
//        };
//
//        class StorerStmt : public IntrinsicStmt {
//            private:
//                std::unique_ptr<Expression> value;
//            public:
//                StorerStmt(Location loc, std::unique_ptr<IdentRefExpr>
//                target, std::unique_ptr<Expression> value) :
//                IntrinsicStmt(loc, std::move(target)),
//                value(std::move(value)) {} std::string display(int depth = 0)
//                {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "StorerStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Target:\n" <<
//                    expr->display(depth + 8); ss << std::string(depth + 4, '
//                    ') << "Value:\n" << value->display(depth + 8); return
//                    ss.str();
//                }
//        };
//
//        class PrintStmt : public IntrinsicStmt {
//            public:
//                PrintStmt(Location loc, std::unique_ptr<Expression> value) :
//                IntrinsicStmt(loc, std::move(value)) {} std::string
//                display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "PrintStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value:\n" <<
//                    expr->display(depth + 8); return ss.str();
//                }
//        };
//
//        class ReturnStmt : public IntrinsicStmt {
//            public:
//                ReturnStmt(Location loc, std::unique_ptr<Expression> value =
//                nullptr) : IntrinsicStmt(loc, std::move(value)) {}
//                std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "ReturnStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Value:\n" <<
//                    expr->display(depth + 8); return ss.str();
//                }
//        };
//
//        class FunStmt : public Statement {
//            protected:
//                std::string name;
//                std::vector<std::unique_ptr<VarDefStmt>> args;
//                std::vector<std::unique_ptr<Statement>> body;
//            public:
//                FunStmt(Location loc, std::string name,
//                std::vector<std::unique_ptr<VarDefStmt>> args,
//                std::vector<std::unique_ptr<Statement>> body) :
//                Statement(loc, ExprType::NONE), name(name),
//                args(std::move(args)), body(std::move(body)) {} std::string
//                display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "FunStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Name: " << name << '\n';
//                    ss << std::string(depth + 4, ' ') << "Args:\n";
//                    for(auto &arg : args) {
//                        ss << arg->display(depth + 8) << '\n';
//                    }
//                    ss << std::string(depth + 4, ' ') << "Body:\n";
//                    for(auto &stmt : body) {
//                        ss << stmt->display(depth + 8) << '\n';
//                    }
//                    return ss.str();
//                }
//        };
//
//        class ControlStmt : public Statement {
//            protected:
//                std::unique_ptr<Expression> condition;
//                std::vector<std::unique_ptr<Statement>> body;
//            public:
//                ControlStmt(Location loc, std::unique_ptr<Expression>
//                condition, std::vector<std::unique_ptr<Statement>> body) :
//                Statement(loc, ExprType::NONE),
//                condition(std::move(condition)), body(std::move(body)) {}
//                std::string display(int depth = 0) {
//                    std::stringstream ss;
//                    ss << std::string(depth, ' ') << "ControlStmt\n";
//                    ss << std::string(depth + 4, ' ') << "Location: " <<
//                    loc.display() << '\n'; ss << std::string(depth + 4, ' ')
//                    << "Type: " << get_enum_name(type) << '\n'; ss <<
//                    std::string(depth + 4, ' ') << "Condition:\n" <<
//                    condition->display(depth + 8); ss << std::string(depth +
//                    4, ' ') << "Body:\n"; for(auto &stmt : body) {
//                        ss << stmt->display(depth + 8) << '\n';
//                    }
//                    return ss.str();
//                }
//        };
//
//        class IfStmt : public ControlStmt {
//            public:
//                IfStmt(Location loc, std::unique_ptr<Expression> condition,
//                std::vector<std::unique_ptr<Statement>> body) :
//                ControlStmt(loc, std::move(condition), std::move(body)) {}
//        };
//
//        class WhileStmt : public ControlStmt {
//            public:
//                WhileStmt(Location loc, std::unique_ptr<Expression>
//                condition, std::vector<std::unique_ptr<Statement>> body) :
//                ControlStmt(loc, std::move(condition), std::move(body)) {}
//        };
} // namespace statements
} // namespace jlang

#define STATEMENTS_FUNCTIONS
#include "Statements.cpp"

#endif