#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "expression.h"
#include "allocate.h"
#include "compile.h"

int num_vars = 0;
type return_type;

void compile_program(char *program, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments){
	char *c;

	c = program;
	skip_whitespace(&c);
	while(*c){
		compile_function(&c, identifier_name, arguments, identifier_length, num_arguments);
		free_local_variables();
		skip_whitespace(&c);
	}
	free_global_variables();
}

void compile_function(char **c, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments){
	type t = EMPTY_TYPE;
	type argument_type = EMPTY_TYPE;
	variable *var;
	variable *local_var;
	unsigned int current_argument = 0;
	unsigned int tsize;
	unsigned int remainder;

	num_vars = 0;
	parse_type(&t, c, identifier_name, arguments, identifier_length, num_arguments);

	if(!identifier_name){
		fprintf(stderr, "Expected identifier name\n");
		exit(1);
	}
	skip_whitespace(c);
	if(peek_type(t) != type_function){
		skip_whitespace(c);
		if(**c != ';'){
			fprintf(stderr, "Expected ';'\n");
			exit(1);
		}
		++*c;
		var = read_dictionary(global_variables, identifier_name, 0);
		if(var){
			fprintf(stderr, "Error: Duplicate definitions of non-function data\n");
			exit(1);
		}
		var = malloc(sizeof(variable));
		var->var_type = t;
		var->varname = malloc(strlen(identifier_name) + 1);
		var->leave_as_address = 0;
		strcpy(var->varname, identifier_name);
		write_dictionary(&global_variables, var->varname, var, 0);
	} else {
		if(!*identifier_name){
			fprintf(stderr, "Expected function name\n");
			exit(1);
		}
		var = read_dictionary(global_variables, identifier_name, 0);
		if(!var){
			var = malloc(sizeof(variable));
			var->var_type = t;
			var->varname = malloc(strlen(identifier_name) + 1);
			var->leave_as_address = 1;
			strcpy(var->varname, identifier_name);
			write_dictionary(&global_variables, var->varname, var, 0);
		}
		if(**c != '{'){
			if(**c != ';'){
				fprintf(stderr, "Expected '{' or ';' instead of '%c'\n", **c);
				exit(1);
			}
			++*c;
			return;
		}
		++*c;
		printf("\n.globl %s\n%s:\n", identifier_name, identifier_name);
		pop_type(&t);
		while(peek_type(t) != type_returns){
			if(current_argument >= num_arguments){
				fprintf(stderr, "Too many arguments!\n");
				exit(1);
			}
			argument_type = get_argument_type(&t);
			local_var = malloc(sizeof(variable));
			local_var->var_type = argument_type;
			tsize = type_size(argument_type);
			remainder = variables_size%tsize;
			if(remainder){
				variables_size += tsize - remainder;
			}

			local_var->varname = malloc(strlen(arguments) + 1);
			strcpy(local_var->varname, arguments);
			local_var->stack_pos = variables_size;
			write_dictionary(&local_variables, local_var->varname, local_var, 0);
			variables_size += type_size(local_var->var_type);
			arguments += identifier_length;
			current_argument++;
			num_vars++;
		}
		pop_type(&t);
		return_type = t;
		compile_block(c, 1);
		++*c;
		var->num_args = num_vars;
		printf("lw $ra, %d($sp)\n", variables_size + 8);
		printf("addi $sp, $sp, %d\n", variables_size + 8);
		printf("jr $ra\n");
	}
}

void compile_block(char **c, unsigned char do_variable_initializers){
	char *temp;
	unsigned int remainder;

	skip_whitespace(c);
	temp = *c;
	while(do_variable_initializers && parse_datatype(NULL, &temp)){
		compile_variable_initializer(c);
		skip_whitespace(c);
		temp = *c;
		num_vars++;
	}

	//Gotta make sure the stack pointer itself stays aligned to integers
	remainder = variables_size%INT_SIZE;
	if(remainder){
		variables_size += INT_SIZE - remainder;
	}
	if(do_variable_initializers){
		printf("addi $sp, $sp, %d\n", -(int) variables_size - 8);
	}

	while(**c != '}'){
		compile_statement(c);
		skip_whitespace(c);
	}
}

