#include "Statements.hpp"
#include <iostream>

using namespace jlang;
using namespace jlang::statements;

bool BlockStmt::codegen(std::ostream &sink) {
    for (auto &stmt : this->get_statements()) {
        if (!stmt->codegen(sink)) {
            return false;
        }
    }
    return true;
}

bool LiteralStmt::codegen(std::ostream &sink) {
    if (this->get_type() == ExprType::INTEGER) {
        sink << "; " << this->loc.display() << ": Push int literal "
             << this->get_int_value() << '\n'
             << "push " << this->get_int_value() << '\n';
        this->parser->pushed_size += 8;
    } else if (this->get_type() == ExprType::POINTER) {
        sink << "; " << this->loc.display() << ": Push pointer literal "
             << this->get_string_value() << '\n'
             << "push " << this->get_string_value() << '\n';
        this->parser->pushed_size += 8;
    } else {
        std::cerr << "Error: Unknown literal type\n";
        return false;
    }
    return true;
}

bool IdentStmt::codegen(std::ostream &sink) {
    switch (this->get_ident_kind()) {
    case IdentType::VARIABLE: {
        auto target = std::get<Variable>(this->target);
        sink << "; " << this->loc.display() << ": Push local variable "
             << target.get_name() << '\n'
             << "mov rax, [rbp - "
             << this->parser->scope_var_offsets[target.get_name()] << "]\n"
             << "push rax\n";
        this->parser->pushed_size += target.get_size();
        break;
    }
    case IdentType::GLOBAL_VARIABLE: {
        auto target = std::get<Variable>(this->target);
        sink << "; " << this->loc.display() << ": Push global variable "
             << target.get_name() << '\n'
             << "mov rax, [" << target.get_name() << "]\n"
             << "push rax\n";
        this->parser->pushed_size += target.get_size();
        break;
    }
    case IdentType::CONSTANT: {
        auto target = std::get<Constant>(this->target);
        sink << "; " << this->loc.display() << ": Push constant "
             << target.get_name() << '\n';
        if (target.is_string()) {
            sink << "push " << target.get_string_value() << '\n';
        } else {
            sink << "push " << target.get_int_value() << '\n';
        }
        this->parser->pushed_size += target.get_size();
        break;
    }
    case IdentType::FUNCTION: {
        auto target = std::get<FunProto>(this->target);
        sink << "; " << this->loc.display() << ": Call function "
             << target.get_name() << '\n';
        // push parameters
        /// TODOO: might have to change this
        this->param->codegen(sink);
        auto param_size = this->parser->pushed_size;
        sink << "mov rbx, rsp\n"
             << "call " << target.get_name() << '\n'
             << "add rsp, " << param_size << '\n';

        if (target.get_return_type() != ExprType::NONE)
            sink << "push rax\n";
        /// TODO: Introduce return value size field in struct
        this->parser->pushed_size = 8;
        break;
    }
    default:
        std::cerr << "Error: Unknown ident type\n";
        return false;
    }
    return true;
}

bool FunStmt::codegen(std::ostream &sink) {
    size_t local_vars_size = 0;
    auto proto = this->get_proto();
    for (auto &var : proto.get_args()) {
        local_vars_size += var.get_size();
        this->parser->scope_var_offsets[var.get_name()] = local_vars_size;
    }

    sink << "; " << this->loc.display() << ": Function "
         << proto.get_name() << '\n'
         << "push rbp\n"
         << "mov rbp, rsp\n"
         << "sub rsp, " << local_vars_size << '\n';

    for (auto &param : proto.get_args()) {
        sink << "mov rax, [rbx + "
             << this->parser->scope_var_offsets[param.get_name()] - 8 << "]\n"
             << "mov [rbp - "
             << this->parser->scope_var_offsets[param.get_name()] << "], rax\n";
    }
    if (!this->body->codegen(sink)) {
        return false;
    }
    sink << ".end:\n"
         << "mov rsp, rbp\n"
         << "pop rbp\n"
         << "ret\n"
         << "; End of function " << proto.get_name() << '\n';
    return true;
}

bool IntrinsicStmt::codegen(std::ostream &sink) {
    throw std::runtime_error("Not implemented");
}

bool ControlStmt::codegen(std::ostream &sink) {
    throw std::runtime_error("Not implemented");
}

bool BinaryStmt::codegen(std::ostream &sink) {
    throw std::runtime_error("Not implemented");
}