from typing import *
from JlangObjects import *

class Tokenizer:
    @staticmethod
    def __try_parse_int_literal(s: str) -> Optional[int]:
        try:
            return int(s)
        except ValueError:
            return None
    @staticmethod
    def __get_word_from_string(string: str) -> str:
        i = 0
        while i < len(string) and string[i].isalnum() or string[i] == '-' or string[i] == '_':
            i += 1
            if i >= len(string):
                break
        return string[0:i]
    @staticmethod
    def __get_int_text_from_string(string: str) -> str:
        i = 0
        while i < len(string) and string[i].isdigit():
            i += 1
        return string[0:i]

    @staticmethod
    def __get_escape_value(char: str) -> str:
        if char == 'n':
            return '\n'
        elif char == '\\':
            return '\\'
        else:
            assert False, f"\\{char}: Escape sequence for this char is not yet supported"

    def __init__(self, filename):
        self.filename = filename
        self.tokens = []
        self.line = 0
        self.column = 1
        with open(filename, 'r') as f:
            lines = f.readlines()
            for (line_number, line) in enumerate(lines):
                self.line += 1
                assert len(TokenType) == 13, "Too many TokenTypes defined at Tokenizer init"
                assert len(Keyword) == 17, "Too many Keywords defined at Tokenizer init"
                assert len(Operator) == 11, "Too many Manipulators defined at Tokenizer init"
                
                char_pos = 0
                while char_pos < len(line):
                    while line[char_pos].isspace():
                        if char_pos + 1 >= len(line):
                            break
                        char_pos += 1
                    self.column = char_pos + 1
                    if line[char_pos] == '\n':
                        self.column = 1
                        break


                    word = ""
                    if line[char_pos].isalpha():
                        word = Tokenizer.__get_word_from_string(line[char_pos:])
                        char_pos += len(word)
                        if word in KEYWORDS_BY_NAME:
                            self.tokens.append(Token(TokenType.KEYWORD, word, (self.filename, self.line, self.column), KEYWORDS_BY_NAME[word]))
                        elif word in OPERATOR_BY_NAME:
                            self.tokens.append(Token(TokenType.OPERATOR, word, (self.filename, self.line, self.column), OPERATOR_BY_NAME[word]))
                        elif word in EXPRTYPE_BY_NAME:
                            self.tokens.append(Token(TokenType.TYPE, word, (self.filename, self.line, self.column), EXPRTYPE_BY_NAME[word]))
                        elif word in SYSCALL_BY_NAME:
                            self.tokens.append(Token(TokenType.SYSCALL, word, (self.filename, self.line, self.column), SYSCALL_BY_NAME[word]))
                        elif word in LOADER_BY_NAME:
                            self.tokens.append(Token(TokenType.LOADER, word, (self.filename, self.line, self.column), LOADER_BY_NAME[word]))
                        elif word in STORER_BY_NAME:
                            self.tokens.append(Token(TokenType.STORER, word, (self.filename, self.line, self.column), STORER_BY_NAME[word]))
                        else:
                            if '-' in word:
                                raise Exception("Invalid identifier: " + word)

                            self.tokens.append(Token(TokenType.IDENTIFIER, word, (self.filename, self.line, self.column), word))  
                    elif line[char_pos].isdigit():
                        word = Tokenizer.__get_int_text_from_string(line[char_pos:])
                        char_pos += len(word)
                        if Tokenizer.__try_parse_int_literal(word) is not None:
                            self.tokens.append(Token(TokenType.INT_LITERAL, word, (self.filename, self.line, self.column), int(word)))
                        else:
                            raise ValueError(f"Invalid integer literal at {self.filename}:{self.line}:{self.column}")
                    elif line[char_pos] == '"':
                        char_pos += 1
                        while line[char_pos] != '"':
                            if line[char_pos] == '\\':
                                char_pos += 1
                                word += Tokenizer.__get_escape_value(line[char_pos])
                            else:
                                word += line[char_pos]
                            char_pos += 1
                        char_pos += 1
                        self.tokens.append(Token(TokenType.STRING_LITERAL, word, (self.filename, self.line, self.column), word))
                    elif line[char_pos] == ',': # end of expression
                        self.tokens.append(Token(TokenType.ARG_DELIMITER, None, (self.filename, self.line, self.column)))
                        char_pos += 1
                    elif line[char_pos] == ';': # this is a comment
                        break
                    elif line[char_pos] == '(': # paren block start
                        char_pos += 1
                        self.tokens.append(Token(TokenType.PAREN_BLOCK_START, '(', (self.filename, self.line, self.column)))
                    elif line[char_pos] == ')': # paren block end
                        char_pos += 1
                        self.tokens.append(Token(TokenType.PAREN_BLOCK_END, ')', (self.filename, self.line, self.column)))
                    else:
                        raise ValueError(f"Invalid starting character {line[char_pos]} for token at {self.filename}:{self.line}:{self.column}")

                    if line_number != len(lines):
                        self.column = char_pos + 1
                    else:
                        self.tokens.append(Token(TokenType.EOE, None, (self.filename, self.line, self.column)))
                        self.column = char_pos + 1
