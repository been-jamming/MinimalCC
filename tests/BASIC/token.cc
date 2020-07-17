void prints(char *);
void printd(int);
void inputs(char *, int);
int inputd();
void *malloc(int);

int INT_TOKEN;
int STR_TOKEN;
int VAR_TOKEN;
int SUBSTRING;

int PLUS;
int MINUS;
int MULTIPLY;
int DIVIDE;
int EQUALS;
int GREATER_THAN;
int LESS_THAN;
int NOT_EQUALS;
int AND;
int OR;

//Variable types
int INTVAR;
int STRVAR;

int PRINT;
int INPUT;
int RUN;
int END;
int IF;
int WHILE;
int LET;
int DIM;
int GOSUB;
int GOTO;
int RETURN;

int error;

void init_tokens(){
	INT_TOKEN = 1;
	STR_TOKEN = 2;
	SUBSTRING = 3;

	PLUS = 4;
	MINUS = 5;
	MULTIPLY = 6;
	DIVIDE = 7;
	EQUALS = 8;
	GREATER_THAN = 9;
	LESS_THAN = 10;
	NOT_EQUALS = 11;
	AND = 12;
	OR = 13;

	VAR_TOKEN = 14;

	INTVAR = 0;
	STRVAR = 1;

	PRINT = 1;
	INPUT = 2;
	RUN = 3;
	END = 4;
	IF = 5;
	WHILE = 6;
	DIM = 7;
	GOSUB = 8;
	GOTO = 9;
	RETURN = 10;
}

