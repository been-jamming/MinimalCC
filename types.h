#include <stdint.h>
#define MAX_LISTS 8

typedef enum {
	type_none = 0,
	type_void = 1,
	type_int = 2,
	type_char = 3,
	type_pointer = 4,
	type_function = 5,
	type_returns = 6,
	type_list = 7} type_entry;

typedef struct {
	uint64_t d0;
	uint64_t d1;
	uint64_t d2;
	unsigned int list_indicies[MAX_LISTS];
	unsigned int current_index;
} type;

unsigned char alpha(char c);
unsigned char alphanumeric(char c);
unsigned char digit(char c);
void add_type_entry(type *t, type_entry entry);
unsigned char is_whitespace(char c);
void skip_whitespace(char **c);
int peek_type(type t);
int pop_type(type *t);
unsigned char types_equal(type t0, type t1);
unsigned char parse_datatype(type *t, char **c);
type get_argument_type(type *t);
void print_type(type t);
void parse_type(type *t, char **c, char *identifier_name, char *argument_names, unsigned int identifier_length, unsigned int num_arguments);
extern const type INT_TYPE;
extern const type CHAR_TYPE;
extern const type VOID_TYPE;
extern const type EMPTY_TYPE;
