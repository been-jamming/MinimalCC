#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "expression.h"
#include "compile.h"

unsigned int scopes[MAX_SCOPE];
int current_scope;

dictionary *local_variables[MAX_SCOPE];
dictionary global_variables;
unsigned int num_labels = 0;
unsigned int current_string = 0;
static unsigned int order_of_operations[] = {0, 9, 9, 10, 10, 1, 7, 7, 6, 6, 5, 4, 9, 3, 2, 8, 8};

type operation_none_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	snprintf(error_message, sizeof(error_message), "Unrecognized operation");
	do_error(1);

	return value_a.data_type;
}

type operation_add_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	type pointer_type;
	value pointer_value;
	char *integer_reg;

	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		snprintf(error_message, sizeof(error_message), "Addition of function type is undefined");
		do_error(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			snprintf(warning_message, sizeof(warning_message), "Adding two pointers together. Treating them as integers instead");
			do_warning();
			fprintf(output_file, "add %s, %s, %s\n", reg_a, reg_a, reg_b);
			return value_a.data_type;
		}
		pointer_value = value_a;
		integer_reg = reg_b;
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			pointer_value = value_b;
			integer_reg = reg_a;
		} else {
			fprintf(output_file, "add %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	}
	pointer_type = pointer_value.data_type;
	pop_type(&pointer_type);
	if(type_size(pointer_type) == 4){
		fprintf(output_file, "sll %s, %s, 2\n", integer_reg, integer_reg);
	} else if(type_size(pointer_type) != 1){
		fprintf(output_file, "li $t2, %d\n", type_size(pointer_type));
		fprintf(output_file, "mult %s, $t2\n", integer_reg);
		fprintf(output_file, "mflo %s\n", integer_reg);
	}
	fprintf(output_file, "add %s, %s, %s\n", reg_a, reg_a, reg_b);

	return pointer_value.data_type;
}

type operation_subtract_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	type pointer_type;
	type pointer_type2;
	value pointer_value;
	char *integer_reg;

	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		snprintf(error_message, sizeof(error_message), "Subtraction of function type is undefined");
		do_error(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			pointer_type = value_a.data_type;
			pointer_type2 = value_b.data_type;
			pop_type(&pointer_type);
			pop_type(&pointer_type2);
			if(!types_equal(pointer_type, pointer_type2)){
				snprintf(error_message, sizeof(error_message), "Cannot subtract pointers to different types");
				do_error(1);
			}
			fprintf(output_file, "sub %s, %s, %s\n", reg_a, reg_a, reg_b);
			if(type_size(pointer_type) == 4){
				fprintf(output_file, "sra %s, %s, 2\n", reg_a, reg_a);
			} else if(type_size(pointer_type) != 1){
				fprintf(output_file, "li $t2, %d\n", type_size(pointer_type));
				fprintf(output_file, "div %s, $t2\n", reg_a);
				fprintf(output_file, "mflo %s\n", reg_a);
			}
			return INT_TYPE;
		}
		pointer_value = value_a;
		integer_reg = reg_b;
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			pointer_value = value_b;
			integer_reg = reg_a;
		} else {
			fprintf(output_file, "sub %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	}
	pointer_type = pointer_value.data_type;
	pop_type(&pointer_type);
	if(type_size(pointer_type) == 4){
		fprintf(output_file, "sll %s, %s, 2\n", integer_reg, integer_reg);
	} else if(type_size(pointer_type) != 1){
		fprintf(output_file, "li $t2, %d\n", type_size(pointer_type));
		fprintf(output_file, "mult %s, $t2\n", integer_reg);
		fprintf(output_file, "mflo %s\n", integer_reg);
	}
	fprintf(output_file, "sub %s, %s, %s\n", reg_a, reg_a, reg_b);

	return pointer_value.data_type;
}

type operation_multiply_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot multiply non-int types");
		do_error(1);
	}
	fprintf(output_file, "mult %s, %s\n", reg_a, reg_b);
	fprintf(output_file, "mflo %s\n", reg_a);
	return INT_TYPE;
}

type operation_divide_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot divide non-int types");
		do_error(1);
	}
	fprintf(output_file, "div %s, %s\n", reg_a, reg_b);
	fprintf(output_file, "mflo %s\n", reg_a);
	return INT_TYPE;
}

type operation_modulo_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot modulo non-int types");
		do_error(1);
	}
	fprintf(output_file, "div %s, %s\n", reg_a, reg_b);
	fprintf(output_file, "mfhi %s\n", reg_a);
	return INT_TYPE;
}

type operation_assign_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if(!value_a.is_reference){
		snprintf(error_message, sizeof(error_message), "Cannot assign to r-value");
		do_error(1);
	}
	pop_type(&(value_a.data_type));
	if(peek_type(value_a.data_type) == type_function){
		snprintf(error_message, sizeof(error_message), "Cannot assign to function");
		do_error(1);
	}
	if(type_size(value_a.data_type) == 4){
		fprintf(output_file, "sw %s, 0(%s)\n", reg_b, reg_a);
	} else if(type_size(value_a.data_type) == 1){
		fprintf(output_file, "sb %s, 0(%s)\n", reg_b, reg_a);
	}
	fprintf(output_file, "move %s, %s\n", reg_a, reg_b);
	return value_b.data_type;
}

