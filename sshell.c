#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/* 	BREAKDOWN OF ABOVE ^^
main args: int argc, char*argv[]
create pointer to string containing command line, hardcoded.
declare return value
set return value equal to system return value from executing command
print success message and what return value was 
*/


// prototypes
char *parseInput();
void pPrompt();

// main fn
int main(int argc, char **argv)   // passes a vector of args to exec
{

	// VARIABLE DECLARATIONS
		pid_t pid;
		int status;
		char *cmd[3] = {"/bin/date", "-u", NULL};
		
		char *msg[512];
		msg;
		char *stat;

	// START OF LOGIC
		// 1) display prompt
		pPrompt();

		// 2) read command from input / parse arguments
		 = parseInput();
		

		// 3) execute



		// 4) wait for completion + display message




		return(0);
}

char* parseInput() {
	int BUFFSIZE = 512;
	char* input = malloc(BUFFSIZE);
	gets(input);

	// for loop, count # of spaces
	int numSpaces = 0;
	for (int i = 0; i < input.size(); i++) {
		if (input[i] == " ") {
			numSpaces++;
		}
	}

	// num of tokens = num of spaces + 1, of args = num of spaces.
	char* cmd[numSpaces + 1];
	// get name of command
	cmd[0] = strtok(input, " ");

	//get list of args
	for(int i = 0; i < input.length(); i++) {

	}


	while (token != null) {
		token = strtok(NULL, " ");
	}












	return input;
}

void pPrompt() {
	printf("sshell$ ");
}