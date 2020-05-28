#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <setjmp.h>
#include <unistd.h>
jmp_buf buffer;

/* *****SOURCES******

BOOK: Computer Systems Programmers Perspecticve pgs. 754, 755    
MAN DOCUMENTATION - MAN Siglongjmp, MAN wait, MAN fork, MAN exec, MAN pipe 
Lab4 sample code execexample.c , forkexample.c , stringparsing.c 
Lab5 signalHandle.c 
http://www.cplusplus.com/reference/cstdio/fgets/   
https://stackoverflow.com/questions/4893537/usage-of-fgets-function-in-c 
https://www.techonthenet.com/c_language/standard_library_functions/stdlib_h/exit.php 
https://www.tutorialspoint.com/c_standard_library/c_function_strcmp.htm  
https://www.dummies.com/programming/c/how-to-use-the-fgets-function-for-text-input-in-c-programming/ 
https://linux.die.net/man/2/write  
https://www.youtube.com/watch?v=xVSPv-9x3gk  
https://linux.die.net/man/3/execv 
https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
https://www.geeksforgeeks.org/g-fact22-concept-of-setjump-and-longjump/  
 

*/

void sigint_handler(int sig){
  
  if(sig == SIGINT){
  	char msg[] = "caught sigint\n";
  	write(1, msg, sizeof(msg));

    siglongjmp(buffer,0);
  }
  else if(sig == SIGTSTP){
	  char msg[] = "caught sigstp\n";
    write(1, msg, sizeof(msg));
    
    siglongjmp(buffer,0);
  }   
}

//create a function to fork and execute 
//need to adjust for the second command of the pipe so I can properl
void forkandExecvp(char *array[], int checker, int piparr[], char buffer[]){
  
  int pid = fork(); 
  if (pid == 0) { //child
    if(checker){
     // printf("%d, %d\n", piparr[0], piparr[1]);
      dup2(piparr[0],0);
      checker = 0;
      close(piparr[1]);
    }

    int x;
    x = execvp(array[0], array); 
    close(piparr[0]);
    if(x < 0){  //pid of the new proccess
      exit(0);
    }
  }
  else{
      int status;

      if(checker){
        close(piparr[0]);
        close(piparr[1]);
      }
      waitpid(pid,&status,WUNTRACED); 

      if(checker){
        printf("%s", buffer);
        checker = 0;
      }
      printf("pid:%d status:%d\n",pid, WEXITSTATUS(status));
  }
}

//create a function to fork and execute for the pipe and takes a bool
void forkandExecvpforpipe(char *array[], char buffer[], int pipearr[]){
  
  int pid = fork(); 
  if (pid == 0) { //child
    int x;
    //printf("%d, %d\n", pipearr[0], pipearr[1]);
    dup2(pipearr[1], 1);
    
    x = execvp(array[0], array); 
    
    if(x < 0){  //pid of the new proccess
      exit(0);
    }
  }
  else{

      int status;
      waitpid(pid,&status,WUNTRACED); 
      
      //print the string inside the array buffer 
      sprintf(buffer,"pid:%d status:%d\n",pid, WEXITSTATUS(status));
  }
}

int main(){
  char *mystring[100]; // used to see if the commands from line 
  char line[500]; //read in commands ex: ls, grep, exit, etc.
 
  int pipefd[2];
  char buf[1000];  
  int check = 0;   //checker -> 0 is false

  
  
  signal(SIGINT, sigint_handler);
  signal(SIGTSTP,sigint_handler);
  sigsetjmp(buffer,1);

  while(1){
     
    
    printf("CS361 >");
    
    fgets(line, 500, stdin);
    
    //without the forloop there is no null terminatimg character to show that exit
    //is 'the last command' so after fgets changes mystring from char to string, 
    //we will go throught the string one-by-one till we reach the null character    
    int i; 
    for(i = 0; line[i] != '\0'; i++) {
    	if(line[i] == '\n') {
            line[i] = '\0';
        }
    }
    

    if(strcmp(line, "exit") == 0){    
    	break;
    }

  //spacing out command from 1 line   
  for(i = 0; i < 20; i++){
	 mystring[i] = (char *)malloc(sizeof(char)*100);
  }

  for(i = 0; i < 1000; i++){
	 buf[i] = '\0';
  }

  char *word = strtok(line, " ");
  int j = 0;
  while(word){
    //if found | -> exec the first commad, what ever the result is use the output as the input and do the next command  
	  if(strcmp(word,"|") == 0){
      check = 1; //true

      if(pipe(pipefd) == -1){
        perror("pipe");  //tell us if it doesnt go through and wheather or not there is an error 
      }

      //printf("%d, %d\n", pipefd[0], pipefd[1]);
      mystring[j] = NULL;       
      forkandExecvpforpipe(mystring, buf, pipefd);

      for(i = 0; i < 20; i++){
      mystring[i] = (char *)malloc(sizeof(char)*100);
      }

      j = 0;                    
      word = strtok(NULL, " ");
      continue;
    }
    //if found ; -> then exec whatever is in array so far, after done exec; reset  array, and j = 0;	  
    if(strcmp(word,";") == 0){
      
      mystring[j] = NULL;
      forkandExecvp(mystring,check, pipefd, buf);
 
      for(i = 0; i < 20; i++){
      mystring[i] = (char *)malloc(sizeof(char)*100);
      }

      j = 0;   
    }
    else{
      strcpy(mystring[j], word);
      j = j + 1;
    }
    
    word = strtok(NULL, " ");
     
  }

  //printf("%d, %d\n", pipefd[0], pipefd[1]);
  mystring[j] = NULL;

  forkandExecvp(mystring,check, pipefd, buf);

 }//end of huge while loop
}
