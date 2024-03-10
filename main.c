#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#define RED   "\x1B[31m"
#define RESET "\x1B[0m"
#define GREEN   "\x1b[32m"

char* varNames[1000];
char* varValues[1000];
int varIndex = 0;

void reap_child_zombie(){
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

void write_to_log_file(char line[]){
    FILE *log;
    log = fopen("LOG_FILE_DIR", "a");
    if (log == NULL)
        return;
    fprintf(log, "%s", line);
    fclose(log);
}

void on_child_exit(){
    reap_child_zombie();
    write_to_log_file("Child terminated");
}

void register_child_signal(void (*on_child_exit)(int)) {
    signal(SIGCHLD, on_child_exit);
}

void execute_command(char** args){
    int pid;
    int status;
    pid = fork();
    if(pid == 0){
        if (args[1] && !strcmp(args[1], "&")) {
            args[1] = NULL;
        }
        else{
            char* token = strtok(args[1]," ");
            int x=1;
            while(token!= NULL){
                args[x] = token;
                x++;
                token = strtok(NULL, " ");
            }
        }
        execvp(args[0], args);
        printf("Error! Command not found\n");
        exit(0);
    }
    else if(pid < 0){
        printf("Error forking!\n");
    }
    else{
        if(!(args[1] && !strcmp(args[1], "&"))) {
            do {
                waitpid(pid, &status, 0);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
}

void execute_shell_builtin(char** command_array){
    if(!strcmp("cd", command_array[0])){
        if(command_array[1] == NULL || !strcmp("~", command_array[1])){
            chdir(getenv("HOME"));
        }
        else if(!strcmp("..", command_array[1])){
            chdir("..");
        }
        else{
            if (chdir(command_array[1]) != 0)
                printf("Error! Directory not found\n");
        }
    }
    else if(!strcmp("echo", command_array[0])){
        if(command_array[1] == NULL){
            printf("\n");
        }
        else{
            int j = 1;
            while(command_array[j] != NULL){
                if(command_array[j][0] == '$'){
                    char* varName = command_array[1] + 1;
                    for(int i = 0; i < varIndex; i++){
                        if(!strcmp(varName, varNames[i])){
                            char* token = strtok(varValues[i], "\"");
                            printf("%s\n", token);
                            return;
                        }
                    }
                    printf("Error! Variable not found\n");
                }
                else{
                    for(int i = 1; command_array[i] != NULL; i++){
                        char* token = strtok(command_array[i], "\"");
                        printf("%s ", token);
                    }
                    printf("\n");
                }
                j++;
            }
        }
    }
    else if(!strcmp("export", command_array[0])){
        char* tk = strtok(command_array[1], "\"");
        if(command_array[2] == NULL){
            char* token = strtok(command_array[1], "=");
            varNames[varIndex] = strdup(token);
            token = strtok(NULL, "=");
            varValues[varIndex] = strdup(token);
            varIndex++;
        }
        else{
            char* token = strtok(tk, "=");
            varNames[varIndex] = strdup(token);
            varValues[varIndex] = command_array[2];
            varIndex++;
        }
    }
}

int evaluate_command(char** command_array){
    if(command_array[0] == NULL)
        return 10;
    if(!strcmp("cd", command_array[0]) || !strcmp("echo", command_array[0]) || !strcmp("export", command_array[0])){
        return 0;
    }
    else if(!strcmp("exit", command_array[0])){
        return -1;
    }
    else if(!strcmp(" ", command_array[0]) || !strcmp("", command_array[0]) || !strcmp("\n", command_array[0]) || !strcmp("\t", command_array[0]) || !strcmp("\r", command_array[0])){
        return 10;
    }
    else{
        return 1;
    }
}

char** parse_command(char input[]){
    static char* strings[100];
    int x = 0;
    while(strings[x] != NULL){
        strings[x] = NULL;
        x++;
    }
    static char* quotations[3];
    x=0;
    while(quotations[x] != NULL){
        quotations[x] = NULL;
        x++;
    }
    int index = 0;
    int iQuotation = 0;
    char* tknQuotation = strtok(input, "\"");
    while(tknQuotation != NULL){
        quotations[iQuotation++] = tknQuotation;
        tknQuotation = strtok(NULL, "\"");
    }
    if(quotations[1] == NULL){
        char* token = strtok(input, " \n"); // split string by space and newline
        while (token != NULL){
            if (token[0] == '$'){
                char* varName = token + 1;
                for(int i = 0; i < varIndex; i++){
                    if(!strcmp(varName, varNames[i])){
                        strings[index++] = varValues[i];
                    }
                }
            }
            else{
                strings[index++] = token ;
            }
            token = strtok(NULL, " \n");
        }
        strings[index] = NULL;
    }
    else{
        for(int i = 0; i < 3; i++) {
            if(i!=1){
                char* token = strtok(quotations[i], " \n");
                while (token != NULL){
                    strings[index++] = token;
                    token = strtok(NULL, " \n");
                }
            }
            else{
                char* q[100];
                int j = 0;
                char* tkn = strtok(quotations[i], " \n");
                while(tkn != NULL){
                    if(tkn[0] == '$'){
                        char* varName = tkn + 1;
                        for(int k = 0; k < varIndex; k++){
                            if(!strcmp(varName, varNames[k])){
                                char* token = strtok(varValues[k], "\"");
                                q[j++] = token;
                            }
                        }
                    }
                    else {
                        q[j++] = tkn;
                    }
                    tkn = strtok(NULL, " \n");
                }
                char str[100] = "";
                strcpy(str, q[0]);
                int k = 1;
                while(k<j){
                    strcat(str, " ");
                    strcat(str, q[k]);
                    k++;
                }
                strings[index++] = strdup(str);
            }
        }
    }
    return strings;
}

void shell_loop(){
    int status = 1;
    do {
        if(!strcmp(getenv("HOME"), getcwd(NULL, 0))){
            printf(RED "%s", "ubuntu@ArmaShell:");
            printf( "%s", "~$ " RESET);
        }
        else{
            printf(RED "%s", "ubuntu@ArmaShell:");
            printf("%s", getcwd(NULL, 0));
            printf("%s", "$ " RESET);
        }
        char input_command[100] ;
        fgets(input_command, 100, stdin);
        char** command_array = parse_command(input_command);  // parse array of char to array of strings seperated by space and ended with null
        int type = evaluate_command(command_array); // 0: shell builtin, 1: executable or error, -1: exit
        switch (type) {
            case 0:
                execute_shell_builtin(command_array);
                break;
            case 1:
                execute_command(command_array);
                break;
            case -1:
                status = 0;
                break;
            default:
                break;
        }

    }
    while(status);
}

void setup_environment(){
    chdir(getenv("HOME"));
}

int main() {
    printf(GREEN"\n"
           "                                 _____ _          _ _ \n"
           "     /\\                         / ____| |        | | |\n"
           "    /  \\   _ __ _ __ ___   __ _| (___ | |__   ___| | |\n"
           "   / /\\ \\ | '__| '_ ` _ \\ / _` |\\___ \\| '_ \\ / _ \\ | |\n"
           "  / ____ \\| |  | | | | | | (_| |____) | | | |  __/ | |\n"
           " /_/    \\_\\_|  |_| |_| |_|\\__,_|_____/|_| |_|\\___|_|_|\n"
           "                                                   \n"
           "   \n" RESET);
    register_child_signal(on_child_exit);
    setup_environment();
    shell_loop();
    return 0;
}