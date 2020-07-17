void **variables;
void *statement_list;

int ERR_VAL;
int INT_VAL;
int STR_VAL;
int do_end;
int do_if;
int do_while;
int do_skip_if;
int do_skip_while;
int do_gosub;
int do_goto;
int do_return;

int goto_label;

int gosubs[32];
int current_gosub;

void *create_value(int type, void *value){
	void **output;

	output = kmalloc(POINTER_SIZE*2);
	output[0] = (void *) type;
	output[1] = value;

	return output;
}

void *create_statement_entry(void *statement, int line_number){
	void **output;

	output = kmalloc(POINTER_SIZE*2);
	output[0] = statement;
	output[1] = (void *) line_number;

	return output;
}

void free_statement_entry(void *statement_entry){
	free_statement(((void **) statement_entry)[0]);
	kfree(statement_entry);
}

void free_value(void **value){
	if((int) value[0] == STR_VAL)
		kfree(value[1]);
	kfree(value);
}

int strlen(char *str){
	int output;

	output = 0;
	while(*str){
		output = output + 1;
		str = str + 1;
	}

	return output;
}

void strncpy(char *str1, char *str2, int max){
	while(max && *str2){
		*str1 = *str2;
		str1 = str1 + 1;
		str2 = str2 + 1;
		max = max - 1;
	}

	*str1 = 0;
}

char *duplicate_str(char *str){
	int str_len;
	int i;
	char *output;

	str_len = strlen(str);
	output = kmalloc(CHAR_SIZE*(str_len + 1));
	for(i = 0; i < str_len; i = i + 1)
		output[i] = str[i];

	output[str_len] = 0;

	return output;
}

char *concat(char *str_a, char *str_b){
	int i;
	int len_a;
	int len_b;
	char *output;

	len_a = strlen(str_a);
	len_b = strlen(str_b);
	output = kmalloc(CHAR_SIZE*(len_a + len_b + 1));

	for(i = 0; i < len_a; i = i + 1){
		output[i] = str_a[i];
	}

	for(i = len_a; i < len_a + len_b; i = i + 1){
		output[i] = str_b[i - len_a];
	}

	output[i] = 0;

	return output;
}

void *add_values(void **value0, void **value1){
	char *new_str;

	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1] + (int) value1[1]);
		free_value(value1);
		return value0;
	} else if((int) value0[0] == STR_VAL && (int) value1[0] == STR_VAL){
		new_str = concat((char *) value0[1], (char *) value1[1]);
		kfree(value0[1]);
		kfree(value1[1]);
		value0[1] = new_str;
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		value0 = create_value(ERR_VAL, (void *) 0);
		return value0;
	}
}

void *subtract_values(void **value0, void **value1){
	char *new_str;

	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1] - (int) value1[1]);
		free_value(value1);
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		value0 = create_value(ERR_VAL, (void *) 0);
		return value0;
	}
}

void *multiply_values(void **value0, void **value1){
	char *new_str;

	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1]*(int) value1[1]);
		free_value(value1);
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		value0 = create_value(ERR_VAL, (void *) 0);
		return value0;
	}
}

void *divide_values(void **value0, void **value1){
	char *new_str;

	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1]/(int) value1[1]);
		free_value(value1);
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		value0 = create_value(ERR_VAL, (void *) 0);
		return value0;
	}
}

void *equals_values(void **value0, void **value1){
	int result;

	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		result = (value0[1] == value1[1]);
		value0[1] = (void *) result;
		free_value(value1);
		return value0;
	} else if((int) value0[0] == STR_VAL && (int) value1[0] == STR_VAL){
		result = !strcmp(value0[1], value1[1]);
		kfree(value0[1]);
		value0[0] = (void *) INT_VAL;
		value0[1] = (void *) result;
		free_value(value1);
		return value0;
	} else if((int) value0[0] != ERR_VAL && (int) value1[0] != ERR_VAL){
		free_value(value0);
		free_value(value1);
		return create_value(INT_VAL, (void *) 0);
	} else {
		free_value(value0);
		free_value(value1);
		return create_value(ERR_VAL, (void *) 0);
	}
}

