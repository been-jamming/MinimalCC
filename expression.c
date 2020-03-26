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
static unsigned int order_of_operations[] = {0, 9, 9, 10, 10, 1, 7, 7, 7, 7, 6, 6, 5, 4, 9, 3, 2, 8, 8};

void operation_none_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	snprintf(error_message, sizeof(error_message), "Unrecognized operation");
	do_error(1);
}

void operation_add_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	value *pointer_value;
	char *integer_reg;
	unsigned int pointer_size;

	if(peek_type(&(value_a->data_type)) == type_function || peek_type(&(value_b->data_type)) == type_function){
		snprintf(error_message, sizeof(error_message), "Addition of function type is undefined");
		do_error(1);
	}
	if(peek_type(&(value_a->data_type)) == type_pointer){
		if(peek_type(&(value_b->data_type)) == type_pointer){
			snprintf(warning_message, sizeof(warning_message), "Adding two pointers together. Treating them as integers instead");
			do_warning();
			fprintf(output_file, "\tadd.l %s, %s\n", reg_b, reg_a);
			*output_type = value_a->data_type;
			return;
		}
		pointer_value = value_a;
		integer_reg = reg_b;
	} else {
		if(peek_type(&(value_b->data_type)) == type_pointer){
			pointer_value = value_b;
			integer_reg = reg_a;
		} else {
			fprintf(output_file, "\tadd.w %s, %s\n", reg_b, reg_a);
			*output_type = INT_TYPE;
			return;
		}
	}
	pointer_size = type_size(&(pointer_value->data_type), 1);
	fprintf(output_file, "\text.l %s\n", integer_reg);
	if(pointer_size == 4){
		fprintf(output_file, "\tlsl.l #2, %s\n", integer_reg);
	} else if(pointer_size == 2){
		fprintf(output_file, "\tlsl.l #1, %s\n", integer_reg);
	} else if(pointer_size != 1){
		fprintf(output_file, "\tmuls.w #%d, %s\n", pointer_size, integer_reg);
	}
	fprintf(output_file, "\tadd.l %s, %s\n", reg_b, reg_a);

	*output_type = pointer_value->data_type;
}

void operation_subtract_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	type *pointer_type;
	type *pointer_type2;
	value *pointer_value;
	char *integer_reg;
	unsigned int pointer_size;

	if(peek_type(&(value_a->data_type)) == type_function || peek_type(&(value_b->data_type)) == type_function){
		snprintf(error_message, sizeof(error_message), "Subtraction of function type is undefined");
		do_error(1);
	}
	if(peek_type(&(value_a->data_type)) == type_pointer){
		if(peek_type(&(value_b->data_type)) == type_pointer){
			pointer_type = &(value_a->data_type);
			pointer_type2 = &(value_b->data_type);
			if(!types_equal(pointer_type, pointer_type2)){
				snprintf(error_message, sizeof(error_message), "Cannot subtract pointers to different types");
				do_error(1);
			}
			fprintf(output_file, "\tsub.l %s, %s\n", reg_b, reg_a);
			if(type_size(pointer_type, 1) == 4){
				fprintf(output_file, "\tasr.l #2, %s\n", reg_a);
			} else if(type_size(pointer_type, 1) == 2){
				fprintf(output_file, "\tasr.l #1, %s\n", reg_a);
			} else if(type_size(pointer_type, 1) != 1){
				fprintf(output_file, "\tdivs.w #%d, %s\n", type_size(pointer_type, 1), reg_a);
			}
			*output_type = INT_TYPE;
			return;
		}
		pointer_value = value_a;
		integer_reg = reg_b;
	} else {
		if(peek_type(&(value_b->data_type)) == type_pointer){
			pointer_value = value_b;
			integer_reg = reg_a;
		} else {
			fprintf(output_file, "\tsub.w %s, %s\n", reg_b, reg_a);
			*output_type = INT_TYPE;
			return;
		}
	}
	pointer_size = type_size(&(pointer_value->data_type), 1);
	fprintf(output_file, "\text.l %s\n", integer_reg);
	if(pointer_size == 4){
		fprintf(output_file, "\tlsl.l #2, %s\n", integer_reg);
	} else if(pointer_size == 2){
		fprintf(output_file, "\tlsl.l #1, %s\n", integer_reg);
	} else if(pointer_size != 1){
		fprintf(output_file, "\tmuls.w #%d, %s\n", pointer_size, integer_reg);
	}
	fprintf(output_file, "\tsub.l %s, %s\n", reg_b, reg_a);

	*output_type = pointer_value->data_type;
}

void operation_multiply_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot multiply non-int types");
		do_error(1);
	}
	fprintf(output_file, "\tmuls.w %s, %s\n", reg_b, reg_a);
	*output_type = INT_TYPE;
}

void operation_divide_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot divide non-int types");
		do_error(1);
	}
	fprintf(output_file, "\tdivs.w %s, %s\n", reg_b, reg_a);
	*output_type = INT_TYPE;
}

void operation_modulo_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot modulo non-int types");
		do_error(1);
	}
	fprintf(output_file, "\tdivs.w %s, %s\n", reg_b, reg_a);
	fprintf(output_file, "\tswap %s\n", reg_a);
	*output_type = INT_TYPE;
}

void operation_assign_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if(!value_a->is_reference){
		snprintf(error_message, sizeof(error_message), "Cannot assign to r-value");
		do_error(1);
	}
	pop_type(&(value_a->data_type));
	if(peek_type(&(value_a->data_type)) == type_function){
		snprintf(error_message, sizeof(error_message), "Cannot assign to function");
		do_error(1);
	}
	fprintf(output_file, "\tmove.l %s, a0\n", reg_a);
	if(type_size(&(value_a->data_type), 0) == 4){
		fprintf(output_file, "\tmove.l %s, (a0)\n", reg_b);
	} else if(type_size(&(value_a->data_type), 0) == 2){
		fprintf(output_file, "\tmove.w %s, (a0)\n", reg_b);
	} else if(type_size(&(value_a->data_type), 0) == 1){
		fprintf(output_file, "\tmove.b %s, (a0)\n", reg_b);
	}
	fprintf(output_file, "\tmove.l %s, %s\n", reg_b, reg_a);
	*output_type = value_b->data_type;
}

