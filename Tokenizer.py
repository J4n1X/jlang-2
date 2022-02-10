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


    def __init__(self, filename):
        self.filename = filename
        self.tokens = []
        self.line = 0
        self.column = 1
        with open(filename, 'r') as f:
            lines = f.readlines()
            for (line_number, line) in enumerate(lines):
                self.line += 1
                assert len(TokenType) == 6, "Too many TokenTypes defined at Tokenizer init"
                assert len(Keyword) == 15, "Too many Keywords defined at Tokenizer init"
                assert len(Manipulator) == 11, "Too many Manipulators defined at Tokenizer init"
                
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
                        elif word in MANIPULATOR_BY_NAME:
                            self.tokens.append(Token(TokenType.MANIPULATOR, word, (self.filename, self.line, self.column), MANIPULATOR_BY_NAME[word]))
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
                            word += line[char_pos]
                            char_pos += 1
                        char_pos += 1
                        self.tokens.append(Token(TokenType.STRING_LITERAL, word, (self.filename, self.line, self.column), word))
                    elif line[char_pos] == '.': # end of expression
                        self.tokens.append(Token(TokenType.EOE, None, (self.filename, self.line, self.column)))
                        char_pos += 1
                    elif line[char_pos] == ';': # this is a comment
                        break
                    else:
                        raise ValueError(f"Invalid starting character {line[char_pos]} for token at {self.filename}:{self.line}:{self.column}")

                    if line_number != len(lines):
                        self.column = char_pos + 1
                    else:
                        self.column = char_pos + 1
