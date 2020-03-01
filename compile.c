#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "expression.h"
#include "allocate.h"
#include "compile.h"

#define MAX_SOURCEFILES 32

int num_vars = 0;
int num_strs = 0;
int current_line = 1;
type return_type;
static char *program_beginning;
static char *program_pointer;
static char *source_files[MAX_SOURCEFILES] = {NULL};
static char **current_source_file;
static FILE *output_file;

char warning_message[256] = {0};
char error_message[256] = {0};

enum cmd_argtype{
	cmd_source_file,
	cmd_output_file
};

static void print_line(){
	char *line_pointer;
	char *line_beginning;

	line_pointer = program_pointer;
	fprintf(stderr, "Line %d in %s:\n", current_line, *current_source_file);
	while(line_pointer != program_beginning && line_pointer[-1] != '\n'){
		line_pointer--;
	}
	while(*line_pointer && line_pointer != program_pointer && (*line_pointer == ' ' || *line_pointer == '\t')){
		line_pointer++;
	}
	line_beginning = line_pointer;
	while(*line_pointer && *line_pointer != '\n'){
		fputc(*line_pointer, stderr);
		line_pointer++;
	}
	fputc('\n', stderr);
	while(line_beginning != program_pointer){
		fputc(' ', stderr);
		line_beginning++;
	}
	fprintf(stderr, "^\n");
}

void do_warning(){
	warning_message[255] = '\0';
	print_line();
	fprintf(stderr, "Warning: %s\n", warning_message);
}

void do_error(int status){
	fclose(output_file);
	error_message[255] = '\0';
	print_line();
	fprintf(stderr, "Error: %s\n", error_message);
	free_local_variables();
	free_global_variables();
	exit(status);
}

void compile_file(char *program, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments, FILE *output_file){
	program_pointer = program;
	program_beginning = program;
	skip_whitespace(&program_pointer);
	fprintf(output_file, ".data\n");
	compile_string_constants(program_pointer, output_file);
	fprintf(output_file, ".text\n\n");
	while(*program_pointer){
		compile_function(&program_pointer, identifier_name, arguments, identifier_length, num_arguments, output_file);
		free_local_variables();
		skip_whitespace(&program_pointer);
	}
}

void compile_function(char **c, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments, FILE *output_file){
	type t = EMPTY_TYPE;
	type argument_type = EMPTY_TYPE;
	variable *var;
	variable *local_var;
	unsigned int current_argument = 0;

	num_vars = 0;
	parse_type(&t, c, identifier_name, arguments, identifier_length, num_arguments);

	if(!identifier_name){
		snprintf(error_message, sizeof(error_message), "Expected identifier name");
		do_error(1);
	}
	skip_whitespace(c);
	if(peek_type(t) != type_function){
		skip_whitespace(c);
		if(**c != ';'){
			snprintf(error_message, sizeof(error_message), "Expected ';'");
			do_error(1);
		}
		++*c;
		var = read_dictionary(global_variables, identifier_name, 0);
		if(var){
			snprintf(error_message, sizeof(error_message), "Duplicate definitions of non-function data");
			do_error(1);
		}
		var = malloc(sizeof(variable));
		var->var_type = t;
		var->varname = malloc(strlen(identifier_name) + 1);
		var->leave_as_address = 0;
		strcpy(var->varname, identifier_name);
		write_dictionary(&global_variables, var->varname, var, 0);
		fprintf(output_file, ".data\n.align 2\n%s:\n.space %d\n.text\n", identifier_name, align4(type_size(t)));
	} else {
		if(!*identifier_name){
			snprintf(error_message, sizeof(error_message), "Expected function name");
			do_error(1);
		}
		var = read_dictionary(global_variables, identifier_name, 0);
		if(!var){
			var = malloc(sizeof(variable));
			var->var_type = t;
			var->varname = malloc(strlen(identifier_name) + 1);
			var->leave_as_address = 1;
			strcpy(var->varname, identifier_name);
			write_dictionary(&global_variables, var->varname, var, 0);
		} else {
			if(!types_equal(var->var_type, t)){
				snprintf(error_message, sizeof(error_message), "Incompatible function definitions");
				do_error(1);
			}
		}
		if(**c != '{'){
			if(**c != ';'){
				snprintf(error_message, sizeof(error_message), "Expected '{' or ';' instead of '%c'", **c);
				do_error(1);
			}
			++*c;
			return;
		}
		++*c;
		fprintf(output_file, "\n.globl %s\n%s:\n", identifier_name, identifier_name);
		pop_type(&t);
		while(peek_type(t) != type_returns){
			if(current_argument >= num_arguments){
				snprintf(error_message, sizeof(error_message), "Too many arguments!");
				do_error(1);
			}
			argument_type = get_argument_type(&t);
			local_var = malloc(sizeof(variable));
			local_var->var_type = argument_type;
			local_var->varname = malloc(strlen(arguments) + 1);
			strcpy(local_var->varname, arguments);
			local_var->stack_pos = variables_size;
			write_dictionary(&local_variables, local_var->varname, local_var, 0);
			variables_size += 4;
			arguments += identifier_length;
			current_argument++;
			num_vars++;
		}
		pop_type(&t);
		return_type = t;
		if(peek_type(return_type) == type_function){
			snprintf(error_message, sizeof(error_message), "Function cannot return function");
			do_error(1);
		}
		compile_block(c, 1, output_file);
		++*c;
		var->num_args = num_vars;
		fprintf(output_file, "addi $sp, $sp, %d\n", variables_size + 8);
		fprintf(output_file, "lw $ra, 0($sp)\n");
		fprintf(output_file, "jr $ra\n");
	}
}

