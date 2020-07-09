#include "allocate.h"
#include "types.h"

#define MAX_SCOPE 32

#define VOID_SIZE 1
#define CHAR_SIZE 1
#define INT_SIZE 4
#define POINTER_SIZE 4

enum operation{
	operation_none,
	operation_add,
	operation_subtract,
	operation_multiply,
	operation_divide,
	operation_assign,
	operation_less_than,
	operation_greater_than,
	operation_less_than_or_equal,
	operation_greater_than_or_equal,
	operation_equals,
	operation_not_equals,
	operation_and,
	operation_or,
	operation_modulo,
	operation_logical_and,
	operation_logical_or,
	operation_shift_left,
	operation_shift_right
};

typedef enum operation operation;

typedef struct variable variable;

struct variable{
	char *varname;
	type var_type;
	union{
		unsigned int stack_pos;
		struct{
			unsigned char leave_as_address;
			unsigned int num_args;
		};
	};
};

typedef struct value value;

struct value{
	data_entry data;
	type data_type;
	unsigned char is_reference;
};

extern dictionary *local_variables[MAX_SCOPE];
extern dictionary global_variables;
void free_local_variables();
void free_global_variables();
extern unsigned int num_labels;
int type_size(type *t, unsigned int start_entry);
void initialize_variables();
unsigned int align4(unsigned int size);
void compile_variable_initializer(char **c);
void cast(value *v, type t, unsigned char do_warn, FILE *output_file);
void compile_expression(value *first_value, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file);
void compile_root_expression(value *first_value, char **c, unsigned char dereference, unsigned char force_stack, FILE *output_file);
void reset_stack_pos(value *first_value, FILE *output_file);

extern unsigned int scopes[MAX_SCOPE];
extern int current_scope;
