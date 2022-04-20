
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include<readline/readline.h>
#include<readline/history.h>
#include <string.h>
#include <fcntl.h>

typedef struct{
    char* name;
    char* value;
}varTable;

varTable* variablesPool;
int variablesPoolctr = 0;
char* built_in[3] = {"cd", "export", "echo"};
int fptr;
struct sigaction act_child;

int read_input(char* str)
{

    char* buf;
    char s[100];
    printf("(%s)", getcwd(s, 100));
    buf = readline(">>> ");
    if (strlen(buf) != 0) {
        if(buf == "\n")
            return 2;
        add_history(buf);
        strcpy(str, buf);
        return 1;
    } else {
        return 0;
    }
}

void parse_input(char* str, char** strArgs, int* noOfArgs){
    char *found;
    int i = 0;
    while( (found = strsep(&str, " ") )!= NULL) {
        strArgs[i++] = found;
        *noOfArgs += 1;
    }
    *noOfArgs -= 1;
    strArgs[i++] = NULL;
    for(int j=0; j< *noOfArgs+1; j++){
        //printf("->%s \n", strArgs[j]);
    }
}

int checkDollar(char* str){
    if(str[0] == '$')
        return 1;
    return 0;
}

char* replaceVariable(char* str){
    char* str2;
    int i;
    for(int i=1; ; i++){
        str2[i-1] = str[i];
        if(str[i] == '\0')
            break;
    }
    str2[i-1]= '\0';
    if(getenv(str2) != NULL) {
        return getenv(str2);
    }
    return str;
}
int evaluate_expression(char** strArgs, int noOfArgs){
    int isBuiltIn = 0;
    for(int i=0; i<noOfArgs+1; i++){
        if(checkDollar(strArgs[i])){
            strArgs[i] = replaceVariable(strArgs[i]);
        }
    }
    for(int i=0; i<3; i++){
        if(strcmp(strArgs[0], built_in[i]) == 0)
            isBuiltIn = 1;
    }
    return isBuiltIn;
}

void execute_shell_bultin(char **strArgs, int noOfArgs){
    int command, j = 0;
    char *found;


    for (int i = 0; i < 3; i++) {
        if (strcmp(strArgs[0], built_in[i]) == 0) {
            command = i + 1;
            break;
        }
    }
    switch(command){
        case 1:
            if(strcmp(strArgs[1], "~") == 0)
                chdir("/home");
            else
                chdir(strArgs[1]);
            break;
        case 2:
            printf("*");
            char* ex1 = strsep(&strArgs[1], "=");
            char* ex2 = strsep(&strArgs[1], "=");
            setenv(ex1,ex2,1);
            break;
        case 3:
            if(strArgs[1][0] == '$') {
                char* str = strdup(strArgs[1]);
                char *found;
                found = strsep(&str, "$");
                found = strsep(&str, "$");
                if(getenv(found) != NULL)
                    printf("%s\n", getenv(found));
                else
                    printf("\n");
            }else{
                printf("%s\n", strArgs[1]);
            }
            break;
        default:
            printf("Error, command not recognized");
            break;
    }
}

void handle(){
    char msg[]="Process terminated\n";
    if(fptr != 1)
    {
        write(fptr,msg,strlen(msg));
    }
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

void execute_command(char** strArgs, int noOfArgs){
    int background_flag = 0;
    for(int i=1; i<noOfArgs+1; i++){
        if(strcmp(strArgs[i], "&") == 0) {
            background_flag = 1;
        }
    }
    pid_t id = fork();
    if(id == -1){
        printf("Error forking.\n");
    }else if(id == 0){
        int x = execvp(strArgs[0], strArgs);
        if ( x < 0 ) {
            printf("Error in execvp.\n");
            exit(x);
        }
        exit(0);
    }else{
        if(!background_flag) {
            //printf(" no back ground \n");
            waitpid(id, NULL, 0);
        }else{
            background_flag = 0;
        }
    }
}
int main(int arc, char* argv[]) {
    int noOfArgs = 0;
    act_child.sa_handler = handle;
    sigaction(SIGCHLD, &act_child, 0);
    char *str = malloc(8 * sizeof(char));
    char **strArgs = malloc(8 * sizeof(char));
    variablesPool = malloc(100 * sizeof(char));
    int input_type = 0;
    int x = read_input(str);


    fptr = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    while(1){
        noOfArgs = 0;
        if( x == 1 ){
            parse_input(str, strArgs, &noOfArgs);
            switch(evaluate_expression(strArgs, noOfArgs)) {
                case 1:
                    execute_shell_bultin(strArgs, noOfArgs);
                    break;
                case 0:
                    execute_command(strArgs, noOfArgs);
                    break;
            }
        }
        x = read_input(str);
    }
}