import re

class Token:
    def __init__(self, type, value, line=None, col=None):
        self.type = type
        self.value = value
        self.line = line
        self.col = col
    def __repr__(self):
        return f"Token({self.type}, {self.value!r})"

class Lexer:
    def __init__(self, text):
        self.text = text
        self.pos = 0
        self.line = 1
        self.col = 1

    def peek_char(self):
        if self.pos < len(self.text):
            return self.text[self.pos]
        return None

    def advance_char(self):
        ch = self.peek_char()
        if ch is not None:
            self.pos += 1
            if ch == '\n':
                self.line += 1
                self.col = 1
            else:
                self.col += 1
        return ch

    def skip_whitespace_and_comments(self):
        while True:
            while self.peek_char() is not None and self.peek_char().isspace():
                self.advance_char()
            if self.peek_char() == '/' and self.pos+1 < len(self.text) and self.text[self.pos+1] == '/':
                while self.peek_char() is not None and self.peek_char() != '\n':
                    self.advance_char()
            elif self.peek_char() == '/' and self.pos+1 < len(self.text) and self.text[self.pos+1] == '*':
                self.advance_char(); self.advance_char()
                while True:
                    if self.peek_char() is None:
                        break
                    if self.peek_char() == '*' and self.pos+1 < len(self.text) and self.text[self.pos+1] == '/':
                        self.advance_char(); self.advance_char()
                        break
                    self.advance_char()
            elif self.peek_char() == '#':
                while self.peek_char() is not None and self.peek_char() != '\n':
                    self.advance_char()
            else:
                break

    def read_identifier(self):
        start_line, start_col = self.line, self.col
        chars = []
        while self.peek_char() is not None and (self.peek_char().isalnum() or self.peek_char() == '_'):
            chars.append(self.advance_char())
        return Token('identifier', ''.join(chars), start_line, start_col)

    def read_number(self):
        start_line, start_col = self.line, self.col
        chars = []
        while self.peek_char() is not None and (self.peek_char().isdigit() or self.peek_char() in '.eE+-'):
            chars.append(self.advance_char())
        return Token('number', ''.join(chars), start_line, start_col)

    def read_string(self):
        start_line, start_col = self.line, self.col
        quote = self.advance_char()
        chars = [quote]
        while True:
            ch = self.advance_char()
            if ch is None: break
            chars.append(ch)
            if ch == '\\':
                nxt = self.advance_char()
                if nxt is not None: chars.append(nxt)
            elif ch == quote:
                break
        return Token('string', ''.join(chars), start_line, start_col)

    def next_token(self):
        self.skip_whitespace_and_comments()
        line, col = self.line, self.col
        ch = self.peek_char()
        if ch is None:
            return Token('eof', None, line, col)
        if ch.isalpha() or ch == '_':
            token = self.read_identifier()
            word = token.value
            if word.startswith('WENGINE_'):
                save_pos, save_line, save_col = self.pos, self.line, self.col
                self.skip_whitespace_and_comments()
                if self.peek_char() == '(':
                    depth = 1
                    chars = [self.advance_char()]
                    while depth > 0 and self.peek_char() is not None:
                        ch2 = self.advance_char()
                        chars.append(ch2)
                        if ch2 == '"' or ch2 == "'":
                            quote = ch2
                            while True:
                                nxt = self.advance_char()
                                if nxt is None: break
                                chars.append(nxt)
                                if nxt == '\\':
                                    esc = self.advance_char()
                                    if esc is not None: chars.append(esc)
                                elif nxt == quote: break
                        elif ch2 == '/' and self.peek_char() == '/':
                            while self.peek_char() is not None and self.peek_char() != '\n':
                                chars.append(self.advance_char())
                        elif ch2 == '/' and self.peek_char() == '*':
                            while True:
                                nxt = self.advance_char()
                                if nxt is None: break
                                chars.append(nxt)
                                if nxt == '*' and self.peek_char() == '/':
                                    chars.append(self.advance_char()); break
                        elif ch2 == '(': depth += 1
                        elif ch2 == ')': depth -= 1
                    return Token('macro_wengine', word + ''.join(chars), token.line, token.col)
                else:
                    self.pos, self.line, self.col = save_pos, save_line, save_col
                    return Token('macro_wengine', word, token.line, token.col)
            keywords = {
                'class','struct','public','private','protected','virtual','static','const','template','typename',
                'int','float','double','void','bool','char','unsigned','long','short','signed','auto','explicit',
                'friend','typedef','using','namespace','operator','new','delete','this','return','if','else',
                'for','while','do','switch','case','default','break','continue','goto','try','catch','throw',
                'constexpr','consteval','constinit','mutable','volatile','inline','enum','union','extern',
                'register','asm','sizeof','alignas','alignof','noexcept','override','final','decltype',
                'nullptr','true','false'
            }
            if word in keywords:
                return Token('keyword', word, token.line, token.col)
            return token
        if ch.isdigit() or (ch == '.' and self.pos+1 < len(self.text) and self.text[self.pos+1].isdigit()):
            return self.read_number()
        if ch == '"':
            return self.read_string()
        two_chars = self.text[self.pos:self.pos+2]
        if two_chars in ('::', '->'):
            self.advance_char(); self.advance_char()
            return Token('operator', two_chars, line, col)
        self.advance_char()
        return Token('punctuation', ch, line, col)

