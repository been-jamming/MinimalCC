void **_kmalloc_heap;

void kmalloc_init(void *heap, int heap_size){
	//malloc doesn't have a free, so we don't need a de-init
	_kmalloc_heap = (void **) heap;
	_kmalloc_heap[0] = (void *) 0;
	_kmalloc_heap[1] = (void *) 0;
	((int *) _kmalloc_heap)[2] = heap_size;
	((int *) _kmalloc_heap)[3] = 1;
	return;
}

void *kmalloc(int size){
	void **current_block;
	void *next_block;
	void *previous_block;

	if(size&3)
		size = size + 4 - (size&3);
	current_block = _kmalloc_heap;
	while((int) current_block && (!((int *) current_block)[3] || size > ((int *) current_block)[2] - 32))
		current_block = (void **) *current_block;

	//If there is no space left on the heap
	if(!(int) current_block)
		return (void *) 0;

	next_block = current_block[0];
	previous_block = (void *) current_block;
	current_block = current_block + (size>>2) + 4;
	current_block[0] = next_block;
	current_block[1] = previous_block;
	((int *) current_block)[2] = ((int *) previous_block)[2] - 16 - size;
	((int *) current_block)[3] = 1;
	((void **) previous_block)[0] = (void *) current_block;
	((int *) previous_block)[2] = size + 16;
	((int *) previous_block)[3] = 0;

	return (void *) ((void **) previous_block + 4);
}

void _kmalloc_merge_left(void **block){
	while((int) block[1] && ((int *) block[1])[3]){
		((void **) block[1])[0] = block[0];
		((int *) block[1])[2] = ((int *) block[1])[2] + ((int *) block)[2];
		block = (void **) block[1];
	}
}

void _kmalloc_merge_right(void **block){
	while((int) block[0] && ((int *) block[0])[3]){
		((int *) block)[2] = ((int *) block[0])[2] + ((int *) block)[2];
		block[0] = ((void **) block[0])[0];
		if(block[0])
			((void **) block[0])[1] = block;
	}
}

void kfree(void *p){
	void **block;

	block = (void **) p - 4;
	((int *) block)[3] = 1;
	_kmalloc_merge_left(block);
	_kmalloc_merge_right(block);
}

