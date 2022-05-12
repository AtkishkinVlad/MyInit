#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include<string.h>
#include<sys/wait.h>

#define LOG_FILENAME "/tmp/myinit.txt"
#define ERROR_FILENAME "/tmp/myinit_errors.txt"
#define ERROR_FILENAME2 "/tmp/myinit_errors2.txt"
#define NEW_WORK_DIR "/"
#define BLOCK 10

char * confFilename = NULL;
FILE * logFILE;
FILE * confFILE;
int restartFlag = 0;
int finishFlag = 0;

struct Task {
    pid_t pid;
    char * inFilename;
    char * outFilename;
    int len;
    char ** arguments;
};

struct TaskArray {
    int len;
    int capacity;
    struct Task ** taskArray;
};

struct TaskArray * ta = NULL;

void addTaskToArray(struct TaskArray * ta, struct Task * t) {
    if (ta -> len == ta -> capacity) {
        struct Task ** newArray =
            (struct Task ** ) realloc(ta -> taskArray,
                sizeof(struct Task * ) *
                ((ta -> capacity) + BLOCK));
        if (newArray == NULL) {
            fprintf(stderr, "Not enough memory\n");
            exit(1);
        }
        
        ta -> taskArray = newArray;
        ta -> capacity += BLOCK;
    }
    
    ta -> taskArray[ta -> len++] = t;
}

struct Task * getTask() {
    struct Task * t = (struct Task * ) malloc(sizeof(struct Task));
    
    if (t == NULL) {
        fprintf(stderr, "Not enough memory\n");
        exit(1);
    }
    
    t -> inFilename = NULL;
    t -> outFilename = NULL;
    t -> arguments = NULL;
    t -> len = 0;
    t -> pid = 0;
    
    return t;
}

void freeTask(struct Task * t) {
    if (t -> inFilename != NULL)
        free(t -> inFilename);
    
    if (t -> outFilename != NULL)
        free(t -> outFilename);
    
    for (int i = 0; i < t -> len; i++) {
        if (t -> arguments[i] != NULL)
            free(t -> arguments[i]);
    }
    
    if (t -> arguments != NULL)
        free(t -> arguments);
    
    free(t);
}

struct TaskArray * getTaskArray() {
    struct TaskArray * ta =
        (struct TaskArray * ) malloc(sizeof(struct TaskArray));
    
    if (ta == NULL) {
        fprintf(stderr, "Not enough memory\n");
        exit(1);
    }
    
    ta -> taskArray = NULL;
    ta -> len = 0;
    ta -> capacity = 0;
    
    return ta;
}

void freeTaskArray(struct TaskArray * ta) {
    for (int i = 0; i < ta -> len; i++)
        if (ta -> taskArray[i] != NULL)
            freeTask(ta -> taskArray[i]);
    
    if (ta -> taskArray != NULL)
        free(ta -> taskArray);
    
    ta -> len = 0;
    ta -> capacity = 0;
    
    free(ta);
}

int isPathnameAbsolute(char * pathname) {
    size_t len = strlen(pathname);
    
    if (len == 0)
        return 0;
    
    if (pathname[0] != '/')
        return 0;
    
    return 1;
}

char * addChar(char * array, int * len, int * capacity, int ch) {
    if (( * len) + 1 >= * capacity) {
        char * newArray = (char * ) realloc(array, ( * capacity) + BLOCK);
        if (newArray == NULL)
            exit(1);
        * capacity += BLOCK;
        array = newArray;
    }
    
    array[( * len) ++] = ch;
    
    return array;
}

char * readLine(FILE * file) {
    int len = 0;
    int capacity = 0;
    char * array = NULL;
    int ch;
    
    while (1) {
        ch = fgetc(file);
        if (ch == EOF) {
            if (len == 0) {
                return NULL;
            }
            break;
        }
        if (ch == '\n')
            break;
        array = addChar(array, & len, & capacity, ch);
    }
    
    array = addChar(array, & len, & capacity, '\0');
    return array;
}

char * skipSpaces(char * p) {
    while ( * p == ' ')
        p++;
    
    return p;
}

void addToArray(char ** * array, int * len, int * capacity, char * p) {
    if ( * len == * capacity) {
        char ** newArray =
            (char ** ) realloc( * array,
                sizeof(char * ) * (( * capacity) + BLOCK));
        
        if (newArray == NULL) {
            fprintf(stderr, "Not enough memory\n");
            exit(1);
        }
        
        * array = newArray;
        * capacity += BLOCK;
    }

    if (p == NULL)
        ( * array)[( * len) ++] = NULL;
    else
        ( * array)[( * len) ++] = strdup(p);
}

