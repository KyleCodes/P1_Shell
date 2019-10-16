# sshell.c by Nikita Andrikanis and Kyle Muldoon


# Headers
```c
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
```
The headers included allow for many key functions of the program. Most importantly wait(), fork(), string parsing, and definitions for opening files and permissions (#include <fcntl.h>).

# Function Prototypes
```c
char* userInput();
```
The only function call made by main is to get user input. The function is as follows:
```c
char* userInput() {
    //static char input[512];
    char *input = malloc(512);
    printf("sshell$ ");
    fgets(input, 512, stdin);
    return input;
```

# main: First Block
#### Note: The rest of the code is located within a while loop.
```c
// Get userInput
char inputstr[512];
strcpy(inputstr, userInput());


// Special Case: check if there's nothing or whitespace
// Reference: https://stackoverflow.com/questions/3981510/getline-check-if-line-is-whitespace
if (strspn(inputstr, " \r\n\t") == strlen(inputstr)) {
	memset(inputstr, 0, sizeof(inputstr));
	continue;
}
```
Aside from calling the function to display the "sshell$", the first special error case is handled. The if condition used checks for any amount of white space found in the input string, as well as making sure there is ONLY white space. If that is the case, the loop is immediately returned to the top and memory is reset for the input string.

We referenced the following for the if-condition technique: https://stackoverflow.com/questions/3981510/getline-check-if-line-is-whitespace

# main: Parsing Begins
```c
// Preserves inputstr for completed message
        char inputstrCOPY[512];

        // Tester Code Snippet
	    if (!isatty(STDIN_FILENO)) {
	        printf("%s", inputstr);
	        fflush(stdout);
	    }
```
The above preserves the input string to later print with the '+completed ...' message at the end of the loop. Additionally, this block includes the required tester code snippet, as specified by the Tester's instructions.

```c
strcpy(inputstrCOPY, inputstr);
        int k = 0;
        while (1) {
        	if (inputstrCOPY[k] == '\n') {
        		inputstrCOPY[k] = 0;
        		break;
        	}
        	k++;
        }
```
We then used this loop to remove the newline character at the end of the preserved input string for the sake of '+completed' printing at the end.

```c
char *temp;
temp = strtok(inputstr, " ");

char parsedInput[16][512];
int index = 0; // ALSO TRACKS ARRAY LENGTH
while (temp != NULL) {
	strcpy(parsedInput[index], temp);
	temp = strtok(NULL, " ");
	index++;
}
```
In this block, the input string is tokenized and each individual argument (whether it's correct or not) is stored in an array of strings. This will allow for iteration over it, analyzing its contents, and identifying the different parts of the user's input. The integer "index" also stores the array length (total amount of arguments). 

```c
// Special Error Case: too many process arguments
if (index >= 16) {
     fprintf(stderr, "Error: too many process arguments\n");
     memset(inputstr, 0, sizeof(inputstr));

     for (int h = 0; h < 16; h++) {
        	memset(parsedInput[h], 0, sizeof(parsedInput[h]));
     }
     continue;
}
```
Using the "index" variable, we check for the special error case of having more than 16 arguments. If true, then the appropriate message is printed to stderr and the used strings are cleared and their memory reset. "continue" returns us to the top of the loop.

```c
parsedInput[index - 1][strlen(parsedInput[index - 1]) - 1] = 0;
```
The above snippet removes the newline character for ease of parsing and use of '\0' (null terminator) character in comparisons, as will be seen shortly.

# main: Flag Identification and Storage
```c
// Find command1
char *command1 = malloc(sizeof(512));
strcpy(command1, parsedInput[0]);
```
The above assumes that the first argument passed is the command (i.e. cat, grep, cd). The error case for this will be handled more explicitly after forking and executing through direct comparisons.

Flag parsing is split into 3 parts...
```c
// Find flags1 and rebuild them
int i = 1;
char *flags1_BUFFER = malloc(sizeof(512));
strcpy(flags1_BUFFER, "");
while((char)parsedInput[i][0] != '\0') { // ORIGINALLY while(parsedInput[i][0] != NULL) ##############
    if (parsedInput[i][0] == '-') {
        strcat(flags1_BUFFER, parsedInput[i]);
    }
    else if (parsedInput[i][0] == '|') {
        break;
    }
    i++;
}
```
The first block (above) creates a buffer for the flags belonging to the first command (as in, before the first pipe, if applicable). The while loop then iterates over the input string and identifies all flags, loading them into our flag buffer. The loop ends at a null terminator or a pipe.
```c
char *tempFlags1 = malloc(sizeof(512));
tempFlags1 = strtok(flags1_BUFFER, "-");
char *flags1 = malloc(sizeof(512));
strcpy(flags1, "");
if (tempFlags1 != NULL) {
    strcat(flags1, "-");
}
```
This next block then essentially removes all the "-" flag indicators and concatenates the flags into a single argument. This allows for the user to place flags anywhere in their input, for example:
```
sshell$ ls -a /someDirectory/ -n
```
This eases the process of parsing any arguments other than flags, as well as input/output redirection, as will be seen later.
```c
index = 1; // Skip over command and begin at first flag
while (tempFlags1 != NULL) {
    strcat(flags1, tempFlags1);
    tempFlags1 = strtok(NULL, "-");
    index++;
}
```
The third block then appends a "-" flag indicator for use by the execvp function. At this point, all flags are identified and stored, and we can move onto command argument parsing.

# main: Command Argument Identification and Storage
```c
// PARSING FOR INPUT1 -> note, this will include < or > for input/output redirection
//            NOTE! This will also parse until either hitting NULL terminator OR a PIPE, toggling pipe1
int parseIndex = 1; // Start after first command again...
int pipe1 = 0;

char *input1 = malloc(sizeof(512));
strcpy(input1, "");
while((char)parsedInput[parseIndex][0] != '|' && (char)parsedInput[parseIndex][0] != '\0') { // ORIGINALLY NULL instead of '\0'
    if (parsedInput[parseIndex][0] != '-' && parsedInput[parseIndex][0] != '|') {
        strcat(input1, parsedInput[parseIndex]);
    }

    if (parsedInput[parseIndex + 1][0] == '|') {
        pipe1 = 1;
        break;
    }

    parseIndex++;
    if (parsedInput[parseIndex + 1][0] != '-' && parsedInput[parseIndex + 1][0] != '|') {
        strcat(input1, " "); // Adds a space in between arguments, but makes sure doesn't end on a space
    }
}
```
As seen by the "parseIndex" being initialized as 1, we begin parsing after the command. The while loop iterates over the array of arguments, ignoring flags and stopping at either a pipe or the end of line (null terminator). This code combines a potentially separated string back into one, storing it as "input1". This is done so that arbitrary flags do not get in the way of parsing for input/output redirection, file names, and strings used by functions like "grep" or "echo", as well as separating the first command from anything following a potential pipe.
```c
int command2_START_INDEX = parseIndex + 2;
if (pipe1 == 1) {
}
```
The above code snippet is essentially dead code, as piping wasn't fully implemented. Thus, it can be ignored.

```c
if (strcmp(flags1, "") == 0) {
      flags1 = NULL;
}
if (strcmp(input1, "") == 0 || strcmp(input1, " ") == 0) {
      input1 = NULL;
}
```
The next code block clears the flags and input strings if there weren't any provided. This was done in order to satisfy the execvp function, which fails when one of the arguments in the passed "args" array contains a space ("") rather than being entirely empty.

# main: Input/Output Redirection Parsing

```c
char outputToFile[20] = "";
char LHS[20] = "";
char *LHS_clean = malloc(sizeof(512));
int outputFileExists = 0;

char inputFromFile[20] = "";
int inputFileExists = 0;
```
```c
bool contProcess = false;
char temp_input1[20] = "";
char IO;
strcpy(temp_input1, input1);
int splitIndex = 0;
```
The above blocks initialize relevant strings and integers that will be used throughout the process of parsing for input/output redirection. This is done in an outer scope to allow for interpretation later in the program, such as identifying whether there is valid input/output redirection and whether a function is specified following the > or < symbols.

```c
if (input1 != NULL) {
```
The above snippet simply means that if there isn't input to check for input/output redirection, the chunk will be skipped, saving valuable time of execution and improving program response time.
```c
for (splitIndex = 0; splitIndex < strlen(temp_input1); splitIndex++) {
	if (temp_input1[splitIndex] == '<' || temp_input1[splitIndex] == '>') {
		contProcess = true;
		IO = temp_input1[splitIndex];
		break;
	}
}
```
This first for loop identifies whether the input or output redirection must be done, storing the relevant symbol in "IO". If one of either < or > is located, a boolean is toggled to resume with parsing the left hand side and the right hand side of the redirection symbol (will be referred to as LHS and RHS from now on).

```c

```
If the boolean is true, then we determine whether we need input redirection or output redirection. The processes are similar, as the input/output file will always be to the RHS of the symbol. However, they are seperated to allow for specific error handling. Additionally, different strings are used to store the input/output files, as well as LHS arguments (i.e. toto or 'Hello').

To better explain what is going on within these blocks, we will look at the output redirection case only (as both are similar).

```c
int u = 0;
int splitIndexCopy = splitIndex;
// Thus, output file will be to the RHS of >...
// ORIGINALLY NULL instead of '\0' below for the sake of warning handling
while (temp_input1[splitIndex + 1] != '\0') { // We can assume that the only thing given here is a file
	if (temp_input1[splitIndex + 1] != ' ') {
		outputToFile[u] = temp_input1[splitIndex + 1];
		u++;
	}
	splitIndex++;
}
```
The first block (above) identifies and stores the output file name (RHS) in "outputToFile", acccounting for any amount of white space (or the lack thereof) through a simple if statement. The splitIndex is kept to later allow for parsing the LHS as well.
```c
int q = 0;
int a = 0;
while (a != splitIndexCopy) { // We can assume that the only thing given here is a file
	if (temp_input1[0] != ' ') {
		LHS[q] = temp_input1[a];
		q++;
	}
	a++;
}

// Clear Whitespace in LHS
LHS_clean = strtok(LHS, " ");
```
The next block then iterates from the beginning of the string, rather than the redirect symbol, identifying and storing the LHS. This will allow us to separate what goes into execvp (the LHS arguments) and what will be handled through file descriptors and redirection.

```c
// Replace input1 with LHS_clean
if (LHS_clean == NULL) {
	//Then there is no output file
	fprintf(stderr, "Error: no output file\n");

	// Clean strings for next loop early b/c we will skip to top
	free(command1);
	free(flags1);
	free(input1);
	free(flags1_BUFFER);
	free(tempFlags1);
	memset(inputstr, 0, sizeof(inputstr));
	free(LHS_clean);
	
	for (int h = 0; h < 16; h++) {
		memset(parsedInput[h], 0, sizeof(parsedInput[h]));
	}

	continue;
}


strcpy(input1, LHS_clean);
outputFileExists = 1;
```
The final block then checks if a LHS exists, printing a special error if there isn't. Note that there is a mistake in the naming convention, as the file is always to the RHS of the redirect symbol.
The final two lines of the block above are important, as input1 is rewritten to now EXCLUDE anything that must be handled by input/output redirection SEPARATELY from execvp. By this point in the program, all flags, arguments, and their command are identified and stored, allowing for assignment to the args array (which will then be passed into execvp).

# Constructing the args array...
```c
char **inn;
inn = malloc(16*sizeof(char*));
for (int z = 0; z < 16; z++) {
	inn[z] = malloc(512*sizeof(char));
}        

int inputArgs = 0;
char *input1_clean;
input1_clean = strtok(input1, " ");
int t = 0;
while (input1_clean != NULL) {
	strcpy(inn[t], input1_clean);
	t++;
	input1_clean = strtok(NULL, " ");
	inputArgs++;
}
free(input1_clean);
```
The char double pointer "inn" serves the purpose of loading all of the data we've processed up to this point. This will be used as the queue for loading CLEANED (i.e. no white space and newline characters) and REQUIRED arguments into the args array in the following block:
```c
char **args;
args = malloc(16*sizeof(char*));
for (int w = 0; w < 16; w++) {
	args[w] = malloc(512*sizeof(char));
}
```
Originally, we had created a struct to store the args array of strings. However, when testing in the CSIF, we encountered Segmentation Faults and Memory Allocation issues, forcing us to use what is seen in the above block. This way, no memory leaks were encountered in the CSIF. 
```c
strcpy(args[0], command1);

int iterator = 0;
if (flags1 == NULL && inn[0] == NULL) {
	args[1] = NULL;        	
}
else if (flags1 == NULL && inn[0] != NULL) {
	while (iterator != inputArgs) {
		args[iterator + 1] = inn[iterator];
		iterator++;
	}

	args[iterator + 1] = NULL;        	
}
else if (flags1 != NULL && inn[0] != NULL) {
	args[1] = flags1;
	while (iterator != inputArgs) {
		args[iterator + 2] = inn[iterator];
		iterator++;
	}


	args[iterator + 2] = NULL;
	
}
else if(flags1 != NULL && inn[0] == NULL) {
	args[1] = flags1;
	args[2] = NULL;
}
```
In the next block, we check for if the parsed data even exists, passing only required data to execvp (which otherwise malfunctions when given an array with empty strings). This was done after encountering "Bad Address" errors thrown by execvp.

# main: exit and cd
```c
if (strcmp(command1, "exit") == 0) {
	fprintf(stderr, "Bye...\n");
	exit(0);
}
```
The above segment merely exits from the entire program when given "exit". The message is printed to stderr as specified.
```c
char directory[100];
if (strcmp(command1, "cd") == 0) {
		getcwd(directory, 100);
		if (chdir(inn[0]) != 0) {
			fprintf(stderr, "Error: no such directory\n");
			fprintf(stderr, "+ completed '%s' [%d]\n", inputstrCOPY, 1);
		}
		else {
			fprintf(stderr, "+ completed '%s' [%d]\n", inputstrCOPY, 0);
		}
		
	// Clear all strings bc wont reach the end block of while loop due to "continue"
	...
    }

	continue;

	}
```
The "cd" command is handled through the "getcwd()" and "chdir()" system calls. The error management is done within this block as the error message for not finding the required directory is unique. The only passed argument is inn[0], which holds the directory specified to the user. This process does not require forking and thus the rest of the while loop is skipped. Memory is then freed and strings are reset, allowing for a clean return to the top of the loop.

# Forking
```c
int status;
pid_t pid;
pid = fork();
```
After initializing status and pid, we then perform a fork.
```c
wait(&status);
```
The parent (when pid > 0) simply waits for the child, expecting its return code upon completion.

Things get more interesting in the child branch...
```c
if (outputFileExists == 1) {

	int fd = open(outputToFile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1) {
		fprintf(stderr, "Error: cannot open output file\n");
		close(fd);
		exit(333);
	}        		
	dup2(fd, 1);
	close(fd);
}
```
Before accessing the execvp function, we must first check for input/output redirection. This must be done first because the input must be redirected into the function from a file, or the output into a file.

In the block above, we first check if an output file exists, which was determined by the parsing block earlier in the program. If it does, we enter the branch. If the specified output file does not exist, it is created using the "open()" function above, with the 2 specified flags. The first one creates a new file if it doesn't exist, the next opens the output as WRITE ONLY. The last argument specifies the group permissions to write to the file.

If an error is not thrown, then dup2 is used in order to redirect output to the specified file.
```c
else if (inputFileExists == 1) {

	if (strcmp(inputFromFile, "") == 0) {
		fprintf(stderr, "Error: no input file\n");
		free(command1);
	free(flags1);
	free(input1);
	free(flags1_BUFFER);
	free(tempFlags1);
	memset(inputstr, 0, sizeof(inputstr));
	if (directory) {
		memset(directory, 0, sizeof(directory));
	}
	for (int h = 0; h < 16; h++) {
		memset(parsedInput[h], 0, sizeof(parsedInput[h]));
	}
	exit(0);
}


int fd = open(inputFromFile, O_RDONLY);
if (fd == -1) {
	fprintf(stderr, "Error: cannot open input file\n");
	close(fd);
	exit(444);
}

dup2(fd, 0);
close(fd);
}
```
The branch for input redirection is marginally more complex, as there are two potential errors that could be thrown. This include includes when there is no input file, as well as if it cannot open the input file. The reason that the lack of an input file (i.e. cat <) must be handled for input rather than output redirection is because the open() function for output has the flag O_CREAT, which creates a new file if it cannot be found.

```c
int status = execvp(args[0], args);
// If there's an error, then we will reach this line
exit(1);
```
The next block is the execvp call, which will only get cleaned and required arguments, avoiding input/output redirection and commands following a pipe, if applicable.

# Special Error Management and '+ completed' Messages
```c
int failedTest1 = 0;
char *testOptions[] = {"|", "<", ">", "&"};
int g = 0;
while(1) {
	if (g == 4)
		break;
	if (strcmp(command1, testOptions[g]) == 0) {
		failedTest1 = 1;
		break;
	}
	g++;
}

...

if (failedTest1) {
	fprintf(stderr, "Error: missing command");
}
```
The first block of this section deals with the error mentioned earlier in the code: missing command. As the case of a completely empty input has been dealt with at the start of the main loop, the first argument (command1) is checked against the 3 possible inputs following a command.
```c
if (inputFileExists == 1 && strcmp(inputFromFile, "") == 0) {
	continue;
}
if (outputFileExists == 1 && strcmp(outputToFile, "") == 0) {
	continue;
}
```
The above block is a bit confusing. The reason this is done is to skip any '+ completed' messages if they aren't required during a failed execution. These were specified in multiple examples in the Project 1 PDF. Most importantly, this is done to exactly match expected output.
```c
if (status == 256) {
	if (inputFileExists == 1 || outputFileExists == 1) {
		fprintf(stderr, "+ completed '%s' [%d]\n", inputstrCOPY, 1);
	}
	else {
		fprintf(stderr, "Error: command not found\n");
		fprintf(stderr, "+ completed '%s' [%d]\n", inputstrCOPY, 1);
	}
}
else if (status == 19712 || status == 48128) {
}
else
		fprintf(stderr, "+ completed '%s' [%d]\n", inputstrCOPY, status);
```
The rest of the block handles printing '+ completed' messages. The last "else if" statement is our workaround for avoiding a '+ completed' message, specifically in the input/output redirection branches of the child process. As a parent processes data cannot be altered by the child, this method allows us to interpret the reason for failure based on exit status: the only method of communication of execution information between parent and child.

# Clearing and Deallocating Strings
The final block of code simply serves the purpose of memory management and emptying strings to be reused by the next cycle of commands:
```c
free(command1);
free(flags1);
free(input1);
free(flags1_BUFFER);
free(tempFlags1);

memset(inputstr, 0, sizeof(inputstr));

if (directory) {
	memset(directory, 0, sizeof(directory));
}

for (int h = 0; h < 16; h++) {
	memset(parsedInput[h], 0, sizeof(parsedInput[h]));
}
```
