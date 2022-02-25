from ast import expr
from pickle import TRUE
from typing import *

from pymysql import Binary

from JlangObjects import *
from Statements import *
from Tokenizer import Tokenizer

class ExpressionParser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.global_const_vars: List[str] = []
        self.index = 0
        self.cur_tok: Optional[Token] = self.__next_token()
        self.prototypes: Dict[str, FunProto] = {}
        self.constants: Dict[str, Constant] = {}
        self.global_vars: Dict[str, VarDefStmt] = {}
        self.scope_vars: Dict[str, VarDefStmt] = {}
        self.anonymous_scope_vars: List[VarDefStmt] = [] # variables without a name in the current scope
        self.in_scope: bool = False

    def __insert_tokens(self, tokens: List[Token]):
        #insert the tokens at the current index
        self.tokens = self.tokens[:self.index] + tokens + self.tokens[self.index:]
#region helper functions

    def __next_token(self) -> Optional[Token]:
        if self.index >= len(self.tokens):
            self.cur_tok = None
        else:
            self.cur_tok = self.tokens[self.index]
            self.index += 1
        return self.cur_tok

    def __get_precedence(self) -> int:
        assert len(Operator) == 11, "Too many binary operators defined at ExpressionParser.__get_precendence"

        if self.cur_tok is None:
            return -1
        elif self.cur_tok.type == TokenType.OPERATOR and self.cur_tok.value in BINOP_PRECEDENCE:
            return BINOP_PRECEDENCE[self.cur_tok.text]
        else:
            return 0

    def __get_block(self, start_keyword: Keyword, end_keyword: Keyword) -> List[Statement]:
        if self.cur_tok is None:
            raise Exception("Unexpected EOF")
        if self.cur_tok.value != start_keyword:
            raise Exception(f"Expected {start_keyword} at {format_location(self.cur_tok.location)}")
        self.__next_token()
        block: List[Statement] = []
        while True:
            if self.cur_tok is None or self.cur_tok.value == end_keyword:
                break
            block.append(self.parse_statement())
        return block

    def __get_proto_params(self) -> Dict[str,VarDefStmt]:
        params: Dict[str,VarDefStmt] = {}
        if self.cur_tok.type != TokenType.PAREN_BLOCK_START:
            raise Exception(f"Expected start of parenthesis block at {format_location(self.cur_tok.location)}")
        self.__next_token()
        
        while self.cur_tok is not None and self.cur_tok.type != TokenType.PAREN_BLOCK_END:
            param = self.parse_var_def_statement(isparam = True)
            params[param.name] = param
            
            if self.cur_tok.type == TokenType.PAREN_BLOCK_END:
                break
            elif self.cur_tok.type != TokenType.ARG_DELIMITER:
                raise Exception(f"Expected ',' at {format_location(self.cur_tok.location)}")
            self.__next_token()
        return params
        
    def __get_call_args(self) -> List[Expression]:
        args: List[Expression] = []
        if self.cur_tok.type != TokenType.PAREN_BLOCK_START:
            raise Exception(f"Expected start of parenthesis block at {format_location(self.cur_tok.location)}")
        self.__next_token()
        while self.cur_tok is not None and self.cur_tok.type != TokenType.PAREN_BLOCK_END:
            arg = self.parse_statement()
            assert isinstance(arg, Expression), f"Expected Expression for argument but got {arg.token}"
            args.append(arg)
            if self.cur_tok.type == TokenType.PAREN_BLOCK_END:
                break
            elif self.cur_tok.type != TokenType.ARG_DELIMITER:
                raise Exception(f"Expected ',' at {format_location(self.cur_tok.location)}, but got {self.cur_tok.value}")
            self.__next_token()
        return args

    # get a reference object for the current identifier token
    def __get_ident_ref(self) -> Optional[IdentRefExpr]:
        assert len(IdentType) == 4, "Too many IdentTypes defined at ExpressionParser.parse_identifier_expression"
        assert self.cur_tok is not None, "Unexpected EOF"
        assert isinstance(self.cur_tok.value, str), "Expected string value for identifier"
        if self.cur_tok is None:
            return None
        elif self.cur_tok.value in self.prototypes:
            type = self.prototypes[self.cur_tok.value].type
            return IdentRefExpr(self.cur_tok, self.cur_tok.value, IdentType.FUNCTION, type)
        elif self.cur_tok.value in self.scope_vars:
            type = self.scope_vars[self.cur_tok.value].type
            return IdentRefExpr(self.cur_tok, self.cur_tok.value, IdentType.VARIABLE, type)
        elif self.cur_tok.value in self.global_vars:
            type = self.global_vars[self.cur_tok.value].type
            return IdentRefExpr(self.cur_tok, self.cur_tok.value, IdentType.GLOBAL_VARIABLE, type)
        elif self.cur_tok.value in self.constants:
            type = self.constants[self.cur_tok.value].type
            return IdentRefExpr(self.cur_tok, self.cur_tok.value, IdentType.CONSTANT, type)
        else:
            return None
    
    # get a mutable reference to the current identifier token
    def __get_ident_def(self):
        assert len(IdentType) == 4, "Too many IdentTypes defined at ExpressionParser.parse_identifier_expression"
        if self.cur_tok.value in self.prototypes:
            raise Exception(f"Cannot get mutable identifiers for functions at {self.cur_tok.value} at {format_location(self.cur_tok.location)}")
        elif self.cur_tok.value in self.scope_vars:
            return self.scope_vars[self.cur_tok.value]
        elif self.cur_tok.value in self.global_vars:
            return self.global_vars[self.cur_tok.value]
        else:
            return None

    # return the value if the current token is a constant or a literal
    def __resolve_if_constant(self, expr: Expression) -> Union[int, str]:
        if isinstance(expr, IntLiteralExpr):
            return expr.value
        elif isinstance(expr, ArrayRefExpr):
            return expr.value
        elif isinstance(expr, IdentRefExpr) and expr.ident_kind == IdentType.CONSTANT:
            return self.constants[expr.value].value
        else:
            raise ValueError(f"Expected constant or literal expression but got {expr.token}")
