from typing import *
from enum import Enum, auto
from dataclasses import dataclass
import ast
import io

# tuple for position in file plus it's name
compiler_current_scope: Dict[str, int] = {}
LocTuple = Tuple[str, int, int]
def format_location(loc: LocTuple) -> str:
        return f"{loc[0]}:{loc[1]}:{loc[2]}"


__abi_regs: List[str] = [
    "rdi",
    "rsi",
    "rdx",
    "r10",
    "r9"
]

def get_abi_reg_name(argnum: int) -> str:
    if argnum > len(__abi_regs):
        return "stack-reverse"
    else:
        return __abi_regs[argnum]
    

class Keyword(Enum):
    # controllers are used to control the flow of the program
    IF = auto()
    WHILE = auto()
    # designators are used for the definition of variables and functions
    FUNCTION = auto()
    DEFINE = auto()
    # particles are used for the grammar to recognize the end of a grammar block
    DO = auto()
    IS = auto()
    AS = auto()
    TO = auto()
    YIELDS = auto()
    DONE = auto()
    # invokers are used to invoke functions or syscalls
    PRINT = auto()
    SYSCALL0 = auto()
    SYSCALL1 = auto()
    SYSCALL2 = auto()
    SYSCALL3 = auto()
    SYSCALL4 = auto()
    SYSCALL5 = auto(),
    # other manipulators
    DROP = auto()
    RETURN = auto()

valid_syscalls: List[Keyword] = [
    Keyword.SYSCALL0,
    Keyword.SYSCALL1,
    Keyword.SYSCALL2,
    Keyword.SYSCALL3,
    Keyword.SYSCALL4,
    Keyword.SYSCALL5
]

assert len(Keyword) == 19, "Too many Keywords defined"
KEYWORDS_BY_NAME: Dict[str, Keyword] = {
    keyword.name.lower(): keyword for keyword in Keyword
}


class TokenType(Enum):
    KEYWORD = auto()                    # Keywords are basically all used for statements except for the syscall keyword
    IDENTIFIER = auto()                 # identifiers for variables and functions
    INT_LITERAL = auto()                # number literals
    STRING_LITERAL = auto()             # string literals
    MANIPULATOR = auto()                # Things that manipulate values on the stack (consume or produce)
    EOE = auto()                        # End of expression
    PAREN_BLOCK_START = auto()          # Start of parenthesis block
    PAREN_BLOCK_END = auto()            # End of parenthesis block
    ARG_DELIMITER = auto()              # Argument delimiter
    TYPE = auto()                       # Type name

assert len(TokenType) == 10, "Too many TokenTypes defined"
TOKENTYPE_BY_NAME: Dict[str, TokenType] = {
    TokenType.name.lower(): TokenType for TokenType in TokenType
}

class IdentType(Enum):
    VARIABLE = auto()
    GLOBAL_VARIABLE = auto()
    FUNCTION = auto()

assert len(IdentType) == 3, "Too many IdentTypes defined"
IDENTTYPE_BY_NAME: Dict[str, IdentType] = {
    IdentType.name.lower(): IdentType for IdentType in IdentType
}

class Manipulator(Enum):
    PLUS = auto()
    MINUS = auto()
    MULTIPLY = auto()
    DIVIDE = auto()
    MODULO = auto()
    GREATER = auto()
    LESS = auto()
    EQUAL = auto()
    NOT_EQUAL = auto()
    GREATER_EQUAL = auto()
    LESS_EQUAL = auto()

assert len(Manipulator) == 11, "Too many ManipulatorTypes defined"
MANIPULATOR_BY_NAME: Dict[str, Manipulator] = {
    #manipulator.name.lower(): manipulator for manipulator in Manipulator
    "plus": Manipulator.PLUS,
    "minus": Manipulator.MINUS,
    "multiply": Manipulator.MULTIPLY,
    "divide": Manipulator.DIVIDE,
    "modulo": Manipulator.MODULO,
    "greater": Manipulator.GREATER,
    "less": Manipulator.LESS,
    "equal": Manipulator.EQUAL,
    "not-equal": Manipulator.NOT_EQUAL,
    "greater-equal": Manipulator.GREATER_EQUAL,
    "less-equal": Manipulator.LESS_EQUAL
}

