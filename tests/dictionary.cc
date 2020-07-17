void *kmalloc(int);
void kfree(void *);

void **create_dictionary(void *value){
	void **output;
	int i;

	output = (void **) kmalloc(POINTER_SIZE*17);
	for(i = 0; i < 16; i = i + 1)
		output[i] = (void *) 0;
	output[16] = value;

	return output;
}

void free_dictionary(void **dict, void (*free_value)(void *)){
	int i;

	for(i = 0; i < 16; i = i + 1)
		if((int) dict[i])
			free_dictionary((void **) dict[i], free_value);
	
	if((int) dict[16])
		free_value(dict[16]);
	kfree(dict);
}

void *read_dictionary(void **dict, char *string, char offset){
	int index;

	if(!*string)
		return dict[16];
	
	if(!offset){
		index = (*string&0xF0)>>4;
		if(!(int) dict[index])
			dict[index] = create_dictionary((void *) 0);
		return read_dictionary(dict[index], string, 1);
	} else {
		index = *string&0x0F;
		if(!(int) dict[index])
			dict[index] = create_dictionary((void *) 0);
		return read_dictionary(dict[index], string + 1, 0);
	}
}

void write_dictionary(void **dict, char *string, void *value, char offset){
	int index;

	if(!*string){
		dict[16] = value;
		return;
	}
	
	if(!offset){
		index = (*string&0xF0)>>4;
		if(!(int) dict[index])
			dict[index] = create_dictionary((void *) 0);
		write_dictionary(dict[index], string, value, 1);
	} else {
		index = *string&0x0F;
		if(!(int) dict[index])
			dict[index] = create_dictionary((void *) 0);
		write_dictionary(dict[index], string + 1, value, 0);
	}
}