void splitString(char * str, char ** * array, int * len) {
    * array = NULL;
    * len = 0;
    int capacity = 0;
    char * p = str;
    
    while ( * p != '\0') {
        p = skipSpaces(p);
        if ( * p == '\0')
            break;
        
        char * p2 = strchr(p, ' ');
        
        if (p2 == NULL) {
            addToArray(array, len, & capacity, p);
            break;
        } else {
            * p2 = '\0';
            addToArray(array, len, & capacity, p);
        }
        
        p = p2 + 1;
    }
    
    addToArray(array, len, & capacity, NULL);
}

struct TaskArray * readConfig(FILE * out) {
    struct TaskArray * ta = getTaskArray();
    confFILE = fopen(confFilename, "r");
    
    if (confFILE == NULL) {
        fprintf(out, "Cannot open file '%s'\n", confFilename);
        free(confFilename);
        exit(1);
    }
    
    while (1) {
        char * line = NULL;
        line = readLine(confFILE);
        
        if (line == NULL)
            break;
        
        fprintf(out, "read line from config file: '%s'\n", line);
        struct Task * t = getTask();
        splitString(line, & t -> arguments, & t -> len);
        
        if (t -> len < 4) {
            fprintf(out, "invalid task: less than 3 parameters\n");
            freeTask(t);
        } else if (!isPathnameAbsolute(t -> arguments[0])) {
            fprintf(out, "program pathname '%s' is not absolute\n",
                t -> arguments[0]);
            freeTask(t);
        } else if (!isPathnameAbsolute(t -> arguments[t -> len - 3])) {
            fprintf(out, "input pathname '%s' is not absolute\n",
                t -> arguments[t -> len - 3]);
            freeTask(t);
        } else if (!isPathnameAbsolute(t -> arguments[t -> len - 2])) {
            fprintf(out, "output pathname '%s' is not absolute\n",
                t -> arguments[t -> len - 2]);
            freeTask(t);
        } else {
            t -> inFilename = strdup(t -> arguments[t -> len - 3]);
            t -> outFilename = strdup(t -> arguments[t -> len - 2]);
            free(t -> arguments[t -> len - 3]);
            t -> arguments[t -> len - 3] = NULL;
            free(t -> arguments[t -> len - 2]);
            t -> arguments[t -> len - 2] = NULL;
            addTaskToArray(ta, t);
            fprintf(out, "the line become task with index %d\n", ta -> len - 1);
        }
        
        free(line);
    }
    
    fclose(confFILE);
    
    return ta;
}

void startTask(int taskIndex, struct TaskArray * ta) {
    char * filename = NULL;
    FILE * f = NULL;
    filename = ta -> taskArray[taskIndex] -> inFilename;
    f = fopen(filename, "r");
    
    if (f == NULL) {
        fprintf(logFILE, "Cannot open file '%s' for reading\n", filename);
        fprintf(logFILE, "Cannot start task with index %d\n", taskIndex);
        return;
    }

    fclose(f);
    filename = ta -> taskArray[taskIndex] -> outFilename;
    f = fopen(filename, "w");
    
    if (f == NULL) {
        fprintf(logFILE, "Cannot open file '%s' for writing\n", filename);
        fprintf(logFILE, "Cannot start task with index %d\n", taskIndex);
        return;
    }
    
    fclose(f);
    unlink(filename);
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("'fork' failed");
        fprintf(logFILE, "Cannot fork and start task with index %d\n", taskIndex);
        return;
    }
    
    if (pid != 0) {
        ta -> taskArray[taskIndex] -> pid = pid;
        fprintf(logFILE, "Started task with index %d\n", taskIndex);
        return;
    }
    
    filename = ta -> taskArray[taskIndex] -> inFilename;
    
    if (freopen(filename, "r", stdin) == NULL) {
        perror("cannot change the file associated with stdin");
        exit(1);
    }
    
    filename = ta -> taskArray[taskIndex] -> outFilename;
    
    if (freopen(filename, "w", stdout) == NULL) {
        perror("cannot change the file associated with stdout");
        exit(1);
    }
    
    freopen("/dev/null", "a", stderr);
    char ** arguments = ta -> taskArray[taskIndex] -> arguments;
    execv(arguments[0], arguments);
    
    FILE * tmp = fopen(ERROR_FILENAME2, "a");
    
    fprintf(tmp, "starting task '%s' failed\n", arguments[0]);
    fclose(tmp);
    exit(1);
}

void startTasks(struct TaskArray * ta) {
    for (int i = 0; i < ta -> len; i++) {
        startTask(i, ta);
    }
}

