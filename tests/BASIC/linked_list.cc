void *create_linked_list(void *value){
	void *output;

	output = kmalloc(POINTER_SIZE*3);
	((void **) output)[0] = value;
	((void **) output)[1] = (void *) 0;
	((void **) output)[2] = (void *) 0;

	return output;
}

void free_linked_list(void *list, void (*free_value)(void *)){
	void *next_list;

	while((int) list){
		free_value(((void **) list)[0]);
		next_list = ((void **) list)[1];
		kfree(list);
		list = next_list;
	}
}

void *get_list_value(void *list){
	return ((void **) list)[0];
}

void *next_list(void *list){
	return ((void **) list)[1];
}

void *previous_list(void *list){
	return ((void **) list)[2];
}

void add_value(void *list, void *value){
	void *new_list;
	void *temp;

	new_list = create_linked_list(value);
	temp = ((void **) list)[1];
	((void **) list)[1] = new_list;
	((void **) new_list)[1] = temp;
	((void **) new_list)[2] = list;
	if((int) temp)
		((void **) temp)[2] = new_list;
}

void remove_value(void *list, void (*free_value)(void *)){
	void *next;
	void *prev;

	free_value(((void **) list)[0]);
	next = ((void **) list)[1];
	prev = ((void **) list)[2];
	kfree(list);
	if((int) next)
		((void **) next)[2] = prev;
	if((int) prev)
		((void **) prev)[1] = next;
}