#endregion

    def parse_top_level(self) -> Optional[Statement]:
    # allowed on top level: function, assign
        if self.cur_tok is None:
            return None
        elif self.cur_tok.type == TokenType.EOE: # try again with the next token
            self.__next_token()
            return self.parse_top_level()
        elif self.cur_tok.type == TokenType.KEYWORD:
            assert len(Keyword) == 14, "Invalid amount of keywords defined at ExpressionParser.parse_top_level"
            if self.cur_tok.value == Keyword.CONSTANT:
                self.parse_const_def()
                return self.parse_top_level()
            elif self.cur_tok.value == Keyword.FUNCTION:
                return self.parse_function_statement()
            elif self.cur_tok.value == Keyword.DEFINE:
                self.parse_var_def_statement()
                # we've added it to the global scope, so we can parse the next statement
                return self.parse_top_level()
            elif self.cur_tok.value == Keyword.IMPORT:
                self.__next_token()
                assert isinstance(self.cur_tok.value, str), "Expected string value for import"
                self.__insert_tokens(Tokenizer(self.cur_tok.value).tokens)
                self.__next_token()
                return self.parse_top_level()

                
        raise Exception(f"Unexpected keyword {self.cur_tok.value} on top level {self.cur_tok}")



    def parse_primary(self) -> Optional[Statement]:
        assert len(TokenType) == 12, "Too many TokenTypes defined at ExpressionParser.parse_primary"

        ret_expr: Optional[Union[Expression, Statement]] = None
        if self.cur_tok is None:
            return None
        elif self.cur_tok.type == TokenType.EOE:
            self.__next_token()
            return self.parse_primary()
        elif self.cur_tok.type == TokenType.KEYWORD:
            ret_expr = self.parse_keyword()
        elif self.cur_tok.type == TokenType.IDENTIFIER:
            # this must be a reference to a variable or a function
            ret_expr = self.parse_ident()
        elif self.cur_tok.type == TokenType.INT_LITERAL:
            ret_expr = self.parse_int_literal_expression()
        elif self.cur_tok.type == TokenType.STRING_LITERAL:
            ret_expr = self.parse_string_literal_expression()
        elif self.cur_tok.type == TokenType.SYSCALL:
            ret_expr = self.parse_syscall_expression()
        elif self.cur_tok.type == TokenType.INTRINSIC:
            ret_expr = self.parse_intrinsic()
        elif self.cur_tok.type == TokenType.TYPE:
            ret_expr = self.parse_cast_expression()
        else:
            raise Exception(f"Unexpected token {self.cur_tok}")
        return ret_expr


    def parse_statement(self) -> Optional[Statement]:
        expr = self.parse_primary()

        if expr is None:
            return None    

        return self.parse_binary_expression(0, expr)
        #if isinstance(expr, BinaryExpr) \
        #    or isinstance(expr, IntLiteralExpr) \
        #    or isinstance(expr, StringLiteralExpr) \
        #    or isinstance(expr, IdentRefExpr):
        #else:
        #    return expr

    def parse_intrinsic(self) -> Expression:
        assert len(Intrinsic) == 11, "Too many Intrinsics defined at ExpressionParser.parse_intrinsic"
        if self.cur_tok.value == Intrinsic.PRINT:
            return self.parse_print_statement()
        elif self.cur_tok.value == Intrinsic.DROP:
            return self.parse_drop_statement()
        elif self.cur_tok.value == Intrinsic.ADDRESS_OF:
            return self.parse_address_of_expression()
        elif Intrinsic.is_loader(self.cur_tok.value):
            return self.parse_loader_expression()
        elif Intrinsic.is_storer(self.cur_tok.value):
            return self.parse_storer_statement()
        else:
            raise Exception(f"Unexpected intrinsic {self.cur_tok.value} at {format_location(self.cur_tok.location)}")
        


    def parse_keyword(self) -> Statement:
        assert len(Keyword) == 14, "Too many keywords defined at ExpressionParser.parse_keyword"
        assert self.cur_tok is not None, "Unexpected EOF"
        if self.cur_tok.value == Keyword.IF:
            return self.parse_control_statement(Keyword.IF)
        elif self.cur_tok.value == Keyword.WHILE:
            return self.parse_control_statement(Keyword.WHILE)
        elif self.cur_tok.value == Keyword.FUNCTION:
            return self.parse_function_statement()
        elif self.cur_tok.value == Keyword.DEFINE:
            return self.parse_var_def_statement()
        elif self.cur_tok.value == Keyword.RETURN:
            return self.parse_return_statement()
        elif self.cur_tok.value == Keyword.ALLOCATE:
            return self.parse_allocate_expression()
        else: 
            raise Exception(f"Unexpected keyword {self.cur_tok}")

