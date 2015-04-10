#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////
/*                        Structs and Typedefs                               */
///////////////////////////////////////////////////////////////////////////////
enum {MAXTOKENS=50, NUMBUILTINS=2, MAXARGS=20};

struct Node
{
	struct Node* next;
	struct Node* foreLink;
	char* data[MAXTOKENS];
	int number;
	int argc;
	int pipe[2];
};
typedef struct Node* Node;

struct builtin {
	char* name;
	int (*function)(int argc, char** argv);
};
typedef struct builtin Builtin;


//FUNCTION DECLARATIONS
void readLine(char *line);

///////////////////////////////////////////////////////////////////////////////
/*                              Globals                                      */
///////////////////////////////////////////////////////////////////////////////

Node head = NULL, tail = NULL;
int nodeCount = 0;
char* _buffer;
int h = 0;
char* args[MAXTOKENS];
int frees = 0;
int mallocs = 0;
Builtin builtins[2];

char *cmd1[] = { "/bin/ls", "-al", "/", 0 };
char *cmd2[] = { "/usr/bin/tr", "a-z", "A-Z", 0 };

///////////////////////////////////////////////////////////////////////////////
/*                   	Linked List Utilities                          	     */
///////////////////////////////////////////////////////////////////////////////

void add(Node newNode){
	nodeCount++;
	newNode->next = NULL;
	newNode->foreLink = NULL;

	if (head == NULL){
		head = newNode;
		tail = newNode;
	}
	else if (head == tail){
		head->next = newNode;
		newNode->foreLink = head;
		tail = newNode;
	}
	else{
		tail->next = newNode;
		newNode->foreLink = tail;
		tail = newNode;
	}

}

void printAll(){
	Node currNode, tempNode;
	int i, cmd = 1;
	printf("Your commands sir...");
	if (currNode == NULL)
		printf("currNode is NULL\n");
	for (currNode = head; currNode != NULL; currNode = currNode->next){
		for (i=0; currNode->data[i] != NULL; i++){
			if (i==0)
				printf("\nCommand %d:\n", cmd++);
			printf("arg: %s^", currNode->data[i]);
			if (currNode->data[i+1] != NULL)
				printf(", ");
		}
	}
	printf("\n\n");
}

void clearList(){
	if (head == NULL)
		return;
	Node currNode = head, tempNode;
	while (currNode != NULL){
		nodeCount--;
		tempNode = currNode;
		currNode = currNode->next;
		free(tempNode);
		frees++;
	}
	head = NULL;
	tail = NULL;
	//printf("Mallocs: %d Frees: %d\n", mallocs, frees);
}

///////////////////////////////////////////////////////////////////////////////
/*                   	    Parsing			                          	     */
///////////////////////////////////////////////////////////////////////////////

int parseString(char* stream) {
	const char* delimiters = " \n\t";
	Node newNode;
    int i = 0;
    int j = 0;
    int doubleQuote = 0, singleQuote, added;
    char* sp = stream;

	//Starting a new command
	beginning:
	newNode = malloc(sizeof(*newNode));
	newNode->argc = 0;
	j = 0;
	added = 0;

	//Adding the next argument to a command
	startup:
	doubleQuote = 0;
	singleQuote = 0;
	if (j > MAXARGS){
		printf("error: too many arguments\n");
		return 2;
	}

    //find the start of the substring, skip delimiters THIS PART IS JUST FINDING THE START
    char* startPos = sp;//Every time before we begin, we set the current position to the previous end spot!!!
    while(1) {
        for(i = 0; i < strlen(delimiters); i++) {//This iterates thru every delimiter!
            if (*startPos == '"' || *startPos == '\''){
				if (*startPos == '"')
					doubleQuote = 1;
				else if (*startPos == '\'')
					singleQuote = 1;
				startPos++;
				break;
			}
            else if(*startPos == delimiters[i] && !doubleQuote && !singleQuote){
                startPos++;
                break;
            }
        }

        if(i == strlen(delimiters)) {
               sp = startPos;//THIS SETS the start of the string
               break;
        }
    }

    //Only exit when we reach the end of the stream
    if(*sp == '\0') {
		if (j == 0)//All whitespace
			return 1;
		newNode->data[j] = 0;
		add(newNode);
        sp = NULL;
        end:

        if (doubleQuote == 1 || singleQuote == 1){
			printf("error: mismatched quote\n");
			return 1;
		}
        return 0;
    }

    //find the end of the substring, and replace the delimiter with null
    while(1) {
        for(i = 0; i < strlen(delimiters); i ++) {
			//Found space and not within quotes or within quotes and found end quote
            if((*sp == delimiters[i] && doubleQuote == 0 && singleQuote == 0) || (*sp == '"' && doubleQuote == 1) || (*sp == '\'' && singleQuote == 1)) {
			   *sp = '\0';
				newNode->data[j++] = startPos;
				newNode->argc++;
				added = 1;
				sp++;
				goto startup;
            }
            //Found pipe: end of current command
            else if (*sp == '|'){//Command is finished
            	if (doubleQuote == 1)
            		goto end;
            	*sp = '\0';
            	if (strlen(startPos) > 0){
            		newNode->data[j++] = startPos;
            		newNode->argc++;
					newNode->data[j] = 0;
					add(newNode);
				}
				else if(added){
					newNode->data[j] = 0;
					add(newNode);
				}
				sp++;
				goto beginning;//Get ready for next command
			}
			else if ((doubleQuote == 1 || singleQuote == 1) && *sp == '\0'){
				newNode->data[j++] = startPos;
				newNode->data[j] = 0;
				add(newNode);
				goto end;
			}

		}
        sp++;
    }
    goto startup;
}

