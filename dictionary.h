#ifndef DICTIONARY_INCLUDED
#define DICTIONARY_INCLUDED
typedef struct dictionary dictionary;

struct dictionary{
	dictionary *next_chars[8];
	void *value;
};

dictionary create_dictionary(void *value);

void free_dictionary(dictionary dict, void (*free_value)(void *));

void *read_dictionary(dictionary dict, char *string, unsigned char offset);

void write_dictionary(dictionary *dict, char *string, void *value, unsigned char offset);

void iterate_dictionary(dictionary dict, void (*func)(void *));
#endif
