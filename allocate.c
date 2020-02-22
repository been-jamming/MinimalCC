#include <stdio.h>
#include "allocate.h"

unsigned int stack_size = 0;
unsigned int variables_size = 0;

void initialize_register_list(){
	unsigned int i;

	register_list.num_allocated = 0;

	for(i = 0; i < NUM_REGISTERS; i++){
		register_list.registers[i] = i;
		register_list.positions[i] = i;
	}
}

data_entry allocate(unsigned int num_bytes, unsigned char force_stack, unsigned int alignment){
	data_entry output;
	unsigned int remainder;

	if(force_stack || num_bytes > REGISTER_SIZE || register_list.num_allocated >= NUM_REGISTERS){
		output.type = data_stack;
		output.prev_stack_size = stack_size;
		remainder = stack_size%alignment;
		if(!remainder){
			output.stack_pos = stack_size;
			stack_size += num_bytes;
		} else {
			output.stack_pos = stack_size + alignment - remainder;
			stack_size += alignment - remainder + num_bytes;
		}
	} else {
		output.type = data_register;
		output.reg = register_list.num_allocated;
		register_list.num_allocated++;
	}

	output.num_bytes = num_bytes;
	return output;
}

//Note that if we are allocating on the stack, block allocated last should be deallocated first
void deallocate(data_entry entry){
	unsigned int temp;

	if(entry.type == data_register){
		register_list.num_allocated--;
		temp = register_list.registers[register_list.num_allocated];
		register_list.registers[register_list.num_allocated] = entry.reg;
		register_list.registers[register_list.positions[entry.reg]] = temp;
		register_list.positions[temp] = register_list.positions[entry.reg];
		register_list.positions[entry.reg] = register_list.num_allocated;
	} else if(entry.type == data_stack){
		stack_size = entry.prev_stack_size;
	}
}

static void store_register(unsigned char reg){
	printf("sw $s%d, %d($sp)\n", (int) reg, -(int) stack_size);
	stack_size += 4;
}

reg_list push_registers(){
	reg_list output;
	int i;

	output = register_list;
	for(i = register_list.num_allocated - 1; i >= 0; i--){
		store_register(register_list.registers[i]);
	}
	register_list.num_allocated = 0;

	return output;
}

int get_reg_stack_pos(reg_list regs, unsigned char reg){
	int index;

	index = regs.positions[reg];
	return stack_size - REGISTER_SIZE*(index + 1);
}

static void load_register(unsigned char reg){
	stack_size -= 4;
	printf("lw $s%d, %d($sp)\n", (int) reg, -(int) stack_size);
}

void pull_registers(reg_list regs){
	unsigned int i;

	for(i = 0; i < regs.num_allocated; i++){
		load_register(regs.registers[i]);
	}

	register_list = regs;
}