type operation_less_than_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot compare non-int types with '<'");
		do_error(1);
	}
	fprintf(output_file, "slt %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_greater_than_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot compare non-int types with '>'");
		do_error(1);
	}
	fprintf(output_file, "sgt %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_equals_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		snprintf(error_message, sizeof(error_message), "Cannot compare function types");
		do_error(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			pop_type(&(value_a.data_type));
			pop_type(&(value_b.data_type));
			if(!types_equal(value_a.data_type, value_b.data_type)){
				snprintf(warning_message, sizeof(warning_message), "Comparing incompatible data types");
				do_warning();
			}
			fprintf(output_file, "seq %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		} else {
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
			fprintf(output_file, "seq %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
		}
		fprintf(output_file, "seq %s, %s, %s\n", reg_a, reg_a, reg_b);
		return INT_TYPE;
	}
}

type operation_not_equals_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if(peek_type(value_a.data_type) == type_function || peek_type(value_b.data_type) == type_function){
		snprintf(error_message, sizeof(error_message), "Cannot compare function types");
		do_error(1);
	}
	if(peek_type(value_a.data_type) == type_pointer){
		if(peek_type(value_b.data_type) == type_pointer){
			pop_type(&(value_a.data_type));
			pop_type(&(value_b.data_type));
			if(!types_equal(value_a.data_type, value_b.data_type)){
				snprintf(warning_message, sizeof(warning_message), "Comparing incompatible data types");
				do_warning();
			}
			fprintf(output_file, "sne %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		} else {
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
			fprintf(output_file, "sne %s, %s, %s\n", reg_a, reg_a, reg_b);
			return INT_TYPE;
		}
	} else {
		if(peek_type(value_b.data_type) == type_pointer){
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
		}
		fprintf(output_file, "sne %s, %s, %s\n", reg_a, reg_a, reg_b);
		return INT_TYPE;
	}
}

type operation_and_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '&' non-int types");
		do_error(1);
	}
	fprintf(output_file, "and %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_or_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '|' non-int types");
		do_error(1);
	}
	fprintf(output_file, "or %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_shift_left_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '<<' non-arithmetic types");
		do_error(1);
	}
	fprintf(output_file, "sllv %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type operation_shift_right_func(char *reg_a, char *reg_b, value value_a, value value_b, FILE *output_file){
	if((!types_equal(value_a.data_type, INT_TYPE) && !types_equal(value_a.data_type, CHAR_TYPE)) || (!types_equal(value_b.data_type, INT_TYPE) && !types_equal(value_b.data_type, CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '>>' non-arithmetic types");
		do_error(1);
	}
	fprintf(output_file, "srav %s, %s, %s\n", reg_a, reg_a, reg_b);
	return INT_TYPE;
}

type (*operation_functions[])(char *, char *, value, value, FILE *) = {operation_none_func, operation_add_func, operation_subtract_func, operation_multiply_func, operation_divide_func, operation_assign_func, operation_less_than_func, operation_greater_than_func, operation_equals_func, operation_not_equals_func, operation_and_func, operation_or_func, operation_modulo_func, operation_none_func, operation_none_func, operation_shift_left_func, operation_shift_right_func};

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
		case type_list:
			t.current_index--;
			return t.list_indicies[t.current_index]*type_size(t);
	}

	return 0;
}

static void free_var(void *v){
	variable *var;

	var = (variable *) v;
	free(var->varname);
	free(var);
}

void free_global_variables(){
	free_dictionary(global_variables, free_var);
}

void free_local_variables(){
	int i;

	for(i = current_scope; i >= 0; i--){
		if(local_variables[i]){
			free_dictionary(*local_variables[i], free_var);
			free(local_variables[i]);
			local_variables[i] = NULL;
		}
	}

	variables_size = 0;
}

unsigned int align4(unsigned int size){
	unsigned int remainder;

	remainder = size%4;
	if(remainder){
		size += 4 - remainder;
	}

	return size;
}

void compile_variable_initializer(char **c){
	variable *var;
	char varname_buf[32];
	type vartype;

	vartype = EMPTY_TYPE;
	parse_type(&vartype, c, varname_buf, NULL, 32, 0);
	var = malloc(sizeof(variable));
	var->varname = malloc(sizeof(char)*32);
	var->var_type = vartype;
	strcpy(var->varname, varname_buf);
	variables_size += align4(type_size(var->var_type));
	var->stack_pos = variables_size - 4;
	if(read_dictionary(*local_variables[current_scope], var->varname, 0)){
		free(var->varname);
		free(var);
		snprintf(error_message, sizeof(error_message), "Duplicate local variable definition");
		do_error(1);
	}
	write_dictionary(local_variables[current_scope], var->varname, var, 0);
	skip_whitespace(c);
	if(**c != ';'){
		snprintf(error_message, sizeof(error_message), "Expected ';'");
		do_error(1);
	}
	++*c;
}

value compile_integer(char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	value output;
	int int_value;

	output.data = allocate(force_stack);
	int_value = strtol(*c, c, 0);
	if(output.data.type == data_register){
		fprintf(output_file, "li $s%d, %d\n", (int) output.data.reg, int_value);
	} else if(output.data.type == data_stack){
		fprintf(output_file, "li $t0, %d\nsw $t0, %d($sp)\n", int_value, -(int) output.data.stack_pos);
	}
	output.data_type = INT_TYPE;
	output.is_reference = 0;

	return output;
}

