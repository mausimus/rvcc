/* rvcc C compiler - lexer */

/* lexer tokens */
typedef enum {
	t_sof,
	t_numeric,
	t_identifier,
	t_comma,
	t_string,
	t_char,
	t_op_bracket,
	t_cl_bracket,
	t_op_curly,
	t_cl_curly,
	t_op_square,
	t_cl_square,
	t_star,
	t_bit_or,
	t_log_and,
	t_log_or,
	t_log_not,
	t_lt,
	t_gt,
	t_le,
	t_ge,
	t_lshift,
	t_rshift,
	t_dot,
	t_arrow,
	t_plus,
	t_minus,
	t_minuseq,
	t_pluseq,
	t_oreq,
	t_andeq,
	t_eq,
	t_noteq,
	t_assign,
	t_plusplus,
	t_minusminus,
	t_colon,
	t_semicolon,
	t_eof,
	t_ampersand,
	t_return,
	t_if,
	t_else,
	t_while,
	t_for,
	t_do,
	t_op_comment,
	t_cl_comment,
	t_define,
	t_include,
	t_typedef,
	t_enum,
	t_struct,
	t_sizeof,
	t_elipsis,
	t_asm,
	t_switch,
	t_case,
	t_break,
	t_default
} l_token;

char _l_token_string[MAX_TOKEN_LEN];
l_token _l_next_token;

void l_skip_whitespace()
{
	while (is_whitespace(_l_next_char)) {
		_l_next_char = _source[++_source_idx];
	}
}

char l_read_char(int skip_whitespace)
{
	_l_next_char = _source[++_source_idx];
	if (skip_whitespace == 1)
		l_skip_whitespace();
	return _l_next_char;
}

int l_read_alnum(char *buffer, int max_len)
{
	int bi = 0;
	while (is_alnum(l_read_char(0))) {
		buffer[bi++] = _l_next_char;
		if (bi >= max_len)
			error("Length exceeded");
	}
	buffer[bi] = 0;
	return bi;
}

