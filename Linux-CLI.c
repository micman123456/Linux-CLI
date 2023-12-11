#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#define MAX_LINE 80
#define BUF_SIZE 65536


/* 
   count_lines function pulled from stackoverflow.com
   url: https://stackoverflow.com/questions/12733105/c-function-that-counts-lines-in-file
   author: Mike Siomkin
   date accessed: 2023-09-30

*/


int count_lines(FILE* file)
{
    char buf[BUF_SIZE];
    int counter = 0;

   
    long original_pos = ftell(file);

    if (original_pos == -1) {
        perror("ftell");
        return -1; 
    }

    
    if (fseek(file, 0, SEEK_SET) == -1) {
        perror("fseek");
        return -1; 
    }

    for (;;) {
        size_t res = fread(buf, 1, BUF_SIZE, file);
        if (ferror(file))
            return -1;

        int i;
        for (i = 0; i < res; i++)
            if (buf[i] == '\n')
                counter++;

        if (feof(file))
            break;
    }

    
    if (fseek(file, original_pos, SEEK_SET) == -1) {
        perror("fseek");
        return -1; 
    }

    return counter;
}


int getSize(char **cmd) {
    int size = 0;
    while (cmd[size] != NULL) {
        size++;
    }
    return size;
}

void displayHelp() {
    printf("\n");
    printf("Welcome to My Command Line Application\n");
    printf("=======================================\n");
    printf("Available Commands:\n");
    printf("1. Most built in Unix commands - cal, ls, top etc...\n");
    printf("2. cd, to change working directory\n");
    printf("3. history - Display command history and PID\n");
    printf("4. !! - Execute most recent command\n");
    printf("5. !N - Execute command with id N\n");
    printf("6. exit, quit or q - Exit the application\n");
    printf("=======================================\n");
    printf("Creator Information:\n");
    printf("- Created by Ali Fadhil\n");
    printf("- B00837578\n");
    printf("- al878898@dal.ca\n");
    printf("=======================================\n");
    printf("\n");
}


int displayFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) { 
        perror("Error opening file");
        return 1;
    }
    
    printf("ID  PID  Command\n");
    int id = 1;
    char line[MAX_LINE];
    
    while (fgets(line, MAX_LINE, file)) {
        int pid;
        char cmd[MAX_LINE];
        char arg[MAX_LINE];
        
        if (sscanf(line, "%d %s %[^\n]", &pid, cmd, arg) == 3) {
            printf("%-4d%-8d%s %s\n", id, pid, cmd, arg);
            id++;
        } 
        else if (sscanf(line, "%d %s", &pid, cmd) == 2) {
            printf("%-4d%-8d%s\n", id, pid, cmd);
            id++;
        }

        else {
            printf("Invalid line format: %s", line);
            fclose(file);
            return 1;
        }   
    }
    
    fclose(file);
    return 0;
}