static value compile_local_variable(variable *var, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	data_entry data;
	type data_type;
	value output;

	data_type = var->var_type;
	data = allocate(force_stack);
	output.data_type = data_type;
	output.data = data;
	if(dereference){
		if(peek_type(var->var_type) == type_list){
			if(data.type == data_register){
				fprintf(output_file, "addi $s%d, $sp, %d\n", data.reg, variables_size - var->stack_pos);
			} else if(data.type == data_stack){
				fprintf(output_file, "addi $t0, $sp, %d\n", variables_size - var->stack_pos);
				fprintf(output_file, "sw $t0, %d($sp)\n", -(int) data.stack_pos);
			}
			pop_type(&(output.data_type));
			output.data_type.current_index--;
			add_type_entry(&(output.data_type), type_pointer);
		} else {
			if(data.type == data_register){
				switch(type_size(data_type)){
					case 1:
						fprintf(output_file, "lb $s%d, %d($sp)\n", data.reg, variables_size - var->stack_pos);
						break;
					case 4:
						fprintf(output_file, "lw $s%d, %d($sp)\n", data.reg, variables_size - var->stack_pos);
						break;

				}
			} else if(data.type == data_stack){
				switch(type_size(data_type)){
					case 1:
						fprintf(output_file, "lb $t0, %d($sp)\n", variables_size - var->stack_pos);
						fprintf(output_file, "sb $t0, %d($sp)\n", -(int) data.stack_pos);
						break;
					case 4:
						fprintf(output_file, "lw $t0, %d($sp)\n", variables_size - var->stack_pos);
						fprintf(output_file, "sw $t0, %d($sp)\n", -(int) data.stack_pos);
						break;
				}
			}
		}
	} else {
		if(peek_type(var->var_type) == type_list){
			if(data.type == data_register){
				fprintf(output_file, "addi $s%d, $sp, %d\n", data.reg, variables_size - var->stack_pos);
			} else if(data.type == data_stack){
				fprintf(output_file, "addi $t0, $sp, %d\n", variables_size - var->stack_pos);
				fprintf(output_file, "sw $t0, %d($sp)\n", -(int) data.stack_pos);
			}
		} else {
			if(data.type == data_register){
				fprintf(output_file, "addi $s%d, $sp, %d\n", data.reg, variables_size - var->stack_pos);
			} else if(data.type == data_stack){
				fprintf(output_file, "addi $t0, $sp, %d\n", variables_size - var->stack_pos);
				fprintf(output_file, "sw $t0, %d($sp)\n", -(int) data.stack_pos);
			}
		}
		add_type_entry(&(output.data_type), type_pointer);
	}

	output.is_reference = !dereference;

	return output;
}

static value compile_global_variable(variable *var, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	data_entry data;
	type data_type;
	value output;

	data_type = var->var_type;
	data = allocate(force_stack);
	if(data.type == data_register){
		fprintf(output_file, "la $s%d, %s\n", data.reg, var->varname);
	} else {
		fprintf(output_file, "la $t0, %s\n", var->varname);
	}
	output.data = data;
	output.is_reference = !dereference;
	if(dereference && !var->leave_as_address){
		if(peek_type(data_type) == type_list){
			pop_type(&data_type);
			data_type.current_index--;
			add_type_entry(&data_type, type_pointer);
			if(data.type == data_stack){
				fprintf(output_file, "sw $t0, %d($sp)\n", -(int) data.stack_pos);
			}
		} else {
			if(data.type == data_register){
				fprintf(output_file, "lw $s%d, 0($s%d)\n", data.reg, data.reg);
			} else if(data.type == data_stack){
				fprintf(output_file, "lw $t0, 0($t0)\n");
				fprintf(output_file, "sw $t0, %d($sp)\n", -(int) data.stack_pos);
			}
		}
	} else {
		add_type_entry(&data_type, type_pointer);
		if(data.type == data_stack){
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) data.stack_pos);
		}
	}
	output.data_type = data_type;

	return output;
}

value compile_variable(char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	char *start;
	char varname[32] = {0};
	unsigned int varname_length = 0;
	variable *var;
	int i;

	skip_whitespace(c);
	start = *c;
	while(alphanumeric(**c) && varname_length < 31){
		++*c;
		varname_length++;
	}
	memcpy(varname, start, varname_length);
	varname[31] = '\0';

	for(i = current_scope; i >= 0; i--){
		var = read_dictionary(*local_variables[i], varname, 0);
		if(var){
			break;
		}
	}

	if(!var){
		var = read_dictionary(global_variables, varname, 0);
		if(!var){
			snprintf(error_message, sizeof(error_message), "Unrecognized variable '%s'", varname);
			do_error(1);
		}
		return compile_global_variable(var, dereference, force_stack, output_file);
	} else {
		return compile_local_variable(var, dereference, force_stack, output_file);
	}
}

value compile_expression(char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file);

