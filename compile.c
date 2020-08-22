#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "expression.h"
#include "allocate.h"
#include "compile.h"

#define MAX_SOURCEFILES 32

int num_vars = 0;
int num_args = 0;
int num_strs = 0;
int current_line;
type return_type;
static char *program;
static char *program_beginning;
static char *program_pointer;
static char *source_files[MAX_SOURCEFILES] = {NULL};
static char **current_source_file;
static FILE *output_file;

static int continue_label = -1;
static int break_label = -1;
static int loop_variables_size;

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
	if(do_print){
		warning_message[255] = '\0';
		print_line();
		fprintf(stderr, "Warning: %s\n", warning_message);
	}
}

void do_error(int status){
	fclose(output_file);
	error_message[255] = '\0';
	print_line();
	fprintf(stderr, "Error: %s\n", error_message);
	free(program);
	free_local_variables();
	free_global_variables();
	exit(status);
}

void compile_file(char *program, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments, FILE *output_file){
	int current_line_temp;

	program_pointer = program;
	program_beginning = program;
	fprintf(output_file, ".data\n");
	current_line = 1;
	skip_whitespace(&program_pointer);
	current_line_temp = current_line;
	compile_string_constants(program_pointer, output_file);
	current_line = current_line_temp;
	fprintf(output_file, ".text\n\n");
	while(*program_pointer){
		compile_function(&program_pointer, identifier_name, arguments, identifier_length, num_arguments, output_file);
		free_local_variables();
		skip_whitespace(&program_pointer);
	}
}

