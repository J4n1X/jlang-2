from typing import *
from enum import Enum, auto
from dataclasses import dataclass

# tuple for position in file plus it's name
# this is global
compiler_current_scope: Dict[str, int] = {}
LocTuple = Tuple[str, int, int]
def format_location(loc: LocTuple) -> str:
        return f"{loc[0]}:{loc[1]}:{loc[2]}"


class AsmInfo:
    # for simplicity, we will not support upper byte registers
    registers: Dict[str, List[str]] = {
        "rax": ["al", "ax", "eax", "rax"],
        "rcx": ["cl", "cx", "ecx", "rcx"],
        "rdx": ["dl", "dx", "edx", "rdx"],
        "rbx": ["bl", "bx", "ebx", "rbx"],
        "rsp": ["spl", "sp", "esp", "rsp"],
        "rbp": ["bpl", "bp", "ebp", "rbp"],
        "rsi": ["sil", "si", "esi", "rsi"],
        "rdi": ["dil", "di", "edi", "rdi"],
        "r8":  ["r8b", "r8w", "r8d", "r8"],
        "r9":  ["r9b", "r9w", "r9d", "r9"],
        "r10": ["r10b", "r10w", "r10d", "r10"],
        "r11": ["r11b", "r11w", "r11d", "r11"],
        "r12": ["r12b", "r12w", "r12d", "r12"],
        "r13": ["r13b", "r13w", "r13d", "r13"],
        "r14": ["r14b", "r14w", "r14d", "r14"],
        "r15": ["r15b", "r15w", "r15d", "r15"],
    }

    mem_size_keywords: List[str] = ["BYTE", "WORD", "DWORD", "QWORD", "PTR", "FAR"]

    __abi_regs: List[str] = [
        registers["rdi"][3],
        registers["rsi"][3],
        registers["rdx"][3],
        registers["r10"][3],
        registers["r9"][3]
    ]

    def get_abi_reg_name(argnum: int) -> str:
        if argnum > len(AsmInfo.__abi_regs):
            return "stack-reverse"
        else:
            return AsmInfo.__abi_regs[argnum]
    

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
    # other manipulators
    ADDRESS_OF = auto()  # get address of identifier
    DROP = auto()
    RETURN = auto()
    ALLOCATE = auto() # allocate memory


assert len(Keyword) == 15, "Too many Keywords defined"
KEYWORDS_BY_NAME: Dict[str, Keyword] = {
    keyword.name.lower().replace("_", "-"): keyword for keyword in Keyword
}


class TokenType(Enum):
    KEYWORD = auto()                    # Keywords are basically all used for statements except for the syscall keyword
    IDENTIFIER = auto()                 # identifiers for variables and functions
    INT_LITERAL = auto()                # number literals
    STRING_LITERAL = auto()             # string literals
    OPERATOR = auto()                   # Things that manipulate values (basically operators but named weird)
    SYSCALL = auto()                    # syscalls
    LOADER = auto()                     # load a value from memory
    STORER = auto()                     # store a value in memory
    EOE = auto()                        # End of expression
    PAREN_BLOCK_START = auto()          # Start of parenthesis block
    PAREN_BLOCK_END = auto()            # End of parenthesis block
    ARG_DELIMITER = auto()              # Argument delimiter
    TYPE = auto()                       # Type name

assert len(TokenType) == 13, "Too many TokenTypes defined"
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

class Operator(Enum):
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

assert len(Operator) == 11, "Too many ManipulatorTypes defined"
OPERATOR_BY_NAME: Dict[str, Operator] = {
    operator.name.lower().replace("_", "-"): operator for operator in Operator
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

class Syscall(Enum):
    SYSCALL0 = 0
    SYSCALL1 = 1
    SYSCALL2 = 2
    SYSCALL3 = 3
    SYSCALL4 = 4
    SYSCALL5 = 5

assert len(Syscall) == 6, "Too many SyscallTypes defined"
SYSCALL_BY_NAME: Dict[str, Syscall] = {
    syscall.name.lower(): syscall for syscall in Syscall
}

class Loader(Enum):
    LOAD8 = 0
    LOAD16 = 1
    LOAD32 = 2
    LOAD64 = 3

assert len(Loader) == 4, "Too many LoaderTypes defined"
LOADER_BY_NAME: Dict[str, Loader] = {
    loader.name.lower(): loader for loader in Loader
}

class Storer(Enum):
    STORE8 = 0
    STORE16 = 1
    STORE32 = 2
    STORE64 = 3

assert len(Storer) == 4, "Too many StorerTypes defined"
STORER_BY_NAME: Dict[str, Storer] = {
    storer.name.lower(): storer for storer in Storer
}

class ExprType(Enum):
    NONE = auto()
    INTEGER = auto()
    POINTER = auto()

assert len(ExprType) == 3, "Too many ExprTypes defined"
EXPRTYPE_BY_NAME: Dict[str, ExprType] = {
    exprtype.name.lower(): exprtype for exprtype in ExprType
}

SIZE_OF_EXPRTYPES: Dict[ExprType, int] = {
    ExprType.NONE: 0,
    ExprType.INTEGER: 8,
    ExprType.POINTER: 8
}

@dataclass
class Token:
    type: TokenType
    text: str
    location: LocTuple
    value: Optional[Union[int, str, Enum]] = None

    def __str__(self):
        if isinstance(self.value, str):

            return f"{format_location(self.location)} {self.type.name} {self.text.encode('utf-8')} {self.value.encode('utf-8')}"
        else:
            return f"{format_location(self.location)} {self.type.name} {self.text} {self.value}"