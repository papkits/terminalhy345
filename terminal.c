#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pwd.h>


//ArgList struct: List of all arguments of some Process

typedef struct arg_t{
    char* name;
    struct arg_t* next;
} argList;


//Process struct: Process object with all information needed to execute
typedef struct prcs_t{
    char* name;
    FILE* inputStream;
    FILE* outputStream;
    FILE* errorStream;
    argList* args;
    //char** argArray;
    int arCount;
}process;




//Proccess List Struct: List of all the processes in execusion order.
typedef struct prcList_t{
    process* prc;
    struct prcList_t* next;
    struct prcList_t* prev;
}prcList;


//process functions:
process* process_initialize(char* newName){
    process* prc = malloc(sizeof(process));
    prc->name = strdup(newName);
    prc->inputStream = stdin;
    prc->outputStream = stdout;
    prc->errorStream = stderr;
    prc->args = malloc(sizeof(argList));
    prc->args->name = strdup(newName);
    prc->args->next = NULL;
    prc->arCount = 1;
    return prc;
}

void process_inputStream(process* prc, FILE* newInput){
    prc->inputStream = newInput;
    return;
}

void process_outputStream(process* prc, FILE* newOutput){
    prc->outputStream = newOutput;
    return;
}

void process_errStream(process* prc, FILE* newErr){
    prc->errorStream = newErr;
    return;
}

char** process_argLtoA(process* prc){
    char** argArray = malloc((prc->arCount+1)*sizeof(char*));
    argList* cur = prc->args;
    for (int i = 0; i<prc->arCount; i++){
        argArray[i]=malloc((strlen(cur->name)+1)*sizeof(char));
        strcpy(argArray[i], cur->name);
        cur = cur->next;
    }
    argArray[prc->arCount] = NULL;
    return argArray;
}


//prcList functions:

prcList* prcList_initialize(void){
    prcList* list = malloc(sizeof(prcList));
    list->prc = NULL;
    list->next = NULL;
    list -> prev= NULL;
    return list;
}

void prcList_add(prcList* list, process* prc){
    prcList* cur = list;
    while(cur->next != NULL){
        cur = cur->next;
    }
    cur->next = malloc(sizeof(prcList));
    cur->next->prc = prc;
    cur->next->prev = cur;
    cur->next->next = NULL;
    return;
}

//argList functions:
argList* argList_initialize(char* argName){
    argList* argument = malloc(sizeof(argList));
    argument->name= malloc(sizeof(char)*(strlen(argName)+1));
    strcpy(argument->name, argName);
    argument->next = NULL;
    return argument;
}

void argList_add(process* prc, argList* new){
    argList* cur = prc->args;
    while(cur->next != NULL){
        cur = cur->next;
    }   
    cur->next = new;
    cur->next->next = NULL;
    prc->arCount++;
    return;
}

/**************************************************/
/*                Main program                    */
/**************************************************/

//evaluate the input and return a list of all the commands ready to be executed
prcList* evalInput(char** tokenInput, int nTokens){                      
    //Initialize states for input state machine
    enum inputState{CMD, ARG, PIP, OVRIN, OVROUT, OVRERR, OVRINAP, VAR};
    //initialize process list
    prcList* processList = prcList_initialize();
    //make a copy of the pointer so we dont lose the head
    prcList* curProcess = processList;


    //start of evaluation:
    //initialize starting state and token index:
    enum inputState state = CMD;
    int index = 0;
    //read loop;
    while(index<nTokens){

        switch (state){
            case CMD: {

                char* tkn = tokenInput[index++];            //read token
                process* prc = process_initialize(tkn);     //initialize a process with the according name
                prcList_add(processList, prc);              //add it to the process list
                curProcess = curProcess->next;              //change current process to the new one
                state = ARG;                                //change state to args so we read the arguments
                break;
            }
            
            case ARG:{
                
                char* tkn = tokenInput[index++];                  //read token
                //TODO: change state if token = >/>>/</2>/|/=
                
                argList* newArg = argList_initialize(tkn);  //create new argument
                argList_add(curProcess->prc, newArg);       //add argument to the list on the current process
                break;        
            }   
            
        }
    }

    return processList;
}

