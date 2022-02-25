from sqlalchemy import false
from JlangObjects import *
from Statements import *
from typing import *
from dataclasses import dataclass

@dataclass 
class FunctionContract:
    token: Token
    args: List[ExprType]
    returns: ExprType

    def __str__(self):
        return f"{format_location(self.token)}: ({self.args}) -> {self.returns}"

@dataclass
class StackEntry:
    token: Token
    type: ExprType

    def __str__(self):
        return f"{format_location(self.token.location)}: ({self.type})"
    
    def __eq__(self, other: 'StackEntry'):
        return self.type == other.type
    
    def __ne__(self, other: 'StackEntry') -> bool:
        return not self == other

class TypeStack:
    stack: Deque[StackEntry]
    def __init__(self, origin_stack: Optional['TypeStack'] = None):
        if origin_stack is None:
            self.stack = Deque()
        else:
            self.stack = origin_stack.stack.copy()

    def push(self, token: Token, type: ExprType):
        self.stack.append(StackEntry(token, type))
    
    def pop(self) -> StackEntry:
        if len(self.stack) == 0:
            raise Exception("TypeStack is empty")
        return self.stack.pop()
    
    # copy yourself
    def copy(self) -> 'TypeStack':
        return TypeStack(self.stack.copy())

    def __eq__(self, other: 'TypeStack') -> bool:
        if len(self.stack) != len(other.stack):
            return False
        for i in range(0, len(self.stack)):
            if self.stack[i] != other.stack[i]:
                return False
        return True
    
    def __ne__(self, other: 'TypeStack') -> bool:
        return not self == other
    
    def diff(self, other: 'TypeStack') -> 'TypeStack':
        new_stack = TypeStack()
        larger_stack = self if len(self.stack) > len(other.stack) else other
        smaller_stack = other if len(self.stack) > len(other.stack) else self
        for item in larger_stack.stack[len(smaller_stack.stack):]:
            new_stack.push(item.token, item.type)
        return new_stack

    def is_mergeable(self, branch: 'TypeStack'):
        return self == branch
        

    def try_merge(self, branch_stmt: Statement):
        prev_branch = self.branches[-2]
        try:
            self.is_mergeable()
        except:
            raise("Merge failed due to stack mismatch")