BINOP_PRECEDENCE: Dict[str, int] = {
    "plus": 20,
    "minus": 20,
    "multiply": 30,
    "divide": 30,
    "modulo": 30,
    "greater": 10,
    "less": 10,
    "equal": 10,
    "not-equal": 10,
    "greater-equal": 10,
    "less-equal": 10
}

class ExprType(Enum):
    NONE = auto()
    INTEGER = auto()
    POINTER = auto()

assert len(ExprType) == 3, "Too many ExprTypes defined"
EXPRTYPE_BY_NAME: Dict[str, ExprType] = {
    exprtype.name.lower(): exprtype for exprtype in ExprType
}

@dataclass
class Token:
    type: TokenType
    text: str
    location: LocTuple
    value: Optional[Union[int, str, Keyword, Manipulator, ExprType]] = None

    def __str__(self):
        if isinstance(self.value, str):

            return f"{format_location(self.location)} {self.type.name} {self.text.encode('utf-8')} {self.value.encode('utf-8')}"
        else:
            return f"{format_location(self.location)} {self.type.name} {self.text} {self.value}"

# Statements don't have a type
class Statement:
    def __init__(self, token: Token, type: ExprType = ExprType.NONE):
        self.token = token
        self.type = type
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Statement Type: {self.type.name}")
        print(f"{' ' * depth}Token: {self.token}")

    def codegen(self, sink: io.StringIO):
        raise NotImplementedError(f"Code generation has not been implemented for {type(self).__name__}")

class Expression(Statement):
    def __init__(self, token: Token, value: Union['Expression', int, str], type: ExprType):
        super().__init__(token, type) # TODO: Implement type checking
        self.value = value
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Expression Type: {self.type.name}")
        print(f"{' ' * depth}Token: {self.token}")
        if isinstance(self.value, Expression):
            print("Value:")
            self.value.print(depth + 4)
        else:
            print(f"{' ' * depth}Value: {self.value}")
        

    def codegen(self, sink: io.StringIO):
        if isinstance(self.value, Expression):
            self.value.codegen(sink)
        else:
            sink.write(f"{self.value}")


class IntLiteralExpr(Expression):
    def __init__(self, token: Token, value: int):
        super().__init__(token, value, ExprType.INTEGER)
        self.type

    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} push int literal {self.value}\n")
        sink.write(f"push {self.value}\n")

class StringLiteralExpr(Expression):
    def __init__(self, token: Token, value: str):
        super().__init__(token, value, ExprType.POINTER)

    def print(self, depth: int = 0):
        assert isinstance(self.value, str), "String literal value must be a string string"
        print(f"{' ' * depth}String Literal")
        print(f"{' ' * depth}Token: {self.token}")
        print(f"{' ' * depth}Value: {self.value}")

    def codegen(self, sink: io.StringIO):
        assert isinstance(self.value, str), "String literal must be a string"
        formatted_str = self.value.replace('\n', '\\n')
        sink.write(f"; {format_location(self.token.location)} push string literal {formatted_str}\n")
        sink.write(f"mov rax, {self.value}\n")
        sink.write("push rax\n")


class IdentRefExpr(Expression):
    def __init__(self, token: Token, name: str, ident_kind: IdentType, type: ExprType):
        super().__init__(token, name, type)
        self.ident_kind = ident_kind
        
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Identifier Reference Type: {self.type.name}")
        print(f"{' ' * depth}Token: {self.token}")
        print(f"{' ' * depth}Name: {self.value}")

    def codegen(self, sink: io.StringIO):
        if self.ident_kind == IdentType.VARIABLE:
            assert isinstance(self.value, str), "Variable name must be a string"
            sink.write(f"; {format_location(self.token.location)} get variable {self.value}\n")
            sink.write(f"mov rax, [rbp - {compiler_current_scope[self.value]}]\n")
            sink.write("push rax\n")
        elif self.ident_kind == IdentType.GLOBAL_VARIABLE:
            sink.write(f"; {format_location(self.token.location)} get global variable {self.value}\n")
            sink.write(f"mov rax, [{self.value}]\n")
            sink.write("push rax\n")
        else:
            raise ValueError(f"Invalid Identifier found for {self.value}")
    

