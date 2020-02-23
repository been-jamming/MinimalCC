#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "expression.h"

dictionary local_variables;
dictionary global_variables;
unsigned int num_labels = 0;
unsigned int current_string = 0;
unsigned int order_of_operations[] = {0, 6, 6, 7, 7, 1, 5, 5, 4, 4, 3, 2};

type operation_none_func(char *reg_a, char *reg_b, value value_a, value value_b){
	fprintf(stderr, "Unrecognized operation\n");
	exit(1);

	return value_a.data_type;
}

type operation_add_func(char *reg_a, char *reg_b, value value_a, value value_b){
	type pointer_type;
	value pointer_value;
	char *integer_reg;

	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		fprintf(stderr, "Error: Addition of function type is undefined\n");
		exit(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			fprintf(stderr, "Warning: adding two pointers together. Treating them as integers instead\n");
			printf("add %s, %s, %s\n", reg_a, reg_a, reg_b);
			return value_a.data_type;
		}
		pointer_value = value_a;
		integer_reg = reg_a;
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			pointer_value = value_b;
			integer_reg = reg_b;
		} else {
			printf("add %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	}
	pointer_type = pointer_value.data_type;
	pop_type(&pointer_type);
	if(type_size(pointer_type) == 4){
		printf("sll %s, %s, 2\n", integer_reg, integer_reg);
	} else if(type_size(pointer_type) != 1){
		fprintf(stderr, "Unrecognized type size: %d\n", type_size(pointer_type));
		exit(1);
	}
	printf("add %s, %s, %s\n", reg_a, reg_a, reg_b);

	return pointer_value.data_type;
}

type operation_subtract_func(char *reg_a, char *reg_b, value value_a, value value_b){
	type pointer_type;
	type pointer_type2;
	value pointer_value;
	char *integer_reg;

	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		fprintf(stderr, "Error: Subtraction of function type is undefined\n");
		exit(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			pointer_type = value_a.data_type;
			pointer_type2 = value_b.data_type;
			pop_type(&pointer_type);
			pop_type(&pointer_type2);
			if(!types_equal(pointer_type, pointer_type2)){
				fprintf(stderr, "Error: cannot subtract pointers to different types\n");
				exit(1);
			}
			printf("sub %s, %s, %s\n", reg_a, reg_a, reg_b);
			if(type_size(pointer_type) == 4){
				printf("sra %s, %s, 2\n", reg_a, reg_a);
			} else if(type_size(pointer_type) != 1){
				fprintf(stderr, "Unrecognized type size: %d\n", type_size(pointer_type));
				exit(1);
			}
			return value_a.data_type;
		}
		pointer_value = value_a;
		integer_reg = reg_a;
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			pointer_value = value_b;
			integer_reg = reg_b;
		} else {
			printf("sub %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	}
	pointer_type = pointer_value.data_type;
	pop_type(&pointer_type);
	if(type_size(pointer_type) == 4){
		printf("sll %s, %s, 2\n", integer_reg, integer_reg);
	} else if(type_size(pointer_type) != 1){
		fprintf(stderr, "Unrecognized type size: %d\n", type_size(pointer_type));
		exit(1);
	}
	printf("sub %s, %s, %s\n", reg_a, reg_a, reg_b);

	return pointer_value.data_type;
}

type operation_multiply_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		fprintf(stderr, "Error: cannot multiply non-int types\n");
		exit(1);
	}
	printf("mult %s, %s\n", reg_a, reg_b);
	printf("mflo %s\n", reg_a);
	return INT_TYPE;
}

type operation_divide_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		fprintf(stderr, "Error: cannot divide non-int types\n");
		exit(1);
	}
	printf("div %s, %s\n", reg_a, reg_b);
	printf("mflo %s\n", reg_a);
	return INT_TYPE;
}