void *not_equals_values(void **value0, void **value1){
	int result;

	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		result = (value0[1] != value1[1]);
		value0[1] = (void *) result;
		free_value(value1);
		return value0;
	} else if((int) value0[0] == STR_VAL && (int) value1[0] == STR_VAL){
		result = !!strcmp(value0[1], value1[1]);
		kfree(value0[1]);
		value0[0] = (void *) INT_VAL;
		value0[1] = (void *) result;
		free_value(value1);
		return value0;
	} else if((int) value0[0] != ERR_VAL && (int) value1[0] != ERR_VAL){
		free_value(value0);
		free_value(value1);
		return create_value(INT_VAL, (void *) 1);
	} else {
		free_value(value0);
		free_value(value1);
		return create_value(ERR_VAL, (void *) 0);
	}
}

void *greater_than_values(void **value0, void **value1){
	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1] > (int) value1[1]);
		free_value(value1);
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		return create_value(ERR_VAL, (void *) 0);
	}
}

void *less_than_values(void **value0, void **value1){
	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1] < (int) value1[1]);
		free_value(value1);
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		return create_value(ERR_VAL, (void *) 0);
	}
}

void *and_values(void **value0, void **value1){
	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1] && (int) value1[1]);
		free_value(value1);
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		return create_value(ERR_VAL, (void *) 0);
	}
}

void *or_values(void **value0, void **value1){
	if((int) value0[0] == INT_VAL && (int) value1[0] == INT_VAL){
		value0[1] = (void *) ((int) value0[1] || (int) value1[1]);
		free_value(value1);
		return value0;
	} else {
		free_value(value0);
		free_value(value1);
		return create_value(ERR_VAL, (void *) 0);
	}
}

void *substring(void **value1, void **value2, void **value3){
	int char0;
	int char1;
	int str_len;
	char *str;
	char *new_str;

	if((int) value1[0] == STR_VAL && (int) value2[0] == INT_VAL && (int) value3[0] == INT_VAL){
		str = value1[1];
		str_len = strlen(str);
		char0 = (int) value2[1];
		char1 = (int) value3[1];
		if(char0 < 0)
			char0 = 0;
		if(char0 >= str_len)
			char0 = str_len - 1;
		if(char1 < 0)
			char1 = 0;
		if(char1 >= str_len)
			char1 = str_len - 1;
		if(char0 > char1){
			free_value(value1);
			free_value(value2);
			free_value(value3);
			str = kmalloc(CHAR_SIZE);
			*str = 0;
			return create_value(STR_VAL, str);
		} else {
			new_str = kmalloc(CHAR_SIZE*(char1 - char0 + 2));
			strncpy(new_str, str + char0, char1 - char0 + 1);
			free_value(value1);
			free_value(value2);
			free_value(value3);
			return create_value(STR_VAL, new_str);
		}
	} else {
		free_value(value1);
		free_value(value2);
		free_value(value3);
		return create_value(ERR_VAL, (void *) 0);
	}
}

void *evaluate(void *expression){
	void **var;
	void *index_expr;
	int index;
	int length;
	void **index_value;
	void **var_value;

	if(((int *) expression)[0] == INT_TOKEN){
		return create_value(INT_VAL, ((void **) expression)[1]);
	} else if(((int *) expression)[0] == STR_TOKEN){
		return create_value(STR_VAL, duplicate_str(((void **) expression)[1]));
	} else if(((int *) expression)[0] == PLUS){
		return add_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == MINUS){
		return subtract_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == MULTIPLY){
		return multiply_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == DIVIDE){
		return divide_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == EQUALS){
		return equals_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == NOT_EQUALS){
		return not_equals_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == GREATER_THAN){
		return greater_than_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == LESS_THAN){
		return less_than_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == OR){
		return or_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == AND){
		return and_values(evaluate(((void **) expression)[1]), evaluate(((void **) expression)[2]));
	} else if(((int *) expression)[0] == VAR_TOKEN){
		var = read_dictionary(variables, ((void **) expression)[1], 0);
		if(!(int) var){
			return create_value(INT_VAL, (void *) 0);
		}

		index_expr = ((void **) expression)[2];
		if((int) index_expr){
			index_value = evaluate(index_expr);
			if(((int *) index_value)[0] == INT_VAL){
				index = (int) index_value[1];
			} else {
				free_value(index_value);
				return create_value(ERR_VAL, (void *) 0);
			}
			free_value(index_value);
			length = ((int *) var)[2];
			if(index >= length || index < 0)
				return create_value(ERR_VAL, (void *) 0);
		} else {
			index = 0;
		}

		var_value = ((void ***) var)[1][index];
		if(((int *) var_value)[0] == STR_VAL)
			return create_value(((int *) var_value)[0], duplicate_str(((char **) var_value)[1]));
		else
			return create_value(((int *) var_value)[0], ((void **) var_value)[1]);
	} else if(((int *) expression)[0] == SUBSTRING){
		void *arg1;
		void *arg2;
		void *arg3;
		void *value1;
		void *value2;
		void *value3;

		arg1 = ((void **) expression)[1];
		arg2 = ((void **) expression)[2];
		arg3 = ((void **) expression)[3];

		value1 = evaluate(arg1);
		value2 = evaluate(arg2);
		value3 = evaluate(arg3);

		return substring(value1, value2, value3);
	} else {
		return create_value(ERR_VAL, (void *) 0);
	}
}