/* New and improved fetchCommand mehtod */
char** fetchCommand(const char *filename, int cmd_index) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    if (cmd_index == 0){
        cmd_index = count_lines(file);
        
    }

    int line_index = 1;
    char line[MAX_LINE];
    int pid;
    char *userString = NULL;
    int maxSize = 10;
    int wordCounter = 0;
    char **cmd = NULL;

    while (fgets(line, sizeof(line), file)) {
       
        
        if (line_index == cmd_index) {
            
            char fetchedCmd[MAX_LINE];
            int index = 0;
            int i = 0;
            cmd = (char **)malloc(MAX_LINE * sizeof(char *));

            if (sscanf(line, "%d %[^\n]", &pid, fetchedCmd) == 2) {
                userString = (char*)malloc(maxSize); // Allocate memory for userString
                if (userString == NULL) {
                    perror("Memory allocation error");
                    fclose(file);
                    return NULL;
                }

                while (fetchedCmd[i] != '\0') {
                    char ch = fetchedCmd[i];
                    

                    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\0') {
                        if (index > 0) {
                            userString[index] = '\0';
                            cmd[wordCounter] = (char *)malloc(MAX_LINE * sizeof(char));
                            strcpy(cmd[wordCounter],userString);
                            
                            if (cmd[wordCounter] == NULL) {
                                printf("Mem error\n");
                                fclose(file);
                                free(userString);
                                return NULL;
                            }
                            wordCounter++;
                            if (wordCounter >= 80 - 1) {
                                fprintf(stderr, "too many args\n");
                                fclose(file);
                                free(userString);
                                return NULL;
                            }
                        }

                        if (fetchedCmd[i+1] == '\0') {
                            fclose(file);
                            free(userString);
                            return cmd;
                        }

                        index = 0;
                    } else {
                        if (index >= maxSize - 1) {
                            maxSize *= 2;
                            char *newUserString = (char*)realloc(userString, maxSize);
                            if (newUserString == NULL) {
                                fclose(file);
                                free(userString);
                                return NULL;
                            }
                            userString = newUserString;
                        }

                        userString[index] = ch;
                        index++;
                        
                    }
                    i++;
                }
            } else {
                fprintf(stderr, "Error in history file: %s", line);
                fclose(file);
                return NULL;
            }
        }

        line_index++;
    }
    

    fclose(file);
    return NULL;
}



 
int main() {
    
    /* Init all necessary variables */
    char ch;	  
    char *filename = "history.txt";
    int index;
    int maxSize; 
    char *userString;
    char *args[MAX_LINE/2 + 1];
    int should_run = 1;
    char cwd[1024];
    pid_t child_pid;
    char **command = NULL;
    int result;
    int status;
    int history_flag;
    uid_t uid = getuid();      
    struct passwd *pw = getpwuid(uid);
    char curdir[MAX_LINE];
    char *homeDir = getenv("HOME");
    char filepath[1024];

    if (homeDir == NULL) {
	    fprintf(stderr, "HOME environment variable not set.\n");
	    return 0;
    }	    

    /*-- Program starts --*/

    while(should_run){
	
	
	snprintf(filepath, sizeof(filepath), "%s/%s", homeDir, filename);

        /*-- Reset index, size, word count & user input string upon each iteration --*/
        history_flag = 0;
        index = 0;
	    maxSize = 10;
	    userString = (char *)malloc(maxSize*sizeof(char));
    	int wordCounter = 0;
        command = (char **)malloc(MAX_LINE * sizeof(char *));
        for (int i = 0; i < MAX_LINE; i++) {
            command[i] = (char *)malloc(MAX_LINE * sizeof(char));
        }
	
	if (getcwd(curdir, sizeof curdir)) {
	            /* current directory might be unreachable: not an error */
	        //*curdir = '\0';
		printf("%s:%s>",pw->pw_name,curdir); 
	}


    /*-- Capture the user input, word by word and store in args --*/
    while ((ch = getchar())) {
        if (ch == ' ' || ch == '\n' || ch == '\t') {
            userString[index] = '\0'; 
            args[wordCounter] = (char *)malloc((strlen(userString)+1)*sizeof(char));  
            strcpy(args[wordCounter], userString);
            index = 0; 
            
            if (ch == '\n') {
		        args[wordCounter+1] = NULL;
                break; 
            }
            wordCounter++;
        } 
        
        else {
            userString[index] = ch;
            index++;

            if (index >= maxSize) {
                maxSize *= 2;
                userString = realloc(userString, maxSize * sizeof(char));
                
                if (userString == NULL) {
                    printf("Mem alloc error\n");
                    return 1;
                }
            }
        }
    }


    /* Check for exit input */
    if (strcmp(args[0],"exit")==0 || strcmp(args[0],"quit")==0 || strcmp(args[0],"q")==0){ 
	    printf("Exiting...\n");
      	/*	
	    FILE *clearFile = fopen(filepath, "w");
	    if (clearFile == NULL) {
		    perror("Error clearing file");
		    return 1;
	    }
	    fclose(clearFile); */
	    return 0;
    } 
    else if (strcmp(args[0],"history")==0){
    	displayFile(filepath); 
    }
    else if (strcmp(args[0],"help") == 0){
        displayHelp();
    }
    else if (strcmp(args[0], "cd") == 0){  
		
	    if (args[1]){
	    	if (args[1][0] == '~'){
			args[1] = homeDir;
		}
	    }


	    if (chdir(args[1]) == 0) {
	    // changed dir's 
	    }
	    else{
		    printf("cd error: %s no such file or directory\n",args[1]);
	    }
    }
    

    
    else { 
	    FILE *file = fopen(filepath, "a"); 
	    if (file == NULL) { 
		    printf("Failed to open the file\n");
		    return 1; }

        /* Create new process  */
        child_pid = fork();
        if (child_pid ==-1){
        printf("fork failed... exiting \n");
        return 1;
        }
        

        /* Child proc executes commands  */
        if (child_pid ==0){
        
        if (args[0][0] == '!') {
        int historyIndex;
	    
      
        if (args[0][1] == '!') {            
            historyIndex = 0;
           
        }

        else {
            historyIndex = atoi(&args[0][1]);
            if (historyIndex < 1 || historyIndex > 10) {
                printf("Invalid history index. Enter a number between 1 and 10.\n");
                exit(1);
            }
              
        }

        command = fetchCommand(filepath,historyIndex); 
        
	    if(command){
            if (execvp(command[0],command) == -1){
                printf("Invalid Command\n");
                exit(1);
            }
        }
        else {
            printf("Error with history Command\n");
            exit(1);}
        
        exit(0);
        }
         
        else{
            if (execvp(args[0], args) == -1) {
                printf("Invalid Command\n");
                exit(1);}

            exit(0); } 
    }
    

        /* Parent proc waits until child is done */
        else{
        if (waitpid(child_pid, &status, 0) == -1) {
            perror("waitpid");
            exit(1);
        }

        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            
            
            if (args[0][0] != '!'){
                fprintf(file, "%d ", (int)child_pid); 
                for (int i = 0; i < wordCounter+1; i++) {	
                fputs(args[i], file); 
        	    fputs(" ", file); 
                }
            
            fputs("\n",file);
            
            } 
            
        } else {
            continue;
        } 
        }
        
	    fclose(file);
        
        }    
    

    /* Once finished, clear and free args, command & userstring   */
    for (int i = 0; i <= wordCounter+1 ; i++) {
	    args[i]=NULL;       
	    free(args[i]);
    }
    for (int i = 0; i < MAX_LINE; i++) {
        free(command[i]);
    }
    free(command);
    userString = NULL;
    free(userString);
    
    }
 
 
    return 0;
}
   