void compile_block(char **c, unsigned char do_variable_initializers, FILE *output_file){
	char *temp;

	skip_whitespace(c);
	temp = *c;
	while(do_variable_initializers && parse_datatype(NULL, &temp)){
		compile_variable_initializer(c);
		skip_whitespace(c);
		temp = *c;
		num_vars++;
	}

	if(do_variable_initializers){
		fprintf(output_file, "addi $sp, $sp, %d\n", -(int) variables_size - 8);
	}

	while(**c != '}'){
		compile_statement(c, output_file);
		skip_whitespace(c);
	}
}

void compile_statement(char **c, FILE *output_file){
	value expression_output;
	unsigned int label_num0;
	unsigned int label_num1;

	skip_whitespace(c);
	if(!strncmp(*c, "if", 2) && !alphanumeric((*c)[2])){
		*c += 2;
		skip_whitespace(c);
		if(**c != '('){
			snprintf(error_message, sizeof(error_message), "Expected '('");
			do_error(1);;
		}
		++*c;
		expression_output = compile_expression(c, 1, 0, output_file);
		label_num0 = num_labels;
		num_labels++;
		if(expression_output.data.type == data_register){
			fprintf(output_file, "beq $s%d, $zero, __L%d\n", expression_output.data.reg, label_num0);
		} else if(expression_output.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) expression_output.data.stack_pos);
			fprintf(output_file, "beq $t0, $zero, __L%d\n", label_num0);
		}
		deallocate(expression_output.data);
		skip_whitespace(c);
		if(**c != ')'){
			snprintf(error_message, sizeof(error_message), "Expected ')'");
			do_error(1);
		}
		++*c;
		skip_whitespace(c);
		if(**c == '{'){
			++*c;
			compile_block(c, 0, output_file);
			if(**c != '}'){
				snprintf(error_message, sizeof(error_message), "Expected '}'");
				do_error(1);
			}
			++*c;
		} else {
			compile_statement(c, output_file);
		}
		fprintf(output_file, "\n__L%d:\n", label_num0);
	} else if(!strncmp(*c, "while", 5) && !alphanumeric((*c)[5])){
		*c += 5;
		skip_whitespace(c);
		if(**c != '('){
			snprintf(error_message, sizeof(error_message), "Expected '('");
			do_error(1);
		}
		++*c;
		label_num0 = num_labels;
		label_num1 = num_labels + 1;
		num_labels += 2;
		fprintf(output_file, "\n__L%d:\n", label_num0);
		expression_output = compile_expression(c, 1, 0, output_file);
		if(expression_output.data.type == data_register){
			fprintf(output_file, "beq $s%d, $zero, __L%d\n", expression_output.data.reg, label_num1);
		} else if(expression_output.data.type == data_stack){
			fprintf(output_file, "lw $t0, %d($sp)\n", -(int) expression_output.data.stack_pos);
			fprintf(output_file, "beq $t0, $zero, __L%d\n", label_num1);
		}
		deallocate(expression_output.data);
		skip_whitespace(c);
		if(**c != ')'){
			snprintf(error_message, sizeof(error_message), "Expected ')'");
			do_error(1);
		}
		++*c;
		skip_whitespace(c);
		if(**c == '{'){
			++*c;
			compile_block(c, 0, output_file);
			if(**c != '}'){
				snprintf(error_message, sizeof(error_message), "Expected '}'");
				do_error(1);
			}
			++*c;
		} else {
			compile_statement(c, output_file);
		}
		fprintf(output_file, "j __L%d\n", label_num0);
		fprintf(output_file, "\n__L%d:\n", label_num1);
	} else if(!strncmp(*c, "return", 6) && !alphanumeric((*c)[6])){
		*c += 6;
		skip_whitespace(c);
		if(types_equal(return_type, VOID_TYPE)){
			if(**c != ';'){
				snprintf(error_message, sizeof(error_message), "Expected ';' for return in void function");
				do_error(1);
			}
			++*c;
		} else {
			expression_output = compile_expression(c, 1, 0, output_file);
			if(**c != ';'){
				snprintf(error_message, sizeof(error_message), "Expected ';'");
				do_error(1);
			}
			++*c;
			cast(expression_output, return_type, 1, output_file);
			if(expression_output.data.type == data_register){
				fprintf(output_file, "sw $s%d, %d($sp)\n", expression_output.data.reg, variables_size + 4);
			} else if(expression_output.data.type == data_stack){
				fprintf(output_file, "lw $t0, %d($sp)\n", -(int) expression_output.data.stack_pos);
				fprintf(output_file, "sw $t0, %d($sp)\n", variables_size + 4);
			}
			deallocate(expression_output.data);
		}
		fprintf(output_file, "addi $sp, $sp, %d\n", variables_size + 8);
		fprintf(output_file, "lw $ra, 0($sp)\n");
		fprintf(output_file, "jr $ra\n");
	} else {
		expression_output = compile_expression(c, 1, 0, output_file);
		deallocate(expression_output.data);
		if(**c == ';'){
			++*c;
		} else {
			snprintf(error_message, sizeof(error_message), "Expected ';'");
			do_error(1);
		}
	}
}

