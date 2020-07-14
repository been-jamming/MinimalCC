void **_kmalloc_heap;
void **_kmalloc_end;
int INT_SIZE;
int POINTER_SIZE;
int CHAR_SIZE;

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

void *kmalloc(int size){
	void **block;
	void **new_block;
	int block_size;

	if(size&3)
		size = (size>>2) + 1;
	else
		size = size>>2;

	block = _kmalloc_heap;

	while((int) block[1]){
		if(!(int) block[2] && (void **) block[1] - block - 3 >= size){
			block[2] = (void *) 1;
			new_block = block + 3 + size;
			new_block[0] = block;
			new_block[1] = block[1];
			new_block[2] = (void *) 0;
			block[1] = new_block;
			((void **) new_block[1])[0] = new_block;

			return block + 3;
		}
		block = block[1];
	}

	return (void *) 0;
}

void _kmalloc_merge_up(void **block){
	while((int) block[0] && !(int) ((void **) block[0])[2]){
		((void **) block[0])[1] = block[1];
		block = block[0];
	}
}

void _kmalloc_merge_down(void **block){
	while((int) block[1] < (int) _kmalloc_end && !(int) ((void **) block[1])[2])
		block[1] = ((void **) block[1])[1];
}

void kfree(void *p){
	void **block;

	block = (void **) p - 3;
	block[2] = (void *) 0;
	_kmalloc_merge_down(block);
	_kmalloc_merge_up(block);
}
