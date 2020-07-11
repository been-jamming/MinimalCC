/*
Brainfuck interpreter
By Ben Jones
Written for MCC
3/04/2020
*/

void inputs(char *, int);
void printc(char);
char inputc();
void prints(char *);

char input_program[200000];
char data[200000];
char *data_pointer;

void initialize(){
	int i;

	i = 0;
	while(i < 200000){
		data[i] = 0;
		input_program[i] = 0;
		i = i + 1;
	}
	data_pointer = data + 100000;
}

char *match_bracket(char *program){
	int depth;

	depth = 0;
	while(depth || *program != ']'){
		if(*program == '['){
			depth = depth + 1;
		}
		if(*program == ']'){
			depth = depth - 1;
		}
		program = program + 1;
	}
	return program;
}

void interpret(char *program){
	char *next_bracket;

	while(*program && *program != ']'){
		if(*program == '>'){
			data_pointer = data_pointer + 1;
		} else if(*program == '<'){
			data_pointer = data_pointer - 1;
		} else if(*program == '+'){
			*data_pointer = *data_pointer + 1;
		} else if(*program == '-'){
			*data_pointer = *data_pointer - 1;
		} else if(*program == '.'){
			printc(*data_pointer);
		} else if(*program == ','){
			*data_pointer = inputc();
		} else if(*program == '['){
			program = program + 1;
			next_bracket = match_bracket(program);
			while(*data_pointer){
				interpret(program);
			}
			program = next_bracket;
		}
		program = program + 1;
	}
}

void entry(){
	initialize();
	prints("Enter program:");
	inputs(input_program, 20000);
	interpret(input_program);
}
