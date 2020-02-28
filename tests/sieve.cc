void printd(int);
void prints(char *);

void entry(){
	int ints[5000];
	int  i;
	int j;

	i = 2;
	while(i < 5000){
		ints[i] = 1;
		i = i + 1;
	}
	i = 2;
	j = 0;
	while(i < 5000){
		if(ints[i]){
			j = i + 1;
			while(j < 5000){
				if(!(j%i))
					ints[j] = 0;
				j = j + 1;
			}
		}
		i = i + 1;
	}

	i = 2;
	while(i < 5000){
		if(ints[i]){
			printd(i);
			prints(" is prime!\n");
		}
		i = i + 1;
	}
}