class FieldInfo:
    def __init__(self):
        self.type = ''
        self.name = ''
        self.macros = []
        self.is_static = False
        self.is_const = False
        self.initializer = None

class MethodInfo:
    def __init__(self):
        self.type = ''
        self.name = ''
        self.params = ''
        self.qualifiers = ''
        self.macros = []
        self.is_static = False
        self.is_virtual = False
        self.is_const = False
        self.is_pure_virtual = False
        self.template_params = None
        self.body = None

class ClassInfo:
    def __init__(self):
        self.name = ''
        self.kind = ''
        self.template_params = None
        self.base_classes = []
        self.macros = []
        self.fields = []
        self.methods = []
        self.nested_classes = []

class Parser:
    def __init__(self, lexer):
        self.lexer = lexer
        self.current_token = None
        self.advance()
        self.keywords_set = {
            'class','struct','public','private','protected','virtual','static','const','template','typename',
            'int','float','double','void','bool','char','unsigned','long','short','signed','auto','explicit',
            'friend','typedef','using','namespace','operator','new','delete','this','return','if','else',
            'for','while','do','switch','case','default','break','continue','goto','try','catch','throw',
            'constexpr','consteval','constinit','mutable','volatile','inline','enum','union','extern',
            'register','asm','sizeof','alignas','alignof','noexcept','override','final','decltype',
            'nullptr','true','false'
        }

    def advance(self):
        self.current_token = self.lexer.next_token()

    def expect(self, token_type=None, value=None):
        tok = self.current_token
        if token_type and tok.type != token_type:
            raise SyntaxError(f"Expected {token_type}, got {tok.type} ({tok.value})")
        if value and tok.value != value:
            raise SyntaxError(f"Expected {value}, got {tok.value}")
        self.advance()
        return tok

    def read_balanced_tokens(self, open_char, close_char):
        """Читает сбалансированный блок, начиная с текущего токена open_char."""
        depth = 1
        tokens_text = []
        self.advance()  # consume open
        while depth > 0:
            tok = self.current_token
            if tok.type == 'eof':
                raise SyntaxError("Unexpected EOF in balanced block")
            if tok.type == 'punctuation':
                if tok.value == open_char:
                    depth += 1
                elif tok.value == close_char:
                    depth -= 1
                    if depth == 0:
                        self.advance()
                        break
            tokens_text.append(tok.value)
            self.advance()
        return ' '.join(tokens_text).strip()

    def read_template_params(self):
        self.advance()  # consume 'template'
        if not (self.current_token.type == 'punctuation' and self.current_token.value == '<'):
            raise SyntaxError("Expected '<' after template")
        return self.read_balanced_tokens('<', '>')

    def parse_class(self, template_params=None, macros=None):
        kind = self.current_token.value  # 'class' или 'struct'
        self.advance()
        name_tok = self.expect('identifier')
        class_info = ClassInfo()
        class_info.kind = kind
        class_info.name = name_tok.value
        if template_params: class_info.template_params = template_params
        if macros: class_info.macros = macros

        # Проверка на forward declaration
        if self.current_token.type == 'punctuation' and self.current_token.value == ';':
            self.advance()  # пропускаем ';'
            return None     # или можно вернуть class_info с пометкой forward

        if self.current_token.type == 'punctuation' and self.current_token.value == ':':
            self.advance()
            while True:
                access = 'private' if kind == 'class' else 'public'
                is_virtual = False
                if self.current_token.type == 'keyword' and self.current_token.value == 'virtual':
                    is_virtual = True; self.advance()
                if self.current_token.type == 'keyword' and self.current_token.value in ('public','private','protected'):
                    access = self.current_token.value; self.advance()
                base_parts = []
                while not (self.current_token.type == 'punctuation' and self.current_token.value in (',', '{')):
                    base_parts.append(self.current_token.value)
                    self.advance()
                class_info.base_classes.append((access, ' '.join(base_parts).strip(), is_virtual))
                if self.current_token.value == ',':
                    self.advance(); continue
                elif self.current_token.value == '{':
                    break
                else:
                    raise SyntaxError("Unexpected token in base class list")
        if not (self.current_token.type == 'punctuation' and self.current_token.value == '{'):
            raise SyntaxError("Expected '{' to start class body")
        self.advance()
        self.parse_class_body(class_info)
        return class_info

    def parse_class_body(self, class_info):
        pending_macros = []
        current_template = None
        while True:
            tok = self.current_token
            if tok.type == 'eof':
                raise SyntaxError("Unexpected EOF in class body")
            if tok.type == 'punctuation':
                if tok.value == ';':
                    self.advance(); continue
                elif tok.value == '}':
                    self.advance()
                    if self.current_token.type == 'punctuation' and self.current_token.value == ';':
                        self.advance()
                    break
                else:
                    self.advance(); continue
            if tok.type == 'keyword':
                if tok.value in ('public','private','protected'):
                    self.advance()
                    if self.current_token.type == 'punctuation' and self.current_token.value == ':':
                        self.advance()
                    continue
                elif tok.value == 'template':
                    current_template = self.read_template_params()
                    continue
                elif tok.value in ('class','struct'):
                    nested = self.parse_class(current_template, pending_macros)
                    class_info.nested_classes.append(nested)
                    current_template = None; pending_macros = []
                    continue
                else:
                    member = self.parse_member(pending_macros, current_template)
                    if member is not None:
                        if isinstance(member, FieldInfo): class_info.fields.append(member)
                        elif isinstance(member, MethodInfo): class_info.methods.append(member)
                    pending_macros = []; current_template = None
                    continue
            if tok.type == 'macro_wengine':
                pending_macros.append(tok.value)
                self.advance(); continue
            if tok.type == 'identifier':
                member = self.parse_member(pending_macros, current_template)
                if member is not None:
                    if isinstance(member, FieldInfo): class_info.fields.append(member)
                    elif isinstance(member, MethodInfo): class_info.methods.append(member)
                pending_macros = []; current_template = None
                continue
            self.advance()

    def parse_member(self, macros, template_params):
        member_tokens = []
        is_static = False; is_virtual = False; is_const = False
        while self.current_token.type == 'keyword' and self.current_token.value in (
            'virtual','static','const','mutable','explicit','inline','constexpr','friend'):
            val = self.current_token.value
            if val == 'static': is_static = True
            elif val == 'virtual': is_virtual = True
            elif val == 'const': is_const = True
            member_tokens.append(val)
            self.advance()
        while True:
            tok = self.current_token
            if tok.type == 'eof':
                break
            if tok.type == 'punctuation' and tok.value in ('(', ';', '=', '{', ':'):
                break
            if tok.type == 'keyword' and tok.value == 'template':
                break
            member_tokens.append(tok.value)
            self.advance()
        if not member_tokens:
            return None
        name = None; name_index = -1
        for i in range(len(member_tokens)-1, -1, -1):
            val = member_tokens[i]
            if val not in self.keywords_set and re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', val):
                name = val; name_index = i; break
        if name is None:
            return None
        type_str = ' '.join(member_tokens[:name_index]).strip()
        if self.current_token.type == 'punctuation' and self.current_token.value == '(':
            method = MethodInfo()
            method.type = type_str; method.name = name
            method.macros = macros; method.template_params = template_params
            method.is_static = is_static; method.is_virtual = is_virtual; method.is_const = is_const
            method.params = self.read_balanced_tokens('(', ')')
            qualifiers = []
            while True:
                tok = self.current_token
                if tok.type == 'eof': break
                if tok.type == 'punctuation' and tok.value in ('{', ';', '='): break
                qualifiers.append(tok.value)
                self.advance()
            method.qualifiers = ' '.join(qualifiers).strip()
            if 'const' in qualifiers: method.is_const = True
            if self.current_token.type == 'punctuation' and self.current_token.value == '=':
                self.advance()
                value_tokens = []
                while not (self.current_token.type == 'punctuation' and self.current_token.value == ';'):
                    value_tokens.append(self.current_token.value)
                    self.advance()
                if ' '.join(value_tokens).strip() == '0':
                    method.is_pure_virtual = True
                self.advance()
            elif self.current_token.type == 'punctuation' and self.current_token.value == '{':
                method.body = self.read_balanced_tokens('{', '}')
            elif self.current_token.type == 'punctuation' and self.current_token.value == ';':
                self.advance()
            return method
        elif self.current_token.type == 'punctuation' and self.current_token.value == ';':
            field = FieldInfo()
            field.type = type_str; field.name = name; field.macros = macros
            field.is_static = is_static; field.is_const = is_const
            self.advance()
            return field
        elif self.current_token.type == 'punctuation' and self.current_token.value == '=':
            field = FieldInfo()
            field.type = type_str; field.name = name; field.macros = macros
            field.is_static = is_static; field.is_const = is_const
            self.advance()
            init_tokens = []
            while not (self.current_token.type == 'punctuation' and self.current_token.value == ';'):
                init_tokens.append(self.current_token.value)
                self.advance()
            field.initializer = ' '.join(init_tokens).strip()
            self.advance()
            return field
        elif self.current_token.type == 'punctuation' and self.current_token.value == '{':
            method = MethodInfo()
            method.type = type_str; method.name = name; method.macros = macros
            method.template_params = template_params
            method.is_static = is_static; method.is_virtual = is_virtual; method.is_const = is_const
            method.params = ''
            method.body = self.read_balanced_tokens('{', '}')
            return method
        return None

    def parse(self):
        classes = []
        pending_macros = []
        current_template = None
        while self.current_token.type != 'eof':
            tok = self.current_token
            if tok.type == 'punctuation' and tok.value == ';':
                self.advance(); continue
            if tok.type == 'macro_wengine':
                pending_macros.append(tok.value)
                self.advance(); continue
            if tok.type == 'keyword' and tok.value == 'template':
                current_template = self.read_template_params()
                continue
            if tok.type == 'keyword' and tok.value in ('class','struct'):
                class_info = self.parse_class(current_template, pending_macros)
                classes.append(class_info)
                pending_macros = []; current_template = None
                continue
            self.advance()
        return classes