void compile_statement(char **c){
	value expression_output;
	unsigned int label_num0;
	unsigned int label_num1;

	skip_whitespace(c);
	if(!strncmp(*c, "if", 2) && !alphanumeric((*c)[2])){
		*c += 2;
		skip_whitespace(c);
		if(**c != '('){
			fprintf(stderr, "Expected '('\n");
			exit(1);
		}
		++*c;
		expression_output = compile_expression(c, 1, 0);
		label_num0 = num_labels;
		num_labels++;
		if(expression_output.data.type == data_register){
			printf("beq $s%d, $zero, __L%d\n", expression_output.data.reg, label_num0);
		} else if(expression_output.data.type == data_stack){
			printf("lw $t0, %d($sp)\n", -(int) expression_output.data.stack_pos);
			printf("beq $t0, $zero, __L%d\n", label_num0);
		}
		deallocate(expression_output.data);
		skip_whitespace(c);
		if(**c != ')'){
			fprintf(stderr, "Expected ')'\n");
			exit(1);
		}
		++*c;
		skip_whitespace(c);
		if(**c == '{'){
			++*c;
			compile_block(c, 0);
			if(**c != '}'){
				fprintf(stderr, "Expected '}'\n");
				exit(1);
			}
			++*c;
		} else {
			compile_statement(c);
		}
		printf("\n__L%d:\n", label_num0);
	} else if(!strncmp(*c, "while", 5) && !alphanumeric((*c)[5])){
		*c += 5;
		skip_whitespace(c);
		if(**c != '('){
			fprintf(stderr, "Expected '('\n");
			exit(1);
		}
		++*c;
		label_num0 = num_labels;
		label_num1 = num_labels + 1;
		num_labels += 2;
		printf("\n__L%d:\n", label_num0);
		expression_output = compile_expression(c, 1, 0);
		if(expression_output.data.type == data_register){
			printf("beq $s%d, $zero, __L%d\n", expression_output.data.reg, label_num1);
		} else if(expression_output.data.type == data_stack){
			printf("lw $t0, %d($sp)\n", -(int) expression_output.data.stack_pos);
			printf("beq $t0, $zero, __L%d\n", label_num1);
		}
		deallocate(expression_output.data);
		skip_whitespace(c);
		if(**c != ')'){
			fprintf(stderr, "Expected ')'\n");
			exit(1);
		}
		++*c;
		skip_whitespace(c);
		if(**c == '{'){
			++*c;
			compile_block(c, 0);
		} else {
			compile_statement(c);
		}
		if(**c != '}'){
			fprintf(stderr, "Expected '}'\n");
			exit(1);
		}
		++*c;
		printf("j __L%d\n", label_num0);
		printf("\n__L%d:\n", label_num1);
	} else if(!strncmp(*c, "return", 6) && !alphanumeric((*c)[6])){
		*c += 6;
		skip_whitespace(c);
		expression_output = compile_expression(c, 1, 0);
		if(**c != ';'){
			fprintf(stderr, "Expected ';'\n");
			exit(1);
		}
		++*c;
		cast(expression_output, return_type);
		if(expression_output.data.type == data_register){
			printf("sw $s%d, %d($sp)\n", expression_output.data.reg, variables_size + 4);
		} else if(expression_output.data.type == data_stack){
			printf("lw $t0, %d($sp)\n", -(int) expression_output.data.stack_pos);
			printf("sw $t0, %d($sp)\n", variables_size + 4);
		}
		deallocate(expression_output.data);
		printf("lw $ra, %d($sp)\n", variables_size + 8);
		printf("addi $sp, $sp, %d\n", variables_size + 8);
		printf("jr $ra\n");
	} else {
		expression_output = compile_expression(c, 1, 0);
		deallocate(expression_output.data);
		if(**c == ';'){
			++*c;
		} else {
			fprintf(stderr, "Expected ';'\n");
			exit(1);
		}
	}
}

int main(int argc, char **argv){
	char identifier_name[32];
	char arguments[32*32];
	FILE *fp;
	unsigned int file_size;
	char *program;
	if(argc != 2){
		fprintf(stderr, "Expected 1 argument\n");
		exit(1);
	}
	fp = fopen(argv[1], "r");
	if(!fp){
		fprintf(stderr, "Could not open file '%s'\n", argv[1]);
		exit(1);
	}
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	program = calloc(file_size + 1, sizeof(char));
	fread(program, sizeof(char), file_size, fp);
	fclose(fp);
	initialize_register_list();
	initialize_variables();
	compile_program(program, identifier_name, arguments, 32, 32);
	free(program);

	return 0;
}
