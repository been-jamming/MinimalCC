#include <stdio.h>
#include "allocate.h"

unsigned int stack_size = 0;
unsigned int variables_size = 0;
reg_list register_list;

void initialize_register_list(){
	unsigned int i;

	register_list.num_allocated = 0;

	for(i = 0; i < NUM_REGISTERS; i++){
		register_list.registers[i] = i;
		register_list.positions[i] = i;
	}
}

//For now, it's impossible to allocate anything but 4 bytes on the stack
//This is because I need to be able to cast values which are on the stack
//Doing so without everything aligned on the stack would require me to
//Know in advance if I need to give space around a value on the stack to
//allow it to be cast and replaced in the same spot.
data_entry allocate(unsigned char force_stack){
	data_entry output;

	if(force_stack || register_list.num_allocated >= NUM_REGISTERS){
		output.type = data_stack;
		output.prev_stack_size = stack_size;
		output.stack_pos = stack_size;
		stack_size += 4;
	} else {
		output.type = data_register;
		output.reg = register_list.num_allocated;
		register_list.num_allocated++;
	}

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

static void store_register(unsigned char reg, FILE *output_file){
	fprintf(output_file, "\tmove.l d%d, %d(a7)\n", (int) reg, -(int) stack_size);
	stack_size += 4;
}

reg_list push_registers(FILE *output_file){
	reg_list output;
	int i;

	output = register_list;
	for(i = register_list.num_allocated - 1; i >= 0; i--){
		store_register(register_list.registers[i], output_file);
	}
	register_list.num_allocated = 0;

	return output;
}

int get_reg_stack_pos(reg_list regs, unsigned char reg){
	int index;

	index = regs.positions[reg];
	return stack_size - REGISTER_SIZE*(index + 1);
}

static void load_register(unsigned char reg, FILE *output_file){
	stack_size -= 4;
	fprintf(output_file, "\tmove.l %d(a7), d%d\n", -(int) stack_size, (int) reg);
}

void pull_registers(reg_list regs, FILE *output_file){
	unsigned int i;

	for(i = 0; i < regs.num_allocated; i++){
		load_register(regs.registers[i], output_file);
	}

	register_list = regs;
}