class TypeChecker:
    branches: Deque[Deque[StackEntry]] = Deque()
    cur_branch: Deque[StackEntry] = Deque()
    AST: List[Statement] = []
    contracts: Dict[str, FunctionContract] = {}
    err_state: bool = False

    def __init__(self, AST: List[Statement], prototypes: Dict[str, FunProto]):
        if len(AST) == 0:
            raise Exception("Can't type check empty AST")
        self.AST = AST
        for proto in prototypes.values():
            self.contracts[proto.name] = FunctionContract(proto.token, [arg.type for arg in proto.args.values()], proto.type)

    def _next_statement(self) -> Optional[Statement]:
        if len(self.AST) == 0:
            return None
        return self.AST.pop()

    #region AST Type Checking

    def _dump_stack_with_error(self, stmt: Statement, exhaustive_types: Deque[StackEntry]):
        print(f"[ERROR] Exhaustive Types on Stack in {stmt.__class__.__name__} at {format_location(stmt.token.location)}")
        self.err_state = True
        for item in exhaustive_types:
            print(item)

    def _check_type_mismatch(self, token: Token, expected: ExprType, found: ExprType):
        if expected != found:
                print(f"[ERROR] Type mismatch at {format_location(token.location)}: Expected {expected}, got {found}")
                self.err_state = True
                return False
        return True

    def _parse_block(self, block: List[Statement]):
        self.enter()
        for stmt in block:
            self.parse_statement(stmt)
            if isinstance(stmt, ReturnStmt):
                return # no need to leave, the return statement exits the block
        try:
            self.leave()
        except Exception as e:
            print(e)
            for item in self.cur_branch:
                print(item)
            # dump the cur_stack


    def parse_program(self) -> bool:
        stmt = self._next_statement()
        while stmt is not None:
            self.parse_statement(stmt)
            stmt = self._next_statement()

    # do not push a context here, it's a generic function
    def parse_statement(self, stmt: Statement) -> bool:
        #print(f"{format_location(stmt.token.location)} Type checking {stmt.__class__.__name__}")
        if stmt is None:
            return False
        #TODO: create a base class for these statements so we can avoid this ridiculous if statement
        elif isinstance(stmt, DropStmt)   or \
             isinstance(stmt, PrintStmt)  or \
             isinstance(stmt, LoaderExpr) or \
             isinstance(stmt, StorerStmt) or \
             isinstance(stmt, AddressOfExpr):
            self.parse_intrinsic_types(stmt)
        elif isinstance(stmt, FunStmt):
            self.parse_function_types(stmt)
        elif isinstance(stmt, SyscallExpr):
            self.parse_syscall_expression_types(stmt)
        elif issubclass(type(stmt), ControlStmt):
            self.parse_control_types(stmt)
        elif isinstance(stmt, ReturnStmt):
            self.parse_return_types(stmt)
        elif issubclass(type(stmt), Expression):
            self.parse_expression_types(stmt)
        return True


    def parse_intrinsic_types(self, stmt: Statement):
        #print(f"{format_location(stmt.token.location)} Parsing intrinsic {stmt.__class__.__name__}")
        if isinstance(stmt, StorerStmt):
            self.parse_expression_types(stmt.target)
            self._check_type_mismatch(stmt.token, ExprType.POINTER, stmt.target.type)
            self.cur_branch.pop()
            # can't check the value type, it can be any type
            self.parse_expression_types(stmt.value)
            self.cur_branch.pop()
        elif isinstance(stmt, LoaderExpr):
            self.parse_expression_types(stmt.value)
            self._check_type_mismatch(stmt.token, ExprType.POINTER, stmt.value.type)
            self.cur_branch.pop()
            self.cur_branch.append(StackEntry(stmt.token, ExprType.INTEGER))
        elif isinstance(stmt, AddressOfExpr):
            self.parse_expression_types(stmt.value)
            # can't type check the value, it is an identifier and could be any type
            self.cur_branch.pop()
            self.cur_branch.append(StackEntry(stmt.token, ExprType.POINTER))
        elif isinstance(stmt, PrintStmt) or isinstance(stmt, DropStmt):
            self.parse_expression_types(stmt.expr)
            # can't check the value, it could be any type
            self.cur_branch.pop()
        else:
            raise Exception(f"Unrecognized intrinsic statement type found at {format_location(stmt.token.location)}")



    # do not push a context here, it's not a block
    def parse_expression_types(self, expr: Expression):
        #print(f"{format_location(expr.token.location)} Parsing expression {expr.__class__.__name__}")
        # special cases
        if isinstance(expr, BinaryExpr): 
            self.parse_binary_expr_types(expr)
        elif issubclass(type(expr), Expression):
            self.cur_branch.append(StackEntry(expr.token, expr.type))

    # do not push a context here, it's not a block
    def parse_binary_expr_types(self, expr: BinaryExpr):
        self.parse_expression_types(expr.value)
        self.parse_expression_types(expr.right)
        LHS_type = self.cur_branch.pop()
        RHS_type = self.cur_branch.pop()
        self._check_type_mismatch(expr.token, LHS_type.type, LHS_type.type)
        self.cur_branch.append(StackEntry(expr.token, expr.type))
    
    def parse_function_types(self, stmt: FunStmt):
        self._parse_block(stmt.block)

    def parse_control_types(self, stmt: ControlStmt):
        self.parse_expression_types(stmt.condition)
        self._check_type_mismatch(stmt.token, ExprType.INTEGER, stmt.condition.type)
        self.cur_branch.pop()

        self._parse_block(stmt.block)

        #self.merge() # merge the control flow block

    # do not push a context here, it's not a block
    def parse_funcall_expression_types(self, funcall: FunCallExpr):
        assert isinstance(funcall.value, List[Expression]), "FunCallExpr.value is not a list of expressions"
        contract = self.contracts[funcall.target.value]
        contract_args = reversed(contract.args)
        
        self._parse_block(funcall.value)

        if contract.returns is not None:
            self.cur_branch.append(funcall.token, funcall.type)
    
    # do not push a context here, it's not a block
    def parse_syscall_expression_types(self, syscall: SyscallExpr):
        #assert isinstance(syscall.value, List[Expression]), "SyscallExpr.value is not a list of expressions"
        
        # we do not type check for syscall args, only if the call value is correct
        if self._check_type_mismatch(syscall.callnum.token, ExprType.INTEGER, syscall.callnum.type):
            self.parse_expression_types(syscall.callnum)
        else:
            raise
        for expr in syscall.value:
            self.parse_expression_types(expr)
            self.cur_branch.pop()
        self.cur_branch.append(StackEntry(syscall.token, syscall.type))

    def parse_return_types(self, stmt: ReturnStmt):
        if stmt.value is not None:
            self.parse_expression_types(stmt.value)
            self.cur_branch.pop() # return value is used
        self.leave()
        

    #region Data Manipulation

    def enter(self):
        if len(self.cur_branch) > 0:
            self.branches.append(self.cur_branch)
            self.cur_branch.clear()

    def leave(self):
        if len(self.branches) > 0:
            prev_branch = self.pop_branch()
            if self.cur_branch == prev_branch:
                self.cur_branch = prev_branch
            else:
                raise Exception(f"[ERROR] Unhandled Data on Stack")

    def print_state(self):
        for branch in self.branches:
            for item in branch:
                print(item)

    def push_branch(self, entry: Deque[TypeStack]):
        self.branches.append(entry)
        self.cur_branch = self.branches[-1]

    def pop_branch(self) -> Deque[TypeStack]:
        if len(self.branches) == 0:
            raise Exception("Can't pop branch: There are no branches")
        ret = self.branches.pop()
        self.cur_branch = ret
        return ret
    #endregion

