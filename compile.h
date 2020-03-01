#ifndef MCC_INCLUDE_COMPILE
#define MCC_INCLUDE_COMPILE
void compile_function(char **c, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments, FILE *output_file);

void compile_block(char **c, unsigned char do_variable_initializers, FILE *output_file);

void compile_statement(char **c, FILE *output_file);

void compile_string_constants(char *c, FILE *output_file);

void place_string_constant(char **c, FILE *output_file);

void do_error(int status);

void do_warning();

extern int current_line;
extern char warning_message[256];
extern char error_message[256];
#endif
