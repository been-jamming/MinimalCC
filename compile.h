void compile_function(char **c, char *identifier_name, char *arguments, unsigned int identifier_length, unsigned int num_arguments);

void compile_block(char **c, unsigned char do_variable_initializers);

void compile_statement(char **c);

void compile_string_constants(char *c);

void place_string_constant(char **c);