#region Statement-Parsers

    def parse_fun_proto_statement(self):
        if(self.cur_tok.value != Keyword.FUNCTION):
            raise Exception(f"Expected keyword {Keyword.FUNCTION} at {format_location(self.cur_tok.location)}")
        prev_tok = self.cur_tok
        self.__next_token()
        if self.cur_tok.type != TokenType.IDENTIFIER:
            raise Exception(f"Expected identifier after function keyword at {format_location(self.cur_tok.location)}, but got {self.cur_tok.value}")

        ident = self.__get_ident_ref()
        if ident is not None:
            assert len(IdentType) == 4, "Too many IdentTypes defined at ExpressionParser.parse_fun_proto_statement"
            if ident.type == IdentType.FUNCTION:
                raise Exception(f"Attempted redefinition of Function {ident.value} at {format_location(self.cur_tok.location)}; already defined at {format_location(ident.token.location)}")
            elif ident.type == IdentType.VARIABLE:
                raise Exception(f"Attempted redefinition of Variable {ident.value} at {format_location(self.cur_tok.location)}; Already defined at {format_location(ident.token.location)}")
            elif ident.type == IdentType.GLOBAL_VARIABLE:
                raise Exception(f"Attempted redefinition of Global variable {ident.value} at {format_location(self.cur_tok.location)}; Already defined at {format_location(ident.token.location)}")
        

        name = self.cur_tok.value

        self.__next_token()
        # later we'd take the parameters and return type
        # [14.02.2022] that time is now!
        params = self.__get_proto_params()
        self.__next_token() # eat last paren
        
        if self.cur_tok.type != TokenType.KEYWORD:
            raise Exception(f"Expected 'yields' keyword at {format_location(self.cur_tok.location)}")
        if self.cur_tok.value != Keyword.YIELDS: 
            raise Exception(f"Expected 'yields' keyword at {format_location(self.cur_tok.location)}")
        self.__next_token() # eat 'yields' keyword

        if self.cur_tok.type != TokenType.TYPE:
            raise Exception(f"Expected type at {format_location(self.cur_tok.location)}")
        exprtype = self.cur_tok.value 
        self.__next_token()
        
        return FunProto(prev_tok, name, params, exprtype)


    def parse_function_statement(self):
        if self.in_scope:
            raise Exception(f"Function statement at {format_location(self.cur_tok.location)} must be at top level {self.cur_tok}")
        
        proto = self.parse_fun_proto_statement()
        self.prototypes[proto.name] = proto
        
        self.in_scope = True
        block = self.__get_block(Keyword.IS, Keyword.DONE)
        self.__next_token() # eat 'done'
        self.in_scope = False 

        scope: Dict[str, VarDefStmt] = {}
        for var in self.scope_vars.values():
            scope[var.name] = var
        for anon_var in self.anonymous_scope_vars:
            scope[anon_var.name] = anon_var

        fun = FunStmt(proto, block, scope, proto.type)

        self.scope_vars.clear()
        self.anonymous_scope_vars.clear()
        return fun

    # TODO: make it so we don't need to add a eoe token at the end
    def parse_control_statement(self, type: Keyword):
        assert self.cur_tok is not None, "Unexpected EOF"
        control_name = type.name.lower()
        if not self.in_scope:
            raise Exception(f"{control_name} statement at {format_location(self.cur_tok.location)} cannot be at top level {self.cur_tok}")
        
        prev_tok = self.cur_tok
        self.__next_token()
        # this is the conditional expression
        cond_expr = self.parse_statement()
        assert isinstance(cond_expr, Expression), "Expected expression after %s statement" % (control_name)
        if cond_expr is None:
            raise Exception(f"Expected expression after {control_name} keyword")
        expr_block = self.__get_block(Keyword.DO, Keyword.DONE)
        self.__next_token() # eat the done keyword

        if type == Keyword.IF:
            return IfStmt(prev_tok, cond_expr, expr_block)
        elif type == Keyword.WHILE:
            return WhileStmt(prev_tok, cond_expr, expr_block)


    def parse_function_call_statement(self):
        prev_tok = self.cur_tok
        target = self.__get_ident_ref()

        if target is None: # the identifier was not found
            raise Exception(f"Invalid identifier after call keyword at {format_location(prev_tok.location)}")
        elif target.ident_kind != IdentType.FUNCTION:
            raise Exception(f"Attempted to invoke function call with non-function identifier at {format_location(prev_tok.location)}")
        self.__next_token() # eat identifier

        # arguments would be parsed here
        # [14.02.2022] We'll do that now!
        args: List[Expression] = self.__get_call_args()
        self.__next_token() # eat the ')'


        #if self.cur_tok.type != TokenType.EOE:
        #    raise Exception(f"At {format_location(self.cur_tok.location)}, expected end of statement")
        #self.__next_token()
        return FunCallExpr(prev_tok, target, args)

    def parse_var_def_statement(self, isparam = False) -> VarDefStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        if not isparam:
            self.__next_token()

        if self.cur_tok.type != TokenType.IDENTIFIER:
            raise Exception(f"Expected identifier after define keyword at {format_location(self.cur_tok.location)}")

        ident_name = self.cur_tok.value
        ident: VarDefStmt = self.__get_ident_def()
        self.__next_token() # eat identifier


        if self.cur_tok.type != TokenType.KEYWORD:
            raise Exception(f"Expected 'as' after identifier at {format_location(self.cur_tok.location)}")
        else:
            if self.cur_tok.value != Keyword.AS:
                raise Exception(f"Expected 'as' after identifier at {format_location(self.cur_tok.location)}")
        self.__next_token() # eat 'as' keyword
        
        if self.cur_tok.type != TokenType.TYPE:
            raise Exception(f"Expected type after identifier at {format_location(self.cur_tok.location)}")
        if not isinstance(self.cur_tok.value, ExprType):
            raise Exception(f"Expected type after identifier at {format_location(self.cur_tok.location)}")
        ident_var_type = self.cur_tok.value
        self.__next_token() # eat type
        
        value: Optional[Expression] = None
        if self.cur_tok.type == TokenType.KEYWORD:
            if self.cur_tok.value == Keyword.IS:
                self.__next_token() # eat 'is' keyword
                value = self.parse_statement()
                if not isinstance(value, Expression):
                    raise ValueError(f"Expected expression after define keyword at {format_location(self.cur_tok.location)}")
        value_size = SIZE_OF_EXPRTYPES[ident_var_type]

        # TODO: put these things in a seperate class or create a comparer function
        #if self.cur_tok.type != TokenType.EOE and \
        #    self.cur_tok.type != TokenType.ARG_DELIMITER and \
        #    self.cur_tok.type != TokenType.PAREN_BLOCK_END:
        #    raise Exception(f"Expected end of expression at {format_location(self.cur_tok.location)}, but got {self.cur_tok.type}")

        if self.in_scope or isparam:
            if ident is not None:
                assert len(IdentType) == 3, "Too many IdentTypes defined at ExpressionParser.parse_var_def_statement"
                if ident.var_type == IdentType.FUNCTION:
                    raise Exception(f"Attempted redefinition of Function {ident.value} at {format_location(self.cur_tok.location)}; already defined at {format_location(ident.token.location)}")
                elif ident.var_type == IdentType.GLOBAL_VARIABLE:
                    # do not add to the list of global variables, as it is already there
                    # instead, make the global variable inaccessible and return a new parameter variable
                    if isparam:
                        var_def = VarDefStmt(prev_tok, ident_name, IdentType.VARIABLE, ident_var_type, value_size, value)
                    else:
                        var_def = VarDefStmt(prev_tok, ident_name, IdentType.GLOBAL_VARIABLE, ident_var_type, value_size, value)
                elif ident.var_type == IdentType.VARIABLE:
                    # the variable exists, modify it
                    assert False, "Redefinition of variable in 'define' not allowed"
                self.scope_vars[ident_name] = var_def
            else:
                # the variable doesn't exist, define it, then add it to the scope
                var_def = VarDefStmt(prev_tok, ident_name, IdentType.VARIABLE, ident_var_type, value_size, value)
                self.scope_vars[ident_name] = var_def
        else: # global scope
            if ident is not None:
                assert len(IdentType) == 3, "Too many IdentTypes defined at ExpressionParser.parse_var_def_statement"
                if ident.var_type == IdentType.FUNCTION:
                    raise Exception(f"Attempted redefinition of Function {ident.value} at {format_location(self.cur_tok.location)}; already defined at {format_location(ident.token.location)}")
                elif ident.var_type == IdentType.GLOBAL_VARIABLE:
                    raise Exception(f"Attempted redefinition of Global variable {ident.value} at {format_location(self.cur_tok.location)}; Already defined at {format_location(ident.token.location)}")
                elif ident.var_type == IdentType.VARIABLE:
                    raise Exception(f"Attempted redefinition of Global variable {ident.value} at {format_location(self.cur_tok.location)}; Already defined at {format_location(ident.token.location)}")
            else:
                # define a new global variable. It is mutable from the global scope
                var_def = VarDefStmt(prev_tok, ident_name, IdentType.GLOBAL_VARIABLE,ident_var_type, value_size, value)
                self.global_vars[ident_name] = var_def
        #self.__next_token() # eat identifier
        #if self.cur_tok.type != TokenType.EOE:
        #    raise Exception(f"At {format_location(self.cur_tok.location)}, expected end of statement")
        
        return var_def

    def parse_var_set_statement(self) -> VarSetStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        ident = self.__get_ident_def()
        if ident is None:
            raise Exception(f"Invalid Identifier {self.cur_tok.value} at {format_location(self.cur_tok.location)}")
        if ident.var_type != IdentType.VARIABLE and ident.var_type != IdentType.GLOBAL_VARIABLE:
            raise Exception(f"Invalid reference to identifier {self.cur_tok.value} of type {ident.ident_kind} at {format_location(self.cur_tok.location)}")
        self.__next_token() # eat identifier

        if self.cur_tok.type != TokenType.KEYWORD and self.cur_tok.value == Keyword.IS:
            raise Exception(f"Expected 'is' after identifer at {format_location(self.cur_tok.location)}")
        self.__next_token() # eat 'is' keyword

        value = self.parse_statement()
        assert isinstance(value, Expression), f"Expected expression after 'is' at {format_location(self.cur_tok.location)}"

        return VarSetStmt(prev_tok, ident.name, ident.var_type, value)

    def parse_const_def(self):
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token() # eat 'constant' keyword
        assert self.cur_tok.type == TokenType.IDENTIFIER, f"Expected identifier after 'constant' at {format_location(self.cur_tok.location)}"
        const_name = self.cur_tok.value
        self.__next_token() # eat identifier
        assert self.cur_tok.type == TokenType.KEYWORD and self.cur_tok.value == Keyword.AS, f"Expected 'as' after identifier at {format_location(self.cur_tok.location)}"
        self.__next_token() # eat 'as' keyword
        assert self.cur_tok.type == TokenType.TYPE, f"Expected type after 'as' at {format_location(self.cur_tok.location)}"
        const_var_type = self.cur_tok.value
        self.__next_token() # eat type
        assert self.cur_tok.type == TokenType.KEYWORD and self.cur_tok.value == Keyword.IS, f"Expected 'is' after type at {format_location(self.cur_tok.location)}"
        self.__next_token() # eat 'is' keyword
        expr = self.parse_statement()
        assert isinstance(expr, BinaryExpr) or \
               isinstance(expr, IntLiteralExpr) or \
               isinstance(expr, ArrayRefExpr), f"Expected expression after 'is' at {format_location(self.cur_tok.location)}"
        const_val = self.eval_expression(expr)
        self.constants[const_name] = Constant(prev_tok, const_name, const_var_type, const_val)
        
        

    def parse_print_statement(self) -> PrintStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token()
        
        params = self.__get_call_args()
        self.__next_token()
        assert len(params) == 1, f"Expected 1 argument to print at {format_location(self.cur_tok.location)}"

        # move past the eoe
        #self.__next_token()
        return PrintStmt(prev_tok, params[0])

    def parse_drop_statement(self) -> DropStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token()

        expr = self.parse_statement()
        assert isinstance(expr, Expression), "Expected expression after print keyword at %s" % (format_location(prev_tok.location))
        
        # move past the eoe
        #self.__next_token()
        return DropStmt(prev_tok, expr)
        
    def parse_return_statement(self) -> ReturnStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token()

        if self.cur_tok.type == TokenType.TYPE and self.cur_tok.value == ExprType.NONE:
            self.__next_token()
            return ReturnStmt(prev_tok, None)

        value = self.parse_statement()
        assert isinstance(value, Expression), f"Value of return must be an Expression at {format_location(prev_tok.location)}"
        return ReturnStmt(prev_tok, value)

    # store<num>(<dst>, <src>)
    def parse_storer_statement(self) -> StorerStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok

        assert isinstance(self.cur_tok.value, Intrinsic), f"Expected Storer at {format_location(self.cur_tok.location)}"
        self.__next_token() # eat the storer keyword
        # now we get the value to be stored
        
        params = self.__get_call_args()
        self.__next_token()
        assert len(params) == 2, f"Expected 2 parameters for storer at {format_location(prev_tok.location)}"

        return StorerStmt(prev_tok, params[0], params[1])


