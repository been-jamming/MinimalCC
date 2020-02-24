void prints(char *);
int inputd();

void entry(){
	int random_seed;
	int secret_int;
	int guess;

	prints("Enter a rng seed: ");
	random_seed = inputd();
	secret_int = (134775813*random_seed + 1)%100;
	if(secret_int < 0)
		secret_int = secret_int + 100;
	guess = -1;
	prints("A secret integer between 0 and 99 has been chosen\n");
	while(guess != secret_int){
		prints("Enter guess: ");
		guess = inputd();
		if(guess < secret_int)
			prints("Guess is too low!\n");
		if(guess > secret_int)
			prints("Guess is too high!\n");
	}
	prints("You guessed the secret integer!\n");
}
