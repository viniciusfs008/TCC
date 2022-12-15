


unsigned int str_len(char *string){
	unsigned int counter = 0;
	while (*(string++)) counter++;
	return counter;
}

unsigned int str_number_of_lines(char *string){
	unsigned int counter = 0;
	while (*string){
		if (*string == '\n'){
			counter++;
		}
		string++;
	}
	return counter + 1;
}


void swap(int *a, int *b){
	int temp = *a;
	*a = *b;
	*b = temp;
}