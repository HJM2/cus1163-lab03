#include "process_manager.h"

/*
 * Function 1: Basic Producer-Consumer Demo
 * Creates one producer child (sends 1,2,3,4,5) and one consumer child (adds them up)
 */
int run_basic_demo(void) {
    int pipe_fd[2];
    pid_t producer_pid, consumer_pid;
    int status;

    printf("\nParent process (PID: %d) creating children...\n", getpid());

    // TODO 1: Create a pipe for communication
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return -1;
    }

    // TODO 2: Fork the producer process
    producer_pid = fork();
    if (producer_pid == -1) {
        perror("fork");
        // Close pipe on error
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    } else if (producer_pid == 0) {
        // Child (producer)
        close(pipe_fd[0]); // close read end
        producer_process(pipe_fd[1], 1);  // Start with number 1
        // producer_process exits
    } else {
        // Parent
        printf("Created producer child (PID: %d)\n", producer_pid);
    }

    // TODO 3: Fork the consumer process
    consumer_pid = fork();
    if (consumer_pid == -1) {
        perror("fork");
        // Parent should close ends and wait for the producer already created
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        // Try to reap producer to avoid zombie
        waitpid(producer_pid, &status, 0);
        return -1;
    } else if (consumer_pid == 0) {
        // Child (consumer)
        close(pipe_fd[1]); // close write end
        consumer_process(pipe_fd[0], 0);  // Pair ID 0 for basic demo
        // consumer_process exits
    } else {
        // Parent
        printf("Created consumer child (PID: %d)\n", consumer_pid);
    }

    // TODO 4: Parent cleanup - close pipe ends and wait for children
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Wait producer
    if (waitpid(producer_pid, &status, 0) > 0) {
        if (WIFEXITED(status)) {
            printf("Producer child (PID: %d) exited with status %d\n",
                   producer_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Producer child (PID: %d) terminated by signal %d\n",
                   producer_pid, WTERMSIG(status));
        }
    }

    // Wait consumer
    if (waitpid(consumer_pid, &status, 0) > 0) {
        if (WIFEXITED(status)) {
            printf("Consumer child (PID: %d) exited with status %d\n",
                   consumer_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Consumer child (PID: %d) terminated by signal %d\n",
                   consumer_pid, WTERMSIG(status));
        }
    }

    return 0;
}

/*
 * Function 2: Multiple Producer-Consumer Pairs
 * Creates multiple pairs: pair 1 uses numbers 1-5, pair 2 uses 6-10, etc.
 */
int run_multiple_pairs(int num_pairs) {
    pid_t pids[10]; // Store all child PIDs
    int pid_count = 0;

    printf("\nParent creating %d producer-consumer pairs...\n", num_pairs);

    // TODO 5: Create multiple producer-consumer pairs
    for (int i = 0; i < num_pairs; i++) {
        int fds[2];

        printf("\n=== Pair %d ===\n", i + 1);

        if (pipe(fds) == -1) {
            perror("pipe");
            // If pipe creation fails, wait for any children created so far
            for (int k = 0; k < pid_count; k++) {
                int st;
                waitpid(pids[k], &st, 0);
            }
            return -1;
        }

        // Fork producer
        pid_t p = fork();
        if (p == -1) {
            perror("fork");
            close(fds[0]);
            close(fds[1]);
            for (int k = 0; k < pid_count; k++) {
                int st;
                waitpid(pids[k], &st, 0);
            }
            return -1;
        } else if (p == 0) {
            // Producer child
            close(fds[0]); // close read end
            producer_process(fds[1], i * 5 + 1);
            // producer_process exits
        } else {
            // Parent
            pids[pid_count++] = p;
        }

        // Fork consumer
        pid_t c = fork();
        if (c == -1) {
            perror("fork");
            // Close ends and try to reap what we have
            close(fds[0]);
            close(fds[1]);
            for (int k = 0; k < pid_count; k++) {
                int st;
                waitpid(pids[k], &st, 0);
            }
            return -1;
        } else if (c == 0) {
            // Consumer child
            close(fds[1]); // close write end
            consumer_process(fds[0], i + 1);
            // consumer_process exits
        } else {
            // Parent
            pids[pid_count++] = c;
        }

        // Parent closes both ends after forking the pair
        close(fds[0]);
        close(fds[1]);
    }

    // TODO 6: Wait for all children
    for (int i = 0; i < pid_count; i++) {
        int status;
        pid_t done = waitpid(pids[i], &status, 0);
        if (done > 0) {
            if (WIFEXITED(status)) {
                printf("Child (PID: %d) exited with status %d\n",
                       done, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child (PID: %d) terminated by signal %d\n",
                       done, WTERMSIG(status));
            }
        }
    }

    printf("\nAll pairs completed successfully!\n");

    return 0;
}

/*
 * Producer Process - Sends 5 sequential numbers starting from start_num
 */
void producer_process(int write_fd, int start_num) {
    printf("Producer (PID: %d) starting...\n", getpid());

    // Send 5 numbers: start_num, start_num+1, start_num+2, start_num+3, start_num+4
    for (int i = 0; i < NUM_VALUES; i++) {
        int number = start_num + i;

        if (write(write_fd, &number, sizeof(number)) != sizeof(number)) {
            perror("write");
            exit(1);
        }

        printf("Producer: Sent number %d\n", number);
        usleep(100000); // Small delay to see output clearly
    }

    printf("Producer: Finished sending %d numbers\n", NUM_VALUES);
    close(write_fd);
    exit(0);
}

/*
 * Consumer Process - Receives numbers and calculates sum
 */
void consumer_process(int read_fd, int pair_id) {
    int number;
    int count = 0;
    int sum = 0;

    printf("Consumer (PID: %d) starting...\n", getpid());

    // Read numbers until pipe is closed
    while (read(read_fd, &number, sizeof(number)) > 0) {
        count++;
        sum += number;
        printf("Consumer: Received %d, running sum: %d\n", number, sum);
    }

    printf("Consumer: Final sum: %d\n", sum);
    close(read_fd);
    exit(0);
}
