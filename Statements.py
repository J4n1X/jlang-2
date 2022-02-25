from JlangObjects import *
import io

#region Generic Classes

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
    def __init__(self, token: Token, value: Union['Expression', int, str, List['Expression']], type: ExprType, size: Optional[int] = None):
        super().__init__(token, type) # TODO: Implement type checking
        self.value = value
        if size is None:
            # get it from the type
            self.size = SIZE_OF_EXPRTYPES[type]
        else:
            self.size = size
    
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

#endregion

#region Expressions

class IntLiteralExpr(Expression):
    def __init__(self, token: Token, value: int):
        super().__init__(token, value, ExprType.INTEGER)
        self.type

    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} push int literal {self.value}\n")
        sink.write(f"push {self.value}\n")

class ArrayRefExpr(Expression):
    def __init__(self, token: Token, value: str):
        super().__init__(token, value, ExprType.POINTER)

    def print(self, depth: int = 0):
        assert isinstance(self.value, str), "Array literal value must be a string"
        print(f"{' ' * depth}String Literal")
        print(f"{' ' * depth}Token: {self.token}")
        print(f"{' ' * depth}Value: {self.value}")

    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} push array ptr {self.value}\n")
        if self.value in compiler_current_scope: # this must be a local anonymous variable
            sink.write(f"lea rax, [rbp - {compiler_current_scope[self.value]}]\n")    
        else:
            assert isinstance(self.value, str), "String literal must be a string"
            sink.write(f"mov rax, {self.value}\n")
        sink.write("push rax\n")

class LoaderExpr(Expression):
    def __init__(self, token: Token, target: Expression):
        super().__init__(token, target,ExprType.INTEGER)

    def print(self, depth: int = 0):
        print(f"{' ' * depth}Loader {self.token.value}")
        print(f"{' ' * depth}Token: {self.token}")
        print(f"{' ' * depth}Target:")
        self.value.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        loader_type = self.token.value
        assert isinstance(loader_type, Intrinsic), "Expected Loader type to be Intrinsic"
        
        sized_keyword = AsmInfo.mem_size_keywords[Intrinsic.get_sized_index(loader_type)]
        sized_register = AsmInfo.registers["rax"][Intrinsic.get_sized_index(loader_type)]

        sink.write(f"; {format_location(self.token.location)} Loader {self.token.value}\n")
        self.value.codegen(sink)
        
        # push the large register for consistency
        sink.write("xor rax, rax\n")
        sink.write("pop rdi\n")
        sink.write(f"mov {sized_register}, {sized_keyword}[rdi]\n")
        sink.write(f"push rax\n")


class IdentRefExpr(Expression):
    def __init__(self, token: Token, name: str, ident_kind: IdentType, type: ExprType):
        super().__init__(token, name, type)
        self.ident_kind = ident_kind
        
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Identifier Reference Type: {self.type}")
        print(f"{' ' * depth}Token: {self.token}")
        print(f"{' ' * depth}Identifier Kind: {self.ident_kind}")
        print(f"{' ' * depth}Name: {self.value}")

    def codegen(self, sink: io.StringIO):
        if self.ident_kind == IdentType.VARIABLE:
            assert isinstance(self.value, str), "Variable name must be a string"
            sink.write(f"; {format_location(self.token.location)} get variable {self.value}\n")
            sink.write(f"mov rax, [rbp - {compiler_current_scope[self.value]}]\n")
            sink.write("push rax\n")
        elif self.ident_kind == IdentType.GLOBAL_VARIABLE:
            sink.write(f"; {format_location(self.token.location)} get global variable {self.value}\n")
            sink.write(f"mov rax, QWORD [{self.value}]\n")
            sink.write("push rax\n")
        elif self.ident_kind == IdentType.CONSTANT:
            sink.write(f"; {format_location(self.token.location)} get constant {self.value}\n")
            sink.write(f"mov rax, QWORD [{self.value}]\n")
            sink.write("push rax\n")
        else:
            raise ValueError(f"Invalid Identifier found for {self.value}")