value cast(value v, type t, unsigned char do_warn, FILE *output_file){
	type t_copy;
	type_entry entry1;
	type_entry entry2;

	if(types_equal(v.data_type, VOID_TYPE) && !types_equal(t, VOID_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't cast void type to non-void type");
		do_error(1);
	}

	if(peek_type(t) == type_list){
		snprintf(error_message, sizeof(error_message), "Can't cast to an array");
		do_error(1);
	}

	if(type_size(v.data_type) == 4 && type_size(t) == 1){
		if(v.data.type == data_register){
			fprintf(output_file, "sll $s%d, $s%d, 24\n", v.data.reg, v.data.reg);
			fprintf(output_file, "sra $s%d, $s%d, 24\n", v.data.reg, v.data.reg);
		} else if(v.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
			fprintf(output_file, "sll $t0, $t0, 24\n");
			fprintf(output_file, "sra $t0, $t0, 24\n");
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		}
	} else if(type_size(v.data_type) == 1 && type_size(t) == 4){
		if(v.data.type == data_stack){
			fprintf(output_file, "lb $t0, %d($sp)\n", -(int) v.data.stack_pos);
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		}
	}

	t_copy = t;
	entry1 = pop_type(&t_copy);
	entry2 = pop_type(&(v.data_type));
	if((entry1 == type_pointer) != (entry2 == type_pointer) && do_warn){
		snprintf(warning_message, sizeof(warning_message), "Casting between pointer and non-pointer types");
		do_warning();
	} else if(entry1 == type_pointer && !types_equal(t_copy, v.data_type) && !types_equal(t_copy, VOID_TYPE) && !types_equal(v.data_type, VOID_TYPE) && do_warn){
		snprintf(warning_message, sizeof(warning_message), "Casting between incompatible pointer types");
		do_warning();
	}

	v.data_type = t;
	return v;
}

value compile_dereference(value v, FILE *output_file){
	type data_type;

	data_type = v.data_type;
	if(pop_type(&data_type) != type_pointer){
		snprintf(error_message, sizeof(error_message), "Cannot dereference non-pointer type");
		do_error(1);
	}
	if(peek_type(data_type) == type_void){
		snprintf(error_message, sizeof(error_message), "Cannot dereference void pointer");
		do_error(1);
	}
	if(peek_type(data_type) == type_list){
		pop_type(&data_type);
		data_type.current_index--;
		add_type_entry(&data_type, type_pointer);
	} else if(peek_type(data_type) != type_function){
		if(v.data.type == data_register){
			if(type_size(data_type) == 1){
				fprintf(output_file, "lb $s%d, 0($s%d)\n", v.data.reg, v.data.reg);
			} else if(type_size(data_type) == 4){
				fprintf(output_file, "lw $s%d, 0($s%d)\n", v.data.reg, v.data.reg);
			}
		} else if(v.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
			if(type_size(data_type) == 1){
				fprintf(output_file, "lb $t0, 0($t0)\n");
				fprintf(output_file, "sb $t0, %d($sp)\n", -(int) v.data.stack_pos);
			} else if(type_size(data_type) == 4){
				fprintf(output_file, "lw $t0, 0($t0)\n");
				fprintf(output_file, "sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
			}
		}
	}

	v.data_type = data_type;
	v.is_reference = 0;
	return v;
}

value compile_function_call(char **c, value func, FILE *output_file){
	reg_list reg_state;
	data_entry return_data;
	data_entry return_address_data;
	type current_argument_type;
	value current_argument_value;
	unsigned int label_num;
	unsigned int func_stack_pos = 0;
	unsigned int stack_size_before;

	if(peek_type(func.data_type) == type_pointer){
		pop_type(&(func.data_type));
	}
	if(pop_type(&(func.data_type)) != type_function){
		snprintf(error_message, sizeof(error_message), "Can't call non-function type");
		do_error(1);
	}

	label_num = num_labels;
	num_labels++;
	reg_state = push_registers(output_file);
	stack_size_before = stack_size;

	if(func.data.type == data_register){
		func_stack_pos = get_reg_stack_pos(reg_state, func.data.reg);
	}
	return_address_data = allocate(1);
	return_data = allocate(1);
	fprintf(output_file, "la $t0, __L%d\n", label_num);
	fprintf(output_file, "sw $t0, %d($sp)\n", -(int) return_address_data.stack_pos);
	if(peek_type(func.data_type) == type_returns){
		skip_whitespace(c);
		if(**c != ')'){
			snprintf(error_message, sizeof(error_message), "Expected ')'");
			do_error(1);
		}
		++*c;
	}
	while(peek_type(func.data_type) != type_returns){
		current_argument_type = get_argument_type(&(func.data_type));
		current_argument_value = compile_expression(c, 1, 1, output_file);
		if(**c != ',' && peek_type(func.data_type) != type_returns){
			snprintf(error_message, sizeof(error_message), "Expected ','");
			do_error(1);
		} else if(**c != ')' && peek_type(func.data_type) == type_returns){
			snprintf(error_message, sizeof(error_message), "Expected ')'");
			do_error(1);
		}
		++*c;
		current_argument_value = cast(current_argument_value, current_argument_type, 1, output_file);
	}

	pop_type(&(func.data_type));
	if(func.data.type == data_register){
		fprintf(output_file, "lw $t0, %d($sp)\n", -(int) func_stack_pos);
		fprintf(output_file, "addi $sp, $sp, %d\n", -(int) stack_size_before);
		fprintf(output_file, "jr $t0\n\n");
	} else if(func.data.type == data_stack){
		fprintf(output_file, "lw $t0, %d($sp)\n", -(int) func.data.stack_pos);
		fprintf(output_file, "addi $sp, $sp, %d\n", -(int) stack_size_before);
		fprintf(output_file, "jr $t0\n\n");
	}
	fprintf(output_file, "__L%d:\n", label_num);
	fprintf(output_file, "addi $sp, $sp, %d\n", (int) stack_size_before);
	fprintf(output_file, "lw $t0, %d($sp)\n", -(int) return_data.stack_pos);
	deallocate(return_data);
	deallocate(return_address_data);
	pull_registers(reg_state, output_file);
	if(func.data.type == data_register){
		fprintf(output_file, "move $s%d, $t0\n", func.data.reg);
	} else if(func.data.type == data_stack){
		fprintf(output_file, "sw $t0, %d($sp)\n", -(int) func.data.stack_pos);
	}

	return func;
}

value compile_list_index(char **c, value address, unsigned char dereference, FILE *output_file){
	value index;
	type address_type;

	address_type = address.data_type;
	if(pop_type(&address_type) != type_pointer){
		snprintf(error_message, sizeof(error_message), "Cannot address non-pointer type");
		do_error(1);
	}
	++*c;
	index = compile_expression(c, 1, 0, output_file);
	if(**c != ']'){
		snprintf(error_message, sizeof(error_message), "Expected closing ']'");
		do_error(1);
	}
	++*c;
	index = cast(index, INT_TYPE, 1, output_file);
	if(index.data.type == data_register){
		if(type_size(address_type) == 4){
			fprintf(output_file, "sll $s%d, $s%d, 2\n", index.data.reg, index.data.reg);
		} else if(type_size(address_type) != 1){
			fprintf(output_file, "li $t0, %d\n", (int) type_size(address_type));
			fprintf(output_file, "mult $s%d, $t0\n", index.data.reg);
			fprintf(output_file, "mflo $s%d\n", index.data.reg);
		}
		if(address.data.type == data_register){
			fprintf(output_file, "add $s%d, $s%d, $s%d\n", address.data.reg, address.data.reg, index.data.reg);
			if(dereference && peek_type(address_type) != type_list){
				if(type_size(address_type) == 4){
					fprintf(output_file, "lw $s%d, 0($s%d)\n", address.data.reg, address.data.reg);
				} else if(type_size(address_type) == 1){
					fprintf(output_file, "lb $s%d, 0($s%d)\n", address.data.reg, address.data.reg);
				} else {
					snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size");
					do_error(1);
				}
			}
		} else if(address.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) address.data.stack_pos);
			fprintf(output_file, "add $t0, $t0, $s%d\n", index.data.reg);
			if(dereference && peek_type(address_type) != type_list){
				if(type_size(address_type) == 4){
					fprintf(output_file, "lw $t0, 0($t0)\n");
				} else if(type_size(address_type) == 1){
					fprintf(output_file, "lb $t0, 0($t0)\n");
				} else {
					snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size");
					do_error(1);
				}
			}
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) address.data.stack_pos);
		}
	} else if(index.data.type == data_stack){
		fprintf(output_file, "lw $t1, %d($sp)\n", -(int) index.data.stack_pos);
		if(type_size(address_type) == 4){
			fprintf(output_file, "sll $t1, $t1, 2\n");
		} else if(type_size(address_type) != 1){
			fprintf(output_file, "li $t0, %d\n", (int) type_size(address_type));
			fprintf(output_file, "mult $t1, $t0\n");
			fprintf(output_file, "mflo $t1\n");
		}
		if(address.data.type == data_register){
			fprintf(output_file, "add $s%d, $s%d, $t1\n", address.data.reg, address.data.reg);
			if(dereference && peek_type(address_type) != type_list){
				if(type_size(address_type) == 4){
					fprintf(output_file, "lw $s%d, 0($s%d)\n", address.data.reg, address.data.reg);
				} else if(type_size(address_type) == 1){
					fprintf(output_file, "lb $s%d, 0($s%d)\n", address.data.reg, address.data.reg);
				}
			}
		} else if(address.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) address.data.stack_pos);
			fprintf(output_file, "add $t0, $t0, $t1\n");
			if(dereference && peek_type(address_type) != type_list){
				if(type_size(address_type) == 4){
					fprintf(output_file, "lw $t0, 0($t0)\n");
				} else if(type_size(address_type) == 1){
					fprintf(output_file, "lb $t0, 0($t0)\n");
				}
			}
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) address.data.stack_pos);
		}
	}
	deallocate(index.data);
	if(dereference){
		pop_type(&(address.data_type));
		if(peek_type(address.data_type) == type_list){
			pop_type(&(address.data_type));
			address.data_type.current_index--;
			add_type_entry(&(address.data_type), type_pointer);
		}
	}
	address.is_reference = !dereference;
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
		snprintf(error_message, sizeof(error_message), "Expected ')'");
		do_error(1);
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
		snprintf(error_message, sizeof(error_message), "Expected ']'");
		do_error(1);
	}
	++*c;
}

