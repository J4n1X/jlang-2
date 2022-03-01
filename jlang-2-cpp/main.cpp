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
    auto token = jlang::Token(jlang::TokenType::INT_LITERAL, "69", jlang::Location(argv[1], 0, 0), jlang::TokenValue("69"));
    jlang::statements::ArrayRefExpr expression = jlang::statements::ArrayRefExpr(token, "69");
    auto ident_ref = jlang::statements::IdentRefExpr(token, "69", jlang::IdentType::VARIABLE, jlang::ExprType::INTEGER);
    auto super_expression = jlang::statements::LoaderExpr(token, ident_ref);
    std::cout << super_expression.display() << '\n';
    std::cout << ident_ref.display() << '\n';
    return 0;
}
