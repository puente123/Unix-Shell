#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 32     

int main( int argc, char * argv[] )
{

    if(argc < 2){

        char * command_string = (char*) malloc( MAX_COMMAND_SIZE );
        while( 1 )
        {
            // Print out the msh prompt
            printf ("msh> ");

            // Read the command from the commandi line.  The
            // maximum command that will be read is MAX_COMMAND_SIZE
            // This while command will wait here until the user
            // inputs something.
            while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

            /* Parse input */
            char *token[MAX_NUM_ARGUMENTS];

            int token_count = 0;                                 
                                                                
            // Pointer to point to the token
            // parsed by strsep
            char *argument_pointer;                                         
                                                                
            char *working_string  = strdup( command_string );                

            // we are going to move the working_string pointer so
            // keep track of its original value so we can deallocate
            // the correct amount at the end
            
            char *head_ptr = working_string;
            
            // Tokenize the input with whitespace used as the delimiter
            while ( ( (argument_pointer = strsep(&working_string, WHITESPACE ) ) != NULL) && (token_count<MAX_NUM_ARGUMENTS)){
                token[token_count] = strndup( argument_pointer, MAX_COMMAND_SIZE );
                if( strlen( token[token_count] ) == 0 ){
                    token[token_count] = NULL;
                } 
                else{
                    token_count++;
                }
            }

            //path that will be passed to execv
            char *paths[] = {"/bin", "/usr/bin", "/usr/local/bin", "./", NULL};
            int redirectionSymbol = -1;

            if(token[0] == NULL){
                continue;
            }


            //Checks if user typed exit
            if(strcmp(token[0],"exit") == 0){

                if(token[1] != NULL){
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message)); 
                }
                else{
                    exit(0);
                }
            }
            //checks if user typed cd
            else if(strcmp(token[0],"cd") == 0){
                if(token[1] == NULL || token[2] != NULL){
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message)); 
                }
                else{
                    chdir(token[1]);
                }
            }
            //calls fork and execv
            else{
            
                pid_t pid = fork();

                //-1 means error
                if(pid == -1){
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                //in child process
                else if(pid == 0){

                    //throws error if shell script does not have path
                    if(strstr(token[0], ".sh") != NULL){
                        if(strstr(token[0], "/") == NULL){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(0); 
                        }
                    }
                        
                    //currently finds the first instance of >
                    for(int i = 0; i<token_count; i++ ){
                        if(token[i] == NULL){
                            break;
                        }
                        if( strcmp( token[i], ">" ) == 0 ){
                            redirectionSymbol = i;
                            break; //adding this breaks after first occurance of > "Test 10"
                        }
                    }


                    //throws error if there is nothing before > symbol
                    if(redirectionSymbol == 0){
                        char error_message[30] = "An error has occurred\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(0);
                        
                    }


                    //no redirection symbol, so operate as normal
                    if(redirectionSymbol == -1){

                        int execvErrors = 0;
                        for(int j=0; j<4; j++){
                            char concat[512];
                            sprintf(concat, "%s/%s", paths[j], token[0]);

                            
                            if(execv(concat, token) == -1){
                                execvErrors++; 
                            }
                        }
                        //if execv was not succesfull error is thrown test 14
                        if(execvErrors == 4){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            return 1; 
                        }

                    }
                    //redirection symbol found
                    else{ 

                        //throws error if there is more than one output file in redirection
                        if(token[redirectionSymbol+2] != NULL){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(0);
                        }

                        int fd = open( token[redirectionSymbol+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
                        if( fd < 0 ){

                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            exit( 0 );                    
                        }
                        dup2( fd, 1 );//replaces stdout with file descriptor opened
                        close( fd );
                        
                        // Trim off the > output part of the command
                        token[redirectionSymbol] = NULL;

                        int execvErrors = 0;
                        for(int j=0; j<4; j++){
                            char concat[512];
                            sprintf(concat, "%s/%s", paths[j], token[0]);


                            if(execv(concat, token) == -1){
                                execvErrors++; 
                            }
                        }
                        if(execvErrors == 4){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(0); 
                        }

                    }
                                         
                }
                //positive number is parent pid
                else{
                    int status;
                    waitpid(pid, &status, 0);
                }
            }

            free( head_ptr );
        }

        return 0;
    }
    


    //opens batch mode
    else if(argc == 2){
        
        //opens file that was given ex. batch.txt
        FILE *fptr = fopen(argv[1], "r+");

        //throw error for test 14 if file does not open
        if(fptr == NULL){
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            return 1;
        }

        char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

        while( 1 )
        {
            

            // Read the command from the commandi line.  The
            // maximum command that will be read is MAX_COMMAND_SIZE
            // This while command will wait here until the user
            // inputs something.
            while( !fgets (command_string, MAX_COMMAND_SIZE, fptr) ) {
                if(feof(fptr)) {
                    fclose(fptr);
                    exit(0);
                }
            }

            /* Parse input */
            char *token[MAX_NUM_ARGUMENTS];

            int token_count = 0;                                 
                                                                
            // Pointer to point to the token
            // parsed by strsep
            char *argument_pointer;                                         
                                                                
            char *working_string  = strdup( command_string );                

            // we are going to move the working_string pointer so
            // keep track of its original value so we can deallocate
            // the correct amount at the end
            
            char *head_ptr = working_string;
            
            // Tokenize the input with whitespace used as the delimiter
            while ( ( (argument_pointer = strsep(&working_string, WHITESPACE ) ) != NULL) && (token_count<MAX_NUM_ARGUMENTS)){
                token[token_count] = strndup( argument_pointer, MAX_COMMAND_SIZE );
                if( strlen( token[token_count] ) == 0 ){
                    token[token_count] = NULL;
                } else {
                    token_count++;
                }
            }

            //path that will be passed to execv
            char *paths[] = {"/bin", "/usr/bin", "/usr/local/bin", "./", NULL};
            int redirectionSymbol = -1;

            if(token[0] == NULL){
                continue;
            }


            //Checks if user typed exit
            if(strcmp(token[0],"exit") == 0){

                if(token[1] != NULL){
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message)); 
                }
                else{
                    exit(0);
                }
            }
            //checks if user typed cd
            else if(strcmp(token[0],"cd") == 0){
                if(token[1] == NULL || token[2] != NULL){
                    //Signal an error?
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message)); 
                }
                else{
                    chdir(token[1]);
                
                }
            }
            //calls fork and execv
            else{
            
                pid_t pid = fork();

                //-1 means error
                if(pid == -1){
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                //in child process
                else if(pid == 0){

                    //throws error if shell script does not have path
                    if(strstr(token[0], ".sh") != NULL){
                        if(strstr(token[0], "/") == NULL){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(0); 
                        }
                    }
                        
                    //currently finds the first instance of >
                    for(int i = 0; i<token_count; i++ ){
                        if(token[i] == NULL){
                            break;
                        }
                        if( strcmp( token[i], ">" ) == 0 ){
                            redirectionSymbol = i;
                            break; //adding this breaks after first occurance of > "Test 10"
                        }
                    }


                    //throws error if there is nothing before > symbol
                    if(redirectionSymbol == 0){
                        char error_message[30] = "An error has occurred\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(0);
                    }


                    //no redirection symbol, so operate as normal
                    if(redirectionSymbol == -1){

                        int execvErrors = 0;
                        for(int j=0; j<4; j++){
                            char concat[512];
                            sprintf(concat, "%s/%s", paths[j], token[0]);

                            //the error might have to be pulled out of for loop
                            if(execv(concat, token) == -1){
                                execvErrors++; 
                            }
                        }

                        //if execv was not succesfull error is thrown test 14
                        if(execvErrors == 4){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            return 1; //double check return code
                        }

                    }
                    //redirection symbol found
                    else{ 

                        //throws error if there is more than one output file in redirection
                        if(token[redirectionSymbol+2] != NULL){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(0);
                        }

                        int fd = open( token[redirectionSymbol+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
                        if( fd < 0 ){

                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            exit( 0 );                    
                        }
                        dup2( fd, 1 );//replaces stdout with file descriptor opened
                        close( fd );
                        
                        // Trim off the > output part of the command
                        token[redirectionSymbol] = NULL;

                        int execvErrors = 0;
                        for(int j=0; j<4; j++){
                            char concat[512];
                            sprintf(concat, "%s/%s", paths[j], token[0]);

                            if(execv(concat, token) == -1){
                                execvErrors++; 
                            }
                        }
                        if(execvErrors == 4){
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(0); 
                        }

                    }
                                         
                }
                //positive number is parent pid
                else{
                    int status;
                    waitpid(pid, &status, 0);
                }
            }

            free( head_ptr );
        }

        return 0;
    }

    //if argc is > 2 throw error test 13
    else{
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }
}