void skip_string(char **c){
	while(**c && **c != '\"'){
		if(**c == '\\'){
			*c += 2;
		} else {
			++*c;
		}
	}
	if(**c){
		++*c;
	}
}

unsigned char is_cast(char *c){
	if(*c != '('){
		return 0;
	}

	c++;
	return parse_datatype(NULL, &c);
}

void skip_value(char **c){
	skip_whitespace(c);
	while(**c == '*' || **c == '&' || **c == '!' || **c == '~' || **c == '-' || is_cast(*c) || is_whitespace(**c)){
		if(**c == '('){//ie if we are casting
			++*c;
			match_parentheses(c);
		} else {
			++*c;
		}
	}
	while(**c == '('){
		++*c;
		match_parentheses(c);
		skip_whitespace(c);
	}
	if(digit(**c)){
		while(digit(**c)){
			++*c;
		}
	} else if(alpha(**c)){
		while(alphanumeric(**c)){
			++*c;
		}
	} else if(**c == '\"'){
		++*c;
		skip_string(c);
	} else if(**c == '\''){
		++*c;
		if(**c == '\\'){
			++*c;
		}
		while(**c && **c != '\''){
			++*c;
		}
		if(**c){
			++*c;
		}
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
		skip_whitespace(c);
	}
}

value compile_string(char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	value output;

	++*c;
	skip_string(c);

	output.data = allocate(force_stack);
	if(output.data.type == data_register){
		fprintf(output_file, "la $s%d, __str%d\n", output.data.reg, current_string);
	} else if(output.data.type == data_stack){
		fprintf(output_file, "la $t0, __str%d\n", current_string);
		fprintf(output_file, "sw $t0, %d($sp)\n", -(int) output.data.stack_pos);
	}
	current_string++;
	output.data_type = CHAR_TYPE;
	add_type_entry(&(output.data_type), type_pointer);
	output.is_reference = 0;

	return output;
}

