int inputd();
void printd(int);
void printc(char);

void guessing_game(int secret_integer){
	int current_guess;

	current_guess = -1;
	while((current_guess == secret_integer) == 0){
		current_guess = inputd();
		if(current_guess > secret_integer){
			printc(118);
		}
		if(current_guess < secret_integer){
			printc(94);
		}
		printc(10);
	}
	printc(61);
	printc(10);
}
