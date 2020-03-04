#include "dictionary.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

dictionary create_dictionary(void *value){
	dictionary output = (dictionary) {.value = value};
	memset(output.next_chars, 0, sizeof(dictionary *)*8);
	return output;
}

static void free_dictionary_recursive(dictionary *dict, void (*free_value)(void *)){
	unsigned char i;

	for(i = 0; i < 8; i++){
		if(dict->next_chars[i]){
			free_dictionary_recursive(dict->next_chars[i], free_value);
			free(dict->next_chars[i]);
		}
	}

	if(dict->value){
		free_value(dict->value);
	}
}

void free_dictionary(dictionary dict, void (*free_value)(void *)){
	unsigned char i;

	for(i = 0; i < 8; i++){
		if(dict.next_chars[i]){
			free_dictionary_recursive(dict.next_chars[i], free_value);
			free(dict.next_chars[i]);
		}
	}

	if(dict.value){
		free_value(dict.value);
	}
}

void *read_dictionary(dictionary dict, char *string, unsigned char offset){
	unsigned char zeros = 0;
	unsigned char c;

	while(*string){
		c = (*string)>>offset | (*(string + 1))<<(8 - offset);
		zeros = 0;

		if(!(c&15)){
			zeros += 4;
			c>>=4;
		}
		if(!(c&3)){
			zeros += 2;
			c>>=2;
		}
		if(!(c&1)){
			zeros++;
		}

		offset += zeros + 1;
		string += (offset&0x08)>>3;
		offset = offset&0x07;
		if(dict.next_chars[zeros]){
			dict = *(dict.next_chars[zeros]);
		} else {
			return (void *) 0;
		}
	}
	
	return dict.value;
}


void write_dictionary(dictionary *dict, char *string, void *value, unsigned char offset){
	unsigned char zeros = 0;
	unsigned char c;

	while(*string){
		c = (*string)>>offset | (*(string + 1))<<(8 - offset);
		zeros = 0;

		if(!(c&15)){
			zeros += 4;
			c>>=4;
		}
		if(!(c&3)){
			zeros += 2;
			c>>=2;
		}
		if(!(c&1)){
			zeros++;
		}

		offset += zeros + 1;
		string += (offset&0x08)>>3;
		offset = offset&0x07;
		if(!dict->next_chars[zeros]){
			dict->next_chars[zeros] = malloc(sizeof(dictionary));
			*(dict->next_chars[zeros]) = create_dictionary((void *) 0);
		}
		dict = dict->next_chars[zeros];
	}
	
	dict->value = value;
}

void iterate_dictionary(dictionary dict, void (*func)(void *)){
	unsigned char i;
	
	if(dict.value){
		func(dict.value);
	}

	for(i = 0; i < 8; i++){
		if(dict.next_chars[i]){
			iterate_dictionary(*dict.next_chars[i], func);
		}
	}
}