value compile_character(char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	value output;
	int char_constant = 0;
	char escape_sequences[] = {'a', 'b', 'e', 'f', 'n', 'r', 't', 'v', '\\', '\'', '\"', '\?'};
	int escape_values[] = {0x07, 0x08, 0x1B, 0x0C, 0x0A, 0x0D, 0x09, 0x0B, 0x5C, 0x27, 0x22, 0x3F};
	unsigned char i;

	++*c;

	if(**c == '\\'){
		++*c;
		for(i = 0; i < sizeof(escape_sequences)/sizeof(char); i++){
			if(**c == escape_sequences[i]){
				char_constant = escape_values[i];
			}
		}
		if(!char_constant){
			snprintf(error_message, sizeof(error_message), "Unrecognized escape sequence\n");
			do_error(1);
		}
	} else {
		char_constant = **c;
	}
	++*c;
	if(**c != '\''){
		snprintf(error_message, sizeof(error_message), "Expected closing '\n");
		do_error(1);
	}
	++*c;

	output.data = allocate(force_stack);
	if(output.data.type == data_register){
		fprintf(output_file, "li $s%d, %d\n", output.data.reg, char_constant);
	} else if(output.data.type == data_stack){
		fprintf(output_file, "li $t0, %d\n", char_constant);
		fprintf(output_file, "sw $t0, %d($sp)\n", -(int) output.data.stack_pos);
	}
	output.data_type = INT_TYPE;
	output.is_reference = 0;

	return output;
}

