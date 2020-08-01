/* rvcc C compiler - global structures */

block_def *_blocks;
int _blocks_idx;

function_def *_functions;
int _functions_idx;

type_def *_types;
int _types_idx;

il_instr *_il;
int _il_idx;

alias_def *_aliases;
int _aliases_idx;

constant_def *_constants;
int _constants_idx;

char *_source;
int _source_idx;
char _l_next_char;

int _c_block_level;

variable_def *_temp_variable;

/* ELF sections */

char *_e_code;
int _e_code_idx;
char *_e_data;
int _e_data_idx;
char *_e_symtab;
int _e_symtab_idx;
char *_e_strtab;
int _e_strtab_idx;
char *_e_header;
int _e_header_idx;
char *_e_footer;
int _e_footer_idx;
int _e_header_len;
int _e_code_start;

type_def *find_type(char *type_name)
{
	int i;
	for (i = 0; i < _types_idx; i++) {
		if (strcmp(_types[i].type_name, type_name) == 0) {
			return &_types[i];
		}
	}
	return NULL;
}

il_instr *add_instr(il_op op)
{
	il_instr *ii = &_il[_il_idx];
	ii->op = op;
	ii->op_len = 0;
	ii->string_param1 = 0;
	ii->il_index = _il_idx;
	_il_idx++;
	return ii;
}

il_instr *add_generic(int generic_op)
{
	il_instr *ii = &_il[_il_idx];
	ii->op = op_generic;
	ii->int_param1 = generic_op;
	ii->op_len = 0;
	ii->il_index = _il_idx;
	_il_idx++;
	return ii;
}

block_def *add_block(block_def *parent, function_def *function)
{
	block_def *bd = &_blocks[_blocks_idx];
	bd->bd_index = _blocks_idx;
	bd->parent = parent;
	bd->function = function;
	bd->next_local = 0;
	_blocks_idx++;
	return bd;
}

void add_alias(char *alias, char *value)
{
	alias_def *al = &_aliases[_aliases_idx];
	strcpy(al->alias, alias);
	strcpy(al->value, value);
	_aliases_idx++;
}

char *find_alias(char alias[])
{
	int i;
	for (i = 0; i < _aliases_idx; i++) {
		if (strcmp(alias, _aliases[i].alias) == 0) {
			return _aliases[i].value;
		}
	}
	return NULL;
}

function_def *add_function(char *name)
{
	function_def *fn;
	int i;

	/* return existing if found */
	for (i = 0; i < _functions_idx; i++) {
		if (strcmp(_functions[i].return_def.variable_name, name) == 0) {
			return &_functions[i];
		}
	}

	fn = &_functions[_functions_idx];
	strcpy(fn->return_def.variable_name, name);
	_functions_idx++;
	return fn;
}

type_def *add_type()
{
	type_def *type = &_types[_types_idx];
	_types_idx++;
	return type;
}

type_def *add_named_type(char *name)
{
	type_def *type = add_type();
	strcpy(type->type_name, name);
	return type;
}

void add_constant(char alias[], int value)
{
	constant_def *constant = &_constants[_constants_idx];
	strcpy(constant->alias, alias);
	constant->value = value;
	_constants_idx++;
}

constant_def *find_constant(char alias[])
{
	int i;
	for (i = 0; i < _constants_idx; i++) {
		if (strcmp(_constants[i].alias, alias) == 0) {
			return &_constants[i];
		}
	}
	return NULL;
}

function_def *find_function(char function_name[])
{
	int i;
	for (i = 0; i < _functions_idx; i++) {
		if (strcmp(_functions[i].return_def.variable_name, function_name) == 0) {
			return &_functions[i];
		}
	}
	return NULL;
}

variable_def *find_member(char token[], type_def *type)
{
	int i;
	for (i = 0; i < type->num_fields; i++) {
		if (strcmp(type->fields[i].variable_name, token) == 0) {
			return &type->fields[i];
		}
	}
	return NULL;
}

variable_def *find_local_variable(char *token, block_def *block)
{
	int i;
	function_def *fn = block->function;

	while (block != NULL) {
		for (i = 0; i < block->next_local; i++) {
			if (strcmp(block->locals[i].variable_name, token) == 0) {
				return &block->locals[i];
			}
		}
		block = block->parent;
	}

	if (fn != NULL) {
		for (i = 0; i < fn->num_params; i++) {
			if (strcmp(fn->param_defs[i].variable_name, token) == 0) {
				return &fn->param_defs[i];
			}
		}
	}
	return NULL;
}

variable_def *find_global_variable(char *token)
{
	int i;
	block_def *block = &_blocks[0];

	for (i = 0; i < block->next_local; i++) {
		if (strcmp(block->locals[i].variable_name, token) == 0) {
			return &block->locals[i];
		}
	}
	return NULL;
}

variable_def *find_variable(char *token, block_def *parent)
{
	variable_def *var = find_local_variable(token, parent);
	if (var == NULL) {
		var = find_global_variable(token);
	}
	return var;
}

int size_variable(variable_def *var)
{
	type_def *td;
	int bs, j, s = 0;

	if (var->is_pointer > 0) {
		s += 4;
	} else {
		td = find_type(var->type_name);
		bs = td->size;
		if (var->array_size > 0) {
			for (j = 0; j < var->array_size; j++) {
				s += bs;
			}
		} else {
			s += bs;
		}
	}
	return s;
}

void g_initialize()
{
	_e_header_len = 0x74; /* ELF fixed: 0x34 + 2 * 0x20 */

	_e_header_idx = 0;
	_e_footer_idx = 0;
	_e_code_idx = 0;
	_e_data_idx = 0;
	_il_idx = 0;
	_source_idx = 0;
	_e_strtab_idx = 0;
	_e_symtab_idx = 0;
	_aliases_idx = 0;
	_constants_idx = 0;
	_blocks_idx = 0;
	_types_idx = 0;
	_functions_idx = 0;

	_e_code_start = ELF_START + _e_header_len;

	_functions = malloc(MAX_FUNCTIONS * sizeof(function_def));
	_blocks = malloc(MAX_BLOCKS * sizeof(block_def));
	_types = malloc(MAX_TYPES * sizeof(type_def));
	_il = malloc(MAX_IL * sizeof(il_instr));
	_source = malloc(MAX_SOURCE);
	_e_code = malloc(MAX_CODE);
	_e_data = malloc(MAX_DATA);
	_e_symtab = malloc(MAX_SYMTAB);
	_e_strtab = malloc(MAX_STRTAB);
	_e_header = malloc(MAX_HEADER);
	_e_footer = malloc(MAX_FOOTER);
	_aliases = malloc(MAX_ALIASES * sizeof(alias_def));
	_constants = malloc(MAX_CONSTANTS * sizeof(constant_def));
	_temp_variable = malloc(sizeof(variable_def));
}

void error(char *msg)
{
	printf("Error %s at source location %d, IL index %d\n", msg, _source_idx, _il_idx);
	abort();
}
