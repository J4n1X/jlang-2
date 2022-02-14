from ast import expr
from pickle import TRUE
from typing import *

from JlangObjects import *

class ExpressionParser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.string_literals: List[str] = []
        self.index = 0
        self.cur_tok: Optional[Token] = self.__next_token()
        self.prototypes: Dict[str, FunProto] = {}
        self.global_vars: Dict[str, VarDefStmt] = {}
        self.scope_vars: Dict[str, VarDefStmt] = {}
        self.in_scope: bool = False

    def __next_token(self) -> Optional[Token]:
        if self.index >= len(self.tokens):
            self.cur_tok = None
        else:
            self.cur_tok = self.tokens[self.index]
            self.index += 1
        return self.cur_tok

    def __get_precedence(self) -> int:
        assert len(Manipulator) == 11, "Too many binary operators defined at ExpressionParser.__get_precendence"

        if self.cur_tok is None:
            return -1
        elif self.cur_tok.type == TokenType.MANIPULATOR and self.cur_tok.value in BINOP_PRECEDENCE:
            return BINOP_PRECEDENCE[self.cur_tok.text]
        else:
            return 0

    def __get_block(self, start_keyword: Keyword, end_keyword: Keyword):
        if self.cur_tok is None:
            raise Exception("Unexpected EOF")
        if self.cur_tok.value != start_keyword:
            raise Exception(f"Expected {start_keyword} at {format_location(self.cur_tok.location)}")
        self.__next_token()
        block = []
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
            arg = self.parse_primary()
            assert isinstance(arg, Expression), f"Expected Expression for argument but got {arg.token}"
            args.append(arg)
            if self.cur_tok.type == TokenType.PAREN_BLOCK_END:
                break
            elif self.cur_tok.type != TokenType.ARG_DELIMITER:
                raise Exception(f"Expected ',' at {format_location(self.cur_tok.location)}")
            self.__next_token()
        return args

    # get a reference object for the current identifier token
    def __get_ident_ref(self) -> Optional[IdentRefExpr]:
        assert len(IdentType) == 3, "Too many IdentTypes defined at ExpressionParser.parse_identifier_expression"
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
        else:
            return None
    
    # get a mutable reference to the current identifier token
    def __get_ident_def(self):
        assert len(IdentType) == 3, "Too many IdentTypes defined at ExpressionParser.parse_identifier_expression"
        if self.cur_tok.value in self.prototypes:
            raise Exception(f"Cannot get mutable identifiers for functions at {self.cur_tok.value} at {format_location(self.cur_tok.location)}")
        elif self.cur_tok.value in self.scope_vars:
            return self.scope_vars[self.cur_tok.value]
        elif self.cur_tok.value in self.global_vars:
            return self.global_vars[self.cur_tok.value]
        else:
            return None

    def parse_top_level(self) -> Optional[Statement]:
    # allowed on top level: function, assign
        if self.cur_tok is None:
            return None
        elif self.cur_tok.type == TokenType.EOE: # try again with the next token
            self.__next_token()
            return self.parse_top_level()
        elif self.cur_tok.type == TokenType.KEYWORD:
            if self.cur_tok.value == Keyword.FUNCTION:
                return self.parse_function_statement()
            elif self.cur_tok.value == Keyword.ASSIGN:
                self.parse_var_def_statement() 
                # we've added it to the global scope, so we can parse the next statement
                return self.parse_top_level()
        raise Exception(f"Unexpected keyword {self.cur_tok.value} on top level {self.cur_tok}")



    def parse_primary(self) -> Optional[Statement]:
        assert len(TokenType) == 10, "Too many TokenTypes defined at ExpressionParser.parse_primary"

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
        else:
            raise Exception(f"Unexpected token {self.cur_tok}")
        return ret_expr


    def parse_statement(self) -> Optional[Statement]:
        expr = self.parse_primary()

        if expr is None:
            return None    

        if isinstance(expr, BinaryExpr) \
            or isinstance(expr, IntLiteralExpr) \
            or isinstance(expr, StringLiteralExpr) \
            or isinstance(expr, IdentRefExpr):
            return self.parse_binary_expression(0, expr)
        else:
            return expr

    def parse_keyword(self) -> Statement:
        assert len(Keyword) == 19, "Too many keywords defined at ExpressionParser.parse_keyword"
        assert self.cur_tok is not None, "Unexpected EOF"
        if self.cur_tok.value == Keyword.PRINT:
            return self.parse_print_statement()
        elif self.cur_tok.value == Keyword.IF:
            return self.parse_control_statement(Keyword.IF)
        elif self.cur_tok.value == Keyword.WHILE:
            return self.parse_control_statement(Keyword.WHILE)
        elif self.cur_tok.value == Keyword.FUNCTION:
            return self.parse_function_statement()
        elif self.cur_tok.value == Keyword.DEFINE:
            return self.parse_var_def_statement()
        elif self.cur_tok.value == Keyword.DROP:
            return self.parse_drop_statement()
        elif self.cur_tok.value == Keyword.RETURN:
            return self.parse_return_statement()
        elif self.cur_tok.value in valid_syscalls:
            return self.parse_syscall_expression()
        else: 
            raise Exception(f"Unexpected keyword {self.cur_tok}")

    def parse_fun_proto_statement(self):
        if(self.cur_tok.value != Keyword.FUNCTION):
            raise Exception(f"Expected keyword {Keyword.FUNCTION} at {format_location(self.cur_tok.location)}")
        prev_tok = self.cur_tok
        self.__next_token()
        if self.cur_tok.type != TokenType.IDENTIFIER:
            raise Exception(f"Expected identifier after function keyword at {format_location(self.cur_tok.location)}, but got {self.cur_tok.value}")

        ident = self.__get_ident_ref()
        if ident is not None:
            assert len(IdentType) == 3, "Too many IdentTypes defined at ExpressionParser.parse_fun_proto_statement"
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
        self.__next_token() # eat the done keyword
        print("Parsed function statement, ended at %s" % (format_location(self.cur_tok.location) if self.cur_tok is not None else "EOF"))
        self.in_scope = False 

        fun = FunStmt(proto, block, self.scope_vars.copy(), proto.type)

        self.scope_vars.clear()
        return fun

    # TODO: make it so we don't need to add a eoe token at the end
    def parse_control_statement(self, type: Keyword):
        assert self.cur_tok is not None, "Unexpected EOF"
        print("Parsing control at ", self.cur_tok)
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
        #self.__next_token() # eat the ')'


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
            raise Exception(f"Expected 'as' after identifier at {format_location(prev_tok.location)}")
        else:
            if self.cur_tok.value != Keyword.AS:
                raise Exception(f"Expected 'as' after identifier at {format_location(prev_tok.location)}")
        self.__next_token() # eat 'as' keyword
        
        if self.cur_tok.type != TokenType.TYPE:
            raise Exception(f"Expected type after identifier at {format_location(prev_tok.location)}")
        if not isinstance(self.cur_tok.value, ExprType):
            raise Exception(f"Expected type after identifier at {format_location(prev_tok.location)}")
        ident_var_type = self.cur_tok.value
        self.__next_token() # eat type
        
        value: Optional[Expression] = None
        if self.cur_tok.type == TokenType.KEYWORD:
            if self.cur_tok.value == Keyword.IS:
                self.__next_token() # eat 'is' keyword
                value = self.parse_statement()
                if not isinstance(value, Expression):
                    raise ValueError(f"Expected expression after define keyword at {format_location(self.cur_tok.location)}")

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
                    # instead, create a new statement with the new value
                    var_def = VarDefStmt(prev_tok, ident_name, IdentType.GLOBAL_VARIABLE, ExprType.ANY, value)
                elif ident.var_type == IdentType.VARIABLE:
                    # the variable exists, modify it
                    assert False, "Redefinition of variable in 'define' not allowed"
                    var_def = VarDefStmt(prev_tok, ident_name, IdentType.VARIABLE, ExprType.ANY, value)
            else:
                # the variable doesn't exist, define it, then add it to the scope
                var_def = VarDefStmt(prev_tok, ident_name, IdentType.VARIABLE, ident_var_type, value)
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
                var_def = VarDefStmt(prev_tok, ident_name, IdentType.GLOBAL_VARIABLE,ident_var_type, value)
                self.global_vars[ident_name] = var_def
        #self.__next_token() # eat identifier
        #if self.cur_tok.type != TokenType.EOE:
        #    raise Exception(f"At {format_location(self.cur_tok.location)}, expected end of statement")
        
        return var_def

    def parse_var_set_statement(self):
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        ident = self.__get_ident_def()
        if ident is None:
            raise Exception(f"Invalid Identifier {self.cur_tok.value} at {format_location(self.cur_tok.location)}")
        if ident.ident_kind != IdentType.VARIABLE and ident.ident_kind != IdentType.VARIABLE:
            raise Exception(f"Invalid reference to identifier {self.cur_tok.value} of type {ident.ident_kind} at {format_location(self.cur_tok.location)}")
        self.__next_token()

        if self.cur_tok.value != TokenType.KEYWORD and self.cur_tok.value == Keyword.IS:
            raise Exception(f"Expected 'is' after identifer at {format_location(self.cur_tok.location)}")

        value = self.parse_statement()
        return VarSetStmt(prev_tok, ident.name, value)

    def parse_print_statement(self) -> PrintStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token()
        # move on and parse more expressions
        expr = self.parse_statement()
        assert isinstance(expr, Expression), "Expected expression after print keyword at %s" % (format_location(prev_tok.location))

        # move past the eoe
        #self.__next_token()
        return PrintStmt(prev_tok, expr)

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
        value = self.parse_statement()
        assert isinstance(value, Expression), f"Value of return must be an Expression at {format_location(prev_tok.location)}"
        return ReturnStmt(prev_tok, value)


    def parse_syscall_expression(self) -> SyscallExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok

        assert isinstance(self.cur_tok.value, Keyword)
        if self.cur_tok.value not in valid_syscalls:
            raise Exception(f"{self.cur_tok} is not a valid syscall number")
        arg_count = valid_syscalls.index(self.cur_tok.value)
        self.__next_token()
        args: List[Expression] = self.__get_call_args()
        self.__next_token() # eat the ')'
        callnum = args[0]
        args.remove(args[0])

        return SyscallExpr(prev_tok, prev_tok.value, callnum, args)

    def parse_ident(self) -> Statement:
        next_token = self.tokens[self.index]
        assert self.cur_tok is not None, "Unexpected EOF"
        ident = self.__get_ident_ref()
        if ident is None:
            raise Exception(f"Expected identifier at {format_location(self.cur_tok.location)}")
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

    def parse_string_literal_expression(self) -> StringLiteralExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        assert isinstance(self.cur_tok.value, str), "Expected string literal at %s" % (format_location(self.cur_tok.location))
        # check if the string literal already exists
        string_id = len(self.string_literals)
        string_ref = f"str_{string_id}"
        self.string_literals.append(self.cur_tok.value)
        retval = StringLiteralExpr(self.cur_tok, string_ref)
        self.__next_token()
        return retval

    def parse_binary_expression(self, prec: int, LHS: Expression) -> Expression:
        assert self.cur_tok is not None, "Unexpected EOF"
        while self.cur_tok.type == TokenType.MANIPULATOR:
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




    def parse_program(self) -> List[Statement]:
        print("Parsing program")
        AST: List[Statement] = []
        while True:
            expr = self.parse_top_level()
            if expr is None:
                print("Done parsing")
                return AST
            
            AST.append(expr)