void print_value(void **value){
	if((int) value[0] == INT_VAL){
		printd((int) value[1]);
	} else if((int) value[0] == STR_VAL){
		prints(value[1]);
	}
}

int run_print(void **statement){
	void **expr_value;

	expr_value = evaluate(statement[1]);
	print_value(expr_value);
	if((int) expr_value[0] == ERR_VAL){
		free_value(expr_value);
		return 1;
	} else {
		prints("\n");
		free_value(expr_value);
		return 0;
	}
}

int run_if(void **statement){
	void **expr_value;

	expr_value = evaluate(statement[1]);
	if((int) expr_value[0] == ERR_VAL){
		free_value(expr_value);
		return 1;
	} else if((int) expr_value[0] != INT_VAL || ((int) expr_value[0] == INT_VAL && (int) expr_value[1])){
		free_value(expr_value);
		return 0;
	} else {
		do_skip_if = 1;
		free_value(expr_value);
		return 0;
	}
}

int run_goto(void **statement){
	void **expr_value;

	expr_value = evaluate(statement[1]);
	if((int) expr_value[0] != INT_VAL){
		free_value(expr_value);
		return 1;
	}
	goto_label = (int) expr_value[1];
	free_value(expr_value);
	do_goto = 1;

	return 0;
}

int run_gosub(void **statement){
	void **expr_value;

	expr_value = evaluate(statement[1]);
	if((int) expr_value[0] != INT_VAL){
		free_value(expr_value);
		return 1;
	}
	goto_label = (int) expr_value[1];
	free_value(expr_value);
	do_gosub = 1;

	return 0;
}

int run_return(void **statement){
	do_return = 1;

	return 0;
}

int run_while(void **statement){
	void **expr_value;

	expr_value = evaluate(statement[1]);
	if((int) expr_value[0] == ERR_VAL){
		free_value(expr_value);
		return 1;
	} else if((int) expr_value[0] != INT_VAL || ((int) expr_value[0] == INT_VAL && (int) expr_value[1])){
		free_value(expr_value);
		return 0;
	} else {
		do_skip_while = 1;
		free_value(expr_value);
		return 0;
	}
}

int set_var(void **var_token, void **value){
	void *var;
	void **index_expr;
	void **index_value;
	void **var_values;
	void **value_before;
	int index;
	int length;

	var = read_dictionary(variables, var_token[1], 0);
	if(!(int) var){
		index_expr = var_token[2];
		if((int) index_expr){
			index_value = evaluate(index_expr);
			if(((int *) index_value)[0] == INT_VAL){
				index = (int) index_value[1];
			} else {
				free_value(index_value);
				return 1;
			}
			free_value(index_value);
		} else {
			index = 0;
		}
		if(index != 0)
			return 1;
		var_values = kmalloc(POINTER_SIZE);
		var_values[0] = value;
		write_dictionary(variables, var_token[1], create_variable(INTVAR, var_values, 1), 0);
	} else {
		index_expr = var_token[2];
		if((int) index_expr){
			index_value = evaluate(index_expr);
			if(((int *) index_value)[0] == INT_VAL){
				index = (int) index_value[1];
			} else {
				free_value(index_value);
				return 1;
			}
			free_value(index_value);
		} else {
			index = 0;
		}
		length = ((int *) var)[2];
		if(index >= length || index < 0)
			return 1;
		value_before = ((void ***) var)[1][index];
		free_value(value_before);
		((void ***) var)[1][index] = value;
	}

	return 0;
}

