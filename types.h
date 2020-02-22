#include <stdint.h>

typedef enum {
	type_none = 0,
	type_void = 1,
	type_int = 2,
	type_char = 3,
	type_pointer = 4,
	type_function = 5,
	type_returns = 6} type_entry;

typedef struct {
	uint64_t d0;
	uint64_t d1;
	uint64_t d2;
} type;

unsigned char alpha(char c);
unsigned char alphanumeric(char c);
unsigned char digit(char c);
void add_type_entry(type *t, type_entry entry);
void skip_whitespace(char **c);
int peek_type(type t);
int pop_type(type *t);
unsigned char types_equal(type t0, type t1);
unsigned char parse_datatype(type *t, char **c);
type get_argument_type(type *t);
void print_type(type t);
void parse_type(type *t, char **c, char *identifier_name, char *argument_names, unsigned int identifier_length, unsigned int num_arguments);
extern type INT_TYPE;
extern type CHAR_TYPE;
extern type VOID_TYPE;
extern type EMPTY_TYPE;
