int inputd();
void printd(int);

int factorial(int n);

void entry(){
	printd(factorial(inputd()));
}

int factorial(int n){
	if(n > 1){
		return n*factorial(n - 1);
	}
	return 1;
}
