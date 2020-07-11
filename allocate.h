#ifndef ALLOCATE_INCLUDED

#define ALLOCATE_INCLUDED
#define REGISTER_SIZE 4
#define NUM_REGISTERS 8

typedef enum{
	data_register,
	data_stack
} data_entry_type;

typedef struct reg_list reg_list;

struct reg_list{
	unsigned int num_allocated;
	unsigned char registers[NUM_REGISTERS];
	unsigned char positions[NUM_REGISTERS];
};

typedef struct data_entry data_entry;

struct data_entry{
	data_entry_type type;
	union{
		int reg;
		int stack_pos;
	};
	unsigned int prev_stack_size;
};

extern int stack_size;
extern int variables_size;
extern int saved_stack_size;
extern reg_list register_list;

void initialize_register_list();
data_entry allocate(unsigned char force_stack);
void deallocate(data_entry entry);
int get_stack_pos(data_entry entry);
reg_list push_registers(FILE *output_file);
int get_reg_stack_pos(reg_list regs, unsigned char reg);
void pull_registers(reg_list regs, FILE *output_file);

#endif