void stopTasks(struct TaskArray * ta) {
    for (int i = 0; i < ta -> len; i++) {
        pid_t pid = ta -> taskArray[i] -> pid;
        
        if (pid != 0)
            kill(pid, SIGTERM);
    }
    
    int count = 0;
    
    for (int i = 0; i < ta -> len; i++)
        if (ta -> taskArray[i] -> pid != 0)
            count++;
    
    while (count > 0) {
        int stat_val = 0;
        pid_t finishedPID = waitpid(-1, & stat_val, 0);
        
        if (finishedPID == -1) {} else {
            int taskIndex;
            
            for (taskIndex = 0; taskIndex < ta -> len; taskIndex++)
                if (ta -> taskArray[taskIndex] -> pid == finishedPID)
                    break;
            
            if (WIFEXITED(stat_val)) {
                fprintf(logFILE, "task with index %d finished with code %d\n",
                    taskIndex, WEXITSTATUS(stat_val));
            } else if (WIFSIGNALED(stat_val)) {
                fprintf(logFILE, "task with index %d was terminated by signal with number %d\n",
                    taskIndex, WTERMSIG(stat_val));
            }
            
            count--;
        }
    }
}
void workCycle(struct TaskArray * ta) {
    while (1) {
        int stat_val = 0;
        pid_t finishedPID = waitpid(-1, & stat_val, 0);
        
        if (finishedPID == -1) {
            if (restartFlag || finishFlag)
                break;
        } else {
            int taskIndex;
            
            for (taskIndex = 0; taskIndex < ta -> len; taskIndex++)
                if (ta -> taskArray[taskIndex] -> pid == finishedPID)
                    break;
            
            if (WIFEXITED(stat_val)) {
                fprintf(logFILE, "task with index %d finished with code %d\n",
                    taskIndex, WEXITSTATUS(stat_val));
            } else if (WIFSIGNALED(stat_val)) {
                fprintf(logFILE, "task with index %d was terminated by signal with number %d\n",
                    taskIndex, WTERMSIG(stat_val));
            }
            
            ta -> taskArray[taskIndex] -> pid = 0;
            
            if (restartFlag || finishFlag)
                break;
            
            startTask(taskIndex, ta);
        }
    }
}

void signalHandler1(int s) {
    finishFlag = 1;
    fprintf(logFILE, "myinit caught signal %d\n", s);
}

void signalHandler2(int s) {
    restartFlag = 1;
    fprintf(logFILE, "myinit caught signal %d\n", s);
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Bad command line\n");
        fprintf(stderr, "Command line:\n");
        fprintf(stderr, "./myinit <config file>\n");
        return 1;
    }
    
    if (!isPathnameAbsolute(argv[1])) {
        fprintf(stderr,
            "Path name of config file '%s' is not absolute\n",
            argv[1]);
        return 1;
    }
    
    confFilename = strdup(argv[1]);
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("'fork' failed");
        return 1;
    }
    
    if (pid != 0)
        return 0;
    
    freopen("/dev/zero", "r", stdin);
    freopen("/dev/null", "a", stdout);
    freopen(ERROR_FILENAME, "w+", stderr);
    pid_t group_id = setsid();
    
    if (group_id == -1) {
        perror("'setsid' failed");
        return 1;
    }
    
    logFILE = fopen(LOG_FILENAME, "w");
    
    if (logFILE == NULL) {
        perror("Cannot create log file");
        return 1;
    }
    if (chdir(NEW_WORK_DIR) == -1) {
        perror("Cannot change working directory");
        return 1;
    }

    struct sigaction sa1;
    sa1.sa_handler = signalHandler1;
    sigemptyset( & sa1.sa_mask);
    sigaddset( & sa1.sa_mask, SIGINT);
    sigaddset( & sa1.sa_mask, SIGTERM);
    sigaddset( & sa1.sa_mask, SIGHUP);
    sa1.sa_flags = 0;
    
    if (sigaction(SIGINT, & sa1, NULL) == -1) {
        perror("Cannot set signal handler for SIGINT\n");
        return 1;
    }
    if (sigaction(SIGTERM, & sa1, NULL) == -1) {
        perror("Cannot set signal handler for SIGTERM\n");
        return 1;
    }
    
    struct sigaction sa2;
    sa2.sa_handler = signalHandler2;
    sigemptyset( & sa2.sa_mask);
    sigaddset( & sa2.sa_mask, SIGINT);
    sigaddset( & sa2.sa_mask, SIGTERM);
    sigaddset( & sa2.sa_mask, SIGHUP);
    sa2.sa_flags = 0;
    
    if (sigaction(SIGHUP, & sa2, NULL) == -1) {
        perror("Cannot set signal handler for SIGHUP\n");
        return 1;
    }
    
    fprintf(logFILE, "myinit started\n");
    
    while (1) {
        ta = readConfig(logFILE);
        startTasks(ta);
        workCycle(ta);
        
        if (restartFlag) {
            stopTasks(ta);
            restartFlag = 0;
        }
        
        freeTaskArray(ta);
        
        if (finishFlag) {
            finishFlag = 0;
            break;
        }
    }
    
    fprintf(logFILE, "myinit stoped\n");
    fclose(logFILE);
    
    return 0;
}