void operation_less_than_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot compare non-int types with '<'");
		do_error(1);
	}
	fprintf(output_file, "\tcmp.w %s, %s\n", reg_b, reg_a);
	fprintf(output_file, "\tsmi.b %s\n", reg_a);
	fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
	*output_type = INT_TYPE;
}

void operation_greater_than_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot compare non-int types with '>'");
		do_error(1);
	}
	fprintf(output_file, "\tcmp.w %s, %s\n", reg_a, reg_b);
	fprintf(output_file, "\tsmi.b %s\n", reg_a);
	fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
	*output_type = INT_TYPE;
}

void operation_less_than_or_equal_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot compare non-int types with '<='");
		do_error(1);
	}
	fprintf(output_file, "\tcmp.w %s, %s\n", reg_a, reg_b);
	fprintf(output_file, "\tspl.b %s\n", reg_a);
	fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
	*output_type = INT_TYPE;
}

void operation_greater_than_or_equal_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot compare non-int types with '>='");
		do_error(1);
	}
	fprintf(output_file, "\tcmp.w %s, %s\n", reg_b, reg_a);
	fprintf(output_file, "\tspl.b %s\n", reg_a);
	fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
	*output_type = INT_TYPE;
}

void operation_equals_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if(peek_type(&(value_a->data_type)) == type_function || peek_type(&(value_b->data_type)) == type_function){
		snprintf(error_message, sizeof(error_message), "Cannot compare function types");
		do_error(1);
	}
	if(peek_type(&(value_a->data_type)) == type_pointer){
		if(peek_type(&(value_b->data_type)) == type_pointer){
			pop_type(&(value_a->data_type));
			pop_type(&(value_b->data_type));
			if(!types_equal(&(value_a->data_type), &(value_b->data_type))){
				snprintf(warning_message, sizeof(warning_message), "Comparing incompatible data types");
				do_warning();
			}
			fprintf(output_file, "\tcmp.l %s, %s\n", reg_a, reg_b);
			fprintf(output_file, "\tseq.b %s\n", reg_a);
			fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
			*output_type = INT_TYPE;
		} else {
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
			fprintf(output_file, "\tcmp.w %s, %s\n", reg_a, reg_b);
			fprintf(output_file, "\tseq.b %s\n", reg_a);
			fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
			*output_type = INT_TYPE;
		}
	} else {
		if(peek_type(&(value_b->data_type)) == type_pointer){
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
		}
		fprintf(output_file, "\tcmp.w %s, %s\n", reg_a, reg_b);
		fprintf(output_file, "\tseq.b %s\n", reg_a);
		fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
		*output_type = INT_TYPE;
	}
}

void operation_not_equals_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if(peek_type(&(value_a->data_type)) == type_function || peek_type(&(value_b->data_type)) == type_function){
		snprintf(error_message, sizeof(error_message), "Cannot compare function types");
		do_error(1);
	}
	if(peek_type(&(value_a->data_type)) == type_pointer){
		if(peek_type(&(value_b->data_type)) == type_pointer){
			pop_type(&(value_a->data_type));
			pop_type(&(value_b->data_type));
			if(!types_equal(&(value_a->data_type), &(value_b->data_type))){
				snprintf(warning_message, sizeof(warning_message), "Comparing incompatible data types");
				do_warning();
			}
			fprintf(output_file, "\tcmp.l %s, %s\n", reg_a, reg_b);
			fprintf(output_file, "\tsne.b %s\n", reg_a);
			fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
			*output_type = INT_TYPE;
		} else {
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
			fprintf(output_file, "\tcmp.w %s, %s\n", reg_a, reg_b);
			fprintf(output_file, "sne.b %s\n", reg_a);
			fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
			*output_type = INT_TYPE;
		}
	} else {
		if(peek_type(&(value_b->data_type)) == type_pointer){
			snprintf(warning_message, sizeof(warning_message), "Comparing pointer and non-pointer types");
			do_warning();
		}
		fprintf(output_file, "\tcmp.w %s, %s\n", reg_a, reg_b);
		fprintf(output_file, "\tsne.b %s\n", reg_a);
		fprintf(output_file, "\tandi.l #1, %s\n", reg_a);
		*output_type = INT_TYPE;
	}
}

void operation_and_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '&' non-int types");
		do_error(1);
	}
	fprintf(output_file, "\tand.w %s, %s\n", reg_b, reg_a);
	*output_type = INT_TYPE;
}

void operation_or_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '|' non-int types");
		do_error(1);
	}
	fprintf(output_file, "\tor.w %s, %s\n", reg_b, reg_a);
	*output_type = INT_TYPE;
}

void operation_shift_left_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '<<' non-arithmetic types");
		do_error(1);
	}
	fprintf(output_file, "\tlsl.w %s, %s\n", reg_b, reg_a);
	*output_type = INT_TYPE;
}

void operation_shift_right_func(char *reg_a, char *reg_b, value *value_a, value *value_b, FILE *output_file, type *output_type){
	if((!types_equal(&(value_a->data_type), &INT_TYPE) && !types_equal(&(value_a->data_type), &CHAR_TYPE)) || (!types_equal(&(value_b->data_type), &INT_TYPE) && !types_equal(&(value_b->data_type), &CHAR_TYPE))){
		snprintf(error_message, sizeof(error_message), "Cannot '>>' non-arithmetic types");
		do_error(1);
	}
	fprintf(output_file, "\tsra.w %s, %s\n", reg_b, reg_a);
	*output_type = INT_TYPE;
}

void (*operation_functions[])(char *, char *, value *, value *, FILE *, type *) = {operation_none_func, operation_add_func, operation_subtract_func, operation_multiply_func, operation_divide_func, operation_assign_func, operation_less_than_func, operation_greater_than_func, operation_less_than_or_equal_func, operation_greater_than_or_equal_func, operation_equals_func, operation_not_equals_func, operation_and_func, operation_or_func, operation_modulo_func, operation_none_func, operation_none_func, operation_shift_left_func, operation_shift_right_func};

