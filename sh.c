#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"


void fixNewLines(char** args){
  int i = 0;
  while(args[i]!=NULL){
    strtok(args[i],"\n");
    i++;
  }
}

void printCommandLine(char** commandLine){
  int i = 0;
  while(commandLine[i]!=NULL){
    printf("%s ",commandLine[i]);
    i++;
  }
  printf("\n");
}

char** convertInputToCommandLine(char* input){
  char** commandLine = (char**)malloc(sizeof(char*) * 256);
  char* token = strtok(input, " ");
  int i = 0;
  while(token != NULL){
    commandLine[i] = token;
    token = strtok(NULL," ");
    i++;
  }
  return commandLine;
}

void freePathElement(struct pathelement *pathElement){
  struct pathelement* currentPath = pathElement;
    while(currentPath!=NULL){
      struct pathelement* tempPathElement = currentPath->next;
      free(currentPath);
      currentPath=tempPathElement;
    }
}

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd = NULL;
  char **args = calloc(MAXARGS, sizeof(char*));
  char **argsMemoryLocation = args;
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry = NULL;
  char *homedir = NULL;
  struct pathelement *pathlist = NULL;
  args = NULL;
  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start out with*/
  if ((pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */
    printf("%s%s$", prompt, pwd);
    /* get command line and process */
    //char* input = (char*)malloc(sizeof(char)  * 256);
    char input[256];
    fgets(input, 256, stdin);
    args = convertInputToCommandLine(input);
    command = args[0];
    /* check for each built in command and implement */
    //EXIT
    fixNewLines(args);
    if(strcmp(command, "exit") == 0)
    {
      printf("Executing built-in EXIT\n");
      //freeing memory
      free(prompt);
      free(commandline);
      free(owd);
      freePathElement(pathlist);
      free(argsMemoryLocation);
      free(pwd);
      go = 0;
    }
    else if(strcmp(command, "which") == 0){
      printf("Executing built-in WHICH\n");
      if(args[1] == NULL){
        printf("which: too few arguments\n");
      }
      else{
        for (int i = 1; i < MAXARGS; i++) 
        {
          if (args[i] != NULL)
          {
            char *path = which(args[i], pathlist);
            if (path != NULL) 
            {
              printf("%s\n", path);
              //free(path);
            } 
            else 
            {
              printf("%s %s: not found\n", args[0], args[1]);
            }
            free(path);
          }
          else
          {
            break;
          }
        }
      }
    }
    else if(strcmp(command, "where") == 0){
      printf("Executing built-in WHERE\n");
      if (args[1] == NULL)
			{
				printf("where: too few arguments\n");
			}
		  else
			{
        for (int i = 1; i < MAXARGS; i++) 
        {
          if (args[i] != NULL)
          {
            char *path = where(args[i], pathlist);
            if (path != NULL) 
            {
              printf("%s\n", path);
              free(path);
            } 
            else 
            {
                printf("%s %s: not found\n", args[0], args[1]);
            }
          }
          else
          {
              break;
          }
        }   
      }
    }
    else if(strcmp(command, "pwd") == 0)
    {
      printf("Executing built-in PWD\n");
      PWD();
    }
    else if(strcmp(command,"list") == 0)
    {
      printf("executing built-in list\n");
      if ((args[1] == NULL) && (args[2] == NULL))
			{
				list(owd);
			}
			else
			{
				for (int i = 1; i < MAXARGS; i++)
				{
					if (args[i] != NULL)
					{
						printf("[%s]:\n", args[i]);
						list(args[i]);
					}
				}
			}
    }
    else if(strcmp(command,"cd") == 0){
      printf("executing built-in cd\n");
      if (args[2]!=NULL)
      {
				fprintf(stderr,"cd: too many arguments\n");
			}
			else if (args[1]) 
      {
				if (!strcmp(args[1],"-"))
        {
					strcpy(pwd,owd);
					free(owd);
					owd = getcwd(NULL,PATH_MAX+1);
					chdir(pwd);
				}
				else 
        {
					free(pwd);
					free(owd);
					owd = getcwd (NULL, PATH_MAX+1);
					chdir(args[1]);
					pwd = getcwd(NULL, PATH_MAX+1);
				}
			}
    }
    else if(strcmp(command,"pid") == 0)
    {
      printf("Executing built-in PID\n");
      printf("Shell PID: %d\n", getPID());
    }
    else if(strcmp(command, "prompt") == 0){
      printf("Executing built-in PROMPT\n");
      changePrompt(args[1],prompt);
    }
    else if(strcmp(command, "printenv") == 0)
    {
      printf("Executing built-in PRINTENV\n");
      if (args[1] == NULL)
      { 
        printENV(envp);
      }
      else if((args[1] != NULL) && (args[2] == NULL)) 
      { 
        printf("%s\n", getenv(args[1]));
      }
      else 
      {
        perror("printenv");
        printf("printenv: too many arguments\n");
      }
    }
    else if(strcmp(command, "setenv") == 0)
    {
      printf("executing built-in setenv\n");
      //zero arguments
      if(args[1] == NULL)
      {
        printENV(envp);
      }
      //one argument
      else if((args[1] != NULL) && (args[2] == NULL)) 
      { 
        setenv(args[1], "",1);
      }
      //two arguments
      else if((args[1] != NULL) && (args[2] != NULL) && (args[3] == NULL)) 
      {
        setenv(args[1],args[2],1);

        if(!strcmp(args[1], "HOME")) 
        {
					homedir = getenv("HOME");
				}
				if(!strcmp(args[1],"PATH")) 
        {
					free(pathlist);
					pathlist = get_path();
				}
			}
      //more than two arguments
      else 
      { 
				perror("setenv");
				printf("setenv: too many arguments\n");
			}
    }
    else if(strcmp(command, "kill") == 0)
    {
      printf("Executing built-in KILL\n");
      if (args[1] != NULL && args[2] == NULL)
			{
				killProcess(atoi(args[1]), 0);
			}
			else if(args[1] != NULL && args[2] != NULL)
      {
				killProcess(atoi(args[2]), -1*atoi(args[1]));
			}
    }
    
     /*  else  program to exec */
    else
    {
       /* find it */
       /* do fork(), execve() and waitpid() */
      char* cmd=which(args[0],pathlist);
			int pid=fork();
      /* do fork(), execve() and waitpid() */
			if (pid)
      {
				free(cmd);
				waitpid(pid,NULL,0);
			}
      /* else */
        /* fprintf(stderr, "%s: Command not found.\n", args[0]); */
      else{
        if (execve(cmd, args, envp) < 0)
				{
					fprintf(stderr, "%s: command not found.\n", args[0]);
					exit(0);
				}
      }
    }
    free(args);
    args = NULL;
  }
  return 0;
} /* sh() */


