void **_kmalloc_heap;
void **_kmalloc_end;
int INT_SIZE;
int POINTER_SIZE;
int CHAR_SIZE;

void prints(char *);
void printd(int);

void kmalloc_init(void *heap, int heap_size){
	_kmalloc_heap = heap;
	_kmalloc_end = (void **) heap + (heap_size>>2) - 3;
	_kmalloc_heap[0] = (void *) 0;
	_kmalloc_heap[1] = _kmalloc_end;
	_kmalloc_heap[2] = (void *) 0;
	_kmalloc_end[0] = _kmalloc_heap;
	_kmalloc_end[1] = (void *) 0;
	_kmalloc_end[2] = (void *) 1;

	INT_SIZE = 4;
	POINTER_SIZE = 4;
	CHAR_SIZE = 4;
}

void _kmalloc_merge(void **block){
	void **next_block;

	if(!(int) block[2]){
		while(!(int) ((void **) block[1])[2]){
			next_block = block[1];
			block[1] = next_block[1];
			((void **) block[1])[0] = block;
		}
	}
}

void *kmalloc(int size){
	void **block;
	void **new_block;

	if(size&3)
		size = (size>>2) + 1;
	else
		size = size>>2;

	block = _kmalloc_heap;

	while((int) block[1]){
		if(!(int) block[2] && (void **) block[1] - block - 6 >= size){
			new_block = block + size + 3;
			new_block[0] = block;
			new_block[1] = block[1];
			new_block[2] = (void *) 0;

			block[1] = new_block;
			block[2] = (void *) 1;

			return block + 3;
		}

		block = block[1];
	}

	return (void *) 0;
}

void kfree(void *p){
	void **block;

	block = (void **) p - 3;
	block[2] = (void *) 0;
}

