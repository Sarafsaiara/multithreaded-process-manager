#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_PROCESSES 64

typedef enum {
    RUNNING,
    WAITING,
    ZOMBIE
} State;

typedef struct {
    int pid;
    int ppid;
    State state;
    int exit_status;
} PCB;

typedef struct {
    int tid;
    char *filename;
} ThreadArg;

PCB process_table[MAX_PROCESSES];
int process_count = 0;
int next_pid = 1;

pthread_mutex_t lock;
pthread_cond_t cond;

FILE *snapshot_file;

// ================= PROCESS MANAGER =================

void notify_monitor() {
    pthread_cond_signal(&cond);
}

int find_process(int pid) {
    for (int i=0;i<process_count;i++) {
        if (process_table[i].pid==pid)
            return i;
    }
    return -1;
}

void pm_ps() {
    fprintf(snapshot_file, "PID\tPPID\tSTATE\tEXIT\n");
    fprintf(snapshot_file, "-----------------------------\n");

    for (int i=0; i<process_count;i++) {
        PCB p=process_table[i];

        fprintf(snapshot_file, "%d\t%d\t", p.pid, p.ppid);

        if (p.state==RUNNING) fprintf(snapshot_file, "RUNNING\t");
        else if (p.state == WAITING) fprintf(snapshot_file, "WAITING\t");
        else fprintf(snapshot_file, "ZOMBIE\t");

        if (p.state==ZOMBIE)
            fprintf(snapshot_file, "%d\n",p.exit_status);
        else
            fprintf(snapshot_file, "-\n");
    }
    fprintf(snapshot_file, "\n");
}

void pm_fork(int parent_pid) {
    pthread_mutex_lock(&lock);

    PCB p;
    p.pid=++next_pid;
    p.ppid=parent_pid;
    p.state=RUNNING;
    p.exit_status=-1;

    process_table[process_count++] = p;

    notify_monitor();
    pthread_mutex_unlock(&lock);
}

void pm_exit(int pid, int status) {
    pthread_mutex_lock(&lock);

    int idx=find_process(pid);
    if (idx!=-1) {
        process_table[idx].state=ZOMBIE;
        process_table[idx].exit_status=status;
    }

    notify_monitor();
    pthread_mutex_unlock(&lock);
}

void pm_wait(int parent_pid,int child_pid) {
    pthread_mutex_lock(&lock);

    for (int i=0;i<process_count;i++) {
        if (process_table[i].ppid==parent_pid &&
           (child_pid==-1 || process_table[i].pid==child_pid)) {

            if (process_table[i].state==ZOMBIE) {
                process_table[i]=process_table[--process_count];
                break;
            }
        }
    }

    notify_monitor();
    pthread_mutex_unlock(&lock);
}

void pm_kill(int pid) {
    pm_exit(pid,-1);
}

// ================= SCRIPT EXECUTION =================

void execute_command(char *cmd, int tid) {
    char op[20];
    int a, b;

    if (sscanf(cmd, "%s %d %d", op, &a, &b) >= 1) {

        pthread_mutex_lock(&lock);

        if (strcmp(op, "fork")==0)
            fprintf(snapshot_file, "Thread %d calls pm_fork %d\n\n", tid, a);

        else if (strcmp(op, "exit")==0)
            fprintf(snapshot_file, "Thread %d calls pm_exit %d %d\n\n", tid, a, b);

        else if (strcmp(op, "wait")==0)
            fprintf(snapshot_file, "Thread %d calls pm_wait %d %d\n\n", tid, a, b);

        else if (strcmp(op, "kill")==0)
            fprintf(snapshot_file, "Thread %d calls pm_kill %d\n\n", tid, a);

        else if (strcmp(op, "sleep")==0)
            fprintf(snapshot_file, "Thread %d calls sleep %d\n\n", tid, a);

        pthread_mutex_unlock(&lock);

        
        if (strcmp(op, "fork")==0)
            pm_fork(a);

        else if (strcmp(op, "exit")==0)
            pm_exit(a, b);

        else if (strcmp(op, "wait")==0)
            pm_wait(a, b);

        else if (strcmp(op, "kill")==0)
            pm_kill(a);

        else if (strcmp(op, "sleep")==0)
            usleep(a * 1000);
    }
}

// ================= THREADS =================

void *worker(void *arg) {
    ThreadArg *t=(ThreadArg*)arg;

    int tid=t->tid;
    char *filename=t->filename;

    FILE *f=fopen(filename, "r");
    if (!f) return NULL;

    char line[100];

    while (fgets(line, sizeof(line), f)) {
        execute_command(line, tid);
    }

    fclose(f);
    return NULL;
}

void *monitor(void *arg) {
    while (1) {
        pthread_mutex_lock(&lock);

        pthread_cond_wait(&cond, &lock);

        pm_ps();

        pthread_mutex_unlock(&lock);
    }
}

// ================= MAIN =================

int main(int argc, char *argv[]) {

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    snapshot_file=fopen("snapshots.txt", "w");

    fprintf(snapshot_file, "Initial Process Table\n");

    PCB init = {1,0,RUNNING, -1};
    process_table[process_count++]=init;

    pm_ps();

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor, NULL);

    pthread_t workers[argc - 1];
    ThreadArg args[argc - 1];

    for (int i=1;i<argc;i++) {
        args[i-1].tid=i-1;
        args[i-1].filename=argv[i];

        pthread_create(&workers[i-1], NULL, worker, &args[i-1]);
    }

    for (int i=0;i<argc-1;i++) {
        pthread_join(workers[i], NULL);
    }

    fclose(snapshot_file);
    return 0;
}