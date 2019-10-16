#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>

// NOTE: No longer in use due to CSIF complications.

/*
typedef struct Array {
	char **args;
} Array;
*/

// Function Prototypes
char* userInput();

// Begin main and functions definitions
int main(int argc, char *argv[])
{
    while(1) { // Begin sshell loop

        // Get userInput
        char inputstr[512];
        strcpy(inputstr, userInput());


        // Special Case: check if there's nothing or whitespace
        // Reference: https://stackoverflow.com/questions/3981510/getline-check-if-line-is-whitespace
        if (strspn(inputstr, " \r\n\t") == strlen(inputstr)) {
        	memset(inputstr, 0, sizeof(inputstr));
        	continue;
        }

        // ###################### BEGIN PARSING ########################

        // Preserves inputstr for completed message
        char inputstrCOPY[512];

        // Tester Code Snippet
	    if (!isatty(STDIN_FILENO)) {
	        printf("%s", inputstr);
	        fflush(stdout);
	    }

	    // Removes newline character at the end of string for sake of printing cleanly
        strcpy(inputstrCOPY, inputstr);
        int k = 0;
        while (1) {
        	if (inputstrCOPY[k] == '\n') {
        		inputstrCOPY[k] = 0;
        		break;
        	}
        	k++;
        }

        // Splits input string into array of arguments
        char *temp;
        temp = strtok(inputstr, " ");

        char parsedInput[16][512];
        int index = 0; // ALSO TRACKS ARRAY LENGTH
        while (temp != NULL) {
            strcpy(parsedInput[index], temp);
            temp = strtok(NULL, " ");
            index++;
        }

        // Special Error Case: too many process arguments
        if (index >= 16) {
        	fprintf(stderr, "Error: too many process arguments\n");
        	memset(inputstr, 0, sizeof(inputstr));

        	for (int h = 0; h < 16; h++) {
        		memset(parsedInput[h], 0, sizeof(parsedInput[h]));
        	}
        	continue;
        }

        // Removes newline character from the end
        parsedInput[index - 1][strlen(parsedInput[index - 1]) - 1] = 0;


        // ################### Error Management and Further Parsing #####################

        // Find command1
        char *command1 = malloc(sizeof(512));
        strcpy(command1, parsedInput[0]);

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
        

        char *tempFlags1 = malloc(sizeof(512));
        tempFlags1 = strtok(flags1_BUFFER, "-");
        char *flags1 = malloc(sizeof(512));
        strcpy(flags1, "");
        if (tempFlags1 != NULL) {
            strcat(flags1, "-");
        }

        index = 1; // Skip over command and begin at first flag
        while (tempFlags1 != NULL) {
            strcat(flags1, tempFlags1);
            tempFlags1 = strtok(NULL, "-");
            index++;
        }

        // PARSING FOR INPUT1 -> note, this will include < or > for input/output redirection
        //            NOTE! This will also parse until either hitting NULL terminator OR a PIPE, toggling pipe1 ********
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

        // By this point, we know if there is a pipe1 or not in the right place
        // Ready to begin second command (if there is one)
        // For all intents and purposes, the below 2 lines are dead code
        int command2_START_INDEX = parseIndex + 2;
        if (pipe1 == 1) {
        }



		// Clear any strings that aren't in use for the sake of *args[] (an error occurs when "" is passed)
        if (strcmp(flags1, "") == 0) {
            flags1 = NULL;
        }
        if (strcmp(input1, "") == 0 || strcmp(input1, " ") == 0) {
            input1 = NULL;
        }


        // ############### Parsing input1 for input/output redirection #####################
        
        char outputToFile[20] = "";
        char LHS[20] = "";
        char *LHS_clean = malloc(sizeof(512));
        int outputFileExists = 0;

        char inputFromFile[20] = "";
        int inputFileExists = 0;


        if (input1 != NULL) {
        	// First, check if < or > exists
        	bool contProcess = false;
        	char temp_input1[20] = "";
        	char IO;
        	strcpy(temp_input1, input1);
        	int splitIndex = 0;
        	for (splitIndex = 0; splitIndex < strlen(temp_input1); splitIndex++) {
        		if (temp_input1[splitIndex] == '<' || temp_input1[splitIndex] == '>') {
        			contProcess = true;
        			IO = temp_input1[splitIndex];
        			break;
        		}
        	}
        	if (contProcess) { // Input/Output redirection exists, we must parse
        		// Note, splitIndex will give location of LHS and RHS
        		if (IO == '>') { // OUTPUT REDIRECTION
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
        		}

        		else if (IO == '<') {
        			int u = 0;
        			int splitIndexCopy = splitIndex;
        			// Thus, input file will be to the RHS of >...
        			// ORIGINALLY same as prev
        			while (temp_input1[splitIndex + 1] != '\0') { // We can assume that the only thing given here is a file
        				if (temp_input1[splitIndex + 1] != ' ') {
        					inputFromFile[u] = temp_input1[splitIndex + 1];
        					u++;
        				}
        				splitIndex++;
        			}
        			
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

        			// Replace input1 with LHS_clean
        			if (LHS_clean == NULL) {
        				//Then there is no input file
        				fprintf(stderr, "Error: no input file\n");
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
        			inputFileExists = 1;
        		}
        	}
        }

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

        // Originally created a struct, but CSIF complications forced the char **args...
        //Array a;

        char **args;
        args = malloc(16*sizeof(char*));
        for (int w = 0; w < 16; w++) {
        	args[w] = malloc(512*sizeof(char));
        }

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

        // ################### ENTER THE FORKING #########################

        // Special Cases: exit and cd
       	if (strcmp(command1, "exit") == 0) {
       		fprintf(stderr, "Bye...\n");
       		exit(0);
       	}

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

       		continue;

       	}

        int status;
        pid_t pid;
        pid = fork();
        if (pid > 0) { // ------- PARENT -------
            // This one will WAIT for the child to complete the necessary process.
            wait(&status);

        }
        else if (pid == 0) { // ------- CHILD -------
            // This one will EXEC the action that required forking
            // First, feed in command into exec, and store result as int, then print it

        	// Check if there's an onput file for output redirection
        	if (outputFileExists == 1) {

        		int fd = open(outputToFile, O_CREAT | O_WRONLY, 0644);
        		if (fd == -1) {
        			fprintf(stderr, "Error: cannot open output file\n");
        			close(fd);
        			exit(333);
        		}        		
        		dup2(fd, 1);
        		close(fd);
        	}

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

            int status = execvp(args[0], args);
            // If there's an error, then we will reach this line
            exit(1);
        }
        else {
            perror("fork");
            exit(1);
        }

        // Special Test Cases for Error Management
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

		if (inputFileExists == 1 && strcmp(inputFromFile, "") == 0) {
			continue;
		}
		if (outputFileExists == 1 && strcmp(outputToFile, "") == 0) {
			continue;
		}
        if (failedTest1) {
        	fprintf(stderr, "Error: missing command");
        }
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


        // #################### Clear and deallocate strings and arrays for next loop ##########
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
        
    } // Closes While Loop
}

// User Input function
char* userInput() {
    char *input = malloc(512);
    printf("sshell$ ");
    fgets(input, 512, stdin);
    return input;
}