l_token l_next_token()
{
	_l_token_string[0] = 0;
	if (_l_next_char == '#') {
		int i = 0;
		do {
			_l_token_string[i++] = _l_next_char;
		} while (is_alnum(l_read_char(0)));
		_l_token_string[i] = 0;
		l_skip_whitespace();

		if (strcmp(_l_token_string, "#include") == 0) {
			i = 0;
			do {
				_l_token_string[i++] = _l_next_char;
			} while (l_read_char(0) != '\n');
			l_skip_whitespace();
			return t_include;
		}
		if (strcmp(_l_token_string, "#define") == 0) {
			l_skip_whitespace();
			return t_define;
		}
		if (strcmp(_l_token_string, "#ifdef") == 0) {
			i = 0;
			do {
				_l_token_string[i++] = _l_next_char;
			} while (l_read_char(0) != '\n');
			_l_token_string[i] = 0;
			/* check if we have this alias/define */
			for (i = 0; i < _aliases_idx; i++) {
				if (strcmp(_l_token_string, _aliases[i].alias) == 0) {
					l_skip_whitespace();
					return l_next_token();
				}
			}
			/* skip lines until #endif */
			do {
				l_skip_whitespace();
				i = 0;
				do {
					_l_token_string[i++] = _l_next_char;
				} while (l_read_char(0) != '\n');
				_l_token_string[i] = 0;
			} while (strcmp(_l_token_string, "#endif") != 0);
			l_skip_whitespace();
			return l_next_token();
		}
		if (strcmp(_l_token_string, "#endif") == 0) {
			l_skip_whitespace();
			return l_next_token();
		}
		error("Unknown directive");
	}
	if (_l_next_char == '/') {
		l_read_char(0);
		if (_l_next_char == '*') {
			/* we are in a comment, skip until end */
			do {
				l_read_char(0);
				if (_l_next_char == '*') {
					l_read_char(0);
					if (_l_next_char == '/') {
						l_read_char(1);
						return l_next_token();
					}
				}
			} while (_l_next_char != 0);
		}
		error("Unexpected '/'"); /* invalid otherwise? */
	}

	if (is_digit(_l_next_char)) {
		int i = 0;
		do {
			_l_token_string[i++] = _l_next_char;
		} while (is_hex(l_read_char(0)));
		_l_token_string[i] = 0;
		l_skip_whitespace();
		return t_numeric;
	}
	if (_l_next_char == '(') {
		l_read_char(1);
		return t_op_bracket;
	}
	if (_l_next_char == ')') {
		l_read_char(1);
		return t_cl_bracket;
	}
	if (_l_next_char == '{') {
		l_read_char(1);
		return t_op_curly;
	}
	if (_l_next_char == '}') {
		l_read_char(1);
		return t_cl_curly;
	}
	if (_l_next_char == '[') {
		l_read_char(1);
		return t_op_square;
	}
	if (_l_next_char == ']') {
		l_read_char(1);
		return t_cl_square;
	}
	if (_l_next_char == ',') {
		l_read_char(1);
		return t_comma;
	}
	if (_l_next_char == '"') {
		int i = 0;
		int special = 0;

		while (l_read_char(0) != '"' || special) {
			if (i > 0 && _l_token_string[i - 1] == '\\') {
				if (_l_next_char == 'n')
					_l_token_string[i - 1] = '\n';
				else if (_l_next_char == '"')
					_l_token_string[i - 1] = '"';
				else if (_l_next_char == 'r')
					_l_token_string[i - 1] = '\r';
				else if (_l_next_char == '\'')
					_l_token_string[i - 1] = '\'';
				else if (_l_next_char == 't')
					_l_token_string[i - 1] = '\t';
				else if (_l_next_char == '\\')
					_l_token_string[i - 1] = '\\';
				else
					abort();
			} else {
				_l_token_string[i++] = _l_next_char;
			}
			if (_l_next_char == '\\')
				special = 1;
			else
				special = 0;
		}
		_l_token_string[i] = 0;
		l_read_char(1);
		return t_string;
	}
	if (_l_next_char == '\'') {
		l_read_char(0);
		if (_l_next_char == '\\') {
			l_read_char(0);
			if (_l_next_char == 'n')
				_l_token_string[0] = '\n';
			else if (_l_next_char == 'r')
				_l_token_string[0] = '\r';
			else if (_l_next_char == '\'')
				_l_token_string[0] = '\'';
			else if (_l_next_char == '"')
				_l_token_string[0] = '"';
			else if (_l_next_char == 't')
				_l_token_string[0] = '\t';
			else if (_l_next_char == '\\')
				_l_token_string[0] = '\\';
			else
				abort();
		} else {
			_l_token_string[0] = _l_next_char;
		}
		_l_token_string[1] = 0;
		if (l_read_char(0) != '\'')
			abort();
		l_read_char(1);
		return t_char;
	}
	if (_l_next_char == '*') {
		l_read_char(1);
		return t_star;
	}
	if (_l_next_char == '&') {
		l_read_char(0);
		if (_l_next_char == '&') {
			l_read_char(1);
			return t_log_and;
		};
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_andeq;
		}
		l_skip_whitespace();
		return t_ampersand;
	}
	if (_l_next_char == '|') {
		l_read_char(0);
		if (_l_next_char == '|') {
			l_read_char(1);
			return t_log_or;
		};
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_oreq;
		}
		l_skip_whitespace();
		return t_bit_or;
	}
	if (_l_next_char == '<') {
		l_read_char(0);
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_le;
		};
		if (_l_next_char == '<') {
			l_read_char(1);
			return t_lshift;
		};
		l_skip_whitespace();
		return t_lt;
	}
	if (_l_next_char == '>') {
		l_read_char(0);
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_ge;
		};
		if (_l_next_char == '>') {
			l_read_char(1);
			return t_rshift;
		};
		l_skip_whitespace();
		return t_gt;
	}
	if (_l_next_char == '!') {
		l_read_char(0);
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_noteq;
		}
		l_skip_whitespace();
		return t_log_not;
	}
	if (_l_next_char == '.') {
		l_read_char(0);
		if (_l_next_char == '.') {
			l_read_char(0);
			if (_l_next_char == '.') {
				l_read_char(1);
				return t_elipsis;
			} else {
				abort();
			}
		}
		l_skip_whitespace();
		return t_dot;
	}
	if (_l_next_char == '-') {
		l_read_char(0);
		if (_l_next_char == '>') {
			l_read_char(1);
			return t_arrow;
		}
		if (_l_next_char == '-') {
			l_read_char(1);
			return t_minusminus;
		}
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_minuseq;
		}
		l_skip_whitespace();
		return t_minus;
	}
	if (_l_next_char == '+') {
		l_read_char(0);
		if (_l_next_char == '+') {
			l_read_char(1);
			return t_plusplus;
		}
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_pluseq;
		}
		l_skip_whitespace();
		return t_plus;
	}
	if (_l_next_char == ';') {
		l_read_char(1);
		return t_semicolon;
	}
	if (_l_next_char == ':') {
		l_read_char(1);
		return t_colon;
	}
	if (_l_next_char == '=') {
		l_read_char(0);
		if (_l_next_char == '=') {
			l_read_char(1);
			return t_eq;
		}
		l_skip_whitespace();
		return t_assign;
	}
	if (_l_next_char == 0 || _l_next_char == -1) {
		return t_eof;
	}
	if (is_alnum(_l_next_char)) {
		char *alias;
		int i = 0;
		do {
			_l_token_string[i++] = _l_next_char;
		} while (is_alnum(l_read_char(0)));
		_l_token_string[i] = 0;
		l_skip_whitespace();

		if (strcmp(_l_token_string, "if") == 0)
			return t_if;
		if (strcmp(_l_token_string, "while") == 0)
			return t_while;
		if (strcmp(_l_token_string, "for") == 0)
			return t_for;
		if (strcmp(_l_token_string, "do") == 0)
			return t_do;
		if (strcmp(_l_token_string, "else") == 0)
			return t_else;
		if (strcmp(_l_token_string, "return") == 0)
			return t_return;
		if (strcmp(_l_token_string, "typedef") == 0)
			return t_typedef;
		if (strcmp(_l_token_string, "enum") == 0)
			return t_enum;
		if (strcmp(_l_token_string, "struct") == 0)
			return t_struct;
		if (strcmp(_l_token_string, "sizeof") == 0)
			return t_sizeof;
		if (strcmp(_l_token_string, "switch") == 0)
			return t_switch;
		if (strcmp(_l_token_string, "case") == 0)
			return t_case;
		if (strcmp(_l_token_string, "break") == 0)
			return t_break;
		if (strcmp(_l_token_string, "default") == 0)
			return t_default;
		if (strcmp(_l_token_string, "#define") == 0)
			return t_define;
		if (strcmp(_l_token_string, "#include") == 0)
			return t_include;
		if (strcmp(_l_token_string, "_asm") == 0)
			return t_asm;

		alias = find_alias(_l_token_string);
		if (alias != NULL) {
			strcpy(_l_token_string, alias);
			return t_numeric;
		}

		return t_identifier;
	}
	error("Unrecognized input");
	return t_eof;
}

int l_accept(l_token token)
{
	if (_l_next_token == token) {
		_l_next_token = l_next_token();
		return 1;
	}
	return 0;
}

int l_peek(l_token token, char *value)
{
	if (_l_next_token == token) {
		if (value == NULL)
			return 1;
		strcpy(value, _l_token_string);
		return 1;
	}
	return 0;
}

void l_ident(l_token token, char *value)
{
	if (_l_next_token != token)
		error("Unexpected token");
	strcpy(value, _l_token_string);
	_l_next_token = l_next_token();
}

void l_expect(l_token token)
{
	if (_l_next_token != token)
		error("Unexpected token");
	_l_next_token = l_next_token();
}

void l_initialize()
{
	_source_idx = 0;
	_l_next_char = _source[0];
	l_expect(t_sof);
}