void runPrcList(prcList* processList){
    prcList* cur = processList->next;  //head is always empty

    while (cur!=NULL){
        int status;
        char** argArray = process_argLtoA(cur->prc);
        int pid = fork();
        if (pid == 0){
            execvp(cur->prc->name, (char*const*)argArray);
            //if it reaches this an error occured and we didn't execute
            perror("shell");
            exit(EXIT_FAILURE);
        }
        else{
            waitpid(-1, &status, 0);
            for (int i = 0; i < cur->prc->arCount; i++) {
                free(argArray[i]);
            }
            free(argArray);
        }
        cur = cur->next;

    }
}
    
    
void printPrompt(){
    uid_t uid = getuid();               // Get real user ID
    struct passwd *pw = getpwuid(uid);  // Look up user entry in password database
    char* username = "unknown";
    
    if (pw != NULL) {
        username=strdup(pw->pw_name);      // Print the username string
    }
    char rn[50] = "csd5739";
    char dir[1024];
    strcpy(dir, getcwd(dir, sizeof(dir)));
    printf("%s@-%s-hy345sh:%s$ ", username, rn, dir);
    return;
}
 

char** getInputTokens(int* nTokens){
    //get the whole line
    char* inputLine;
    size_t inputBuf = 0; //initialize input buffer to 0 so getline can dynamically give more memory as needed
    getline(&inputLine, &inputBuf, stdin);

    int tokens= 16;
    char** tokenInput = malloc(tokens*sizeof(char*));
    char* curToken = strtok(inputLine, " ;\t\n");
    for (int i = 0; curToken != NULL; i++) {
        tokenInput[i] = strdup(curToken);
        if (i == (tokens-1)){ //if the buffer gets full increase size by another 16
            tokens += 16;
            tokenInput = realloc(tokenInput, tokens*sizeof(char*));
        }
        *nTokens=i+1;
        curToken = strtok(NULL, " ;\t\n");
    }
    free(inputLine);
    return tokenInput;
}

void printTokens(char** tokenInput, int nTokens){
    for (int i = 0; i<nTokens; i++){
        printf("%s \n", tokenInput[i]);
    }
    return;
}

void reset(char** tokenInput,int nTokens, prcList* processList){
    //free token input:
    for (int i = 0; i<nTokens; i++){
        free(tokenInput[i]);
    }
    free(tokenInput);

    //free processes:
    prcList* curList = processList;
    prcList* nextList;
    while(curList!=NULL){
        process* prc = curList->prc;
        //free everything in the process
        if (prc!=NULL){                                             //check this so we dont free the head's prc which is null
            free(prc->name);
            if(prc->inputStream!=stdin) fclose(prc->inputStream);         /****************************************/
            if(prc->outputStream!=stdout) fclose(prc->outputStream);      /* if checks so we dont free std streams*/
            if(prc->errorStream!=stderr) fclose(prc->errorStream);        /****************************************/
            //free argument list:
            argList* curArg = prc->args;
            argList* nextArg;
            while (curArg!=NULL){
                free(curArg->name);
                nextArg = curArg->next;
                free(curArg);
                curArg=nextArg;
            }
            //free the proccess itself
        }
        free(prc);
        nextList = curList->next;
        //free curList (Don't free previous since: A) the previous has already been freed by the free(curlist) and b) on the first one prev points to null
        free(curList);
        curList=nextList;   
    }

}

int main(void){
    while (1){
        
        printPrompt();
        int nTokens;
        char** tokenInput = getInputTokens(&nTokens);
        prcList* processList = evalInput(tokenInput, nTokens);
        runPrcList(processList);
        reset(tokenInput, nTokens,processList);
    }

    return 0;
}