char *which(char *command, struct pathelement *pathlist )
{
  // loop through pathlist until finding command and return it. Return NULL when not found.
  
  char* cmd = (char*)malloc(sizeof(char)*128);
  while (pathlist) 
  { //WHICH
    sprintf(cmd, "%s/%s", pathlist->element, command);
    if (access(cmd, X_OK) == 0) 
    {
      return cmd;
    }
    pathlist = pathlist->next;
  }
  free(cmd);
  return NULL;
  
} //which ()

char *where(char *command, struct pathelement *pathlist )
{
  /* similarly loop through finding all locations of command */
  char* cmd = (char *)malloc(sizeof(char)*128);
  while (pathlist) 
  { //WHERE
    sprintf(cmd, "%s/%s", pathlist->element, command);
    if (access(cmd, F_OK) == 0) 
    {
      return cmd;
    }
    pathlist = pathlist->next;
  }
  free(cmd);
  return NULL;
} /* where() */

void PWD(){
  char* currentWorkingDirectory[PATH_MAX];
  //char *currentWorkingDirectory = (char*)malloc(sizeof(char)*128);
	getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory));
  //return currentWorkingDirectory;
  printf("%s\n", currentWorkingDirectory);
}

int getPID(){
  return getpid();
}

void changePrompt(char *command, char *p) 
{
  char buffer[128];
  int len;
  if (command == NULL) 
  {
    command = malloc(sizeof(char) * PROMPTMAX);
    printf("input prompt prefix: ");
    if (fgets(buffer, 128, stdin) != NULL) {
    len = (int) strlen(buffer);
    buffer[len - 1] = '\0';
    strcpy(command, buffer);
    }
    strcpy(p, command);
    free(command);
  }
  else 
  {
    strcpy(p, command);
  }
} /* changePrompt() */

void printENV(char **envp)
{
  char **currEnv = envp;
  while (*currEnv)
  {
    printf("%s \n", *(currEnv++));
  }
} /* printenv() */

void killProcess(pid_t pid, int sig)
{
	if (sig == 0)
  {
		kill(pid,SIGTERM);
	}
	else {
		kill(pid, sig);
	}
} /* killProcess() */

void list(char *dir)
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
  DIR *dr;
  struct dirent *de;
  dr = opendir(dir);
  if (dr == NULL) 
  {
    perror(dir);
  } 
  else 
  {
    while ((de = readdir(dr)) != NULL) 
    {
      printf("%s\n", de->d_name);
    }
  }
  closedir(dr);
} /* list() */