int alpha(char c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int digit(char c){
	return c >= '0' && c <= '9';
}

void skip_whitespace(char **c){
	while(**c && (**c == ' ' || **c == '\t' || **c == '\n'))
		*c = *c + 1;
}

int strcmp(char *a, char *b){
	while(*a && *a == *b){
		a = a + 1;
		b = b + 1;
	}

	return *b - *a;
}

void *create_variable(int type, void *values, int num_elements){
	void **output;

	output = kmalloc(POINTER_SIZE*3);
	output[0] = (void *) type;
	output[1] = values;
	output[2] = (void *) num_elements;

	return output;
}

void get_word(char *buffer, char **c, int buffer_length){
	skip_whitespace(c);

	while(buffer_length && (alpha(**c) || digit(**c) || **c == '_')){
		*buffer = **c;
		buffer = buffer + 1;
		*c = *c + 1;
		buffer_length = buffer_length - 1;
	}
	if(!buffer_length)
		buffer = buffer - 1;
	*buffer = 0;
}

int order_of_operations(int token_type){
	if(token_type == PLUS || token_type == MINUS){
		return 4;
	} else if(token_type == MULTIPLY || token_type == DIVIDE){
		return 5;
	} else if(token_type == EQUALS || token_type == GREATER_THAN || token_type == LESS_THAN){
		return 3;
	} else if(token_type == AND){
		return 2;
	} else if(token_type == OR){
		return 1;
	}
	return 0;
}

void *create_token(int type, void *value1, void *value2){
	void *output;

	output = kmalloc(POINTER_SIZE*3);
	((int *) output)[0] = type;
	((void **) output)[1] = value1;
	((void **) output)[2] = value2;

	return output;
}

int get_int(char **c){
	int invert;
	int output;

	if(**c == '-'){
		invert = 1;
		*c = *c + 1;
	} else {
		invert = 0;
	}

	output = 0;

	while(**c >= '0' && **c <= '9'){
		output = output*10 + **c - '0';
		*c = *c + 1;
	}

	if(invert)
		output = -output;

	return output;
}

char *get_str(char **c){
	int str_len;
	char *temp;
	char *output;

	str_len = 0;
	temp = *c;
	while(*temp && *temp != '"'){
		str_len = str_len + 1;
		temp = temp + 1;
	}

	output = kmalloc(CHAR_SIZE*(str_len + 1));
	temp = output;

	while(**c && **c != '"'){
		*temp = **c;
		*c = *c + 1;
		temp = temp + 1;
	}

	*temp = 0;

	return output;
}

void *get_expression(char **c);

void free_expression(void *);

void *get_var(char **c){
	char *buffer;
	void *index;

	buffer = kmalloc(32);
	get_word(buffer, c, 32);

	skip_whitespace(c);
	if(**c == '['){
		*c = *c + 1;
		index = get_expression(c);
		if(**c == ']')
			*c = *c + 1;
	} else {
		index = (void *) 0;
	}
	
	return create_token(VAR_TOKEN, buffer, index);
}

void *get_function(char **c){
	char buffer[32];
	char *temp;
	void *arg1;
	void *arg2;
	void *arg3;
	void **output;

	temp = *c;
	get_word(buffer, &temp, 32);
	if(!strcmp(buffer, "SUBSTR")){
		*c = temp;
		skip_whitespace(c);
		if(**c == '(')
			*c = *c + 1;
		else
			return (void *) 0;
		arg1 = get_expression(c);
		if(**c == ',' && (int) arg1){
			*c = *c + 1;
		} else {
			if((int) arg1)
				free_expression(arg1);
			return (void *) 0;
		}
		arg2 = get_expression(c);
		if(**c == ',' && (int) arg2){
			*c = *c + 1;
		} else {
			free_expression(arg1);
			if(arg2)
				free_expression(arg2);
			return (void *) 0;
		}
		arg3 = get_expression(c);
		if(**c == ')' && (int) arg3){
			*c = *c + 1;
		} else {
			free_expression(arg1);
			free_expression(arg2);
			if((int) arg3)
				free_expression(arg3);
			return (void *) 0;
		}
		output = kmalloc(POINTER_SIZE*4);
		output[0] = (void *) SUBSTRING;
		output[1] = arg1;
		output[2] = arg2;
		output[3] = arg3;
		return output;
	} else {
		return (void *) 0;
	}
}

void *get_value(char **c){
	char *str;
	void *output;

	skip_whitespace(c);
	if(**c == '-' || (**c >= '0' && **c <= '9')){
		return create_token(INT_TOKEN, (void *) get_int(c), (void *) 0);
	} else if(**c == '('){
		*c = *c + 1;
		output = get_expression(c);
		if(**c == ')')
			*c = *c + 1;
		return output;
	} else if(alpha(**c)){
		output = get_function(c);
		if(!output)
			return get_var(c);
		return output;
	} else if(**c == '"'){
		*c = *c + 1;
		str = get_str(c);
		if(**c == '"')
			*c = *c + 1;
		return create_token(STR_TOKEN, str, (void *) 0);
	}

	return (void *) 0;
}

int get_operation(char **c){
	skip_whitespace(c);
	if(**c == '+'){
		*c = *c + 1;
		return PLUS;
	} else if(**c == '-'){
		*c = *c + 1;
		return MINUS;
	} else if(**c == '*'){
		*c = *c + 1;
		return MULTIPLY;
	} else if(**c == '/'){
		*c = *c + 1;
		return DIVIDE;
	} else if(**c == '='){
		*c = *c + 1;
		return EQUALS;
	} else if(**c == '>'){
		*c = *c + 1;
		return GREATER_THAN;
	} else if(**c == '<'){
		*c = *c + 1;
		return LESS_THAN;
	} else if(**c == '!' && *(*c + 1) == '='){
		*c = *c + 2;
		return NOT_EQUALS;
	} else if(**c == '&'){
		*c = *c + 1;
		return AND;
	} else if(**c == '|'){
		*c = *c + 1;
		return OR;
	}
	return 0;
}

void *get_expression_recursive(void *output, char **c){
	void *value;
	int current_operation;
	int next_operation;
	char *temp;

	while(current_operation = get_operation(c)){
		value = get_value(c);
		if(!(int) value){
			free_expression(output);
			return (void *) 0;
		}
		temp = *c;
		next_operation = get_operation(&temp);
		if(order_of_operations(next_operation) > order_of_operations(current_operation)){
			value = get_expression_recursive(value, c);
		}
		if((int) value){
			output = create_token(current_operation, output, value);
		} else {
			free_expression(output);
			return (void *) 0;
		}
	}

	return output;
}

void *get_expression(char **c){
	void *value;

	value = get_value(c);
	if(!(int) value){
		return (void *) 0;
	}
	return get_expression_recursive(value, c);
}

void free_expression(void *expr){
	if(((int *) expr)[0] == INT_TOKEN){
		kfree(expr);
	} else if(((int *) expr)[0] == STR_TOKEN){
		kfree(((void **) expr)[1]);
		kfree(expr);
	} else if(((int *) expr)[0] == VAR_TOKEN){
		if((int) ((void **) expr)[2])
			free_expression(((void **) expr)[2]);
		kfree(((void **) expr)[1]);
		kfree(expr);
	} else if(((int *) expr)[0] == PLUS || ((int *) expr)[0] == MINUS || ((int *) expr)[0] == MULTIPLY || ((int *) expr)[0] == DIVIDE || ((int *) expr)[0] == EQUALS || ((int *) expr)[0] == NOT_EQUALS || ((int *) expr)[0] == GREATER_THAN || ((int *) expr)[0] == LESS_THAN || ((int *) expr)[0] == AND || ((int *) expr)[0] == OR){
		free_expression(((void **) expr)[1]);
		free_expression(((void **) expr)[2]);
		kfree(expr);
	} else if(((int *) expr)[0] == SUBSTRING){
		free_expression(((void **) expr)[1]);
		free_expression(((void **) expr)[2]);
		free_expression(((void **) expr)[3]);
		kfree(expr);
	}
}

void **parse_print(char **c){
	void **output;

	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) PRINT;
	output[1] = get_expression(c);

	skip_whitespace(c);
	if(!(int) output[1]){
		kfree(output);
		return (void **) 0;
	} else if(**c){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_input(char **c){
	void **output;

	skip_whitespace(c);
	if(!alpha(**c))
		return (void **) 0;
	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) INPUT;
	output[1] = get_var(c);
	skip_whitespace(c);
	if(**c){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_let(char **c){
	void **output;

	skip_whitespace(c);
	if(!alpha(**c))
		return (void **) 0;
	output = kmalloc(POINTER_SIZE*3);
	output[0] = (void *) LET;
	output[1] = get_var(c);
	skip_whitespace(c);
	if(**c != '='){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}
	*c = *c + 1;
	skip_whitespace(c);
	output[2] = get_expression(c);
	if(**c || !(int) output[2]){
		free_expression(output[1]);
		if((int) output[2])
			free_expression(output[2]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_dim(char **c){
	void **output;

	skip_whitespace(c);
	if(!alpha(**c))
		return (void **) 0;
	output = kmalloc(POINTER_SIZE*3);
	output[0] = (void *) DIM;
	output[1] = get_var(c);
	skip_whitespace(c);
	if(**c != ','){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}
	*c = *c + 1;
	skip_whitespace(c);
	output[2] = get_expression(c);
	skip_whitespace(c);
	if(**c || !(int) output[2]){
		free_expression(output[1]);
		if((int) output[2])
			free_expression(output[2]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_if(char **c){
	void **output;

	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) IF;
	output[1] = get_expression(c);

	skip_whitespace(c);
	if(!(int) output[1]){
		kfree(output);
		return (void **) 0;
	} else if(**c){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_goto(char **c){
	void **output;

	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) GOTO;
	output[1] = get_expression(c);

	skip_whitespace(c);
	if(!(int) output[1]){
		kfree(output);
		return (void **) 0;
	} else if(**c){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_gosub(char **c){
	void **output;

	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) GOSUB;
	output[1] = get_expression(c);

	skip_whitespace(c);
	if(!(int) output[1]){
		kfree(output);
		return (void **) 0;
	} else if(**c){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_while(char **c){
	void **output;

	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) WHILE;
	output[1] = get_expression(c);

	skip_whitespace(c);
	if(!(int) output[1]){
		kfree(output);
		return (void **) 0;
	} else if(**c){
		free_expression(output[1]);
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_run(char **c){
	void **output;

	skip_whitespace(c);
	if(**c)
		return (void **) 0;
	output = kmalloc(POINTER_SIZE);
	output[0] = (void *) RUN;

	return output;
}

void **parse_end(char **c){
	void **output;
	char buffer[32];

	skip_whitespace(c);
	if(!**c){
		output = kmalloc(POINTER_SIZE*2);
		output[0] = (void *) END;
		output[1] = (void *) 0;
		return output;
	}
	get_word(buffer, c, 32);
	skip_whitespace(c);
	if(**c)
		return (void **) 0;
	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) END;
	if(!strcmp(buffer, "IF"))
		output[1] = (void *) IF;
	else if(!strcmp(buffer, "WHILE"))
		output[1] = (void *) WHILE;
	else {
		kfree(output);
		return (void **) 0;
	}

	return output;
}

void **parse_return(char **c){
	void **output;

	skip_whitespace(c);
	if(!**c){
		output = kmalloc(POINTER_SIZE);
		output[0] = (void *) RETURN;
		return output;
	}

	return (void **) 0;
}

void **get_statement(char **c){
	char buffer[32];

	get_word(buffer, c, 32);
	if(!strcmp(buffer, "PRINT"))
		return parse_print(c);
	else if(!strcmp(buffer, "INPUT"))
		return parse_input(c);
	else if(!strcmp(buffer, "RUN"))
		return parse_run(c);
	else if(!strcmp(buffer, "END"))
		return parse_end(c);
	else if(!strcmp(buffer, "IF"))
		return parse_if(c);
	else if(!strcmp(buffer, "WHILE"))
		return parse_while(c);
	else if(!strcmp(buffer, "LET"))
		return parse_let(c);
	else if(!strcmp(buffer, "DIM"))
		return parse_dim(c);
	else if(!strcmp(buffer, "GOTO"))
		return parse_goto(c);
	else if(!strcmp(buffer, "GOSUB"))
		return parse_gosub(c);
	else if(!strcmp(buffer, "RETURN"))
		return parse_return(c);
	else
		return (void **) 0;
}

void free_statement(void **statement){
	if((int) statement[0] == PRINT){
		free_expression(statement[1]);
		kfree(statement);
	} else if((int) statement[0] == INPUT){
		if(((int *) statement[1])[2])
			free_expression(((void **) statement[1])[2]);
		kfree(((void **) statement[1])[1]);
		kfree(statement[1]);
		kfree(statement);
	} else if((int) statement[0] == RUN){
		kfree(statement);
	} else if((int) statement[0] == END){
		kfree(statement);
	} else if((int) statement[0] == IF){
		free_expression(statement[1]);
		kfree(statement);
	} else if((int) statement[0] == WHILE){
		free_expression(statement[1]);
		kfree(statement);
	} else if((int) statement[0] == LET){
		free_expression(statement[1]);
		free_expression(statement[2]);
		kfree(statement);
	} else if((int) statement[0] == DIM){
		free_expression(statement[1]);
		free_expression(statement[2]);
		kfree(statement);
	} else if((int) statement[0] == GOTO){
		free_expression(statement[1]);
		kfree(statement);
	} else if((int) statement[0] == GOSUB){
		free_expression(statement[1]);
		kfree(statement);
	}
}

