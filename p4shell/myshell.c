#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
int MAX_LEN = 512;
pid_t forkret1 = 0;
pid_t forkret = 0;
char* redirectFile;
int batchfd;
int isAdv;
int parseLen;
int newfd;
void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}
void printNum(int x)
{
    write(STDOUT_FILENO, &x, sizeof(x));
}
void printError()
{
    char error_message[30] = "An error has occurred\n";
    myPrint(error_message);
}
int validCommandLine(char *command)
{
    //char *temp = strdup(command);
    int i = 0;
    while(command[i] != '\0')// && strcmp(temp, "\0") != 0)
    {
        if(command[i] != ' ' && command[i] != '\t' && command[i] != '\n' && strlen(command) < 513) // if there is any character that is not one of these
            return 1;
        i++;
    }
    return 0;
}
char **parseArgs(char *pinput, int *parsedlen)
{
    char **result = (char**)malloc(sizeof(char*));
    int i = 0;
    //char current;
    char *current = (char*)malloc(sizeof(char)*512);
    while((current = strtok_r(pinput, " \t", &pinput))!= NULL) // separates individual arguments, fills array
    { 
        //myPrint(current);
        //myPrint("/");
        if( strchr(current, '>') != NULL ) // > present 
        {
            int rdind = 0;
            int adv = 0;
            while( current[rdind] != '\0')
            {
                if(current[rdind] == '>') //first instance of >
                {
                    
                    if(rdind != 0)
                    {
                        result[i] = strndup(current, rdind);
                        //myPrint("first: ");
                        //strncpy(result[i], current, rdind);  
                        i++;
                    }
                    if(current[rdind+1] == '+')
                    {
                        adv = 1;
                        result[i] = strdup(">+");
                        rdind++;
                    }    
                    else
                    {
                        result[i] = strdup(">");
                    }
                    break;          
                }
                rdind++; // where is the >
            }
            
            i++;
            if(adv == 0)
                current = strchr(current, '>') + 1; // after >
            if(adv == 1)
                current = strchr(current, '+') + 1;
            //result = realloc(result, sizeof(char*)*i);
        }
        
        if( current != NULL && strlen(current) != 0 && strcmp(current, "\0") != 0)// && strcmp(current, "\t") != 0)
        {
            result[i] = strdup(current);
            //myPrint("bad length");
        }
            
        else
        {
            i--;
        }
        
        i++;
        //myPrint(current);
          //  myPrint("/");
       // result = realloc(result, sizeof(char*)*i);
    }
    if(i < 1)
        return NULL;
    *parsedlen = i;
    result[i] = NULL;
    return result;
}
int numRedirects(char **parsed, int plen)
{
    //int present = 0;
    int i = 0;
    int num = 0;
    while(i < plen)
    {
        if(strcmp(parsed[i], ">") == 0 || strcmp(parsed[i], ">+") == 0)
        {
            num++;
            
        }
        i++;
     }
     return num;//present; // how many redirections??
}
void setRedirectFile(char **parsed) // only called if one redirection present
{
    int i = 0;
    while(i < parseLen)
    {
        if(strcmp(parsed[i], ">") == 0 || strcmp(parsed[i], ">+") == 0)
            redirectFile = parsed[i+1];
        i++;
    } 
}
char **redirect(char **parsed) // only called if not built in and if theres only one redirect
{
    char **newparsed = (char**)malloc(sizeof(char*));
    int basic = 0;
    isAdv = 0;
    int i = 0;
    int direfd = 0;
    
    while(i < parseLen)
    {
        if(strcmp(parsed[i], ">") == 0)
            basic = 1;
        if(strcmp(parsed[i], ">+") == 0)
            isAdv = 1;
        if(isAdv || basic) // this character at i is the redirection character 
        {           
            if(parsed[i+1] == NULL) // file not specified
            {
                //myPrint("no redirect file specified\n");
                printError();
                _exit(1);
                break;
            }                      
            if(i+2 < parseLen) // more than one arg
            {
                //myPrint("more than one arg\n");
                printError();
                _exit(1);
                break;
            }
            
            // if file exists
            if(access(parsed[i+1], F_OK) == 0) 
            {
                if(basic)
                {
                    //myPrint("redirect file exists\n");
                    printError();
                    _exit(1); // exit child process
                    break;
                }
                else if(isAdv)
                {
                    int tempfd = open("temp", O_CREAT|O_WRONLY|O_TRUNC|O_APPEND, 0777);
                    int oldfd = open(parsed[i+1], O_RDONLY, 0664);
                    
                    char buf[512];
                    int r;
                    while((r = read(oldfd, buf, 512)))
                        write(tempfd, buf, r);
                    close(oldfd);
                    
                    close(tempfd);
                    newfd = open(parsed[i+1], O_CREAT|O_WRONLY|O_TRUNC|O_APPEND, 0664);
                    
                    lseek(newfd, 0, SEEK_SET);
                    
                    dup2(newfd, STDOUT_FILENO);
                    //close(mainfd);
                }
            }
            else // file does not exist
            {
                //myPrint("making new file");
                // check if dir exists
                char *targetfile = strrchr(parsed[i+1], '/');
                if(targetfile != NULL) // if its a directory
                {
                    int dirlen = strlen(parsed[i+1]) - strlen(targetfile);
                    char *dirpath = strndup(parsed[i+1], dirlen);
                  //  myPrint(dirpath);
                    DIR* dir = opendir(dirpath);
                    if(dir)
                    {
                        direfd = dirfd(dir);
                    }
                    if(direfd > 0) // if its a directory that exists
                    {
                        newfd = openat(direfd, targetfile+1, O_CREAT|O_RDWR|O_APPEND, 0664);
                    }
                    else  //directory does not exist
                    {
                        printError();
                        _exit(0);
                    }
                }
                
                //myPrint("file does not exists! creating new file\n");
                
                else // not a directory
                {
                   // myPrint("opening file");
                    newfd = open(parsed[i+1], O_CREAT|O_RDWR|O_TRUNC, 0664);//creat(parsed[i+1], O_RDWR);
                }
                dup2(newfd, STDOUT_FILENO);
                //fflush(stdout);
                //close(newfd);
                
            }
            free(parsed[i]);
            free(parsed[i+1]);
            while(i+2 < parseLen) // new parsed array without the redirections
            {
                //free(newparsed[i]);
                newparsed[i] = strdup(parsed[i+2]);
                free(parsed[i+2]);
                
                i++;
            }
            parseLen = i;
            free(parsed);
            newparsed[i] = NULL;
            return newparsed;
        }
        else // no redirect means same array
        {
            newparsed[i] = strdup(parsed[i]);
            free(parsed[i]);
        }
        i++;
    }
    free(parsed);
    newparsed[i] = NULL;
    return newparsed;
}
void execute(int mode)
{
    char cmd_buff[514];
    char *pinput, *newpinput;
    FILE *batchfile;
    //signal(SIGUSR1, oob_handler);
    if(mode == 1)
        {
            if (!batchfd)
            {
                printError();
            }
            batchfile = fdopen(batchfd, "r");
        }
    
    while (1) 
    {
        if(mode == 0)
        {
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 514, stdin);
        }
        else if(mode == 1)
        {
            if((pinput = fgets(cmd_buff, 514, batchfile)) == NULL)
            {
                exit(0);
            }
            if(validCommandLine(pinput))
            {
                myPrint(pinput);
                //myPrint("\n");
            }
            
        }
        
        if (!pinput)// || strcmp(pinput, "exit\n") == 0) 
        {
            exit(0);
        }
        //printf("%lu\n", strlen(pinput));
        if(strlen(pinput) >= 512)
        {
            myPrint(pinput); //first 512 characters
            while( pinput != NULL && pinput[strlen(pinput)-1] != '\n' ) //(pinput = (mode ? fgets(cmd_buff, 514, batchfile) : fgets(cmd_buff, 514, stdin))) != NULL && pinput[strlen(pinput)-1] != '\n' )
            {
                pinput = (mode ? fgets(cmd_buff, 514, batchfile) : fgets(cmd_buff, 514, stdin));
                myPrint(pinput); 
            }
            printError();
        }
        
        
        else
        {
            if((pinput = strtok_r(pinput, "\n", &pinput)) != NULL) // ignore enter 
            {
                
                char **parsed;
                while( (newpinput = strtok_r(pinput, ";", &pinput)) != NULL) // separates multiple commands
                { 
                    //new pinput is the first command
                    parsed = parseArgs(newpinput, &parseLen);
                    
                    
                    if(parsed != NULL)
                    {
                        
                    
                        int isRedirect = numRedirects(parsed, parseLen); // how many redirects r therels 
                        
        
                        if(isRedirect > 1) // more than one redirect? error
                        {
                            //myPrint("more than one redirection: ");
                            printError();
                        }
                        else if(strcmp(parsed[0], "exit") == 0)
                        {
                            if(isRedirect > 0 || parsed[1] != NULL) // built in cant have redirect
                            {
                                //myPrint("exit redirection not allowed\nonly one command allowed\n");
                                printError();
                            }
                            else
                                exit(0);
                        }
                        else if(strcmp(parsed[0], "cd") == 0) // built in
                        {
                            if(isRedirect > 0)
                            {
                            //  myPrint("built in redirect not allowed \n");
                                printError();
                            }
                            else if(parsed[1] == NULL)
                            {
                                chdir(getenv("HOME"));
                            }
                            else if(chdir(parsed[1]) < 0)// || redirected == 1)
                            {
                                //chdir(parsed[1]);
                                printError();
                            }
                            else if(parseLen > 2)
                            {
                                printError();
                            }
                        }
                        else if(strstr(parsed[0], "cd") != NULL && strcmp(parsed[0], "cd") == 0)
                        {
                            //chdir(parsed[0]+2);
                            printError();
                        }
                        else if(strcmp(parsed[0], "pwd") == 0 )//|| redirected == 1) // built in
                        {
                            if(isRedirect > 0)
                            {
                                //myPrint("pwd built in redirect not allowed\n");
                                printError();
                            }
                            else if(parseLen > 1)
                            {
                                printError();
                            }
                            else
                            {
                                char cwd[512];
                                getcwd(cwd, 514);
                                myPrint(cwd);
                                myPrint("\n");
                            }
                        //;
                        }
                        else
                        {
                            if(isRedirect == 1)
                                setRedirectFile(parsed); // before fork
                            
                            forkret = fork(); // new process for every command, forkret = pid
                            if(forkret == 0)
                            {
                                if(isRedirect == 1)
                                {
                                    
                                    parsed = redirect(parsed);
                                }
                                
                                if(execvp(parsed[0], parsed) < 0)
                                {
                                    //printf("error: %d\n", errno);
                                    printError();
                        
                                }
                                //fflush(stdout);
                                //close(newfd);
                                _exit(0);
                            }
                            else // parent
                            {
                                int status;
                                if(waitpid(forkret, &status, 0) < 0) // wait for child to exit the process
                                    printError();
                                //if(isAdv)
                                {
                                    char oldcontents[512];
                                    int tempfd = open("temp", O_RDONLY, 0777);                                    
                                    if(tempfd >0)
                                    {                                        
                                        //myPrint(oldcontents);
                                        newfd = open(redirectFile, O_RDWR|O_APPEND, 0664);
                                        lseek(newfd, 0, SEEK_END);
                                        if(newfd > 0)
                                        {
                                            int r;
                                            while((r = read(tempfd, oldcontents, 512)))
                                                write(newfd, oldcontents, r);
                                        }
                                    }
                                    remove("temp");
                               }
                                
                            }
                        }
                    }
                    free(parsed);
                }
            }
        }
        
        
        //myPrint(cmd_buff);
    }
}
int main(int argc, char *argv[]) 
{
    int batchMode = 0;
    if(argc == 2)
    {
        batchMode = 1;
        batchfd = open(argv[1], O_RDONLY);
        if(batchfd < 0)
        {
            printError();
            exit(1);
        }
    }
    else if(argc > 2)
    {
        printError();
        exit(1);
    }
    execute(batchMode);
}