# we take the entire IdentRefExpr for type checking later
class AddressOfExpr(Expression):
    def __init__(self, token: Token, target: IdentRefExpr):
        super().__init__(token, target, ExprType.POINTER)
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}AddressOf {self.token.value}")
        print(f"{' ' * depth}Token: {self.token}")
        print(f"{' ' * depth}Target:")
        self.value.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        assert isinstance(self.value, IdentRefExpr), "AddressOf must be an IdentRefExpr"
        sink.write(f"; {format_location(self.token.location)} AddressOf {self.token.value}\n")
        if self.value.ident_kind == IdentType.VARIABLE:
            sink.write(f"lea rax, [rbp - {compiler_current_scope[self.value.value]}]\n")
        elif self.value.ident_kind == IdentType.GLOBAL_VARIABLE:
            sink.write(f"mov rax, {self.value.value}\n") # value is name
        elif self.value.ident_kind == IdentType.CONSTANT: 
            sink.write(f"mov rax, {self.value.value}\n") # value is name
        else:
            raise ValueError(f"Invalid Identifier found for {self.value.value}")
        sink.write("push rax\n")
   

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
        
        if self.token.value == Operator.PLUS:
            sink.write(f"; {format_location(self.token.location)} Plus\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("add rax, rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Operator.MINUS:
            sink.write(f"; {format_location(self.token.location)} Minus\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("sub rax, rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Operator.MULTIPLY:
            sink.write(f"; {format_location(self.token.location)} Multiply\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("imul rax, rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Operator.DIVIDE:
            sink.write(f"; {format_location(self.token.location)} Divide\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cqo\n")
            sink.write("idiv rdi\n")
            sink.write("push rax\n")
        elif self.token.value == Operator.MODULO:
            sink.write(f"; {format_location(self.token.location)} Modulo\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cqo\n")
            sink.write("div rdi\n")
            sink.write("push rdx\n")
        elif self.token.value == Operator.EQUAL:
            sink.write(f"; {format_location(self.token.location)} Equal\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmove rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Operator.NOT_EQUAL:
            sink.write(f"; {format_location(self.token.location)} Not Equal\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovne rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Operator.LESS:
            sink.write(f"; {format_location(self.token.location)} Less Than\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovl rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Operator.LESS_EQUAL:
            sink.write(f"; {format_location(self.token.location)} Less Than or Equal\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovle rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Operator.GREATER:
            sink.write(f"; {format_location(self.token.location)} Greater Than\n")
            sink.write("xor rcx, rcx\n")
            sink.write("mov rbx, 1\n")
            sink.write("pop rdi\n")
            sink.write("pop rax\n")
            sink.write("cmp rax, rdi\n")
            sink.write("cmovg rcx, rbx\n")
            sink.write("push rcx\n")
        elif self.token.value == Operator.GREATER_EQUAL:
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

class CallExpr(Expression):
    def __init__(self, token: Token, args: List[Expression], target_type: ExprType):
        super().__init__(token, args, target_type)
    
    def print(self, indent: int = 0):
        print(f"{' ' * indent}Call Expression")
        self.target.print(indent + 2)
        for arg in self.value:
            arg.print(indent + 2)

class SyscallExpr(CallExpr): 
    def __init__(self, token: Token, calltype: Keyword, callnum: Expression, args: List[Expression] = []):
        if calltype not in Syscall:
            raise Exception(f"{calltype} is not a valid Syscall type")
        
        super().__init__(token, args, ExprType.INTEGER)
        self.calltype = calltype
        self.callnum = callnum
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}System Call: {self.calltype}")
        print(f"{' ' * depth}Call Number: {self.value}")
        print(f"{' ' * depth}Arguments:")
        for arg in self.value:
            arg.print(depth + 4)
        
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {self.token} System Call\n")
        for arg in self.value:
            arg.codegen(sink)
        
        # retrieve the values from the stack
        for i in reversed(range(len(self.value))):
            sink.write(f"pop {AsmInfo.get_abi_reg_name(i)}\n")

        # mov rax last, as it's used to push/pop
        self.callnum.codegen(sink)
        sink.write("pop rax\n")
        sink.write("syscall\n")
        sink.write("push rax\n")

class ConstantExpr(Expression):
    def __init__(self, token: Token, value: Union[str, int], type: ExprType):
        super().__init__(token, value, type)
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Constant: {self.value}")
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {self.token} Constant\n")
        sink.write(f"push {self.value}\n")

#endregion

#region Statements

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

#region Variable and Memory Manipulation Statments

class VarDefStmt(Statement):
    def __init__(self, token: Token, name: str, var_type: IdentType, type: ExprType, size: int, value = None):
        super().__init__(token, type)
        self.name = name
        self.value = value
        self.var_type = var_type
        self.size = size
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}VarDefStmt: {self.name}")
        print(f"{' ' * depth}Value:")
        if self.value is None:
            print(f"{' ' * depth}None")
        else:
            self.value.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        assert len(IdentType) == 4, "Too many IdentTypes defined"
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

class VarSetStmt(Statement):
    def __init__(self, token: Token, target: str, var_type: IdentType, value: Expression):
        super().__init__(token, ExprType.NONE)
        self.target = target
        self.value = value
        self.var_type = var_type
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Set Variable: {self.target}")
        print(f"{' ' * depth}Value:")
        self.value.print(depth + 4)
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} Set Variable {self.target}")
        if self.var_type == IdentType.GLOBAL_VARIABLE:  # TODO: evaluate global variables at compile time
            if self.value is not None:
                self.value.codegen(sink)
                sink.write(f"pop rax\n")
                sink.write(f"mov [{self.target}], rax\n")
        elif self.var_type == IdentType.VARIABLE:
            if self.value is not None:
                self.value.codegen(sink)
                sink.write(f"pop rax\n")
                sink.write(f"mov [rbp - {compiler_current_scope[self.target]}], rax\n")
        else:
            raise ValueError(f"Unexpected identifier type found: {self.var_type}")

class StorerStmt(Statement):
    def __init__(self, token: Token, target: Expression, value: Expression):
        super().__init__(token, ExprType.INTEGER)
        self.target = target
        self.value = value
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Storer Statement")
        print(f"{' ' * depth}Target:")
        self.target.print(depth + 4)
        print(f"{' ' * depth}Value:")
        self.value.print(depth + 4)

    def codegen(self, sink: io.StringIO):
        storer_type = self.token.value
        assert isinstance(storer_type, Intrinsic), "Expected Storer type to be Intrinsic"
        
        sized_keyword = AsmInfo.mem_size_keywords[Intrinsic.get_sized_index(storer_type)]
        sized_register = AsmInfo.registers["rax"][Intrinsic.get_sized_index(storer_type)]
                
        sink.write(f"; {format_location(self.token.location)} Storer Statement\n")
        self.target.codegen(sink)
        self.value.codegen(sink)
        sink.write("pop rax\n")
        sink.write("pop rdi\n")
        sink.write(f"mov {sized_keyword} [rdi], {sized_register}\n") # for example mov BYTE [rdi], al
        

#endregion Variable and Memory Manipulation Statments

# TODO: maybe reimplement this correctly later on 
class FunProto(Statement):
    def __init__(self, token: Token, name: str, arguments: Dict[str, VarDefStmt], ret_type: ExprType):
        super().__init__(token, ret_type)
        self.name: str = name
        self.args: Dict[str,VarDefStmt]  = arguments
    
    def print(self, depth: int = 0):
        print(f"{' ' * depth}Function Prototype: {self.name}")
        print(f"{' ' * depth}Parameters:")
        if len(self.args) > 0:
            for argument in self.args.values():
                argument.print(depth + 4)
        else:
            print(f"{' ' * depth + 4}None")
            

class FunCallExpr(CallExpr):
    def __init__(self, token: Token, target: IdentRefExpr, args: List[Expression]):
        #if len(args) > 0:
        #    raise ValueError(f"Function calls cannot have arguments yet")
        
        super().__init__(token, args, target.type)
        self.target = target

    def print(self, depth: int = 0):
        print(f"{' ' * depth}Function Call: {self.target.value}")
        print(f"{' ' * depth}Parameters:")
        if len(self.value) > 0:
            for arg in self.value:
                arg.print(depth + 4)
        else:
            print(f"{' ' * depth + 4}None")
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} Function Call\n")
        
        # push arguments in reverse order
        
        for arg in reversed(self.value):
            arg.codegen(sink)

        # tell the function where the stack variables are located
        sink.write(f"mov rbx, rsp\n")
        sink.write(f"call {self.target.value}\n")

        # realign stack
        args_size = 0
        for arg in self.value:
            args_size += arg.size

        sink.write(f"add rsp, {args_size}\n")

        if self.type != ExprType.NONE:
            sink.write(f"push rax\n")

class PrintStmt(Statement):
    def __init__(self, token: Token, value: Expression):
        super().__init__(token, ExprType.NONE)
        self.expr = value

    def print(self, depth: int = 0):
        print(f"Token: {self.token.value}")
        print(f"Type: {self.type}")
        print(f"Value:")
        self.expr.print(depth + 4)

    def codegen(self, sink: io.StringIO):
        self.expr.codegen(sink)
        sink.write(f"; {format_location(self.token.location)} Print \n")
        sink.write(f"pop rdi\n")
        sink.write(f"call print\n")

#region Control Flow Statements

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
        local_vars_size = 0
        # arguments have been inserted into the scope already
        for var in self.scope.values():
            local_vars_size += var.size
            # TODO: adjust size to variable type
            compiler_current_scope[var.name] = local_vars_size

        sink.write(f"; Function Definition {self.proto.name}\n")
        sink.write(f"{self.proto.name}:\n")

        sink.write("push rbp\n")
        sink.write("mov rbp, rsp\n")

        #make space for variables on stack (rbp)
        if len(self.scope) > 0:
            sink.write(f"sub rsp, {local_vars_size }\n")

        # arguments are now on stack
        # the stack grows downwards, meaning that the first argument is at the top of the stack, the second is at the top of the stack minus 8, etc.
        # rbx contains the callee stack variables
        # transfer arguments to local variables
        for param in self.proto.args.values():
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
    def __init__(self, token: Token, value: Optional[Expression]):
        if value is None:
            super().__init__(token, ExprType.NONE)
            self.value = None
        else:
            super().__init__(token, value.type)
            self.value = value

    def print(self, depth: int = 0):
        print(f"{' ' * depth}Return Statement")
        if self.value is not None:
            print(f"{' ' * depth}Value: {self.value.print()}")
    
    def codegen(self, sink: io.StringIO):
        sink.write(f"; {format_location(self.token.location)} Return Statment\n")
        if self.value is not None:
            self.value.codegen(sink)
            sink.write("pop rax\n")
        sink.write("jmp .end\n")
#endregion Control-Flow Statements

#endregion Statements