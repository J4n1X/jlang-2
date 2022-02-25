import os
import sys
import io
from typing import *
from enum import Enum, auto


from ExpressionParser import ExpressionParser
from JlangObjects import *
from Tokenizer import Tokenizer
from TypeChecker import TypeChecker

class Program:
    def __init__(self, filename: str, dump_ast: bool = False, dump_tokens: bool = False, dump_functions: bool = False, dump_globals: bool = False):
        self.filename: str = filename
        self.output_name: str = filename.replace(".j", ".asm")
        self.tokens: List[Token] = Tokenizer(filename).tokens
        self.parser: ExpressionParser = ExpressionParser(self.tokens)
        self.dump_ast: bool = dump_ast
        self.dump_tokens: bool = dump_tokens
        self.dump_functions: bool = dump_functions
        self.dump_globals: bool = dump_globals

    def generate_program(self):
        if self.dump_tokens:
            print("--------------------------------")
            print("Tokens:\n")
            for token in self.parser.tokens:
                print(token)
        

        AST = self.parser.parse_program()
        if AST is None:
            raise Exception("Failed to parse program")
        
        checker = TypeChecker(AST.copy(), self.parser.prototypes.copy())
        checker.parse_program()
        checker.print_state()

        if self.dump_functions:
            print("--------------------------------")
            print("Function table:\n")
            for proto in self.parser.prototypes.values():
                print(proto.name)

        if self.dump_globals:
            print("--------------------------------")
            print("Global Variables:\n")
            for var in self.parser.global_vars.values():
                print(var.name)

        if self.dump_ast:
            print("--------------------------------")
            print("Generated AST:\n")
            for expr in AST:
                expr.print(0)


        if "main" not in self.parser.prototypes:
            raise Exception("No main function found")

        with open(self.output_name, "w+") as out:
            out.write("BITS 64\n")
            out.write("segment .text\n")
            out.write("print:\n")
            out.write("    mov     r9, -3689348814741910323\n")
            out.write("    sub     rsp, 40\n")
            out.write("    mov     BYTE [rsp+31], 10\n")
            out.write("    lea     rcx, [rsp+30]\n")
            out.write(".L2:\n")
            out.write("    mov     rax, rdi\n")
            out.write("    lea     r8, [rsp+32]\n")
            out.write("    mul     r9\n")
            out.write("    mov     rax, rdi\n")
            out.write("    sub     r8, rcx\n")
            out.write("    shr     rdx, 3\n")
            out.write("    lea     rsi, [rdx+rdx*4]\n")
            out.write("    add     rsi, rsi\n")
            out.write("    sub     rax, rsi\n")
            out.write("    add     eax, 48\n")
            out.write("    mov     BYTE [rcx], al\n")
            out.write("    mov     rax, rdi\n")
            out.write("    mov     rdi, rdx\n")
            out.write("    mov     rdx, rcx\n")
            out.write("    sub     rcx, 1\n")
            out.write("    cmp     rax, 9\n")
            out.write("    ja      .L2\n")
            out.write("    lea     rax, [rsp+32]\n")
            out.write("    mov     edi, 1\n")
            out.write("    sub     rdx, rax\n")
            out.write("    xor     eax, eax\n")
            out.write("    lea     rsi, [rsp+32+rdx]\n")
            out.write("    mov     rdx, r8\n")
            out.write("    mov     rax, 1\n")
            out.write("    syscall\n")
            out.write("    add     rsp, 40\n")
            out.write("    ret\n")

            for expr in AST:
                expr.codegen(out)

            out.write("\n\nglobal _start\n")
            out.write("_start:\n")

            out.write("\n\nglob_var_defs:\n")
            for var in self.parser.global_vars.values():
                var.codegen(out)

            out.write("\ncall main\n")
            out.write("push rax\n")
            # TODO: last number on stack should be the return value
            out.write("; exit\n")
            out.write("mov rax, 60\n")
            #out.write("mov rdi, 0\n")
            out.write("pop rdi\n")
            out.write("syscall\n")

            if len(self.parser.global_const_vars) > 0:
                out.write("\n\nsegment .data\n")
                for index, s in enumerate(self.parser.global_const_vars):
                    out.write("_anon_str_%d: db %s,0\n" % (index, ','.join(map(str, list(map(ord, s))))))
            
            if len(self.parser.constants) > 0:
                for const in self.parser.constants.values():
                    out.write(f"{const.name}: dq {const.value}\n")

            if len(self.parser.global_vars) > 0:
                out.write("\n\nsegment .bss\n")
                for var in self.parser.global_vars.values():
                    out.write(f"{var.name}: resb {var.size}\n")


        print(f"Program successfully generated to {self.output_name}")
        nasm_output = os.popen(f"nasm -f elf64 -g {self.output_name}")
        if nasm_output.close() is not None:
            print("Error while running nasm:")
            print(nasm_output.read())
            return
        print("Generated object file")


        ld_output = os.popen(f"ld -m elf_x86_64 -o {self.output_name.replace('.asm', '.exe')} {self.output_name.replace('.asm', '.o')}")
        if ld_output.close() is not None:
            print("Error while running ld:")
            print(ld_output.read())
            return
        print("Generated executable")
        



def main():
    if len(sys.argv) < 2:
        print("Usage: jlang.py <filename> [--dump-ast] [--dump-tokens] [--dump-functions] [--dump-globals]")
        return

    program = Program( \
        sys.argv[1], \
        "--dump-ast" in sys.argv, \
        "--dump-tokens" in sys.argv, \
        "--dump-functions" in sys.argv, \
        "--dump-globals" in sys.argv \
    )   
    program.generate_program()

if __name__ == "__main__":
    main()