void del_var(void **var_token){
	void *var;
	void **values;
	int i;
	int size;

	var = read_dictionary(variables, var_token[1], 0);
	if(!var)
		return;
	values = ((void **) var)[1];
	size = ((int *) var)[2];
	for(i = 0; i < size; i = i + 1)
		free_value(values[i]);
	kfree(values);
	kfree(var);
	write_dictionary(variables, var_token[1], (void *) 0, 0);
}

int run_dim(void **statement){
	void **var_token;
	void *expr;
	void **value;
	void **values;
	void *var;
	int size;
	int i;

	var_token = statement[1];
	expr = statement[2];
	value = evaluate(expr);
	if((int) value[0] != INT_VAL || (int) value[1] < 1){
		free_value(value);
		return 1;
	}
	del_var(var_token);
	size = (int) value[1];
	free_value(value);
	values = kmalloc(POINTER_SIZE*size);
	for(i = 0; i < size; i = i + 1)
		values[i] = create_value(INT_VAL, (void *) 0);
	var = create_variable(INTVAR, values, size);
	write_dictionary(variables, var_token[1], var, 0);

	return 0;
}

int run_input(void **statement){
	int inp;
	void **var_token;
	void **value;

	inp = inputd();

	var_token = statement[1];
	value = create_value(INT_VAL, (void *) inp);
	if(set_var(var_token, value)){
		free_value(value);
		return 1;
	}

	return 0;
}

int run_let(void **statement){
	void **var_token;
	void **expr;
	void **value;

	var_token = statement[1];
	value = evaluate(statement[2]);
	if(((int *) value)[0] == ERR_VAL){
		free_value(value);
		return 1;
	}
	if(set_var(var_token, value)){
		free_value(value);
		return 1;
	}

	return 0;
}

int run_end(void **statement){
	int mode;

	mode = ((int *) statement)[1];
	if(!mode)
		do_end = 1;
	else if(mode == WHILE){
		do_while = 1;
	} else if(mode != IF){
		return 1;
	}

	return 0;
}

void *skip_to_end(void *current_statement_list, int mode){
	int counter;
	void *list_value;
	void *next;

	counter = 1;
	next = next_list(current_statement_list);
	if((int) next)
		current_statement_list = next;
	while((int) (next = next_list(current_statement_list))){
		list_value = get_list_value(current_statement_list);
		if(list_value){
			if(((int **) list_value)[0][0] == mode)
				counter = counter + 1;
			else if(((int **) list_value)[0][0] == END && ((int **) list_value)[0][1] == mode)
				counter = counter - 1;
			if(!counter)
				return current_statement_list;
		}
		current_statement_list = next;
	}

	return current_statement_list;
}

void *seek(void *current_statement_list, int mode){
	int counter;
	void *list_value;
	void *prev;

	counter = 1;
	current_statement_list = previous_list(current_statement_list);
	while((int) (prev = previous_list(current_statement_list)) && counter){
		list_value = get_list_value(current_statement_list);
		if(((int **) list_value)[0][0] == END && ((int **) list_value)[0][1] == mode)
			counter = counter + 1;
		else if(((int **) list_value)[0][0] == mode)
			counter = counter - 1;
		if(!counter)
			return previous_list(current_statement_list);
		current_statement_list = prev;
	}

	return (void *) 0;
}

void *seek_goto(int line){
	void *current_statement_list;
	void **list_value;

	current_statement_list = next_list(statement_list);
	while((int) current_statement_list){
		list_value = get_list_value(current_statement_list);
		if(((int *) list_value)[1] == line)
			return current_statement_list;
		current_statement_list = next_list(current_statement_list);
	}

	return (void *) 0;
}

int run_statement(void **statement);

int run_program(void *current_statement_list){
	void *current_statement_entry;

	while((int) current_statement_list){
		current_statement_entry = get_list_value(current_statement_list);
		if(run_statement(((void **) current_statement_entry)[0]))
			return 1;
		if(do_end){
			do_end = 0;
			return 0;
		} else if(do_skip_if){
			current_statement_list = skip_to_end(current_statement_list, IF);
			do_skip_if = 0;
			if(!(int) next_list(current_statement_list))
				return 1;
		} else if(do_skip_while){
			current_statement_list = skip_to_end(current_statement_list, WHILE);
			do_skip_while = 0;
			if(!(int) next_list(current_statement_list))
				return 1;
		} else if(do_while){
			current_statement_list = seek(current_statement_list, WHILE);
			do_while = 0;
			if(!(int) current_statement_list)
				return 1;
		} else if(do_goto){
			current_statement_list = previous_list(seek_goto(goto_label));
			do_goto = 0;
			if(!(int) current_statement_list)
				return 1;
		} else if(do_gosub){
			if(current_gosub < 32){
				gosubs[current_gosub] = ((int *) current_statement_entry)[1];
				current_gosub = current_gosub + 1;
				current_statement_list = previous_list(seek_goto(goto_label));
				do_gosub = 0;
				if(!(int) current_statement_list)
					return 1;
			} else {
				return 1;
			}
		} else if(do_return){
			if(current_gosub){
				current_gosub = current_gosub - 1;
				current_statement_list = seek_goto(gosubs[current_gosub]);
				do_return = 0;
				if(!(int) current_statement_list)
					return 1;
			} else {
				return 1;
			}
		}
		current_statement_list = next_list(current_statement_list);
	}

	return 0;
}