#endregion

#region Expression-Parsers
    def parse_cast_expression(self) -> Expression:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token()
        
        params = self.__get_call_args()
        self.__next_token()
        assert len(params) == 1, f"Expected 1 parameter for cast at {format_location(prev_tok.location)}"
        if isinstance(params[0], IdentRefExpr):
            return IdentRefExpr(params[0].token, params[0].value, params[0].ident_kind, prev_tok.value)
        else:
            params[0].type = prev_tok.value
            return params[0]

    def parse_syscall_expression(self) -> SyscallExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok

        assert isinstance(self.cur_tok.value, Syscall), f"Expected syscall keyword at {format_location(self.cur_tok.location)}, but got {self.cur_tok.value}"
        if self.cur_tok.value not in Syscall:
            raise Exception(f"{self.cur_tok} is not a valid syscall number")
        arg_count = self.cur_tok.value.value # the number of the enum entry
        self.__next_token()
        args: List[Expression] = self.__get_call_args()
        self.__next_token() # eat the ')'
        callnum = args[0]
        args.remove(args[0])
        if arg_count != len(args):
            raise Exception(f"Expected {callnum} arguments for syscall{callnum} at {format_location(prev_tok.location)} but got {len(args)}")
        return SyscallExpr(prev_tok, prev_tok.value, callnum, args)

    # can return a value, the type should be dependent on the pointer type in the future
    def parse_loader_expression(self) -> LoaderExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token() 
        # now we must parse the binary expression that is the pointer

        params = self.__get_call_args()
        self.__next_token()
        assert len(params) == 1, f"Expected 1 parameter for Loader at {format_location(prev_tok.location)}"

        # we are done
        return LoaderExpr(prev_tok, params[0])


    def parse_ident(self) -> Statement:
        next_token = self.tokens[self.index]
        assert self.cur_tok is not None, "Unexpected EOF"
        ident = self.__get_ident_ref()
        if ident is None:
            raise Exception(f"Expected valid identifier at {format_location(self.cur_tok.location)}")
        if next_token.type == TokenType.PAREN_BLOCK_START:
            retval = self.parse_function_call_statement()
        elif next_token.type == TokenType.KEYWORD and \
             next_token.value == Keyword.IS:
            retval = self.parse_var_set_statement()
        else:
            retval = ident
            self.__next_token()
        return retval

    def parse_int_literal_expression(self) -> IntLiteralExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        assert isinstance(self.cur_tok.value, int), "Expected integer literal at %s" % (format_location(self.cur_tok.location))
        retval = IntLiteralExpr(self.cur_tok, self.cur_tok.value)
        self.__next_token()
        return retval

    def parse_string_literal_expression(self) -> ArrayRefExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        assert isinstance(self.cur_tok.value, str), "Expected string literal at %s" % (format_location(self.cur_tok.location))
        # check if the string literal already exists
        string_id = len(self.global_const_vars)
        string_ref = f"_anon_str_{string_id}"
        self.global_const_vars.append(self.cur_tok.value)
        retval = ArrayRefExpr(self.cur_tok, string_ref)
        self.__next_token()
        return retval

    def parse_allocate_expression(self) -> ArrayRefExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_token = self.cur_tok
        self.__next_token()

        params = self.__get_call_args()
        self.__next_token()
        assert len(params) == 1, f"Expected 1 parameter for allocate at {format_location(prev_token.location)}"
        
        ident_val = self.__resolve_if_constant(params[0])
        # add it to the scope anonymous variables under a unique name
        array_ref: str = ""
        if self.in_scope:
            array_ref = f"arr_{len(self.anonymous_scope_vars)}"
            self.anonymous_scope_vars.append(VarDefStmt(prev_token, array_ref, IdentType.VARIABLE, ExprType.NONE, ident_val))
        else: 
            array_ref = f"glob_arr_{len(self.global_vars)}"
            self.global_vars[array_ref] = VarDefStmt(prev_token, array_ref, IdentType.GLOBAL_VARIABLE, ExprType.NONE, ident_val)

        return ArrayRefExpr(prev_token, array_ref)



    def parse_binary_expression(self, prec: int, LHS: Expression) -> Expression:
        if self.cur_tok is None:
            return LHS
        while self.cur_tok.type == TokenType.OPERATOR:
            tok_prec = self.__get_precedence()
            if tok_prec < prec: # if we're done with the binary expression
                return LHS
        
            # otherwise, parse more binary expressions
            op_tok = self.cur_tok
            prev_tok = self.cur_tok
            self.__next_token()

            RHS = self.parse_primary()
            if RHS is None:
                #return None
                raise Exception(f"{format_location(prev_tok.location)}: Expected expression after {prev_tok.text}")
            
            next_prec = self.__get_precedence()
            
            if tok_prec < next_prec:
                prev_tok = self.cur_tok
                assert isinstance(RHS, Expression), "Expected expression after %s" % (prev_tok.text)
                RHS = self.parse_binary_expression(tok_prec + 1, RHS)
                if RHS is None:
                    raise Exception(f"{format_location(prev_tok.location)}: Expected expression after {prev_tok.text}")
            
            # merge and continue
            assert isinstance(RHS, Expression), "Expected expression after %s" % (prev_tok.text)
            LHS = BinaryExpr(op_tok, LHS, RHS)
        return LHS

    def parse_address_of_expression(self) -> AddressOfExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok

        assert self.cur_tok.value == Keyword.ADDRESS_OF, f"Expected address of keyword at {format_location(prev_tok.location)}"
        self.__next_token()
        params = self.__get_call_args()
        self.__next_token() # eat the ')'
        assert len(params) == 1, f"Expected 1 parameter for AddressOf at {format_location(prev_tok.location)}"
        assert isinstance(params[0], IdentRefExpr), f"Expected identifier after address of at {format_location(prev_tok.location)}"
        assert params[0].ident_kind == IdentType.VARIABLE or \
               params[0].ident_kind == IdentType.GLOBAL_VARIABLE or \
               params[0].ident_kind == IdentType.CONSTANT, \
               f"Expected variable after address of at {format_location(prev_tok.location)}"

        return AddressOfExpr(prev_tok, params[0])