type operation_assign_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if(!value_a.is_reference){
		fprintf(stderr, "Cannot assign to r-value\n");
		exit(1);
	}
	pop_type(&(value_a.data_type));
	cast(value_b, value_a.data_type);
	if(type_size(value_a.data_type) == 4){
		printf("sw %s, 0(%s)\n", reg_b, reg_a);
	} else if(type_size(value_a.data_type) == 1){
		printf("sb %s, 0(%s)\n", reg_b, reg_a);
	}
	printf("move %s, %s\n", reg_a, reg_b);
	return value_b.data_type;
}

type operation_less_than_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		fprintf(stderr, "Error: cannot compare non-int types with '<'\n");
		exit(1);
	}
	printf("slt %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_greater_than_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		fprintf(stderr, "Error: cannot compare non-int types with '>'\n");
		exit(1);
	}
	printf("sgt %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_equals_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		fprintf(stderr, "Error: cannot compare function types\n");
		exit(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			pop_type(&(value_a.data_type));
			pop_type(&(value_b.data_type));
			if(!types_equal(value_a.data_type, value_b.data_type)){
				fprintf(stderr, "Warning: comparing incompatible data types\n");
			}
			printf("seq %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		} else {
			fprintf(stderr, "Warning: comparing pointer and non-pointer types\n");
			printf("seq %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			fprintf(stderr, "Warning: comparing pointer and non-pointer types\n");
		}
		printf("seq %s, %s, %s\n", reg_a, reg_a, reg_b);
		return INT_TYPE;
	}
}

type operation_not_equals_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		fprintf(stderr, "Error: cannot compare function types\n");
		exit(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			pop_type(&(value_a.data_type));
			pop_type(&(value_b.data_type));
			if(!types_equal(value_a.data_type, value_b.data_type)){
				fprintf(stderr, "Warning: comparing incompatible data types\n");
			}
			printf("sne %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		} else {
			fprintf(stderr, "Warning: comparing pointer and non-pointer types\n");
			printf("sne %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			fprintf(stderr, "Warning: comparing pointer and non-pointer types\n");
		}
		printf("sne %s, %s, %s\n", reg_a, reg_a, reg_b);
		return INT_TYPE;
	}
}

type operation_and_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		fprintf(stderr, "Error: cannot '&' non-int types\n");
		exit(1);
	}
	printf("and %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_or_func(char *reg_a, char *reg_b, value value_a, value value_b){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		fprintf(stderr, "Error: cannot '&' non-int types\n");
		exit(1);
	}
	printf("or %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type (*operation_functions[])(char *, char *, value, value) = {operation_none_func, operation_add_func, operation_subtract_func, operation_multiply_func, operation_divide_func, operation_assign_func, operation_less_than_func, operation_greater_than_func, operation_equals_func, operation_not_equals_func, operation_and_func, operation_or_func};

int type_size(type t){
	switch(pop_type(&t)){
		case type_void:
			return VOID_SIZE;
		case type_int:
			return INT_SIZE;
		case type_char:
			return CHAR_SIZE;
		case type_pointer:
			return POINTER_SIZE;
		case type_function:
			return POINTER_SIZE;
	}

	return 0;
}

void initialize_variables(){
	local_variables = create_dictionary(NULL);
}

void free_var(void *v){
	variable *var;

	var = (variable *) v;
	free(var->varname);
	free(var);
}

void free_local_variables(){
	free_dictionary(local_variables, free_var);
	initialize_variables();
	variables_size = 0;
}

void free_global_variables(){
	free_dictionary(global_variables, free_var);
}

void compile_variable_initializer(char **c){
	variable *var;
	char *varname;

	var = malloc(sizeof(variable));
	varname = malloc(sizeof(char)*32);
	parse_type(&(var->var_type), c, varname, NULL, 32, 0);
	var->varname = varname;
	var->stack_pos = variables_size;
	variables_size += type_size(var->var_type);
	write_dictionary(&local_variables, var->varname, var, 0);
	skip_whitespace(c);
	if(**c != ';'){
		fprintf(stderr, "Expected ';'");
		exit(1);
	}
	++*c;
}

value compile_integer(char **c, unsigned char dereference, unsigned char force_stack){
	value output;
	int int_value;

	if(!dereference){
		fprintf(stderr, "Cannot take address of r-value\n");
		exit(1);
	}
	output.data = allocate(force_stack);
	int_value = strtol(*c, c, 10);
	if(output.data.type == data_register){
		printf("li $s%d, %d\n", (int) output.data.reg, int_value);
	} else if(output.data.type == data_stack){
		printf("li $t0, %d\nsw $t0, %d($sp)\n", int_value, -(int) output.data.stack_pos);
	}
	output.data_type = INT_TYPE;

	return output;
}

static value compile_local_variable(variable *var, unsigned char dereference, unsigned char force_stack){
	data_entry data;
	type data_type;
	value output;

	data_type = var->var_type;
	data = allocate(force_stack);
	output.data_type = data_type;
	output.data = data;
	if(dereference){
		if(data.type == data_register){
			switch(type_size(data_type)){
				case 1:
					printf("lb $s%d, %d($sp)\n", data.reg, variables_size - var->stack_pos);
					break;
				case 4:
					printf("lw $s%d, %d($sp)\n", data.reg, variables_size - var->stack_pos);
					break;

			}
		} else if(data.type == data_stack){
			switch(type_size(data_type)){
				case 1:
					printf("lb $t0, %d($sp)\n", variables_size - var->stack_pos);
					printf("sb $t0, %d($sp)\n", -(int) data.stack_pos);
					break;
				case 4:
					printf("lw $t0, %d($sp)\n", variables_size - var->stack_pos);
					printf("sw $t0, %d($sp)\n", -(int) data.stack_pos);
					break;
			}
		}
	} else {
		if(data.type == data_register){
			printf("addi $s%d, $sp, %d\n", data.reg, variables_size - var->stack_pos);
		} else if(data.type == data_stack){
			printf("addi $t0, $sp, %d\n", variables_size - var->stack_pos);
			printf("sw $t0, %d($sp)\n", -(int) data.stack_pos);
		}
		add_type_entry(&(output.data_type), type_pointer);
	}

	return output;
}

static value compile_global_variable(variable *var, unsigned char dereference, unsigned char force_stack){
	data_entry data;
	type data_type;
	value output;

	data_type = var->var_type;
	data = allocate(force_stack);
	if(data.type == data_register){
		printf("la $s%d, %s\n", data.reg, var->varname);
	} else {
		printf("la $t0, %s\n", var->varname);
	}
	output.data = data;
	output.is_reference = !dereference;
	if(dereference && !var->leave_as_address){
		if(data.type == data_register){
			printf("lw $s%d, 0($s%d)\n", data.reg, data.reg);
		} else {
			printf("lw $t0, 0($t0)\n");
			printf("sw $t0, %d($sp)\n", -(int) data.stack_pos);
		}
	} else {
		add_type_entry(&data_type, type_pointer);
		if(data.type == data_stack){
			printf("sw $t0, %d($sp)\n", -(int) data.stack_pos);
		}
	}
	output.data_type = data_type;

	return output;
}

value compile_variable(char **c, unsigned char dereference, unsigned char force_stack){
	char *start;
	char varname[32] = {0};
	unsigned int varname_length = 0;
	variable *var;

	skip_whitespace(c);
	start = *c;
	while(alphanumeric(**c) && varname_length < 31){
		++*c;
		varname_length++;
	}
	memcpy(varname, start, varname_length);
	varname[31] = '\0';

	var = read_dictionary(local_variables, varname, 0);
	if(!var){
		var = read_dictionary(global_variables, varname, 0);
		if(!var){
			fprintf(stderr, "Unrecognized variable '%s'\n", varname);
			exit(1);
		}
		return compile_global_variable(var, dereference, force_stack);
	} else {
		return compile_local_variable(var, dereference, force_stack);
	}
}

value compile_expression(char **c, unsigned char dereference, unsigned char force_stack);

value cast(value v, type t){
	type t_copy;

	if(type_size(v.data_type) == 4 && type_size(t) == 1){
		if(v.data.type == data_register){
			printf("sll $s%d, $s%d, 24\n", v.data.reg, v.data.reg);
			printf("sra $s%d, $s%d, 24\n", v.data.reg, v.data.reg);
		} else if(v.data.type == data_stack){
			printf("lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
			printf("sll $t0, $t0, 24\n");
			printf("sra $t0, $t0, 24\n");
			printf("sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		}
	} else if(type_size(v.data_type) == 1 && type_size(t) == 4){
		if(v.data.type == data_stack){
			printf("lb $t0, %d($sp)\n", -(int) v.data.stack_pos);
			printf("sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		}
	}

	t_copy = t;
	if((pop_type(&t_copy) == type_pointer) != (pop_type(&(v.data_type)) == type_pointer)){
		fprintf(stderr, "Warning: casting between pointer and non-pointer types\n");
	}

	v.data_type = t;
	return v;
}

value compile_dereference(value v){
	type data_type;

	data_type = v.data_type;
	if(pop_type(&data_type) != type_pointer){
		fprintf(stderr, "Cannot dereference non-pointer type\n");
		exit(1);
	}
	if(peek_type(data_type) == type_void){
		fprintf(stderr, "Cannot dereference void pointer\n");
		exit(1);
	}
	if(v.data.type == data_register){
		if(type_size(data_type) == 1){
			printf("lb $s%d, 0($s%d)\n", v.data.reg, v.data.reg);
		} else if(type_size(data_type) == 4){
			printf("lw $s%d, 0($s%d)\n", v.data.reg, v.data.reg);
		}
	} else if(v.data.type == data_stack){
		printf("lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		if(type_size(data_type) == 1){
			printf("lb $t0, 0($t0)\n");
			printf("sb $t0, %d($sp)\n", -(int) v.data.stack_pos);
		} else if(type_size(data_type) == 4){
			printf("lw $t0, 0($t0)\n");
			printf("sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		}
	}

	v.data_type = data_type;
	return v;
}

value compile_function_call(char **c, value func){
	reg_list reg_state;
	data_entry return_data;
	data_entry return_address_data;
	type current_argument_type;
	value current_argument_value;
	unsigned int label_num;
	unsigned int func_stack_pos;
	unsigned int stack_size_before;

	if(peek_type(func.data_type) == type_pointer){
		pop_type(&(func.data_type));
	}
	if(pop_type(&(func.data_type)) != type_function){
		fprintf(stderr, "Can't call non-function type\n");
		exit(1);
	}

	label_num = num_labels;
	num_labels++;
	reg_state = push_registers();
	stack_size_before = stack_size;

	if(func.data.type == data_register){
		func_stack_pos = get_reg_stack_pos(reg_state, func.data.reg);
	}
	return_address_data = allocate(1);
	return_data = allocate(1);
	printf("la $t0, __L%d\n", label_num);
	printf("sw $t0, %d($sp)\n", -(int) return_address_data.stack_pos);
	if(peek_type(func.data_type) == type_returns){
		skip_whitespace(c);
		if(**c != ')'){
			fprintf(stderr, "Expected ')'\n");
			exit(1);
		}
		++*c;
	}
	while(peek_type(func.data_type) != type_returns){
		current_argument_type = get_argument_type(&(func.data_type));
		current_argument_value = compile_expression(c, 1, 1);
		if(**c != ',' && peek_type(func.data_type) != type_returns){
			fprintf(stderr, "Expected ','\n");
			exit(1);
		} else if(**c != ')' && peek_type(func.data_type) == type_returns){
			fprintf(stderr, "Expected ')'\n");
			exit(1);
		}
		++*c;
		current_argument_value = cast(current_argument_value, current_argument_type);
	}

	pop_type(&(func.data_type));
	if(func.data.type == data_register){
		printf("lw $t0, %d($sp)\n", -(int) func_stack_pos);
		printf("addi $sp, $sp, %d\n", -(int) stack_size_before);
		printf("jr $t0\n\n");
	} else if(func.data.type == data_stack){
		printf("lw $t0, %d($sp)\n", -(int) func.data.stack_pos);
		printf("addi $sp, $sp, %d\n", -(int) stack_size_before);
		printf("jr $t0\n\n");
	}
	printf("__L%d:\n", label_num);
	printf("addi $sp, $sp, %d\n", (int) stack_size_before);
	printf("lw $t0, %d($sp)\n", -(int) return_data.stack_pos);
	deallocate(return_data);
	deallocate(return_address_data);
	pull_registers(reg_state);
	if(func.data.type == data_register){
		printf("move $s%d, $t0\n", func.data.reg);
	} else if(func.data.type == data_stack){
		printf("sw $t0, %d($sp)\n", -(int) func.data.stack_pos);
	}

	return func;
}

value compile_list_index(char **c, value address, unsigned char dereference){
	value index;
	type address_type;

	address_type = address.data_type;
	if(pop_type(&address_type) != type_pointer){
		fprintf(stderr, "Cannot address non-pointer type\n");
		exit(1);
	}
	++*c;
	index = compile_expression(c, 1, 0);
	if(**c != ']'){
		fprintf(stderr, "Expected closing ']'\n");
		exit(1);
	}
	++*c;
	index = cast(index, INT_TYPE);
	if(index.data.type == data_register){
		if(type_size(address_type) == 4){
			printf("sll $s%d, $s%d, 2\n", index.data.reg, index.data.reg);
		} else if(type_size(address_type) != 1){
			fprintf(stderr, "Error: unrecognized type size: %d\n", type_size(address_type));
			exit(1);
		}
		if(address.data.type == data_register){
			printf("add $s%d, $s%d, $s%d\n", address.data.reg, address.data.reg, index.data.reg);
			if(dereference){
				printf("lw $s%d, 0($s%d)\n", address.data.reg, address.data.reg);
			}
		} else if(address.data.type == data_stack){
			printf("lw $t0, %d($sp)\n", -(int) address.data.stack_pos);
			printf("add $t0, $t0, $s%d\n", index.data.reg);
			if(dereference){
				printf("lw $t0, 0($t0)\n");
			}
			printf("sw $t0, %d($sp)\n", -(int) address.data.stack_pos);
		}
	} else if(index.data.type == data_stack){
		printf("lw $t1, %d($sp)\n", -(int) index.data.stack_pos);
		if(type_size(address_type) == 4){
			printf("sll $t1, $t1, 2\n");
		}
		if(address.data.type == data_register){
			printf("add $s%d, $s%d, $t1\n", address.data.reg, address.data.reg);
			if(dereference){
				printf("lw $s%d, 0($s%d)\n", address.data.reg, address.data.reg);
			}
		} else if(address.data.type == data_stack){
			printf("lw $t0, %d($sp)\n", -(int) address.data.stack_pos);
			printf("add $t0, $t0, $t1\n");
			if(dereference){
				printf("lw $t0, 0($t0)\n");
			}
			printf("sw $t0, %d($sp)\n", -(int) address.data.stack_pos);
		}
	}
	deallocate(index.data);
	if(dereference){
		pop_type(&(address.data_type));
	}
	return address;
}

void match_brackets(char **c);

void match_parentheses(char **c){
	while(**c && **c != ')' && **c != ']'){
		if(**c == '('){
			++*c;
			match_parentheses(c);
		} else if(**c == '['){
			++*c;
			match_brackets(c);
		} else {
			++*c;
		}
	}
	if(!**c || **c == ']'){
		fprintf(stderr, "Expected ')'\n");
		exit(1);
	}
	++*c;
}

void match_brackets(char **c){
	while(**c && **c != ']' && **c != ')'){
		if(**c == '('){
			++*c;
			match_parentheses(c);
		} else if(**c == '['){
			++*c;
			match_brackets(c);
		} else {
			++*c;
		}
	}
	if(!**c || **c == ')'){
		fprintf(stderr, "Expected ']'\n");
		exit(1);
	}
	++*c;
}

void skip_string(char **c){
	while(**c != '\"'){
		if(**c == '\\'){
			*c += 2;
		} else {
			++*c;
		}
	}
	++*c;
}

void skip_value(char **c){
	skip_whitespace(c);
	if(**c == '\"'){
		++*c;
		skip_string(c);
		return;
	}
	while(**c == '*' || **c == '&'){
		++*c;
	}
	skip_whitespace(c);
	if(**c == '('){
		++*c;
		match_parentheses(c);
	} else if(**c == '-' || digit(**c)){
		while(digit(**c)){
			++*c;
		}
	} else if(alpha(**c)){
		while(alphanumeric(**c)){
			++*c;
		}
	} else {
		return;
	}
	skip_whitespace(c);
	while(**c == '[' || **c == '('){
		if(**c == '('){
			++*c;
			match_parentheses(c);
		} else if(**c == '['){
			++*c;
			match_brackets(c);
		}
		skip_whitespace(c);
	}
}

value compile_string(char **c, unsigned char dereference, unsigned char force_stack){
	value output;

	if(!dereference){
		fprintf(stderr, "Error: can't get address of r-value\n");
		exit(1);
	}
	++*c;
	skip_string(c);

	output.data = allocate(force_stack);
	if(output.data.type == data_register){
		printf("la $s%d, __str%d\n", output.data.reg, current_string);
	} else if(output.data.type == data_stack){
		printf("la $t0, __str%d\n", current_string);
		printf("sw $t0, %d($sp)\n", -(int) output.data.stack_pos);
	}
	current_string++;
	output.data_type = CHAR_TYPE;
	add_type_entry(&(output.data_type), type_pointer);
	output.is_reference = 0;

	return output;
}

value compile_logical_not(value v){
	if(v.data.type == data_register){
		printf("seq $s%d, $s%d, $zero\n", v.data.reg, v.data.reg);
	} else if(v.data.type == data_stack){
		printf("lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		printf("seq $t0, $t0, $zero\n");
		printf("sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
	}

	v.data_type = INT_TYPE;
	return v;
}

value compile_not(value v){
	if(!types_equal(v.data_type, INT_TYPE) && !types_equal(v.data_type, CHAR_TYPE)){
		fprintf(stderr, "Error: Can't perform bitwise not of non-numerical type\n");
		exit(1);
	}
	if(v.data.type == data_register){
		printf("seq $s%d, $s%d, $s%d\n", v.data.reg, v.data.reg, v.data.reg);
	} else if(v.data.type == data_stack){
		printf("lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		printf("nor $t0, $t0, $t0\n");
		printf("sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
	}

	v.data_type = INT_TYPE;
	return v;
}

value compile_negate(value v){
	if(!types_equal(v.data_type, INT_TYPE) && !types_equal(v.data_type, CHAR_TYPE)){
		fprintf(stderr, "Error: Can't negate non-numerical type\n");
		exit(1);
	}
	if(v.data.type == data_register){
		printf("sub $s%d, $zero, $s%d\n", v.data.reg, v.data.reg);
	} else if(v.data.type == data_stack){
		printf("lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		printf("sub $t0, $zero, $t0\n");
		printf("sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
	}

	v.data_type = INT_TYPE;
	return v;
}

value compile_value(char **c, unsigned char dereference, unsigned char force_stack){
	value output;
	char *temp_c;
	type cast_type;

	skip_whitespace(c);

	if(**c == '\"'){
		return compile_string(c, dereference, force_stack);
	}

	if(**c == '*'){
		++*c;
		output = compile_value(c, 1, force_stack);
		if(dereference){
			output = compile_dereference(output);
		}
		output.is_reference = !dereference;
		return output;
	} else if(**c == '&'){
		++*c;
		if(dereference){
			output = compile_value(c, 0, force_stack);
		} else {
			fprintf(stderr, "Can't get address of r-value\n");
			exit(1);
		}
		output.is_reference = 0;
		return output;
	} else if(**c == '!'){
		++*c;
		if(dereference){
			output = compile_logical_not(compile_value(c, 1, force_stack));
		} else {
			fprintf(stderr, "Can't get address of r-value\n");
			exit(1);
		}
		output.is_reference = 0;
		return output;
	} else if(**c == '~'){
		++*c;
		if(dereference){
			output = compile_not(compile_value(c, 1, force_stack));
		} else {
			fprintf(stderr, "Can't get address of r-value\n");
			exit(1);
		}
		output.is_reference = 0;
		return output;
	} else if(**c == '-'){
		++*c;
		if(dereference){
			output = compile_negate(compile_value(c, 1, force_stack));
		} else {
			fprintf(stderr, "Can't get address of r-value\n");
		}
		output.is_reference = 0;
		return output;
	}

	skip_whitespace(c);

	if(**c == '('){
		++*c;
		skip_whitespace(c);
		temp_c = *c;
		//Type casting
		if(parse_datatype(NULL, &temp_c)){
			if(!dereference){
				fprintf(stderr, "Can't get address of r-value\n");
				exit(1);
			}
			parse_type(&cast_type, c, NULL, NULL, 0, 0);
			if(**c != ')'){
				fprintf(stderr, "Error: Expected ')'\n");
				exit(1);
			}
			++*c;
			return cast(compile_value(c, 1, force_stack), cast_type);
		//Associative parentheses
		} else {
			output = compile_expression(c, dereference, force_stack);
			if(**c == ')'){
				++*c;
			} else {
				fprintf(stderr, "Expected closing ')'\n");
			}
		}
	} else if(**c == '-' || digit(**c)){
		output = compile_integer(c, dereference, force_stack);
	} else if(alpha(**c)){
		output = compile_variable(c, dereference, force_stack);
	} else {
		fprintf(stderr, "unrecognized expression value %s\n", *c);
		exit(1);
	}

	skip_whitespace(c);
	while(**c == '[' || **c == '('){
		if(**c == '['){
			if(!dereference){
				output = compile_dereference(output);
			}
			output = compile_list_index(c, output, dereference);
		} else if(**c == '('){
			++*c;
			if(!dereference){
				fprintf(stderr, "Cannot get address of return value of function\n");
				exit(1);
			}
			output = compile_function_call(c, output);
		}

		skip_whitespace(c);
	}

	output.is_reference = !dereference;
	return output;
}

operation peek_operation(char *c){
	switch(*c){
		case '+':
			return operation_add;
		case '-':
			return operation_subtract;
		case '*':
			return operation_multiply;
		case '/':
			return operation_divide;
		case '=':
			if(c[1] == '='){
				return operation_equals;
			} else {
				return operation_assign;
			}
		case '!':
			if(c[1] == '='){
				return operation_not_equals;
			} else {
				return operation_none;
			}
		case '>':
			return operation_greater_than;
		case '<':
			return operation_less_than;
		case '&':
			return operation_and;
		case '|':
			return operation_or;
	}

	return operation_none;
}

operation get_operation(char **c){
	operation output;

	skip_whitespace(c);
	switch(**c){
		case '+':
			output = operation_add;
			break;
		case '-':
			output = operation_subtract;
			break;
		case '*':
			output = operation_multiply;
			break;
		case '/':
			output = operation_divide;
			break;
		case '=':
			if((*c)[1] == '='){
				output = operation_equals;
				++*c;
			} else {
				output = operation_assign;
			}
			break;
		case '!':
			if((*c)[1] == '='){
				output = operation_not_equals;
				++*c;
			} else {
				output = operation_none;
			}
			break;
		case '>':
			output = operation_greater_than;
			break;
		case '<':
			output = operation_less_than;
			break;
		case '&':
			output = operation_and;
			break;
		case '|':
			output = operation_or;
			break;
		default:
			output = operation_none;
			break;
	}
	++*c;
	return output;
}

value compile_operation(value first_value, value next_value, operation op, unsigned char dereference){
	char reg0_str_buffer[5];
	char reg1_str_buffer[5];
	char *reg0_str;
	char *reg1_str;
	type output_type;

	if(!dereference){
		fprintf(stderr, "Can't get address of r-value expression\n");
		exit(1);
	}

	if(first_value.data.type == data_register){
		snprintf(reg0_str_buffer, sizeof(reg0_str_buffer)/sizeof(char), "$s%d", first_value.data.reg);
		reg0_str = reg0_str_buffer;
	} else if(first_value.data.type == data_stack){
		reg0_str = "$t0";
		if(type_size(first_value.data_type) == 4){
			printf("lw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else if(type_size(first_value.data_type) == 1){
			printf("lb $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else {
			fprintf(stderr, "Unusable type size %d\n", type_size(first_value.data_type));
		}
	}
	if(next_value.data.type == data_register){
		snprintf(reg1_str_buffer, sizeof(reg1_str_buffer)/sizeof(char), "$s%d", next_value.data.reg);
		reg1_str = reg1_str_buffer;
	} else if(next_value.data.type == data_stack){
		reg1_str = "$t1";
		if(type_size(next_value.data_type) == 4){
			printf("lw $t1, %d($sp)\n", -(int) next_value.data.stack_pos);
		} else if(type_size(next_value.data_type) == 1){
			printf("lb $t1, %d($sp)\n", -(int) next_value.data.stack_pos);
		} else {
			fprintf(stderr, "Unusable type size %d\n", type_size(next_value.data_type));
		}
	}
	output_type = operation_functions[op](reg0_str, reg1_str, first_value, next_value);
	deallocate(next_value.data);
	if(first_value.data.type == data_stack){
		if(type_size(output_type) == 4){
			printf("sw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else if(type_size(output_type) == 1){
			printf("sw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else {
			fprintf(stderr, "Unusable type size %d\n", type_size(output_type));
		}
	}
	first_value.data_type = output_type;
	first_value.is_reference = 0;
	return first_value;
}

//Note: dereference = 0 and an operation is compiled will cause an error!
//This is intended behavior. In essence, at this point dereference just tells whether
//compiling an operation should cause an error.
static value compile_expression_recursive(value first_value, char **c, unsigned char dereference){
	operation current_operation;
	operation next_operation;
	value next_value;
	char *temp_c;

	skip_whitespace(c);
	while(**c && **c != ';' && **c != ',' && **c != ')' && **c != ']'){
		current_operation = get_operation(c);
		temp_c = *c;
		skip_value(&temp_c);
		next_operation = peek_operation(temp_c);
		if(next_operation == operation_assign){
			next_value = compile_value(c, 0, 0);
		} else {
			next_value = compile_value(c, 1, 0);
		}
		skip_whitespace(c);
		while(order_of_operations[next_operation] > order_of_operations[current_operation]){
			next_value = compile_expression_recursive(next_value, c, 1);
			skip_whitespace(c);
			next_operation = peek_operation(*c);
		}
		first_value = compile_operation(first_value, next_value, current_operation, dereference);
		skip_whitespace(c);
	}

	return first_value;
}

value compile_expression(char **c, unsigned char dereference, unsigned char force_stack){
	value first_value;
	char *temp_c;

	temp_c = *c;
	skip_value(&temp_c);
	skip_whitespace(&temp_c);

	if(peek_operation(temp_c) == operation_assign){
		first_value = compile_value(c, 0, force_stack);
	} else {
		first_value = compile_value(c, dereference, force_stack);
	}
	return compile_expression_recursive(first_value, c, dereference);
}
/*
int main(){
	char *t0 = "int (*test)(int, int);";
	char *t1 = "int *test2;";
	char *t2 = "test2[test(1,2)] = 1;";
	initialize_variables();
	initialize_register_list();
	compile_variable_initializer(&t0);
	compile_variable_initializer(&t1);
	compile_expression(&t2, 1, 0);
}*/