int type_size(type *t, unsigned int start_entry){
	uint64_t d0;
	uint64_t d1;
	uint64_t d2;
	unsigned int index;
	unsigned int current_entry = 0;
	int entry;
	int output = 1;

	d0 = t->d0;
	d1 = t->d1;
	d2 = t->d2;
	index = t->current_index;
	while(1){
		entry = 0;
		if(d0&1){
			entry |= 1;
		}
		if(d1&1){
			entry |= 2;
		}
		if(d2&1){
			entry |= 4;
		}
		if(current_entry >= start_entry){
			switch(entry){
				case type_void:
					return output*VOID_SIZE;
				case type_int:
					return output*INT_SIZE;
				case type_char:
					return output*CHAR_SIZE;
				case type_pointer:
					return output*POINTER_SIZE;
				case type_function:
					return output*POINTER_SIZE;
				case type_list:
					index--;
					output *= t->list_indicies[index];
					break;
				default:
					return 0;
			}
		} else if(entry == type_list){
			index--;
		}
		d0 >>= 1;
		d1 >>= 1;
		d2 >>= 1;
		current_entry++;
	}
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
	variables_size += align4(type_size(&(var->var_type), 0));
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

void compile_integer(value *output, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	int int_value;

	output->data = allocate(force_stack);
	int_value = strtol(*c, c, 0);
	if(output->data.type == data_register){
		fprintf(output_file, "\tmove.l #%d, d%d\n", int_value, (int) output->data.reg);
	} else if(output->data.type == data_stack){
		fprintf(output_file, "\tmove.l #%d, %d(sp)\n", int_value, -(int) output->data.stack_pos);
	}
	output->data_type = INT_TYPE;
	output->is_reference = 0;
}

static void compile_local_variable(value *output, variable *var, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	data_entry data;
	type data_type;

	data_type = var->var_type;
	data = allocate(force_stack);
	output->data_type = data_type;
	output->data = data;
	if(dereference){
		if(peek_type(&(var->var_type)) == type_list){
			if(data.type == data_register){
				fprintf(output_file, "\tmove.l sp, d%d\n", data.reg);
				fprintf(output_file, "\taddi.l #%d, d%d\n", variables_size - var->stack_pos, data.reg);
			} else if(data.type == data_stack){
				fprintf(output_file, "\tmove.l sp, %d(sp)\n", -(int) data.stack_pos);
				fprintf(output_file, "\taddi.l #%d, %d(sp)\n", variables_size - var->stack_pos, -(int) data.stack_pos);
			}
			pop_type(&(output->data_type));
			output->data_type.current_index--;
			add_type_entry(&(output->data_type), type_pointer);
		} else {
			if(data.type == data_register){
				switch(type_size(&data_type, 0)){
					case 1:
						fprintf(output_file, "\tmove.b %d(sp), d%d\n", variables_size - var->stack_pos, data.reg);
						fprintf(output_file, "\text.w d%d\n", data.reg);
						fprintf(output_file, "\text.l d%d\n", data.reg);
						break;
					case 2:
						fprintf(output_file, "\tmove.w %d(sp), d%d\n", variables_size - var->stack_pos, data.reg);
						fprintf(output_file, "\text.l d%d\n", data.reg);
						break;
					case 4:
						fprintf(output_file, "\tmove.l %d(sp), d%d\n", variables_size - var->stack_pos, data.reg);
						break;

				}
			} else if(data.type == data_stack){
				switch(type_size(&data_type, 0)){
					case 1:
						fprintf(output_file, "\tmove.b %d(sp), %d(sp)\n", variables_size - var->stack_pos, -(int) data.stack_pos);
						fprintf(output_file, "\text.w %d(sp)\n", -(int) data.stack_pos);
						fprintf(output_file, "\text.l %d(sp)\n", -(int) data.stack_pos);
						break;
					case 2:
						fprintf(output_file, "\tmove.w %d(sp), %d(sp)\n", variables_size - var->stack_pos, -(int) data.stack_pos);
						fprintf(output_file, "\text.l %d(sp)\n", -(int) data.stack_pos);
						break;
					case 4:
						fprintf(output_file, "\tmove.l %d(sp), %d(sp)\n", variables_size - var->stack_pos, -(int) data.stack_pos);
						break;
				}
			}
		}
	} else {
		if(data.type == data_register){
			fprintf(output_file, "\tmove.l sp, d%d\n", data.reg);
			fprintf(output_file, "\taddi.l #%d, d%d\n", variables_size - var->stack_pos, data.reg);
		} else if(data.type == data_stack){
			fprintf(output_file, "\tmove.l sp, %d(sp)\n", -(int) data.stack_pos);
			fprintf(output_file, "\taddi.l #%d, %d(sp)\n", variables_size - var->stack_pos, -(int) data.stack_pos);
		}
		add_type_entry(&(output->data_type), type_pointer);
	}

	output->is_reference = !dereference;
}

static void compile_global_variable(value *output, variable *var, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	data_entry data;
	type data_type;

	data_type = var->var_type;
	data = allocate(force_stack);
	output->data = data;
	output->is_reference = !dereference;
	if(dereference && !var->leave_as_address){
		fprintf(output_file, "\tlea %s, a0\n", var->varname);
		if(peek_type(&data_type) == type_list){
			pop_type(&data_type);
			data_type.current_index--;
			add_type_entry(&data_type, type_pointer);
			if(data.type == data_register){
				fprintf(output_file, "\tmove.l (a0), d%d\n", data.reg);
			} else if(data.type == data_stack){
				fprintf(output_file, "\tmove.l a0, %d(sp)\n", -(int) data.stack_pos);
			}
		} else {
			if(data.type == data_register){
				fprintf(output_file, "\tmove.l (a0), d%d\n", data.reg);
			} else if(data.type == data_stack){
				fprintf(output_file, "\tmove.l (a0), %d(sp)\n", -(int) data.stack_pos);
			}
		}
	} else {
		add_type_entry(&data_type, type_pointer);
		if(data.type == data_register){
			fprintf(output_file, "\tlea %s, a0\n", var->varname);
			fprintf(output_file, "\tmove.l a0, d%d\n", data.reg);
		} else if(data.type == data_stack){
			fprintf(output_file, "\tlea %s, a0\n", var->varname);
			fprintf(output_file, "\tmove.l a0, %d(sp)\n", -(int) data.stack_pos);
		}
	}
	output->data_type = data_type;
}

void compile_variable(value *output, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
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
		compile_global_variable(output, var, dereference, force_stack, output_file);
	} else {
		compile_local_variable(output, var, dereference, force_stack, output_file);
	}
}

