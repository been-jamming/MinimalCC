int inputd();
void printd(int);
void prints(char *c);

int factorial(int n);

void entry(){
	int n;

	prints("Enter n:\n");
	n = inputd();
	prints("n! = ");
	printd(factorial(n));
	prints("\n");
}

int factorial(int n){
	if(n > 1){
		return n*factorial(n - 1);
	}
	return 1;
}
