/*
Ben Jones
Calculator
Made for MCC compiler
3/04/2020
*/

void prints(char *);
int inputd();
void inputs(char *, int);
void printd(int);

void entry(){
	char quit;
	char error;
	int a;
	int b;
	int result;
	char operation[3];

	prints("Calculator compiled with MCC\n");
	quit = 0;
	while(!quit){
		error = 0;
		prints("\nEnter first number: ");
		a = inputd();
		prints("Enter second number: ");
		b = inputd();
		prints("Enter operation: ");
		inputs(operation, 3);
		if(operation[0] == '+'){
			result = a + b;
		} else if(operation[0] == '-'){
			result = a - b;
		} else if(operation[0] == '*'){
			result = a*b;
		} else if(operation[0] == '/'){
			result = a/b;
		} else {
			prints("Unknown operation");
			error = 1;
		}
		if(!error){
			prints("Result: ");
			printd(result);
		}
	}
}