void compile_expression(value *output, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file);

void cast(value *v, type t, unsigned char do_warn, FILE *output_file){
	type t_copy;
	type_entry entry1;
	type_entry entry2;

	if(types_equal(&(v->data_type), &VOID_TYPE) && !types_equal(&t, &VOID_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't cast void type to non-void type");
		do_error(1);
	}

	if(peek_type(&t) == type_list){
		snprintf(error_message, sizeof(error_message), "Can't cast to an array");
		do_error(1);
	}

	if(type_size(&(v->data_type), 0) != type_size(&t, 0)){
		if(type_size(&(v->data_type), 0) == 1){
			if(v->data.type == data_register){
				fprintf(output_file, "\text.w d%d\n", v->data.reg);
				fprintf(output_file, "\text.l d%d\n", v->data.reg);
			} else if(v->data.type == data_stack){
				fprintf(output_file, "\tmove.b %d(sp), d7\n", -(int) v->data.stack_pos);
				fprintf(output_file, "\text.w d7\n");
				fprintf(output_file, "\text.l d7\n");
				fprintf(output_file, "\tmove.l d7, %d(sp)\n", -(int) v->data.stack_pos);
			}
		} else if(type_size(&(v->data_type), 0) == 2){
			if(v->data.type == data_register){
				fprintf(output_file, "\text.l d%d\n", v->data.reg);
			} else if(v->data.type == data_stack){
				fprintf(output_file, "\tmove.w %d(sp), d7\n", -(int) v->data.stack_pos);
				fprintf(output_file, "\text.l d7\n");
				fprintf(output_file, "\tmove.l d7, %d(sp)\n", -(int) v->data.stack_pos);
			}
		}
	}

	t_copy = t;
	entry1 = pop_type(&t_copy);
	entry2 = pop_type(&(v->data_type));
	if((entry1 == type_pointer) != (entry2 == type_pointer) && do_warn){
		snprintf(warning_message, sizeof(warning_message), "Casting between pointer and non-pointer types");
		do_warning();
	} else if(entry1 == type_pointer && !types_equal(&t_copy, &(v->data_type)) && !types_equal(&t_copy, &VOID_TYPE) && !types_equal(&(v->data_type), &VOID_TYPE) && do_warn){
		snprintf(warning_message, sizeof(warning_message), "Casting between incompatible pointer types");
		do_warning();
	}

	v->data_type = t;
}

void compile_dereference(value *v, FILE *output_file){
	type data_type;

	data_type = v->data_type;
	if(pop_type(&data_type) != type_pointer){
		snprintf(error_message, sizeof(error_message), "Cannot dereference non-pointer type");
		do_error(1);
	}
	if(peek_type(&data_type) == type_void){
		snprintf(error_message, sizeof(error_message), "Cannot dereference void pointer");
		do_error(1);
	}
	if(peek_type(&data_type) == type_list){
		pop_type(&data_type);
		data_type.current_index--;
		add_type_entry(&data_type, type_pointer);
	} else if(peek_type(&data_type) != type_function){
		if(v->data.type == data_register){
			if(type_size(&data_type, 0) == 1){
				fprintf(output_file, "\tmove.l d%d, a0\n", v->data.reg);
				fprintf(output_file, "\tmove.b (a0), d%d\n", v->data.reg);
				fprintf(output_file, "\text.w d%d\n", v->data.reg);
				data_type = INT_TYPE;
			} else if(type_size(&data_type, 0) == 2){
				fprintf(output_file, "\tmove.l d%d, a0\n", v->data.reg);
				fprintf(output_file, "\tmove.w (a0), d%d\n", v->data.reg);
			} else if(type_size(&data_type, 0) == 4){
				fprintf(output_file, "\tmove.l d%d, a0\n", v->data.reg);
				fprintf(output_file, "\tmove.l (a0), d%d\n", v->data.reg);
			}
		} else if(v->data.type == data_stack){
			fprintf(output_file, "\tmove.l %d(sp), a0\n", -(int) v->data.stack_pos);
			if(type_size(&data_type, 0) == 1){
				fprintf(output_file, "\tmove.b (a0), %d(sp)\n", -(int) v->data.stack_pos);
			} else if(type_size(&data_type, 0) == 2){
				fprintf(output_file, "\tmove.w (a0), %d(sp)\n", -(int) v->data.stack_pos);
			} else if(type_size(&data_type, 0) == 4){
				fprintf(output_file, "\tmove.l (a0), %d(sp)\n", -(int) v->data.stack_pos);
			}
		}
	}

	v->data_type = data_type;
	v->is_reference = 0;
}

void compile_function_call(char **c, value *func, FILE *output_file){
	reg_list reg_state;
	data_entry return_data;
	data_entry return_address_data;
	type current_argument_type;
	value current_argument_value;
	unsigned int label_num;
	unsigned int func_stack_pos = 0;
	unsigned int stack_size_before;

	if(peek_type(&(func->data_type)) == type_pointer){
		pop_type(&(func->data_type));
	}
	if(pop_type(&(func->data_type)) != type_function){
		snprintf(error_message, sizeof(error_message), "Can't call non-function type");
		do_error(1);
	}

	label_num = num_labels;
	num_labels++;
	reg_state = push_registers(output_file);
	stack_size_before = stack_size;

	if(func->data.type == data_register){
		func_stack_pos = get_reg_stack_pos(reg_state, func->data.reg);
	}
	return_address_data = allocate(1);
	return_data = allocate(1);
	fprintf(output_file, "\tlea __L%d, a0\n", label_num);
	fprintf(output_file, "\tmove.l a0, %d(sp)\n", -(int) return_address_data.stack_pos);
	if(peek_type(&(func->data_type)) == type_returns){
		skip_whitespace(c);
		if(**c != ')'){
			snprintf(error_message, sizeof(error_message), "Expected ')'");
			do_error(1);
		}
		++*c;
	}
	while(peek_type(&(func->data_type)) != type_returns){
		current_argument_type = get_argument_type(&(func->data_type));
		compile_expression(&current_argument_value, c, 1, 1, output_file);
		if(**c != ',' && peek_type(&(func->data_type)) != type_returns){
			snprintf(error_message, sizeof(error_message), "Expected ','");
			do_error(1);
		} else if(**c != ')' && peek_type(&(func->data_type)) == type_returns){
			snprintf(error_message, sizeof(error_message), "Expected ')'");
			do_error(1);
		}
		++*c;
		cast(&current_argument_value, current_argument_type, 1, output_file);
	}

	pop_type(&(func->data_type));
	if(func->data.type == data_register){
		fprintf(output_file, "\tmove.l %d(sp), a0\n", -(int) func_stack_pos);
		fprintf(output_file, "\tadda.l #%d, sp\n", -(int) stack_size_before);
		fprintf(output_file, "\tjmp (a0)\n\n");
	} else if(func->data.type == data_stack){
		fprintf(output_file, "\tmove.l %d(sp), a0\n", -(int) func->data.stack_pos);
		fprintf(output_file, "\tadda.l #%d, sp\n", -(int) stack_size_before);
		fprintf(output_file, "\tjmp (a0)\n\n");
	}
	fprintf(output_file, "__L%d\n", label_num);
	fprintf(output_file, "\tadda.l #%d, sp\n", (int) stack_size_before);
	fprintf(output_file, "\tmove.l %d(sp), d7\n", -(int) return_data.stack_pos);
	deallocate(return_data);
	deallocate(return_address_data);
	pull_registers(reg_state, output_file);
	if(func->data.type == data_register){
		fprintf(output_file, "\tmove.l d7, d%d\n", func->data.reg);
	} else if(func->data.type == data_stack){
		fprintf(output_file, "\tmove.l d7, %d(sp)\n", -(int) func->data.stack_pos);
	}
}

void compile_list_index(char **c, value *address, unsigned char dereference, FILE *output_file){
	value index;
	type address_type;

	address_type = address->data_type;
	if(pop_type(&address_type) != type_pointer){
		snprintf(error_message, sizeof(error_message), "Cannot address non-pointer type");
		do_error(1);
	}
	++*c;
	compile_expression(&index, c, 1, 0, output_file);
	if(**c != ']'){
		snprintf(error_message, sizeof(error_message), "Expected closing ']'");
		do_error(1);
	}
	++*c;
	cast(&index, INT_TYPE, 1, output_file);
	if(index.data.type == data_register){
		if(type_size(&address_type, 0) == 4){
			fprintf(output_file, "\tlsl.l #2, d%d\n", index.data.reg);
		} else if(type_size(&address_type, 0) == 2){
			fprintf(output_file, "\tlsl.l #1, d%d\n", index.data.reg);
		} else if(type_size(&address_type, 0) != 1){
			fprintf(output_file, "\tmuls.w #%d, d%d\n", (int) type_size(&address_type, 0), index.data.reg);
		}
		if(address->data.type == data_register){
			fprintf(output_file, "\tadd.l d%d, d%d\n", index.data.reg, address->data.reg);
			if(dereference && peek_type(&address_type) != type_list){
				if(type_size(&address_type, 0) == 4){
					fprintf(output_file, "\tmove.l d%d, a0\n", address->data.reg);
					fprintf(output_file, "\tmove.l (a0), d%d\n", address->data.reg);
				} else if(type_size(&address_type, 0) == 2){
					fprintf(output_file, "\tmove.l d%d, a0\n", address->data.reg);
					fprintf(output_file, "\tmove.w (a0), d%d\n", address->data.reg);
					fprintf(output_file, "\text.l d%d\n", address->data.reg);
				} else if(type_size(&address_type, 0) == 1){
					fprintf(output_file, "\tmove.l d%d, a0\n", address->data.reg);
					fprintf(output_file, "\tmove.b (a0), d%d\n", address->data.reg);
					fprintf(output_file, "\text.w d%d\n", address->data.reg);
				} else {
					snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size");
					do_error(1);
				}
			}
		} else if(address->data.type == data_stack){
			if(dereference && peek_type(&address_type) != type_list){
				fprintf(output_file, "\tmove.l %d(sp), a0\n", -(int) address->data.stack_pos);
				fprintf(output_file, "\tadd.l d%d, a0\n", index.data.reg);
				if(type_size(&address_type, 0) == 4){
					fprintf(output_file, "\tmove.l (a0), d7\n");
				} else if(type_size(&address_type, 0) == 2){
					fprintf(output_file, "\tmove.w (a0), d7\n");
					fprintf(output_file, "\text.l d7\n");
				} else if(type_size(&address_type, 0) == 1){
					fprintf(output_file, "\tmove.b (a0), d7\n");
					fprintf(output_file, "\text.w d7\n");
				} else {
					snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size");
					do_error(1);
				}
			} else {
				fprintf(output_file, "\tmove.l %d(sp), d7\n", -(int) address->data.stack_pos);
				fprintf(output_file, "\tadd.l d%d, d7\n", index.data.reg);
			}
			fprintf(output_file, "\tmove.l d7, %d(sp)\n", -(int) address->data.stack_pos);
		}
	} else if(index.data.type == data_stack){
		fprintf(output_file, "\tmove.w %d(sp), d7\n", -(int) index.data.stack_pos);
		fprintf(output_file, "\text.l d7\n");
		if(type_size(&address_type, 0) == 4){
			fprintf(output_file, "\tlsl.l #2, d7\n");
		} else if(type_size(&address_type, 0) == 2){
			fprintf(output_file, "\tlsl.l #1, d7\n");
		} else if(type_size(&address_type, 0) != 1){
			fprintf(output_file, "\tmuls.w #%d, d7\n", (int) type_size(&address_type, 0));
		}
		if(address->data.type == data_register){
			fprintf(output_file, "\tadd.l d7, d%d\n", address->data.reg);
			if(dereference && peek_type(&address_type) != type_list){
				if(type_size(&address_type, 0) == 4){
					fprintf(output_file, "\tmove.l d%d, a0\n", address->data.reg);
					fprintf(output_file, "\tmove.l (a0), d%d\n", address->data.reg);
				} else if(type_size(&address_type, 0) == 2){
					fprintf(output_file, "\tmove.l d%d, a0\n", address->data.reg);
					fprintf(output_file, "\tmove.w (a0), d%d\n", address->data.reg);
					fprintf(output_file, "\text.l d%d\n", address->data.reg);
				} else if(type_size(&address_type, 0) == 1){
					fprintf(output_file, "\tmove.l d%d, a0\n", address->data.reg);
					fprintf(output_file, "\tmove.b (a0), d%d\n", address->data.reg);
					fprintf(output_file, "\text.w d%d\n", address->data.reg);
				}
			}
		} else if(address->data.type == data_stack){
			if(dereference && peek_type(&address_type) != type_list){
				fprintf(output_file, "\tmove.l %d(sp), a0\n", -(int) address->data.stack_pos);
				if(type_size(&address_type, 0) == 4){
					fprintf(output_file, "\tmove.l (a0), d7\n");
				} else if(type_size(&address_type, 0) == 2){
					fprintf(output_file, "\tmove.w (a0), d7\n");
					fprintf(output_file, "\text.l d7\n");
				} else if(type_size(&address_type, 0) == 1){
					fprintf(output_file, "\tmove.b (a0), d7\n");
					fprintf(output_file, "\text.w d7\n");
				}
			} else {
				fprintf(output_file, "\tmove.l %d(sp), d7\n", -(int) address->data.stack_pos);
			}
			fprintf(output_file, "\tmove.l d7, %d(sp)\n", -(int) address->data.stack_pos);
		}
	}
	deallocate(index.data);
	if(dereference){
		pop_type(&(address->data_type));
		if(peek_type(&(address->data_type)) == type_list){
			pop_type(&(address->data_type));
			address->data_type.current_index--;
			add_type_entry(&(address->data_type), type_pointer);
		}
	}
	address->is_reference = !dereference;
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

void skip_comment(char **c){
	++*c;
	if(**c == '/'){
		while(**c && **c != '\n'){
			++*c;
		}
		if(**c){
			++*c;
		}
	} else {
		++*c;
		while(**c && (**c != '*' || (*c)[1] != '/')){
			++*c;
		}
		if(**c){
			*c += 2;
		}
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
	while(**c == '*' || **c == '&' || **c == '!' || **c == '~' || **c == '-' || is_cast(*c) || is_whitespace(*c)){
		if(**c == '('){//ie if we are casting
			++*c;
			match_parentheses(c);
		} else if(**c == '/'){
			skip_comment(c);
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

void compile_string(value *output, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	++*c;
	skip_string(c);

	output->data = allocate(force_stack);
	if(output->data.type == data_register){
		fprintf(output_file, "\tlea __str%d, a0\n", current_string);
		fprintf(output_file, "\tmove.l a0, d%d\n", output->data.reg);
	} else if(output->data.type == data_stack){
		fprintf(output_file, "\tlea __str%d, a0\n", current_string);
		fprintf(output_file, "\tmove.l a0, %d(sp)\n", -(int) output->data.stack_pos);
	}
	current_string++;
	output->data_type = CHAR_TYPE;
	add_type_entry(&(output->data_type), type_pointer);
	output->is_reference = 0;
}

void compile_character(value *output, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
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

	output->data = allocate(force_stack);
	if(output->data.type == data_register){
		fprintf(output_file, "\tmove.l #%d, d%d\n", char_constant, output->data.reg);
	} else if(output->data.type == data_stack){
		fprintf(output_file, "\tmove.l #%d, %d(sp)\n", char_constant, -(int) output->data.stack_pos);
	}
	output->data_type = INT_TYPE;
	output->is_reference = 0;
}

void compile_logical_not(value *v, FILE *output_file){
	if(v->data.type == data_register){
		fprintf(output_file, "\ttst.l d%d\n", v->data.reg);
		fprintf(output_file, "\tseq.b d%d\n", v->data.reg);
		fprintf(output_file, "\tandi.b #1, d%d\n", v->data.reg);
	} else if(v->data.type == data_stack){
		fprintf(output_file, "\tmove.l %d(sp), d7\n", -(int) v->data.stack_pos);
		fprintf(output_file, "\ttst.l d7\n");
		fprintf(output_file, "\tseq.b d7\n");
		fprintf(output_file, "\tandi.b #1, d7\n");
		fprintf(output_file, "\tmove.l d7, %d(sp)\n", -(int) v->data.stack_pos);
	}

	v->data_type = INT_TYPE;
}

void compile_not(value *v, FILE *output_file){
	if(!types_equal(&(v->data_type), &INT_TYPE) && !types_equal(&(v->data_type), &CHAR_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't perform bitwise not of non-numerical type");
		do_error(1);
	}
	if(v->data.type == data_register){
		fprintf(output_file, "\tnot.w d%d\n", v->data.reg);
	} else if(v->data.type == data_stack){
		fprintf(output_file, "\tnot.w %d(sp)\n", -(int) v->data.stack_pos);
	}

	v->data_type = INT_TYPE;
}

void compile_negate(value *v, FILE *output_file){
	if(!types_equal(&(v->data_type), &INT_TYPE) && !types_equal(&(v->data_type), &CHAR_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't negate non-numerical type");
		do_error(1);
	}
	if(v->data.type == data_register){
		fprintf(output_file, "\tneg.w d%d\n", v->data.reg);
	} else if(v->data.type == data_stack){
		fprintf(output_file, "\tneg.w %d(sp)\n", -(int) v->data.stack_pos);
	}

	v->data_type = INT_TYPE;
}

void compile_value(value *output, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	char *temp_c;
	type cast_type;

	skip_whitespace(c);

	if(**c == '*'){
		++*c;
		compile_value(output, c, 1, force_stack, output_file);
		if(dereference){
			compile_dereference(output, output_file);
		}
		output->is_reference = !dereference;
		return;
	} else if(**c == '&'){
		++*c;
		if(dereference){
			compile_value(output, c, 0, force_stack, output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output->is_reference = 0;
		return;
	} else if(**c == '!'){
		++*c;
		if(dereference){
			compile_value(output, c, 1, force_stack, output_file);
			compile_logical_not(output, output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output->is_reference = 0;
		return;
	} else if(**c == '~'){
		++*c;
		if(dereference){
			compile_value(output, c, 1, force_stack, output_file);
			compile_not(output, output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output->is_reference = 0;
		return;
	} else if(**c == '-' && !digit((*c)[1])){
		++*c;
		if(dereference){
			compile_value(output, c, 1, force_stack, output_file);
			compile_negate(output, output_file);
		} else {
			snprintf(error_message, sizeof(error_message), "Can't get address of r-value");
			do_error(1);
		}
		output->is_reference = 0;
		return;
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
			compile_value(output, c, 1, force_stack, output_file);
			cast(output, cast_type, 0, output_file);
		//Associative parentheses
		} else {
			compile_expression(output, c, dereference, force_stack, output_file);
			if(**c == ')'){
				++*c;
			} else {
				snprintf(error_message, sizeof(error_message), "Expected closing ')'");
				do_error(1);
			}
		}
	} else if(**c == '-' || digit(**c)){
		compile_integer(output, c, dereference, force_stack, output_file);
	} else if(alpha(**c)){
		compile_variable(output, c, dereference, force_stack, output_file);
	} else if(**c == '\"'){
		compile_string(output, c, dereference, force_stack, output_file);
	} else if(**c == '\''){
		compile_character(output, c, dereference, force_stack, output_file);
	} else {
		snprintf(error_message, sizeof(error_message), "Unrecognized expression value");
		do_error(1);
	}

	skip_whitespace(c);
	while(**c == '[' || **c == '('){
		if(**c == '['){
			if(output->is_reference){
				compile_dereference(output, output_file);
			}
			compile_list_index(c, output, dereference, output_file);
		} else if(**c == '('){
			++*c;
			if(output->is_reference){
				compile_dereference(output, output_file);
			}
			compile_function_call(c, output, output_file);
		}

		skip_whitespace(c);
	}
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
			} else if(c[1] == '='){
				return operation_greater_than_or_equal;
			} else {
				return operation_greater_than;
			}
		case '<':
			if(c[1] == '<'){
				return operation_shift_left;
			} else if(c[1] == '='){
				return operation_less_than_or_equal;
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
			} else if((*c)[1] == '='){
				output = operation_greater_than_or_equal;
				++*c;
			} else {
				output = operation_greater_than;
			}
			break;
		case '<':
			if((*c)[1] == '<'){
				output = operation_shift_left;
				++*c;
			} else if((*c)[1] == '='){
				output = operation_less_than_or_equal;
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

void compile_operation(value *first_value, value *next_value, operation op, FILE *output_file){
	char reg0_str_buffer[3] = {0, 0, 0};
	char reg1_str_buffer[3] = {0, 0, 0};
	char *reg0_str = "";
	char *reg1_str = "";
	type output_type;

	if(types_equal(&(first_value->data_type), &VOID_TYPE) || types_equal(&(next_value->data_type), &VOID_TYPE)){
		snprintf(error_message, sizeof(error_message), "Can't operate on void value");
		do_error(1);
	}

	if(first_value->data.type == data_register){
		snprintf(reg0_str_buffer, sizeof(reg0_str_buffer)/sizeof(char), "d%d", first_value->data.reg);
		reg0_str = reg0_str_buffer;
	} else if(first_value->data.type == data_stack){
		reg0_str = "d6";
		if(type_size(&(first_value->data_type), 0) == 4){
			fprintf(output_file, "\tmove.l %d(sp), d6\n", -(int) first_value->data.stack_pos);
		} else if(type_size(&(first_value->data_type), 0) == 2){
			fprintf(output_file, "\tmove.w %d(sp), d6\n", -(int) first_value->data.stack_pos);
		} else if(type_size(&(first_value->data_type), 0) == 1){
			fprintf(output_file, "\tmove.b %d(sp), d6\n", -(int) first_value->data.stack_pos);
			fprintf(output_file, "\text.w d6\n");
		} else {
			snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size %d", type_size(&(first_value->data_type), 0));
			do_error(1);
		}
	}
	if(next_value->data.type == data_register){
		snprintf(reg1_str_buffer, sizeof(reg1_str_buffer)/sizeof(char), "d%d", next_value->data.reg);
		reg1_str = reg1_str_buffer;
	} else if(next_value->data.type == data_stack){
		reg1_str = "d7";
		if(type_size(&(next_value->data_type), 0) == 4){
			fprintf(output_file, "\tmove.l %d(sp), d7\n", -(int) next_value->data.stack_pos);
		} else if(type_size(&(next_value->data_type), 0) == 2){
			fprintf(output_file, "\tmove.w %d(sp), d7\n", -(int) next_value->data.stack_pos);
		} else if(type_size(&(next_value->data_type), 0) == 1){
			fprintf(output_file, "\tmove.b %d(sp), d7\n", -(int) next_value->data.stack_pos);
			fprintf(output_file, "\text.w d7\n");
		} else {
			snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size %d", type_size(&(next_value->data_type), 0));
			do_error(1);
		}
	}
	operation_functions[op](reg0_str, reg1_str, first_value, next_value, output_file, &output_type);
	deallocate(next_value->data);
	if(first_value->data.type == data_stack){
		if(type_size(&output_type, 0) == 4){
			fprintf(output_file, "\tmove.l d6, %d(sp)\n", -(int) first_value->data.stack_pos);
		} else if(type_size(&output_type, 0) == 2){
			fprintf(output_file, "\tmove.w d6, %d(sp)\n", -(int) first_value->data.stack_pos);
		} else if(type_size(&output_type, 0) == 1){
			fprintf(output_file, "\tmove.b d6, %d(sp)\n", -(int) first_value->data.stack_pos);
		} else {
			snprintf(error_message, sizeof(error_message), "[INTERNAL] Unusable type size %d", type_size(&output_type, 0));
			do_error(1);
		}
	}
	first_value->data_type = output_type;
	first_value->is_reference = 0;
}

static void compile_expression_recursive(value *first_value, char **c, FILE *output_file){
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
		cast(first_value, INT_TYPE, 1, output_file);
		if(first_value->data.type == data_register){
			fprintf(output_file, "\ttst.w d%d\n", first_value->data.reg);
			fprintf(output_file, "\tsne.b d%d\n", first_value->data.reg);
			fprintf(output_file, "\tandi.l #1, d%d\n", first_value->data.reg);
			fprintf(output_file, "\tbne __L%d\n", label_num);
		} else if(first_value->data.type == data_stack){
			fprintf(output_file, "\ttst.w %d(sp)\n", -(int) first_value->data.stack_pos);
			fprintf(output_file, "\tsne.b %d(sp)\n", -(int) first_value->data.stack_pos);
			fprintf(output_file, "\tandi.l #1, %d(sp)\n", -(int) first_value->data.stack_pos);
			fprintf(output_file, "\tbne __L%d\n", label_num);
		}
	} else if(current_operation == operation_logical_and){
		label_num = num_labels;
		num_labels++;
		cast(first_value, INT_TYPE, 1, output_file);
		if(first_value->data.type == data_register){
			fprintf(output_file, "\ttst.w d%d\n", first_value->data.reg);
			fprintf(output_file, "\tsne.b d%d\n", first_value->data.reg);
			fprintf(output_file, "\tandi.l #1, d%d\n", first_value->data.reg);
			fprintf(output_file, "\tbeq __L%d\n", label_num);
		} else if(first_value->data.type == data_stack){
			fprintf(output_file, "\ttst.w %d(sp)\n", -(int) first_value->data.stack_pos);
			fprintf(output_file, "\tsne.b %d(sp)\n", -(int) first_value->data.stack_pos);
			fprintf(output_file, "\tandi.l #1, %d(sp)\n", -(int) first_value->data.stack_pos);
			fprintf(output_file, "\tbeq __L%d\n", label_num);
		}
	}

	temp_c = *c;
	current_line_temp = current_line;
	skip_value(&temp_c);
	current_line = current_line_temp;
	next_operation = peek_operation(temp_c);
	if(next_operation == operation_assign && order_of_operations[operation_assign] > order_of_operations[current_operation]){
		compile_value(&next_value, c, 0, 0, output_file);
	} else {
		compile_value(&next_value, c, 1, 0, output_file);
	}
	skip_whitespace(c);
	while(order_of_operations[next_operation] > order_of_operations[current_operation]){
		compile_expression_recursive(&next_value, c, output_file);
		skip_whitespace(c);
		next_operation = peek_operation(*c);
	}
	if(current_operation == operation_assign){
		cast_to = first_value->data_type;
		pop_type(&cast_to);
		cast(&next_value, cast_to, 1, output_file);
		compile_operation(first_value, &next_value, current_operation, output_file);
	} else if(current_operation == operation_logical_or || current_operation == operation_logical_and){
		cast(&next_value, INT_TYPE, 1, output_file);
		if(next_value.data.type == data_register){
			fprintf(output_file, "\ttst.w d%d\n", next_value.data.reg);
			fprintf(output_file, "\tsne.b d%d\n", next_value.data.reg);
			fprintf(output_file, "\tandi.l #1, d%d\n", next_value.data.reg);
			if(first_value->data.type == data_register){
				fprintf(output_file, "\tmove.w d%d, d%d\n", next_value.data.reg, first_value->data.reg);
			} else if(first_value->data.type == data_stack){
				fprintf(output_file, "\tmove.w d%d, %d(sp)\n", next_value.data.reg, -(int) first_value->data.stack_pos);
			}
		} else if(next_value.data.type == data_stack){
			if(first_value->data.type == data_register){
				fprintf(output_file, "\tmove.w %d(sp), d%d\n", -(int) next_value.data.stack_pos, first_value->data.reg);
				fprintf(output_file, "\ttst.w d%d\n", first_value->data.reg);
				fprintf(output_file, "\tsne.b d%d\n", first_value->data.reg);
				fprintf(output_file, "\tandi.l #1, d%d\n", first_value->data.reg);
			} else if(first_value->data.type == data_stack){
				fprintf(output_file, "\tmove.w %d(sp), d7\n", -(int) next_value.data.stack_pos);
				fprintf(output_file, "\ttst.w d7\n");
				fprintf(output_file, "\tsne.b d7\n");
				fprintf(output_file, "\tandi.l #1, d7\n");
				fprintf(output_file, "\tmove.w d7, %d(sp)\n", -(int) first_value->data.stack_pos);
			}
		}
		deallocate(next_value.data);
		fprintf(output_file, "\n__L%d\n", label_num);
	} else {
		compile_operation(first_value, &next_value, current_operation, output_file);
	}
	skip_whitespace(c);
}

void compile_expression(value *first_value, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file){
	char *temp_c;
	int current_line_temp;
	operation next_operation;

	temp_c = *c;
	current_line_temp = current_line;
	skip_value(&temp_c);
	current_line = current_line_temp;

	next_operation = peek_operation(temp_c);
	if(peek_operation(temp_c) == operation_assign){
		compile_value(first_value, c, 0, force_stack, output_file);
	} else {
		compile_value(first_value, c, dereference || next_operation != operation_none, force_stack, output_file);
	}
	while(**c && **c != ';' && **c != ',' && **c != ')' && **c != ']'){
		compile_expression_recursive(first_value, c, output_file);
	}
}

