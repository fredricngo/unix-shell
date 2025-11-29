#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h> 
#include <fcntl.h>

#define MAX_LINE_LENGTH 514
#define MAX_ARGS 64 

char error_message[30] = "An error has occurred\n"; 

void printError(char *msg)
{
    write(STDOUT_FILENO, error_message, strlen(error_message)); 
}

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void execute_builtin(char *args[]){

        if (strcmp(args[0], "exit") == 0){
            
            if (args[1] != NULL){
                printError(error_message);
                return; 
            }
            exit(0);
        }

        if (strcmp(args[0], "cd") == 0){
            
            if (args[1] != NULL && args[2] != NULL){
                printError(error_message); 
                return; 
            }

            if (args[1] == NULL || (strcmp(args[1], " ") == 0)){
                char *home = getenv("HOME"); 
                if (home == NULL || chdir(home) < 0) {
                    printError(error_message);
                }
                return; 
            }

            if (chdir(args[1]) < 0){
                printError(error_message); 
            }
        }

        if(strcmp(args[0], "pwd") == 0){
  

            if (args[1] != NULL){
                printError(error_message); 
                return;
            }

            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) == NULL){
                printError(error_message); 
                return;
            }

            strcat(cwd, "\n");
            myPrint(cwd);
            return; 
        }
}


void execute_command(char *command){

    //parse commands into args array 
    char *args[MAX_ARGS];
    int i = 0; 
    char *outfile = NULL;
    int redirect_pos = -1;
    int redirect_count = 0;
    char *temp = command; 
    char *last_redirect = NULL; 
    int is_advanced_redirect = 0; 
    char *old_content = NULL;
    size_t old_size = 0;

    while ((temp = strchr(temp, '>')) != NULL){
        redirect_count++; 
        last_redirect = temp; 

        if (*(temp+1) == '+'){
            is_advanced_redirect = 1; 
            temp += 2; 
        } else{
            temp++; 
        }  
    }

    if (last_redirect != NULL) {
        char *after_last = last_redirect + 1;
        if (is_advanced_redirect) after_last++; 
        while (*after_last == ' ' || *after_last == '\t') {
            after_last++;
        }
        if (*after_last == '\0' || *after_last == '>') {
            printError(error_message);
            return;
        }
    }
    if (redirect_count > 1) {
        printError(error_message);
        return;
    }

    char *redirect_char = strchr(command, '>'); 

    if (redirect_char != NULL) {
        *redirect_char = '\0';
        outfile = redirect_char + 1;
        redirect_pos = i;

        if (*outfile == '+'){
            is_advanced_redirect = 1; 
            outfile++;
        }

        while (*outfile == ' ' || *outfile == '\t') {
            outfile++;
        }

        char *end = outfile + strlen(outfile) - 1;
        while (end > outfile && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

        if (*outfile == '\0') {
            printError(error_message);
            return;
        }
        redirect_pos = 1; 
    }

    char *token = strtok(command, " \t");
    while (token != NULL && i < MAX_ARGS -1){
        char *end = token + strlen(token) - 1; 
        while (end > token && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        if (*token != '\0') {  
            args[i++] = token;
        } 
        token = strtok(NULL, " \t");
    }

    args[i] = NULL; 
    if (i == 0) return; 

    //check for built-ins 
    if (strcmp(args[0], "exit") == 0 || 
        strcmp(args[0], "cd") == 0 ||
        strcmp(args[0], "pwd") == 0){
        if (redirect_pos != -1){
            printError(error_message); 
            return;
        }
        execute_builtin(args);
        return; 
    } 

    if (is_advanced_redirect){
        char buffer[4096];
        size_t old_capacity = 4096;
        
        int old_fd = open(outfile, O_RDONLY);
        if (old_fd != -1) {
            old_content = malloc(old_capacity);
            if (!old_content) {
                printError(error_message);
                close(old_fd);
                return;
            }

            ssize_t bytes_read;
            while ((bytes_read = read(old_fd, buffer, sizeof(buffer))) > 0) {
                if (old_size + bytes_read > old_capacity) {
                    old_capacity *= 2;
                    char *temp = realloc(old_content, old_capacity);
                    if (!temp) {
                        free(old_content);
                        close(old_fd);
                        printError(error_message);
                        return;
                    }
                    old_content = temp;
                }
                memcpy(old_content + old_size, buffer, bytes_read);
                old_size += bytes_read;
            }
            close(old_fd);
        }
    }

        pid_t pid = fork();
        if (pid == 0) { //c
        if (redirect_pos != -1) {
            int output_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd == -1) {
                printError(error_message);
                _exit(1);
            }
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                printError(error_message);
                close(output_fd);
                _exit(1);
            }
            close(output_fd);
        }
        execvp(args[0], args);
        printError(error_message);
        _exit(1);
    } else { //p
        wait(NULL);
        
        //now in advancedredirection, append old content after command completes
        if (is_advanced_redirect && old_content) {
            int output_fd = open(outfile, O_WRONLY | O_APPEND, 0644);
            if (output_fd != -1) {
                write(output_fd, old_content, old_size);
                close(output_fd);
            }
            free(old_content);
        }
    }
}

void process_input(char *input){
    char *command; 
    char *saveptr; 

    //bruh are there newlines from the original input string
    char *end = input + strlen(input) - 1;
    while (end > input && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    command = strtok_r(input, ";", &saveptr);

    while ((command != NULL)){
        //we begin by parsing commands by semi-colons
        while (*command == ' ' || *command == '\t'){
            command++;
        }

        if (*command != '\0'){
            execute_command(command);
        }
        command = strtok_r(NULL, ";", &saveptr);
    }
}

    

int main(int argc, char *argv[]) {
    char cmd_buff[MAX_LINE_LENGTH];
    char *pinput;
    char c;
    FILE *input_stream; 
    int batch_mode = 0;

    if (argc > 2){
        printError(error_message); 
        exit(1);
    }

    if (argc == 2){
        batch_mode = 1; 
        input_stream = fopen(argv[1], "r"); 
        if (!input_stream){
            printError(error_message);
            exit(1);
        }
    } else {
        batch_mode = 0; 
        input_stream = stdin; 
    }

    while (1) {

        if (!batch_mode){
            myPrint("myshell> ");
        }

        pinput = fgets(cmd_buff, MAX_LINE_LENGTH, input_stream);
        if (!pinput) {
            if (batch_mode){
                fclose(input_stream);
            }
            exit(0);
        }

        if (batch_mode){

            int is_empty = 1; 
            char *s = cmd_buff; 
            while (*s != '\0' && *s != '\n'){
                if (*s != ' ' && *s != '\t'){
                    is_empty = 0;
                    break;
                }
                s++;
            }
            if (is_empty){
                continue;
            }
        }

        if (strchr(pinput, '\n') == NULL){
            //print everything we've received so far
            if (batch_mode){
                myPrint(pinput); 
                //print the rest 
                while ((c = fgetc(input_stream)) != '\n' && c != EOF){
                    write(STDOUT_FILENO, &c, 1);
                }
                myPrint("\n"); 
                printError(error_message);
            } else {
                myPrint(pinput); 
                while ((c = fgetc(input_stream)) != '\n' && c != EOF){
                    write(STDOUT_FILENO, &c, 1);
                }
                myPrint("\n"); 
                printError(error_message);
            //throw the line away
            }
            continue; 
        }

        if (batch_mode){
            myPrint(cmd_buff); 
        }

        


        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        process_input(cmd_buff); 


    }
}