class BinaryExpr(Expression):
    def __init__(self, token: Token, left: Expression, right: Expression):
        super().__init__(token, left, ExprType.INTEGER)
        self.right = right
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Expression Type: {self.type.name}")
        print(f"{' ' * depth}Token: {self.token}")
        print(f"{' ' * depth}Left:")
        assert isinstance(self.value, Expression), "Left of Binary Expression must be an Expression"
        self.value.print(depth + 4)
        print(f"{' ' * depth}Right:")
        assert isinstance(self.right, Expression), "Right of Binary Expression must be an Expression"
        self.right.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        assert isinstance(self.value, Expression) and isinstance(self.right, Expression), "Binary expressions must have expressions as their left and right values"
        self.value.codegen(sink)
        self.right.codegen(sink)
        #sink.write(f"; {format_location(self.token.location)}: Binary Expression\n")
        
        if self.token.value == Manipulator.PLUS:
            sink.write(f"; {format_location(self.token.location)} Plus\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("add rax, rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Manipulator.MINUS:
            sink.write(f"; {format_location(self.token.location)} Minus\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("sub rax, rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Manipulator.MULTIPLY:
            sink.write(f"; {format_location(self.token.location)} Multiply\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("imul rax, rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Manipulator.DIVIDE:
            sink.write(f"; {format_location(self.token.location)} Divide\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cqo\n")
            sink.write("idiv rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Manipulator.MODULO:
            sink.write(f"; {format_location(self.token.location)} Modulo\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cqo\n")
            sink.write("div rdi\n")
            sink.write("push rdx\n")
        elif self.token.value == Manipulator.EQUAL:
            sink.write(f"; {format_location(self.token.location)} Equal\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmove rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Manipulator.NOT_EQUAL:
            sink.write(f"; {format_location(self.token.location)} Not Equal\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovne rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Manipulator.LESS:
            sink.write(f"; {format_location(self.token.location)} Less Than\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovl rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Manipulator.LESS_EQUAL:
            sink.write(f"; {format_location(self.token.location)} Less Than or Equal\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovle rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Manipulator.GREATER:
            sink.write(f"; {format_location(self.token.location)} Greater Than\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovg rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Manipulator.GREATER_EQUAL:
            sink.write(f"; {format_location(self.token.location)} Greater Than or Equal\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovge rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.type == TokenType.EOE:
            sink.write(f"; {format_location(self.token.location)} End of Expression\n")
        else:
            raise ValueError(f"Unknown binary operator {self.token.value} at {format_location(self.token.location)}")

class SyscallExpr(Expression): 
    def __init__(self, token: Token, calltype: Keyword, callnum: int, args: List[Expression] = []):
        if calltype not in valid_syscalls:
            raise Exception(f"{calltype} is not a valid Syscall type")
        self.calltype = calltype
        super().__init__(token, callnum, ExprType.INTEGER)
        self.type = ExprType.INTEGER
        self.args = args
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}System Call: {self.calltype}")
        print(f"{' ' * depth}Call Number: {self.value}")
        print(f"{' ' * depth}Arguments:")
        for arg in self.args:
            arg.print(depth + 4)
        
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {self.token} System Call\n")
        for i, arg in enumerate(self.args):
            arg.codegen(sink)
            sink.write(f"pop {get_abi_reg_name(i)}\n")
        # mov rax last, as it's used to push/pop
        self.value.codegen(sink)
        sink.write("pop rax\n")
        sink.write("syscall\n")
        sink.write("push rax\n")


class DropStmt(Statement):
    def __init__(self, token: Token, expr: Expression):
        super().__init__(token)
        self.expr = expr
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Drop Statement")
        print(f"{' ' * depth}Dropped Expression:")
        self.expr.print(depth + 4)

    def codegen(self, sink: io.StringIO):
        sink.write("; Drop Statement\n")
        self.expr.codegen(sink)
        sink.write("pop rax\n")



class VarDefStmt(Statement):
    def __init__(self, token: Token, name: str, var_type: IdentType, type: ExprType,value = None):
        super().__init__(token, type)
        self.name = name
        self.value = value
        self.var_type = var_type
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}VarDefStmt: {self.name}")
        print(f"{' ' * depth}Value:")
        if self.value is None:
            print(f"{' ' * depth}None")
        else:
            self.value.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        assert len(IdentType) == 3, "Too many IdentTypes defined"
        if self.var_type == IdentType.GLOBAL_VARIABLE:  # TODO: evaluate global variables at compile time
            if self.value is not None:
                sink.write(f"; {format_location(self.token.location)}: Variable Definition\n")
                self.value.codegen(sink)
                sink.write(f"pop rax\n")
                sink.write(f"mov [{self.name}], rax\n")
        elif self.var_type == IdentType.VARIABLE:
            if self.value is not None:
                sink.write(f"; {format_location(self.token.location)}: Variable Definition\n")
                self.value.codegen(sink)
                sink.write(f"pop rax\n")
                sink.write(f"mov [rbp - {compiler_current_scope[self.name]}], rax\n")
        else:
            raise ValueError("Unexpected identifier type found")

# TODO: maybe reimplement this correctly later on 
class FunProto(Statement):
    def __init__(self, token: Token, name: str, arguments: Dict[str, VarDefStmt], ret_type: ExprType):
        super().__init__(token, ret_type)
        self.name: str = name
        self.arguments: Dict[str,VarDefStmt]  = arguments
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Function Prototype: {self.name}")
        print(f"{' ' * depth}Parameters: {self.arguments if len(self.arguments) > 0 else 'None'}")

class FunCallExpr(Expression):
    def __init__(self, token: Token, target: IdentRefExpr, args: List[Expression]):
        #if len(args) > 0:
        #    raise ValueError(f"Function calls cannot have arguments yet")
        
        super().__init__(token, target, target.type)
        self.args: List[Expression] = args

    def print(self, depth: int = 0):
        print(f"{' ' * depth}Function Call: {self.value.value}")
        print(f"{' ' * depth}Parameters: {self.args if len(self.args) > 0 else 'None'}")
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} Function Call\n")
        
        # push arguments in reverse order
        
        for arg in reversed(self.args):
            arg.codegen(sink)

        # tell the function where the stack variables are located
        sink.write(f"mov rbx, rsp\n")
        sink.write(f"call {self.value.value}\n")

        # realign stack
        sink.write(f"add rsp, {len(self.args) * 8}\n")

        sink.write(f"push rax\n") # rax will hold the return value


class VarSetStmt(Statement):
    def __init__(self, token: Token, target: str, value):
        super().__init__(token, ExprType.NONE)
        self.target = target
        self.value = value
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Set Variable: {self.name}")
        print(f"{' ' * depth}Value:")
        self.value.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} Set Variable {self.target}")
        if self.var_type == IdentType.GLOBAL_VARIABLE:  # TODO: evaluate global variables at compile time
            if self.value is not None:
                self.value.codegen(sink)
                sink.write(f"pop rax\n")
                sink.write(f"mov [{self.name}], rax\n")
        elif self.var_type == IdentType.VARIABLE:
            if self.value is not None:
                self.value.codegen(sink)
                sink.write(f"pop rax\n")
                sink.write(f"mov [rbp - {compiler_current_scope[self.name]}], rax\n")
        else:
            raise ValueError("Unexpected identifier type found")

class PrintStmt(Statement):
    def __init__(self, token: Token, value: Expression):
        super().__init__(token, ExprType.NONE)
        self.value = value

    def print(self, depth: int = 0):
        print(f"Token: {self.token.value}")
        print(f"Type: {self.type}")
        print(f"Value:")
        self.value.print(depth + 4)

    def codegen(self, sink: io.StringIO):
        self.value.codegen(sink)
        sink.write(f"; {format_location(self.token.location)} Print \n")
        sink.write(f"pop rdi\n")
        sink.write(f"call print\n")


class FunStmt(Statement):
    def __init__(self, proto: FunProto, block: List[Statement], scope: Dict[str, VarDefStmt], type: ExprType):
        super().__init__(proto.token, type)
        self.proto: FunProto = proto
        self.block: List[Statement] = block
        self.scope: Dict[str, VarDefStmt] = scope

    def print(self, depth: int = 0):
        print(f"{' ' * depth}Function Expression: {self.proto.name}")
        print(f"{' ' * depth}Parameters: ")
        if len(self.scope) > 0:
            for var in self.scope.values():
                var.print(depth + 4)
        else:
            print(f"{' ' * (depth + 4)}None")

        print(f"{' ' * depth}Block:")
        for expr in self.block:
            if expr is None:
                print(f"{' ' * depth}None")
            else:
                expr.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        i = 8
        
        # arguments have been inserted into the scope already
        for var_name in self.scope:
            # TODO: adjust size to variable type
            compiler_current_scope[var_name] = i
            i += 8

        sink.write(f"; Function Definition {self.proto.name}\n")
        sink.write(f"{self.proto.name}:\n")

        sink.write("push rbp\n")
        sink.write("mov rbp, rsp\n")

        #make space for variables on stack (rbp)
        if len(self.scope) > 0:
            sink.write(f"sub rsp, {len(self.scope) * 8 }\n")

        # arguments are now on stack
        # the stack grows downwards, meaning that the first argument is at the top of the stack, the second is at the top of the stack minus 8, etc.
        # rbx contains the callee stack variables
        # transfer arguments to local variables
        for param in self.proto.arguments.values():
            sink.write(f"mov rax, [rbx + {compiler_current_scope[param.name] - 8}]\n")
            sink.write(f"mov [rbp - {compiler_current_scope[param.name]}], rax\n")

        
        for stmt in self.block:
            stmt.codegen(sink)

        sink.write(f".end:\n")
        sink.write("mov rsp, rbp\n")
        sink.write("pop rbp\n")
        sink.write("ret\n")
        sink.write(f"; End of Function {self.proto.name}\n\n")

        # clear the scope for the next function
        compiler_current_scope.clear()

class ControlStmt(Statement):
    def __init__(self, token: Token, condition: Expression, block: List[Statement]):
        super().__init__(token)
        self.condition: Expression = condition
        self.block: List[Statement] = block
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Control Statement: {self.type}")
        print(f"{' ' * depth}Condition:")
        self.condition.print(depth + 4)
        print(f"{' ' * depth}Block:")
        for stmt in self.block:
            stmt.print(depth + 4)

class IfStmt(ControlStmt):
    def __init__(self, token: Token, condition: Expression, block: List[Statement]):
        super().__init__(token, condition, block)

    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} If block\n")
        # use location to name the label
        label_base = f"l{self.token.location[1]}_c{self.token.location[2]}"

        self.condition.codegen(sink) # condition
        
        sink.write(f".if_cmp_{label_base}:\n")
        sink.write("pop rax\n")
        sink.write("cmp rax, 0\n")
        sink.write(f"je .if_block_end_{label_base}\n")
        sink.write(f".if_block_{label_base}:\n")

        # create sink to capture the code for the if block
        #if_sink = io.StringIO()
        for stmt in self.block:
            stmt.codegen(sink)

        sink.write(f".if_block_end_{label_base}:\n")

class WhileStmt(ControlStmt):
    def __init__(self, token: Token, condition: Expression, block: List[Statement]):
        super().__init__(token, condition, block)
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} While block\n")
        # use location to name the label
        label_base = f"l{self.token.location[1]}_c{self.token.location[2]}"

        sink.write(f".while_cmp_{label_base}:\n")
        self.condition.codegen(sink)
        sink.write("pop rax\n")
        sink.write("cmp rax, 0\n")
        sink.write(f"je .while_end_{label_base}\n")
        sink.write(f".while_block_{label_base}:\n")
        for stmt in self.block:
            stmt.codegen(sink)
        sink.write(f"jmp .while_cmp_{label_base}\n")
        sink.write(f".while_end_{label_base}:\n")

class ReturnStmt(Statement):
    def __init__(self, token: Token, value: Expression):
        super().__init__(token, ExprType.NONE)
        self.value = value

    def print(self, depth: int = 0):
        print(f"{' ' * depth}Return Statement")
        print(f"{' ' * depth}Value: {self.value}")
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} Return Statment\n")
        self.value.codegen(sink)
        sink.write("pop rax\n")
        sink.write("jmp .end\n")