#endregion

#region const evaluator

    def eval_expression(self, expr: Expression) -> Union[str,int]:
        if isinstance(expr, IntLiteralExpr):
            return self.eval_integer_literal_expr(expr)
        elif isinstance(expr, IdentRefExpr):
            return self.deref_ident(expr)
        elif isinstance(expr, ArrayRefExpr):
            return self.eval_string_literal(expr)
        elif isinstance(expr, BinaryExpr):
            return self.eval_binary_expr(expr)
    
    def eval_binary_expr(self, expr: BinaryExpr) -> int:
        LHS = self.eval_expression(expr.value)
        assert isinstance(LHS, int), "Expected integer literal on LHS of BinaryExpr"
        RHS = self.eval_expression(expr.right)
        assert isinstance(RHS, int), "Expected integer literal on RHS of BinaryExpr"
        return LHS + RHS
        

    def eval_integer_literal_expr(self, expr: IntLiteralExpr) -> int:
        return expr.value

    # get the pointer name to the string literal
    def eval_string_literal(self, literal: ArrayRefExpr) -> str:
        assert isinstance(literal.value, str), "Expected string literal pointer name to be a string"
        return literal.value


    def deref_ident(self, ident_ref: IdentRefExpr) -> Union[str, int]:
        ident_ref.print()
        assert ident_ref.ident_kind == IdentType.CONSTANT, f"Expected constant identifier at {format_location(ident_ref.token.location)}"
        assert ident_ref.value in self.constants, f"Identifier not defined as constant at {format_location(ident_ref.token.location)}"
        return self.constants[ident_ref.value].value

#endregion


    def parse_program(self) -> List[Statement]:
        print("Parsing program")
        AST: List[Statement] = []
        while True:
            expr = self.parse_top_level()
            if expr is None:
                print("Done parsing")
                return AST
            
            AST.append(expr)