///////////////////////////////////////////////////////////////////////////////
/*                   	   Builtins			                        	     */
///////////////////////////////////////////////////////////////////////////////

int exitFunction(int argc, char** argv){
	int status = 0;
	if (argc >1){
		printf("exiting with code %s\n", argv[1]);
		exit(atoi(argv[1]));
	}
	else{
		printf("exiting with code 0\n");
		exit(0);
	}
}

int cdFunction(int argc, char** argv){
	char buffer[500];
	if (argc > 2){
		printf("cd: too many arguments\n");
		return 1;
	}

	if (argc == 1){//Change to user's HOME directory
		if (chdir(getenv("HOME")) == 0){
			printf("Changing to user's HOME directory...\n");
			printf("%s\n", getcwd(buffer, 500));
		}
	}
	else if (chdir(argv[1]) == 0){//Success
		printf("%s\n", getcwd(buffer, 500));
		return 0;
	}

	else{
		perror("Error, unable to change directory");
		return 1;
	}
}


///////////////////////////////////////////////////////////////////////////////
/*                        Pipe Functions                                     */
///////////////////////////////////////////////////////////////////////////////

//cat moby.txt |tr A-Z a-z|tr -C a-z '\n' |sed  '/^$/d' |sort|uniq -c|sort -nr|sed 10q
int runCommands(){
	int fd[2];
	int n = 0;
	int in = 0;
	Node currNode;
	int status, pid;
	int saved_stdout, saved_stdin;

	if (nodeCount == 1){//only one command, no piping
		if ((pid = fork()) == 0)//child
			execvp(head->data[0], head->data);

		else
			while((pid = wait(&status)) != -1)
				fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));
	}
	else {
		for (currNode = head; currNode != NULL; currNode = currNode->next){
			pipe(currNode->pipe);//Reading end: fd[0] Writing end: fd[1]
			if ((pid = fork()) == 0){

				//Get the pid
				char pid[10];
				snprintf(pid, 10,"%d",(int)getpid());

				//We don't set the input descriptor for the head
				if (currNode->foreLink != NULL)
					dup2(currNode->foreLink->pipe[0], 0);

				close(currNode->pipe[0]);//Close after reading

				//We don't set the output descriptor for the tail
				if (currNode->next != NULL)
					dup2(currNode->pipe[1], 1);
				close(currNode->pipe[1]);

				//fprintf(stderr, "execing: %s pid: %s\n", currNode->data[0], pid);
				execvp(currNode->data[0], currNode->data);
				perror("execvp\n");
			}
			else{
				close(currNode->pipe[1]);
				if (currNode->next == NULL){
					for (currNode = head; currNode->next != NULL; currNode = currNode->next){
						close(currNode->pipe[0]);
						close(currNode->pipe[1]);
					}
				}

			}
		}
	}
	while((pid = wait(&status)) != -1);
		//fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));
}

int checkBuiltins(){
	Node currNode = head;//builtins will not be piped!
	int i;

	if (currNode == NULL)
		return -1;

	for (i=0; i < NUMBUILTINS; i++)
		if (strcmp(currNode->data[0], builtins[i].name) == 0){
			builtins[i].function(currNode->argc, currNode->data);
			return 1;
		}
	return 0;
}

int getCmd(char* buffer){
	if (isatty(0))
		fputs("$ ", stdout);
	clearList();
	memset(buffer,0,strlen(buffer));
	if(fgets(buffer, 1024, stdin) == NULL)
		exit(0);
	if (buffer[0] == 0)
		return -1;

	return 0;
}

int main (int argc, char** argv)
{
	char buffer[512];
	char* cmdString;

	//Assign function table
	builtins[0].name = "exit";
	builtins[0].function = exitFunction;
	builtins[1].name = "cd";
	builtins[1].function = cdFunction;

	//Loop for user input
	while(1){
		firstLine:
		memset(buffer,0,strlen(buffer));
		clearList();
		while(getCmd(buffer) >= 0){
			if (strlen(buffer) > 200){
				printf("error: too many arguments\n");
				goto firstLine;
			}
			if (parseString(buffer) != 0)
				goto firstLine;

			if (!checkBuiltins()){
				runCommands();
			}
		}
	}
	exit(0);
}