void compile_function(char **c, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments, FILE *output_file){
	variable *var;
	variable *local_var;
	unsigned int current_argument = 0;

	num_vars = 0;
	variables_size = 0;
	return_type = EMPTY_TYPE;
	parse_type(&return_type, c, identifier_name, arguments, identifier_length, num_arguments);

	if(!identifier_name){
		snprintf(error_message, sizeof(error_message), "Expected identifier name");
		do_error(1);
	}
	skip_whitespace(c);
	if(peek_type(&return_type) != type_function){
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
		var->var_type = return_type;
		var->varname = malloc(strlen(identifier_name) + 1);
		var->leave_as_address = 0;
		strcpy(var->varname, identifier_name);
		write_dictionary(&global_variables, var->varname, var, 0);
		fprintf(output_file, ".data\n.align 2\n%s:\n.space %d\n.text\n", identifier_name, align4(type_size(&return_type, 0)));
	} else {
		if(!*identifier_name){
			snprintf(error_message, sizeof(error_message), "Expected function name");
			do_error(1);
		}
		var = read_dictionary(global_variables, identifier_name, 0);
		if(!var){
			var = malloc(sizeof(variable));
			var->var_type = return_type;
			var->varname = malloc(strlen(identifier_name) + 1);
			var->leave_as_address = 1;
			strcpy(var->varname, identifier_name);
			write_dictionary(&global_variables, var->varname, var, 0);
		} else {
			if(!types_equal(&(var->var_type), &return_type)){
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
		local_variables[0] = malloc(sizeof(dictionary));
		*local_variables[0] = create_dictionary(NULL);
		fprintf(output_file, "\n.globl %s\n%s:\n", identifier_name, identifier_name);
		pop_type(&return_type);
		while(peek_type(&return_type) != type_returns){
			if(current_argument >= num_arguments){
				snprintf(error_message, sizeof(error_message), "Too many arguments!");
				do_error(1);
			}
			if(read_dictionary(*local_variables[0], arguments, 0)){
				snprintf(error_message, sizeof(error_message), "Duplicate argument names");
				do_error(1);
			}
			local_var = malloc(sizeof(variable));
			local_var->var_type = get_argument_type(&return_type);
			local_var->varname = malloc(strlen(arguments) + 1);
			strcpy(local_var->varname, arguments);
			local_var->stack_pos = variables_size;
			write_dictionary(local_variables[0], local_var->varname, local_var, 0);
			variables_size += 4;
			arguments += identifier_length;
			current_argument++;
			num_vars++;
		}
		pop_type(&return_type);
		if(peek_type(&return_type) == type_function){
			snprintf(error_message, sizeof(error_message), "Function cannot return function");
			do_error(1);
		}
		num_args = num_vars;
		compile_block(c, 1, output_file);
		++*c;
		var->num_args = num_vars;
		fprintf(output_file, "lw $ra, %d($sp)\n", 4 + num_args*4);
		fprintf(output_file, "jr $ra\n");
	}
}

static void free_var(void *v){
	variable *var;

	var = (variable *) v;
	free(var->varname);
	free(var);
}

void compile_block(char **c, unsigned char function_beginning, FILE *output_file){
	char *temp;
	unsigned char new_scope = 0;
	int variables_size_before;

	variables_size_before = variables_size;
	skip_whitespace(c);
	temp = *c;
	if(function_beginning || parse_datatype(NULL, &temp)){
		new_scope = 1;
		current_scope++;
		if(current_scope >= MAX_SCOPE){
			snprintf(error_message, sizeof(error_message), "Scope depth too large");
			do_error(1);
		}
		if(!local_variables[current_scope]){
			local_variables[current_scope] = malloc(sizeof(dictionary));
			*local_variables[current_scope] = create_dictionary(NULL);
		}
	}
	temp = *c;
	while(parse_datatype(NULL, &temp)){
		compile_variable_initializer(c);
		skip_whitespace(c);
		temp = *c;
		num_vars++;
	}

	if(new_scope){
		fprintf(output_file, "addi $sp, $sp, %d\n", -variables_size + variables_size_before);
	}

	while(**c != '}'){
		compile_statement(c, output_file);
		skip_whitespace(c);
	}

	if(new_scope){
		fprintf(output_file, "addi $sp, $sp, %d\n", variables_size - variables_size_before);
		free_dictionary(*local_variables[current_scope], free_var);
		free(local_variables[current_scope]);
		local_variables[current_scope] = NULL;
		current_scope--;
		variables_size = variables_size_before;
	}
}

void compile_statement(char **c, FILE *output_file){
	value expression_output;
	unsigned int label_num0;
	unsigned int label_num1;
	unsigned int label_num2;
	unsigned int label_num3;
	int prev_break_label;
	int prev_continue_label;
	int prev_loop_variables_size;

	skip_whitespace(c);
	if(!strncmp(*c, "if", 2) && !alphanumeric((*c)[2])){
		*c += 2;
		skip_whitespace(c);
		if(**c != '('){
			snprintf(error_message, sizeof(error_message), "Expected '('");
			do_error(1);;
		}
		++*c;
		compile_root_expression(&expression_output, c, 1, 0, output_file);
		cast(&expression_output, INT_TYPE, 0, output_file);
		reset_stack_pos(&expression_output, output_file);
		label_num0 = num_labels;
		num_labels++;
		if(expression_output.data.type == data_register){
			fprintf(output_file, "beq $s%d, $zero, __L%d\n", expression_output.data.reg, label_num0);
		} else if(expression_output.data.type == data_stack){
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
		compile_statement(c, output_file);
		skip_whitespace(c);
		if(!strncmp(*c, "else", 2) && !alphanumeric((*c)[4])){
			*c += 4;
			label_num1 = num_labels;
			num_labels++;
			fprintf(output_file, "j __L%d\n", label_num1);
			fprintf(output_file, "\n__L%d:\n", label_num0);
			skip_whitespace(c);
			compile_statement(c, output_file);
			fprintf(output_file, "\n__L%d:\n", label_num1);
		} else {
			fprintf(output_file, "\n__L%d:\n", label_num0);
		}
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

		prev_break_label = break_label;
		prev_continue_label = continue_label;
		prev_loop_variables_size = loop_variables_size;
		break_label = label_num1;
		continue_label = label_num0;
		loop_variables_size = variables_size;

		fprintf(output_file, "\n__L%d:\n", label_num0);
		compile_root_expression(&expression_output, c, 1, 0, output_file);
		cast(&expression_output, INT_TYPE, 0, output_file);
		reset_stack_pos(&expression_output, output_file);
		if(expression_output.data.type == data_register){
			fprintf(output_file, "beq $s%d, $zero, __L%d\n", expression_output.data.reg, label_num1);
		} else if(expression_output.data.type == data_stack){
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
		compile_statement(c, output_file);
		fprintf(output_file, "j __L%d\n", label_num0);
		fprintf(output_file, "\n__L%d:\n", label_num1);

		loop_variables_size = prev_loop_variables_size;
		break_label = prev_break_label;
		continue_label = prev_continue_label;
	} else if(!strncmp(*c, "for", 3) && !alphanumeric((*c)[3])){
		*c += 3;
		skip_whitespace(c);
		if(**c != '('){
			snprintf(error_message, sizeof(error_message), "Expected '('");
			do_error(1);
		}
		++*c;
		label_num0 = num_labels;
		label_num1 = num_labels + 1;
		label_num2 = num_labels + 2;
		label_num3 = num_labels + 3;
		num_labels += 4;

		prev_break_label = break_label;
		prev_continue_label = continue_label;
		prev_loop_variables_size = loop_variables_size;
		break_label = label_num1;
		continue_label = label_num3;
		loop_variables_size = variables_size;

		skip_whitespace(c);
		if(**c != ';'){
			compile_root_expression(&expression_output, c, 1, 0, output_file);
			reset_stack_pos(&expression_output, output_file);
			deallocate(expression_output.data);
			skip_whitespace(c);
			if(**c != ';'){
				snprintf(error_message, sizeof(error_message), "Expected ';'");
				do_error(1);
			}
		}
		++*c;
		fprintf(output_file, "\n__L%d:\n", label_num0);
		skip_whitespace(c);
		if(**c != ';'){
			compile_root_expression(&expression_output, c, 1, 0, output_file);
			cast(&expression_output, INT_TYPE, 0, output_file);
			reset_stack_pos(&expression_output, output_file);
			if(expression_output.data.type == data_register){
				fprintf(output_file, "beq $s%d, $zero, __L%d\n", expression_output.data.reg, label_num1);
			} else if(expression_output.data.type == data_stack){
				fprintf(output_file, "beq $t0, $zero, __L%d\n", label_num1);
			}
			deallocate(expression_output.data);
			skip_whitespace(c);
			if(**c != ';'){
				snprintf(error_message, sizeof(error_message), "Expected ';'");
				do_error(1);
			}
		}
		++*c;
		fprintf(output_file, "j __L%d\n\n__L%d:\n", label_num2, label_num3);
		skip_whitespace(c);
		if(**c != ')'){
			compile_root_expression(&expression_output, c, 1, 0, output_file);
			reset_stack_pos(&expression_output, output_file);
			deallocate(expression_output.data);
			skip_whitespace(c);
			if(**c != ')'){
				snprintf(error_message, sizeof(error_message), "Expected '('");
				do_error(1);
			}
		}
		++*c;
		fprintf(output_file, "j __L%d\n\n__L%d:\n", label_num0, label_num2);
		skip_whitespace(c);
		compile_statement(c, output_file);
		fprintf(output_file, "j __L%d\n\n__L%d:\n", label_num3, label_num1);

		loop_variables_size = prev_loop_variables_size;
		break_label = prev_break_label;
		continue_label = prev_continue_label;
	} else if(!strncmp(*c, "return", 6) && !alphanumeric((*c)[6])){
		*c += 6;
		skip_whitespace(c);
		if(types_equal(&return_type, &VOID_TYPE)){
			if(**c != ';'){
				snprintf(error_message, sizeof(error_message), "Expected ';' for return in void function");
				do_error(1);
			}
			++*c;
		} else {
			compile_root_expression(&expression_output, c, 1, 0, output_file);
			if(**c != ';'){
				snprintf(error_message, sizeof(error_message), "Expected ';'");
				do_error(1);
			}
			++*c;
			cast(&expression_output, return_type, 1, output_file);
			reset_stack_pos(&expression_output, output_file);
			if(expression_output.data.type == data_register){
				fprintf(output_file, "sw $s%d, %d($sp)\n", expression_output.data.reg, variables_size);
			} else if(expression_output.data.type == data_stack){
				fprintf(output_file, "sw $t0, %d($sp)\n", variables_size);
			}
			deallocate(expression_output.data);
		}
		fprintf(output_file, "addi $sp, $sp, %d\n", variables_size - num_args*4);
		fprintf(output_file, "lw $ra, %d($sp)\n", 4 + num_args*4);
		fprintf(output_file, "jr $ra\n");
	} else if(!strncmp(*c, "break", 5) && !alphanumeric((*c)[5])){
		*c += 5;
		skip_whitespace(c);
		if(**c != ';'){
			snprintf(error_message, sizeof(error_message), "Expected ';' after break");
			do_error(1);
		}
		if(break_label < 0){
			snprintf(error_message, sizeof(error_message), "No loop to break out of");
			do_error(1);
		}
		if(variables_size != loop_variables_size)
			fprintf(output_file, "addi $sp, $sp, %d\n", variables_size - loop_variables_size);
		fprintf(output_file, "j __L%d\n", break_label);
	} else if(!strncmp(*c, "continue", 8) && !alphanumeric((*c)[8])){
		*c += 8;
		skip_whitespace(c);
		if(**c != ';'){
			snprintf(error_message, sizeof(error_message), "Expected ';' after continue");
			do_error(1);
		}
		if(continue_label < 0){
			snprintf(error_message, sizeof(error_message), "No loop to continue in");
			do_error(1);
		}
		if(variables_size != loop_variables_size)
			fprintf(output_file, "addi $sp, $sp, %d\n", variables_size - loop_variables_size);
		fprintf(output_file, "j __L%d\n", continue_label);
	} else if(**c == ';'){
		//Empty statement, so pass
		++*c;
	} else if(**c == '{'){
		++*c;
		compile_block(c, 0, output_file);
		if(**c != '}'){
			snprintf(error_message, sizeof(error_message), "Expected '}'");
			do_error(1);
		}
		++*c;
	} else {
		compile_root_expression(&expression_output, c, 1, 0, output_file);
		reset_stack_pos(&expression_output, output_file);
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
	unsigned char ignore_next = 0;
	char *beginning;

	beginning = *c;

	while(**c && (**c != '"' || ignore_next)){
		fprintf(output_file, "%c", **c);
		if(**c == '\\'){
			ignore_next = 1;
		} else {
			ignore_next = 0;
		}
		if(**c == '\n'){
			program_pointer = *c;
			snprintf(error_message, sizeof(error_message), "Expected closing '\"'");
			do_error(1);
		}
		++*c;
	}

	if(!**c){
		program_pointer = beginning;
		snprintf(error_message, sizeof(error_message), "Expected closing '\"'");
		do_error(1);
	}
	fprintf(output_file, "\"");
	++*c;
}

void compile_string_constants(char *c, FILE *output_file){
	unsigned char ignore;

	ignore = 0;
	skip_whitespace(&c);
	while(*c){
		if(ignore || *c != '"'){
			ignore = 0;
			if(*c == '\\' || *c == '\''){
				ignore = 1;
			} else if(*c == '\n'){
				current_line++;
			}
			c++;
		} else {
			c++;
			fprintf(output_file, "__str%d:\n", num_strs);
			fprintf(output_file, ".asciiz \"");
			place_string_constant(&c, output_file);
			fprintf(output_file, "\n");
			num_strs++;
		}
		skip_whitespace(&c);
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
	char *output_filename = NULL;

	if(argc < 2){
		printf("Minimal C Compiler for MIPS\nby Ben Jones\n2/29/2020\n");
		return 0;
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
	current_scope = -1;
	memset(local_variables, 0, sizeof(local_variables));
	while(*current_source_file){
		fp = fopen(*current_source_file, "rb");
		if(!fp){
			fprintf(stderr, "Could not open file '%s' for reading\n", *current_source_file);
			free_global_variables();
			exit(1);
		}
		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		rewind(fp);
		program = calloc(file_size + 1, sizeof(char));
		if(fread(program, sizeof(char), file_size, fp) < file_size){
			fclose(fp);
			fprintf(stderr, "Failed to read file '%s'\n", *current_source_file);
			free_global_variables();
			exit(1);
		}
		fclose(fp);
		initialize_register_list();
		compile_file(program, identifier_name, arguments, 32, 32, output_file);
		free(program);
		current_source_file++;
	}
	fclose(output_file);
	free_global_variables();

	return 0;
}
