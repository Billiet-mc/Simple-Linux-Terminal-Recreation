/*
* Name(s): Robert Moore
* Date: 2/21/24
* Program Description: Terminal emulator
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>

//basic execute function with no redirection
void execute(int argc, char *argv[]) {
    //parse arguments by adding NULL to the end
    char *arguments[argc+1];
    for (int i = 0; i < argc; i++) {
        arguments[i] = malloc(sizeof(argv[i])+1);
        strcpy(arguments[i], argv[i]);
    }
    arguments[argc] = NULL;
    
    
    
    //special case commands (cd and exit)
    if (!strcmp(arguments[0], "cd")) {
        char *filepath = malloc(256);
        sprintf(filepath, "%s/%s", getcwd(NULL, 256), arguments[1]);
        if (chdir(filepath) != 0 && chdir(arguments[1]) != 0) {
            printf("Error %d (%s)\n", errno, strerror(errno));
        }
    }

    else if (!strcmp(arguments[0], "exit")) {
        exit(EXIT_SUCCESS);
    }

    //all other commands execute here
    else {

        if (fork() == 0) {
            if (execvp(arguments[0], arguments) == -1) {
                printf("Error %d (%s)\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        else {
            wait(NULL);
        }

        for (int i = 0; i < argc+1; i++) {
            free(arguments[i]);
        }
    }
}

//more complicated version of execute that supports redirection
void executeRedirect(int argc, char *argv[], int hasOut, char *outfilePath, int hasIn, char *infilePath) {
    // parse arguments by adding NULL to the end
    char *arguments[argc+1];
    for (int i = 0; i < argc+1; i++) {
        arguments[i] = malloc(sizeof(argv[i])+1);
        strcpy(arguments[i], argv[i]);
    }
    arguments[argc] = NULL;

    //create input/output files if necessary
    FILE* outfile = hasOut ? fopen(outfilePath, "w") : NULL;
    FILE* infile = hasIn ? fopen(infilePath, "r") : NULL; 


    //special case commands (cd exit)
    if (!strcmp(arguments[0], "cd")) {
        char *filepath = malloc(256);
        sprintf(filepath, "%s/%s", getcwd(NULL, 256), arguments[1]);
        if (chdir(filepath) != 0 && chdir(arguments[1]) != 0) {
            printf("Error %d (%s)\n", errno, strerror(errno));
        }
    }

    else if (!strcmp(arguments[0], "exit")) {
        exit(EXIT_SUCCESS);
    }

    //all other commands
    else if (fork() == 0) {
        if (hasOut) {
            dup2(fileno(outfile), STDOUT_FILENO);
            fclose(outfile);
        }
        if (hasIn) {
            dup2(fileno(infile), STDIN_FILENO);
            fclose(infile);
        }


        if (execvp(arguments[0], arguments) == -1) {
            printf("Error %d (%s)\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    else {
        wait(NULL);
        dup2(STDOUT_FILENO, STDOUT_FILENO);
        dup2(STDIN_FILENO, STDIN_FILENO);
    }

    for (int i = 0; i < argc+1; i++) {
        free(arguments[i]);
    }
}


int main(int argc, char * argv[]) {
    char input[256]; 
    char *tinput[256]; //tokenized input
    char output[256];


    mainloop: while (1) {

        printf("%s$ ", getcwd(NULL, 256));

        //get commands + arguments
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        //if no input continue
        if (strspn(input, " \t\n\r") == strlen(input)) continue;
        

        //remove leading whitespace
        int index = 0;
        while (isspace(input[index])) { index++; }
        strcpy(input, &input[index]);


        //tokenize input and store in char *tinput[]
        char *token = strtok(input, " \n\r\t");
        int numElements = 0;
        char *operators = "<>";

        //main tokenization loop
        while ( token != NULL )  {
            char *tinputBuffer = (char *)malloc(strlen(token) + 1);
            int tinputBufferIndex = 0;

            //if token is one character immediately save it as into tinput and continue
            if ((int)strlen(token) == 1) {
                tinput[numElements] = (char *)malloc(2);
                strcpy(tinput[numElements], token);
                numElements++;
                token = strtok(NULL, " \n\r\t");
                continue;
            }

            //take the token and create substrings seperated by '<' or '>' and save all substrings into tinput seperately
            //loop through each character in token
            for(int i = 0; i < strlen(token); i++) {
                if ( token[i] == '<' || token[i] == '>' ) {
                    //tinput buffer already has contents clear the buffer and save the substring as a token in tinput
                    if ((int)strlen(tinputBuffer) > 0) {
                        tinput[numElements] = (char *)malloc(strlen(tinputBuffer)+1);
                        strcpy(tinput[numElements], tinputBuffer);
                        numElements++;
                        tinputBuffer[0] = '\0';
                        tinputBufferIndex = 0;
                    }
                    //save '<' or '>' as a token in tinput
                    tinput[numElements] = (char *)malloc(2);
                    sprintf(tinput[numElements], "%c", token[i]);
                    numElements++;
                }
                //add characters to tinputBuffer
                else {
                    tinputBuffer[tinputBufferIndex] = token[i];
                    tinputBuffer[tinputBufferIndex + 1] = '\0';
                    tinputBufferIndex++;
                    //if character is last in the token copy contents of buffer as a token in tinput
                    if ( i == (int)strlen(token)-1 ) {
                        tinput[numElements] = (char *)malloc(strlen(tinputBuffer));
                        strcpy(tinput[numElements], tinputBuffer);
                        numElements++;
                    }
                }
            }
            //move to next token
            token = strtok(NULL, " \n\r\t");
        }


        //check for unexpected token error
        //if first or last token is '<' or  '>'
        if(!strcspn(tinput[0], operators) || !strcspn(tinput[numElements-1], operators)) {
            printf("UNEXPECTED TOKEN ERROR!!!!!!1!!1\n");
            continue;
        }

        //check if two '>' or '<' are in a row
        for (int i = 0; i < numElements; i++) {
            if (!strcmp(tinput[i], ">") || !strcmp(tinput[i], "<")) {
                if (!strcmp(tinput[i-1], ">") || !strcmp(tinput[i-1], "<") || !strcmp(tinput[i+1], ">") || !strcmp(tinput[i+1], "<")) {
                    printf("UNEXPECTED TOKEN ERROR!!!!!!1!!1\n");
                    //how do i break out of 2 loops
                    goto mainloop;
                }
            }
        }

        //check if '<' comes after '>' in the tokenized input
        int lastOutIndex = 0;
        int lastInIndex = 0;
        for (int i = 0; i < numElements; i++) {
            if (!strcmp(tinput[i], ">")) { lastOutIndex = i; }
            if (!strcmp(tinput[i], "<")) { lastInIndex = i; }
        }
        if (lastOutIndex != 0 && lastInIndex > lastOutIndex) { 
            printf("UNEXPECTED TOKEN ERROR!!!!!!1!!1\n");
            continue;
         }

        
        //execute command(s)
        //commandBuffer will take some tokens and execute them as a command
        char *commandBuffer[numElements];
        for (int i = 0; i <= numElements; i++) {
            commandBuffer[i] = malloc(256);
        }
        int commandBufferSize = 0;
        
        //check if there's no redirection
        int redirectionBool = 0;
        for(int i = 0; i < numElements; i++) {
            if (!strcspn(tinput[i], operators)) { redirectionBool = 1; }
        }

        //if there's no redirection
        if (!redirectionBool) { 
            execute(numElements, tinput);
        }

        // if there is redirection
        else {
            char *infilePath = malloc(256);
            char *outfilePath = malloc(256);
            int hasIn = 0;
            int hasOut = 0;
            int executed = 0;

            //check which type of redirection is present
            for (int j = 0; j < numElements; j++) {
                //if input is detected interpret token after '<' as file
                if (tinput[j][0] == '<') {
                    hasIn = 1;
                    realloc(infilePath, strlen(tinput[j+1]));
                    strcpy(infilePath, tinput[j+1]);
                }
                //if output is detected set flag
                //the reason im not setting the token after '>' as outfile is bc in real terminal if the input is somethign like:
                //  ls -l > a.out > b.out > c.out
                // then the output is copied to all three files, not just one. So this behavior is handled seperately
                else if (tinput[j][0] == '>') { 
                    hasOut = 1;
                }
            }

            //create command buffer (tokenized input up to '<' or '>', or tokenized command)
            for (int i = 0; i < numElements; i++) {
                if (tinput[i][0] == '<' || tinput[i][0] == '>') { break; }

                realloc(commandBuffer[commandBufferSize], strlen(tinput[i]));
                strcpy(commandBuffer[commandBufferSize], tinput[i]);
                commandBufferSize++;
            }

            //loop through tinput and when '>' is found, executre command buffer and redirect output to token after '>'
            for (int i = 0; i < numElements; i++) {
                if (tinput[i][0] == '>') {
                    executeRedirect(commandBufferSize, commandBuffer, hasOut, tinput[i+1], hasIn, infilePath);
                    executed = 1;
                }
            }
            //if no '>' was present, execute the command
            if (!executed) { executeRedirect(commandBufferSize, commandBuffer, hasOut, outfilePath, hasIn, infilePath); }

        }

    }

}