int run_statement(void **statement){
	if((int) statement[0] == PRINT)
		return run_print(statement);
	else if((int) statement[0] == INPUT)
		return run_input(statement);
	else if((int) statement[0] == RUN)
		return run_program(next_list(statement_list));
	else if((int) statement[0] == END)
		return run_end(statement);
	else if((int) statement[0] == IF)
		return run_if(statement);
	else if((int) statement[0] == WHILE)
		return run_while(statement);
	else if((int) statement[0] == LET)
		return run_let(statement);
	else if((int) statement[0] == DIM)
		return run_dim(statement);
	else if((int) statement[0] == GOTO)
		return run_goto(statement);
	else if((int) statement[0] == GOSUB)
		return run_gosub(statement);
	else if((int) statement[0] == RETURN)
		return run_return(statement);
	else
		return 1;
}

void remove_line(int line_number){
	void *current_statement_list;
	void *current_statement;

	current_statement_list = next_list(statement_list);
	while((int) current_statement_list){
		current_statement = get_list_value(current_statement_list);
		if(((int *) current_statement)[1] == line_number){
			free_statement(((void **) current_statement)[0]);
			remove_value(current_statement_list, kfree);
			return;
		}
		current_statement_list = next_list(current_statement_list);
	}
}

void insert_statement(void *statement, int line_number){
	void *statement_entry;
	void *current_statement_list;
	void *current_statement_entry;
	void *new_statement_entry;
	void *previous_statement_list;
	int current_line_number;

	new_statement_entry = create_statement_entry(statement, line_number);
	previous_statement_list = statement_list;
	current_statement_list = next_list(previous_statement_list);
	while((int) current_statement_list){
		current_statement_entry = get_list_value(current_statement_list);
		current_line_number = ((int *) current_statement_entry)[1];
		if(current_line_number > line_number){
			add_value(previous_statement_list, new_statement_entry);
			return;
		} else if(current_line_number == line_number){
			remove_value(current_statement_list, free_statement_entry);
			add_value(previous_statement_list, new_statement_entry);
			return;
		}
		previous_statement_list = current_statement_list;
		current_statement_list = next_list(current_statement_list);
	}

	add_value(previous_statement_list, new_statement_entry);
}

void add_statement(char **c){
	int line_number;
	void *statement;

	line_number = get_int(c);
	skip_whitespace(c);
	if(!**c){
		remove_line(line_number);
		return;
	}

	statement = get_statement(c);
	if((int) statement)
		insert_statement(statement, line_number);
	else
		prints("SYNTAX ERROR");
}

void entry(){
	char buffer[256];
	char *c;
	void *statement;

	ERR_VAL = 0;
	INT_VAL = 1;
	STR_VAL = 2;
	kmalloc_init(malloc(100000), 100000);

	variables = create_dictionary((void *) 0);
	statement_list = create_linked_list(&ERR_VAL);
	init_tokens();

	do_end = 0;
	do_if = 0;
	do_while = 0;
	do_skip_if = 0;
	do_skip_while = 0;
	do_goto = 0;
	do_gosub = 0;
	do_return = 0;

	current_gosub = 0;

	while(1){
		prints("\n>: ");
		inputs(buffer, 256);
		c = buffer;
		skip_whitespace(&c);
		if(digit(*c)){
			add_statement(&c);
		} else {
			statement = get_statement(&c);
			if(!(int) statement){
				prints("SYNTAX ERROR");
			} else {
				if(run_statement(statement))
					prints("RUNTIME ERROR");
				free_statement(statement);
			}
		}
	}
}

