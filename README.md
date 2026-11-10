Multithreaded Process Manager:


SimulatorA C-based operating system project that simulates a process management subsystem using POSIX threads (pthreads) and synchronization primitives (mutexes and condition variables).This project models how an operating system's process table operates under concurrent workloads, tracking process states, parent-child relationships, and lifecycle management without executing actual OS processes.


Features:


Global Process Table:
Tracks up to 64 simulated processes with unique PIDs, PPIDs, state tracking, and exit statuses.


Process Lifecycle States: 
Implements lifecycle transitions across RUNNING, WAITING, and ZOMBIE states.


Process Operations API:


pm_fork(parent_pid): Spawns child processes.


pm_exit(pid, status): Terminates processes and records exit codes.


pm_wait(parent_pid, child_pid): Reaps zombie processes and cleans up resources.


pm_kill(pid): Sends termination requests.pm_ps(): Generates snapshots of active system processes. 


Concurrent Worker Threads: Reads worker task scripts concurrently to simulate multi-threaded interactions.


Real-Time Monitor Thread: Uses POSIX condition variables (pthread_cond_wait) to wait for state updates and automatically log snapshot tables to snapshots.txt without busy waiting.


Architecture Overview


               |           Process Manager            |
               | (Shared PCB Array & Synchronization) |
               +----------------------------------+
                             ^         ^
             +---------------|         |---------------+
             |                                         |
     +---------------+                         +---------------+
     | Worker Thread |                         | Monitor Thread|
     | (Parses Script|                         | (Captures     |
     |   Commands)   |                         |  Snapshots)   |
     +---------------+                         +---------------+


Getting Started:
Prerequisites:

GCC Compiler (gcc)


POSIX Threads Library (pthreads ) 


Compilation:
To compile the project with POSIX threads enabled, run:


gcc -pthread pm_sim.c -o pm_sim
Usage:


Execute the program by passing one or more worker script text files as command-line arguments:


./pm_sim thread0.txt thread1.txt

Task Script Syntax:

Each worker thread executes operations defined in a script file line-by-line:
Command-Syntax -Description :


fork -fork <parent_pid> -Creates a new process under the specified parent PID.


exit -exit <pid> <status> -Terminates a process with an exit status code.


wait -wait <parent_pid> <child_pid> -Waits for a specific child (child_pid) or any child (-1).


kill -kill <pid> -Terminates the specified process.


sleep -sleep <ms> -Pauses the worker thread for the specified milliseconds. 


Output Log: 
All state snapshots and system execution logs are automatically written to snapshots.txt upon process table updates.
