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
            print("Currently at: ",self.cur_tok)
            if self.cur_tok is None or self.cur_tok.value == end_keyword:
                break
            block.append(self.parse_statement())
        return block

    # get a reference object for the current identifier token
    def __get_ident_ref(self) -> Optional[IdentRefExpr]:
        assert len(IdentType) == 3, "Too many IdentTypes defined at ExpressionParser.parse_identifier_expression"
        assert self.cur_tok is not None, "Unexpected EOF"
        assert isinstance(self.cur_tok.value, str), "Expected string value for identifier"
        if self.cur_tok is None:
            return None
        elif self.cur_tok.value in self.prototypes:
            return IdentRefExpr(self.cur_tok, self.cur_tok.value, IdentType.FUNCTION)
        elif self.cur_tok.value in self.scope_vars:
            return IdentRefExpr(self.cur_tok, self.cur_tok.value, IdentType.VARIABLE)
        elif self.cur_tok.value in self.global_vars:
            return IdentRefExpr(self.cur_tok, self.cur_tok.value, IdentType.GLOBAL_VARIABLE)
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
        assert len(TokenType) == 6, "Too many TokenTypes defined at ExpressionParser.parse_primary"

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
            ret_expr = self.parse_ident_expression()
        elif self.cur_tok.type == TokenType.INT_LITERAL:
            ret_expr = self.parse_int_literal_expression()
        elif self.cur_tok.type == TokenType.STRING_LITERAL:
            ret_expr = self.parse_string_literal_expression()
        else:
            raise Exception(f"Unexpected token {self.cur_tok}")
        
        self.__next_token()
        return ret_expr


    def parse_statement(self) -> Optional[Statement]:
        expr = self.parse_primary()

        if expr is None:
            return None
        
        expr.print()

        if isinstance(expr, BinaryExpr) \
            or isinstance(expr, IntLiteralExpr) \
            or isinstance(expr, StringLiteralExpr) \
            or isinstance(expr, IdentRefExpr):
            return self.parse_binary_expression(0, expr)
        else:
            return expr

    def parse_keyword(self) -> Statement:
        assert len(Keyword) == 15, "Too many keywords defined at ExpressionParser.parse_keyword"
        assert self.cur_tok is not None, "Unexpected EOF"
        if self.cur_tok.value == Keyword.PRINT:
            return self.parse_print_statement()
        elif self.cur_tok.value == Keyword.IF:
            return self.parse_control_statement(Keyword.IF)
        elif self.cur_tok.value == Keyword.WHILE.WHILE:
            return self.parse_control_statement(Keyword.WHILE)
        elif self.cur_tok.value == Keyword.FUNCTION:
            return self.parse_function_statement()
        elif self.cur_tok.value == Keyword.CALL:
            return self.parse_function_call_statement()
        elif self.cur_tok.value == Keyword.ASSIGN:
            return self.parse_var_def_statement()
        else: 
            raise Exception(f"Unexpected keyword {self.cur_tok}")

    def parse_fun_proto_statement(self):
        if(self.cur_tok.value != Keyword.FUNCTION):
            raise Exception(f"Expected keyword {Keyword.FUNCTION} at {format_location(self.cur_tok.location)}")
        prev_tok = self.cur_tok
        self.__next_token()
        if self.cur_tok.type != TokenType.IDENTIFIER:
            raise Exception(f"Expected identifier after function keyword at {format_location(prev_tok)}")

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

        return FunProto(prev_tok, name)


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

        fun = FunStmt(proto, block, self.scope_vars.copy())

        self.scope_vars.clear()
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
        self.__next_token()
        target = self.__get_ident_ref()

        if target is None: # the identifier was not found
            raise Exception(f"Invalid identifier after call keyword at {format_location(prev_tok.location)}")
        elif target.ident_type != IdentType.FUNCTION:
            raise Exception(f"Attempted to invoke function call with non-function identifier at {format_location(prev_tok.location)}")
        self.__next_token() # eat identifier

        # arguments would be parsed here

        if self.cur_tok.type != TokenType.EOE:
            raise Exception(f"At {format_location(self.cur_tok.location)}, expected end of statement")
        #self.__next_token()
        return FunCallStmt(prev_tok, target.value)

    def parse_var_def_statement(self) -> VarDefStmt:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token()

        if self.cur_tok.type != TokenType.INT_LITERAL \
            and self.cur_tok.type != TokenType.STRING_LITERAL \
            and self.cur_tok.type != TokenType.IDENTIFIER:
            raise Exception(f"Expected literal or identifier after assign keyword at {format_location(prev_tok.location)}")
        
        value = self.parse_statement()
        if not isinstance(value, Expression):
            raise ValueError(f"Expected expression after assign keyword at {format_location(prev_tok.location)}")
        
        if self.cur_tok.value != Keyword.TO:
            raise Exception(f"Expected to keyword after expression at {format_location(prev_tok.location)}")
        self.__next_token() # eat to keyword

        if self.cur_tok.type != TokenType.IDENTIFIER:
            raise Exception(f"Expected identifier after to keyword at {format_location(prev_tok.location)}")

        ident: VarDefStmt = self.__get_ident_def()
        
        assert isinstance(self.cur_tok.value, str), "Expected identifier string after to keyword at %s" % (format_location(prev_tok.location))
        name = self.cur_tok.value

        if self.in_scope:
            if ident is not None:
                assert len(IdentType) == 3, "Too many IdentTypes defined at ExpressionParser.parse_var_def_statement"
                if ident.var_type == IdentType.FUNCTION:
                    raise Exception(f"Attempted redefinition of Function {ident.value} at {format_location(self.cur_tok.location)}; already defined at {format_location(ident.token.location)}")
                elif ident.var_type == IdentType.GLOBAL_VARIABLE:
                    # do not add to the list of global variables, as it is already there
                    # instead, create a new statement with the new value
                    var_def = VarDefStmt(prev_tok, name, IdentType.GLOBAL_VARIABLE, ExprType.ANY, value)
                elif ident.var_type == IdentType.VARIABLE:
                    # the variable exists, modify it
                    var_def = VarDefStmt(prev_tok, name, IdentType.VARIABLE, ExprType.ANY, value)
            else:
                # the variable doesn't exist, define it, then add it to the scope
                var_def = VarDefStmt(prev_tok, name, IdentType.VARIABLE, ExprType.ANY, value)
                self.scope_vars[name] = var_def
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
                var_def = VarDefStmt(prev_tok, name, IdentType.GLOBAL_VARIABLE,ExprType.ANY, value)
                self.global_vars[name] = var_def
        self.__next_token() # eat identifier
        if self.cur_tok.type != TokenType.EOE:
            raise Exception(f"At {format_location(self.cur_tok.location)}, expected end of statement")
        
        return var_def

    def parse_print_statement(self) -> Statement:
        assert self.cur_tok is not None, "Unexpected EOF"
        prev_tok = self.cur_tok
        self.__next_token()
        # move on and parse more expressions
        expr = self.parse_statement()
        assert isinstance(expr, Expression), "Expected expression after print keyword at %s" % (format_location(prev_tok.location))
        return PrintStmt(prev_tok, expr)

    def parse_ident_expression(self) -> IdentRefExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        ident = self.__get_ident_ref()
        if ident is None:
            raise Exception(f"Expected identifier at {format_location(self.cur_tok.location)}")
        return ident

    def parse_int_literal_expression(self) -> IntLiteralExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        assert isinstance(self.cur_tok.value, int), "Expected integer literal at %s" % (format_location(self.cur_tok.location))
        return IntLiteralExpr(self.cur_tok, self.cur_tok.value)

    def parse_string_literal_expression(self) -> StringLiteralExpr:
        assert self.cur_tok is not None, "Unexpected EOF"
        assert isinstance(self.cur_tok.value, str), "Expected string literal at %s" % (format_location(self.cur_tok.location))
        # check if the string literal already exists
        string_id = len(self.string_literals)
        string_ref = f"str_{string_id}"
        self.string_literals.append(self.cur_tok.value)
        
        return StringLiteralExpr(self.cur_tok, string_ref)

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