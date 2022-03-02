#include <iostream>
#include "JlangObjects.hpp"
#include "Tokenizer.hpp"
#include "Statements.hpp"

int main(int argc, char** argv) {
    if(argv[1] == NULL){
        std::cout << "Please provide a file path\n";
        return 1;
    }
    auto tokenizer = jlang::Tokenizer(argv[1]);
    for(auto& token : tokenizer.get_tokens()){
        std::cout << token.display() << '\n';
    }
    auto token = tokenizer.get_tokens()[0];
    auto intexpr1 = std::make_unique<jlang::statements::IntLiteralExpr>(token.location, 68);
    auto intexpr2 = std::make_unique<jlang::statements::IntLiteralExpr>(token.location, 1);
    auto binaryexpr = std::make_unique<jlang::statements::BinaryExpr>(token.location, 
        jlang::ExprType::INTEGER, 
        jlang::Operator::PLUS, 
        std::move(intexpr1),
        std::move(intexpr2));
    auto args = std::vector<std::unique_ptr<jlang::statements::Expression>>();
    args.push_back(std::move(binaryexpr));
    auto calltarget = std::make_unique<jlang::statements::IntLiteralExpr>(token.location, 1);
    auto callexpr = new jlang::statements::SyscallExpr(token.location, std::move(calltarget), std::move(args));

    std::cout << callexpr->display() << '\n';
    return 0;
}
