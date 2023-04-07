#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//Constant variables
#define SIGINT 2   
#define SIGTSTP 20 

//Global Variables
struct sigaction actionS, actionSTP, action_INTER;
int pid_arr[1000];
int pid_num = 0;
int st;
int Background_permision = 1;



//Struct that holds input line arguments
struct ArgumentLine {
	bool isCommand;
	bool isForeground;
	char* args[512];
	int args_num;
};

//Function that handles SIGNSTP
void catch(int signo) {
	

	//allows the Background action 
	if (Background_permision == 1) {
		char* message = "Entering foreground-only mode (& is now ignored)\n";
        char* message1 = ": "; 
		write(1, message, 49); 
        write(1, message1, 2); 
		fflush(stdout);
		Background_permision = 0; 
	}

	//does not allow the Background action
	else {
		char* message = "Exiting foreground-only mode\n";
        char* message1 = ": ";
		write (1, message, 29); 
        write(1, message1, 2);  
		fflush(stdout);
		Background_permision = 1; 
	}
}


//status command function
void status (int *cal) {
	int errorHold=0, sigHold=0, comp;

	waitpid(getpid(), &st, 0); 

	if ( WIFEXITED(st) )  
		errorHold = WEXITSTATUS(st); 

	if ( WIFSIGNALED(st) ) 
		sigHold = WTERMSIG(st); 

	if ( errorHold == 0 && sigHold == 0 ) 
		comp = 0;

	else // if problems occur on exit
		comp = 1;

	if ( sigHold != 0 ){ 
		*cal = 1; // handles the terminated by signal for control c
		printf( "terminated by signal %d\n", sigHold); 
		fflush(stdout);
	}
	else { 
		printf( "exit value %d\n", comp);
		fflush(stdout); 
	}
}

//Function that gets the input line and store is in the struct
struct ArgumentLine parseArgs(){
	struct ArgumentLine argumentLine;
	char userInput[2048];

	fflush(stdout);
	printf(": ");
	fgets(userInput, 2048, stdin);
	strtok(userInput, "\n");
	char* buffer = strtok(userInput, " ");

	//Checks if input is a comment
	if(strcmp(buffer, "#") == 0 || strcmp(buffer, "\n") == 0)
		argumentLine.isCommand = false;
	else
		argumentLine.isCommand = true;

	//Puts all agruments into struct
	int i = 0;
	while(buffer){
		argumentLine.args[i] = buffer;
		//strcpy(argumentLine.args[i], buffer);
		buffer = strtok(NULL, " ");
		i++;
	}
	argumentLine.args_num = i;

	//Checks if input should be Foreground or Background
	if(strcmp(argumentLine.args[i-1], "&") == 0){
		argumentLine.isForeground = false;
		//argumentLine.args[i-1] = NULL;	
	}
	else
		argumentLine.isForeground = true;

	return argumentLine;
}


//Non bult-in commands function
void commands(struct ArgumentLine argumentLine, int* cal){
	pid_t pid, tempPID;
	int descriptorIN = 0;
	int descriptorOUT = 0;
	pid = fork();
	pid_arr[pid_num++] = pid; //stores a proccess


	if(pid == 0){
		int i = 0;
		
		while(argumentLine.args[i] != NULL){

			//If output redirection
			if(strcmp(argumentLine.args[i], ">") == 0){
				char outputFile[1024];
				argumentLine.args[i] = NULL;
				strcpy(outputFile, argumentLine.args[i+1]);
				descriptorOUT = creat(outputFile, 06444);
				dup2(descriptorOUT, 1);
				//close(descriptorOUT);
			}

			//If input redirection
			if(strcmp(argumentLine.args[i], "<") == 0){
				char inputFile[1024];
				argumentLine.args[i] = NULL;
				strcpy(inputFile, argumentLine.args[i+1]);
				descriptorIN = open(inputFile, 0);
				dup2(descriptorIN, 0);
				close(descriptorIN);
			}
			i++;
		}
		if (argumentLine.isForeground == true )
            actionS.sa_handler=SIG_DFL; 
		execvp(argumentLine.args[0], argumentLine.args);
	}

	else if (pid > 0){ // if parent
    
        if (argumentLine.isForeground == false && Background_permision == 1 ) { 
				tempPID = waitpid(pid, &st, WNOHANG); 
				printf("Background pid: %d\n", pid); 
				fflush(stdout); 
		}
        else {  //Executing the cmd it like a normal one
            // printf("else is working!\n");
            fflush(stdout);
            tempPID = waitpid(pid, &st, 0);
        }
    }

	while((pid = waitpid(-1, &st, WNOHANG)) > 0){
		printf("pid %d is done\n", pid);
		status(cal);
		fflush(stdout);
	}
}

//Change directory function
void cd(struct ArgumentLine argumentLine) {
	if(argumentLine.args_num == 1)
		chdir(getenv("HOME"));
	
	else
		chdir(argumentLine.args[1]);
}

//Exit the program function
void exit1(){
	int i;
	for(i = 0; i <= pid_num; i++){
		if(pid_arr[i] == 0)
			exit(0);
		kill(pid_arr[i], SIGTERM);
	}

}

//This function handles all the commands
void runCommand(struct ArgumentLine argumentLine){
	int cal = 0;
	if(!argumentLine.isCommand){
		
	}
	else if(strcmp("cd", argumentLine.args[0]) == 0){
		cd(argumentLine);
	}
	else if(strcmp("exit", argumentLine.args[0]) == 0)
		exit1();
	else if(strcmp("status", argumentLine.args[0]) == 0)
		status(&cal);
	
	else {
		commands(argumentLine, &cal);
		if ( WIFSIGNALED(st) && cal == 0 ){ 
            status(&cal); 
        }
	}

}



int main(){
	actionSTP.sa_handler=catch; 
    actionSTP.sa_flags = SA_RESTART; 
    sigaction(SIGTSTP, &actionSTP, NULL);
	sigfillset(&actionSTP.sa_mask);

    actionS.sa_handler=SIG_IGN;
	sigaction(SIGINT, &actionS, NULL); 
    sigfillset(&actionS.sa_mask); 

	//Command input line
	while(true){
		struct ArgumentLine argumentLine = parseArgs();
		int i = 0;
		if(pid_arr[1] == 0)
			printf("asdfasdf\n", pid_arr[0]);
		argumentLine.args[argumentLine.args_num] = NULL;
		runCommand(argumentLine);
	}
	return 0; 
}