void place_string_constant(char **c, FILE *output_file){
	unsigned char ignore_next;
	while(**c && (**c != '"' || ignore_next)){
		fprintf(output_file, "%c", **c);
		if(**c == '\\'){
			ignore_next = 1;
		} else {
			ignore_next = 0;
		}
		++*c;
	}

	if(!**c){
		snprintf(error_message, sizeof(error_message), "Expected closing '\"'");
		do_error(1);
	}
	fprintf(output_file, "\"");
	++*c;
}

void compile_string_constants(char *c, FILE *output_file){
	while(*c){
		if(*c != '"'){
			c++;
		} else {
			c++;
			fprintf(output_file, "__str%d:\n", num_strs);
			fprintf(output_file, ".asciiz \"");
			place_string_constant(&c, output_file);
			fprintf(output_file, "\n");
			num_strs++;
		}
	}
}

void parse_arguments(int argc, char **argv, char *filenames[], unsigned int num_filenames, char **output_filename){
	unsigned int current_arg;
	unsigned int current_filename = 0;
	enum cmd_argtype argtype = cmd_source_file;

	for(current_arg = 1; current_arg < argc; current_arg++){
		switch(argtype){
			case cmd_source_file:
				if(argv[current_arg][0] == '-'){
					if(argv[current_arg][1] == 'h' || (argv[current_arg][1] == '-' && argv[current_arg][2] == 'h')){//Display help message
						printf("Arguments:\n-h --help     Displays this help message\n-o            Specify output file\n\nTo compile:\nmcc [SOURCE1] [SOURCE2] [SOURCE3] ... -o [OUTPUT_FILE]\n");
						exit(1);
					} else if(argv[current_arg][1] == 'o'){
						argtype = cmd_output_file;
					} else {
						fprintf(stderr, "Unrecognized option '%s'\n", argv[current_arg]);
						exit(1);
					}
				} else {
					if(current_filename < num_filenames){
						filenames[current_filename] = argv[current_arg];
						current_filename++;
					} else {
						fprintf(stderr, "Too many source files\n");
						exit(1);
					}
				}
				break;
			case cmd_output_file:
				if(argv[current_arg][0] == '-'){
					fprintf(stderr, "Expected output file name after '-o'\n");
					exit(1);
				}
				*output_filename = argv[current_arg];
				argtype = cmd_source_file;
				break;
		}
	}
}

int main(int argc, char **argv){
	char identifier_name[32];
	char arguments[32*32];
	FILE *fp;
	unsigned int file_size;
	char *program;
	char *output_filename = NULL;

	if(argc < 2){
		printf("Minimal C Compiler for MIPS\nby Ben Jones\n2/29/2020\n");
		exit(1);
	}
	parse_arguments(argc, argv, source_files, MAX_SOURCEFILES - 1, &output_filename);
	if(!output_filename){
		output_filename = "a.s";
	}
	output_file = fopen(output_filename, "w");
	if(!output_file){
		fprintf(stderr, "Could not open file '%s' for writing\n", output_filename);
		exit(1);
	}
	global_variables = create_dictionary(NULL);
	current_source_file = source_files;
	while(*current_source_file){
		fp = fopen(*current_source_file, "r");
		if(!fp){
			fprintf(stderr, "Could not open file '%s' for reading\n", *source_files);
			free_global_variables();
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
		compile_file(program, identifier_name, arguments, 32, 32, output_file);
		free(program);
		current_source_file++;
	}
	fclose(output_file);
	free_global_variables();

	return 0;
}