value compile_logical_not(value v, FILE *output_file){
	if(v.data.type == data_register){
		fprintf(output_file, "seq $s%d, $s%d, $zero\n", v.data.reg, v.data.reg);
	} else if(v.data.type == data_stack){
		fprintf(output_file, "lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		fprintf(output_file, "seq $t0, $t0, $zero\n");
		fprintf(output_file, "sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
	}

	v.data_type = INT_TYPE;
	return v;
}

value compile_not(value v, FILE *output_file){
	if(!types_equal(v.data_type, INT_TYPE) && !types_equal(v.data_type, CHAR_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't perform bitwise not of non-numerical type");
		do_error(1);
	}
	if(v.data.type == data_register){
		fprintf(output_file, "seq $s%d, $s%d, $s%d\n", v.data.reg, v.data.reg, v.data.reg);
	} else if(v.data.type == data_stack){
		fprintf(output_file, "lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		fprintf(output_file, "nor $t0, $t0, $t0\n");
		fprintf(output_file, "sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
	}

	v.data_type = INT_TYPE;
	return v;
}

value compile_negate(value v, FILE *output_file){
	if(!types_equal(v.data_type, INT_TYPE) && !types_equal(v.data_type, CHAR_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't negate non-numerical type");
		do_error(1);
	}
	if(v.data.type == data_register){
		fprintf(output_file, "sub $s%d, $zero, $s%d\n", v.data.reg, v.data.reg);
	} else if(v.data.type == data_stack){
		fprintf(output_file, "lw $t0, %d($sp)\n", -(int) v.data.stack_pos);
		fprintf(output_file, "sub $t0, $zero, $t0\n");
		fprintf(output_file, "sw $t0, %d($sp)\n", -(int) v.data.stack_pos);
	}

	v.data_type = INT_TYPE;
	return v;
}

value compile_value(char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	value output;
	char *temp_c;
	type cast_type;

	skip_whitespace(c);

	if(**c == '*'){
		++*c;
		output = compile_value(c, 1, force_stack, output_file);
		if(dereference){
			output = compile_dereference(output, output_file);
		}
		output.is_reference = !dereference;
		return output;
	} else if(**c == '&'){
		++*c;
		if(dereference){
			output = compile_value(c, 0, force_stack, output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output.is_reference = 0;
		return output;
	} else if(**c == '!'){
		++*c;
		if(dereference){
			output = compile_logical_not(compile_value(c, 1, force_stack, output_file), output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output.is_reference = 0;
		return output;
	} else if(**c == '~'){
		++*c;
		if(dereference){
			output = compile_not(compile_value(c, 1, force_stack, output_file), output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output.is_reference = 0;
		return output;
	} else if(**c == '-' && !digit((*c)[1])){
		++*c;
		if(dereference){
			output = compile_negate(compile_value(c, 1, force_stack, output_file), output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output.is_reference = 0;
		return output;
	}

	if(**c == '('){
		++*c;
		skip_whitespace(c);
		temp_c = *c;
		//Type casting
		if(parse_datatype(NULL, &temp_c)){
			cast_type = EMPTY_TYPE;
			parse_type(&cast_type, c, NULL, NULL, 0, 0);
			if(**c != ')'){
				snprintf(error_message, sizeof(error_message), "Expected ')'");
				do_error(1);
			}
			++*c;
			return cast(compile_value(c, 1, force_stack, output_file), cast_type, 0, output_file);
		//Associative parentheses
		} else {
			output = compile_expression(c, dereference, force_stack, output_file);
			if(**c == ')'){
				++*c;
			} else {
				snprintf(error_message, sizeof(error_message), "Expected closing ')'");
				do_error(1);
			}
		}
	} else if(**c == '-' || digit(**c)){
		output = compile_integer(c, dereference, force_stack, output_file);
	} else if(alpha(**c)){
		output = compile_variable(c, dereference, force_stack, output_file);
	} else if(**c == '\"'){
		output = compile_string(c, dereference, force_stack, output_file);
	} else if(**c == '\''){
		output = compile_character(c, dereference, force_stack, output_file);
	} else {
		snprintf(error_message, sizeof(error_message), "Unrecognized expression value");
		do_error(1);
	}

	skip_whitespace(c);
	while(**c == '[' || **c == '('){
		if(**c == '['){
			if(output.is_reference){
				output = compile_dereference(output, output_file);
			}
			output = compile_list_index(c, output, dereference, output_file);
		} else if(**c == '('){
			++*c;
			if(output.is_reference){
				output = compile_dereference(output, output_file);
			}
			output = compile_function_call(c, output, output_file);
		}

		skip_whitespace(c);
	}

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
		case '%':
			return operation_modulo;
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
			if(c[1] == '>'){
				return operation_shift_right;
			} else {
				return operation_greater_than;
			}
		case '<':
			if(c[1] == '<'){
				return operation_shift_left;
			} else {
				return operation_less_than;
			}
		case '&':
			if(c[1] == '&'){
				return operation_logical_and;
			} else {
				return operation_and;
			}
		case '|':
			if(c[1] == '|'){
				return operation_logical_or;
			} else {
				return operation_or;
			}
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
		case '%':
			output = operation_modulo;
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
			if((*c)[1] == '>'){
				output = operation_shift_right;
				++*c;
			} else {
				output = operation_greater_than;
			}
			break;
		case '<':
			if((*c)[1] == '<'){
				output = operation_shift_left;
				++*c;
			} else {
				output = operation_less_than;
			}
			break;
		case '&':
			if((*c)[1] == '&'){
				output = operation_logical_and;
				++*c;
			} else {
				output = operation_and;
			}
			break;
		case '|':
			if((*c)[1] == '|'){
				output = operation_logical_or;
				++*c;
			} else {
				output = operation_or;
			}
			break;
		default:
			output = operation_none;
			break;
	}
	++*c;
	return output;
}

value compile_operation(value first_value, value next_value, operation op, FILE *output_file){
	char reg0_str_buffer[5] = {0, 0, 0, 0, 0};
	char reg1_str_buffer[5] = {0, 0, 0, 0, 0};
	char *reg0_str = "";
	char *reg1_str = "";
	type output_type;

	if(types_equal(first_value.data_type, VOID_TYPE) || types_equal(next_value.data_type, VOID_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't operate on void value");
		do_error(1);
	}

	if(first_value.data.type == data_register){
		snprintf(reg0_str_buffer, sizeof(reg0_str_buffer)/sizeof(char), "$s%d", first_value.data.reg);
		reg0_str = reg0_str_buffer;
	} else if(first_value.data.type == data_stack){
		reg0_str = "$t0";
		if(type_size(first_value.data_type) == 4){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else if(type_size(first_value.data_type) == 1){
			fprintf(output_file, "lb $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else {
			snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size %d", type_size(first_value.data_type));
			do_error(1);
		}
	}
	if(next_value.data.type == data_register){
		snprintf(reg1_str_buffer, sizeof(reg1_str_buffer)/sizeof(char), "$s%d", next_value.data.reg);
		reg1_str = reg1_str_buffer;
	} else if(next_value.data.type == data_stack){
		reg1_str = "$t1";
		if(type_size(next_value.data_type) == 4){
			fprintf(output_file, "lw $t1, %d($sp)\n", -(int) next_value.data.stack_pos);
		} else if(type_size(next_value.data_type) == 1){
			fprintf(output_file, "lb $t1, %d($sp)\n", -(int) next_value.data.stack_pos);
		} else {
			snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size %d", type_size(next_value.data_type));
			do_error(1);
		}
	}
	output_type = operation_functions[op](reg0_str, reg1_str, first_value, next_value, output_file);
	deallocate(next_value.data);
	if(first_value.data.type == data_stack){
		if(type_size(output_type) == 4){
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else if(type_size(output_type) == 1){
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
		} else {
			snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size %d", type_size(output_type));
			do_error(1);
		}
	}
	first_value.data_type = output_type;
	first_value.is_reference = 0;
	return first_value;
}

static value compile_expression_recursive(value first_value, char **c, FILE *output_file){
	operation current_operation;
	operation next_operation;
	unsigned int label_num;
	value next_value;
	char *temp_c;
	type cast_to;
	int current_line_temp;

	skip_whitespace(c);
	current_operation = get_operation(c);
	if(current_operation == operation_none){
		snprintf(error_message, sizeof(error_message), "Unrecognized operation");
		do_error(1);
	} else if(current_operation == operation_logical_or){
		label_num = num_labels;
		num_labels++;
		first_value = cast(first_value, INT_TYPE, 1, output_file);
		if(first_value.data.type == data_register){
			fprintf(output_file, "sne $s%d, $s%d, $zero\n", first_value.data.reg, first_value.data.reg);
			fprintf(output_file, "bne $s%d, $zero, __L%d\n", first_value.data.reg, label_num);
		} else if(first_value.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
			fprintf(output_file, "sne $t0, $t0, $zero\n");
			fprintf(output_file, "sw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
			fprintf(output_file, "bne $t0, $zero, __L%d\n", label_num);
		}
	} else if(current_operation == operation_logical_and){
		label_num = num_labels;
		num_labels++;
		first_value = cast(first_value, INT_TYPE, 1, output_file);
		if(first_value.data.type == data_register){
			fprintf(output_file, "beq $s%d, $zero, __L%d\n", first_value.data.reg, label_num);
		} else if(first_value.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
			fprintf(output_file, "beq $t0, $zero, __L%d\n", label_num);
		}
	}

	temp_c = *c;
	current_line_temp = current_line;
	skip_value(&temp_c);
	current_line = current_line_temp;
	next_operation = peek_operation(temp_c);
	if(next_operation == operation_assign && order_of_operations[operation_assign] > order_of_operations[current_operation]){
		next_value = compile_value(c, 0, 0, output_file);
	} else {
		next_value = compile_value(c, 1, 0, output_file);
	}
	skip_whitespace(c);
	while(order_of_operations[next_operation] > order_of_operations[current_operation]){
		next_value = compile_expression_recursive(next_value, c, output_file);
		skip_whitespace(c);
		next_operation = peek_operation(*c);
	}
	if(current_operation == operation_assign){
		cast_to = first_value.data_type;
		pop_type(&cast_to);
		next_value = cast(next_value, cast_to, 1, output_file);
		first_value = compile_operation(first_value, next_value, current_operation, output_file);
	} else if(current_operation == operation_logical_or || current_operation == operation_logical_and){
		next_value = cast(next_value, INT_TYPE, 1, output_file);
		if(next_value.data.type == data_register){
			fprintf(output_file, "sne $s%d, $s%d, $zero\n", next_value.data.reg, next_value.data.reg);
			if(first_value.data.type == data_register){
				fprintf(output_file, "move $s%d, $s%d\n", first_value.data.reg, next_value.data.reg);
			} else if(first_value.data.type == data_stack){
				fprintf(output_file, "sw $s%d, %d($sp)\n", next_value.data.reg, -(int) first_value.data.stack_pos);
			}
		} else if(next_value.data.type == data_stack){
			if(first_value.data.type == data_register){
				fprintf(output_file, "lw $s%d, %d($sp)\n", first_value.data.reg, -(int) next_value.data.stack_pos);
				fprintf(output_file, "sne $s%d, $s%d, $zero\n", first_value.data.reg, first_value.data.reg);
			} else if(first_value.data.type == data_stack){
				fprintf(output_file, "lw $t0, %d($sp)\n", -(int) next_value.data.stack_pos);
				fprintf(output_file, "sne $t0, $t0, $zero\n");
				fprintf(output_file, "sw $t0, %d($sp)\n", -(int) first_value.data.stack_pos);
			}
		}
		deallocate(next_value.data);
		fprintf(output_file, "\n__L%d:\n", label_num);
	} else {
		first_value = compile_operation(first_value, next_value, current_operation, output_file);
	}
	skip_whitespace(c);

	return first_value;
}

value compile_expression(char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	value first_value;
	char *temp_c;
	int current_line_temp;
	operation next_operation;

	temp_c = *c;
	current_line_temp = current_line;
	skip_value(&temp_c);
	current_line = current_line_temp;

	next_operation = peek_operation(temp_c);
	if(peek_operation(temp_c) == operation_assign){
		first_value = compile_value(c, 0, force_stack, output_file);
	} else {
		first_value = compile_value(c, dereference || next_operation != operation_none, force_stack, output_file);
	}
	while(**c && **c != ';' && **c != ',' && **c != ')' && **c != ']'){
		first_value = compile_expression_recursive(first_value, c, output_file);
	}

	return first_value;
}

