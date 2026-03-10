#include "process_manager.h"

int run_basic_demo(void) {
    int pipe_fd[2];
    pid_t producer_pid, consumer_pid;
    int status;

    printf("\nParent process (PID: %d) creating children...\n", getpid());

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return -1;
    }

    producer_pid = fork();
    if (producer_pid < 0) {
        perror("fork (producer)");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    }
    if (producer_pid == 0) {
        close(pipe_fd[0]);
        producer_process(pipe_fd[1], 1);
    } else {
        printf("Created producer child (PID: %d)\n", producer_pid);
    }

    consumer_pid = fork();
    if (consumer_pid < 0) {
        perror("fork (consumer)");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        waitpid(producer_pid, &status, 0);
        return -1;
    }
    if (consumer_pid == 0) {
        close(pipe_fd[1]);
        consumer_process(pipe_fd[0], 0);
    } else {
        printf("Created consumer child (PID: %d)\n", consumer_pid);
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    if (waitpid(producer_pid, &status, 0) > 0) {
        if (WIFEXITED(status))
            printf("Producer child (PID: %d) exited with status %d\n", producer_pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Producer child (PID: %d) terminated by signal %d\n", producer_pid, WTERMSIG(status));
    }

    if (waitpid(consumer_pid, &status, 0) > 0) {
        if (WIFEXITED(status))
            printf("Consumer child (PID: %d) exited with status %d\n", consumer_pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Consumer child (PID: %d) terminated by signal %d\n", consumer_pid, WTERMSIG(status));
    }

    return 0;
}

int run_multiple_pairs(int num_pairs) {
    pid_t pids[20];
    int pid_count = 0;

    printf("\nParent creating %d producer-consumer pairs...\n", num_pairs);

    for (int i = 0; i < num_pairs; i++) {
        int pipe_fd[2];
        pid_t prod_pid, cons_pid;

        printf("\n=== Pair %d ===\n", i + 1);

        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            return -1;
        }

        prod_pid = fork();
        if (prod_pid < 0) {
            perror("fork (producer)");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            return -1;
        }
        if (prod_pid == 0) {
            close(pipe_fd[0]);
            producer_process(pipe_fd[1], i * 5 + 1);
        } else {
            pids[pid_count++] = prod_pid;
            printf("Created producer for pair %d (PID: %d)\n", i + 1, prod_pid);
        }

        cons_pid = fork();
        if (cons_pid < 0) {
            perror("fork (consumer)");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            waitpid(prod_pid, NULL, 0);
            return -1;
        }
        if (cons_pid == 0) {
            close(pipe_fd[1]);
            consumer_process(pipe_fd[0], i + 1);
        } else {
            pids[pid_count++] = cons_pid;
            printf("Created consumer for pair %d (PID: %d)\n", i + 1, cons_pid);
        }

        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }

    for (int i = 0; i < pid_count; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) > 0) {
            if (WIFEXITED(status))
                printf("Child (PID %d) exited with status %d\n", pids[i], WEXITSTATUS(status));
            else if (WIFSIGNALED(status))
                printf("Child (PID %d) exited with signal %d\n", pids[i], WTERMSIG(status));
        }
    }

    printf("\nAll pairs completed successfully!\n");
    return 0;
}

void producer_process(int write_fd, int start_num) {
    printf("Producer (PID: %d) starting...\n", getpid());

    for (int i = 0; i < NUM_VALUES; i++) {
        int number = start_num + i;

        if (write(write_fd, &number, sizeof(number)) != sizeof(number)) {
            perror("write");
            exit(1);
        }

        printf("Producer: Sent number %d\n", number);
        usleep(100000);
    }

    printf("Producer: Finished sending %d numbers\n", NUM_VALUES);
    close(write_fd);
    exit(0);
}

void consumer_process(int read_fd, int pair_id) {
    int number;
    int count = 0;
    int sum = 0;

    printf("Consumer (PID: %d) starting for pair %d...\n", getpid(), pair_id);

    while (read(read_fd, &number, sizeof(number)) > 0) {
        count++;
        sum += number;
        printf("Consumer: Received %d, running sum: %d\n", number, sum);
    }

    printf("Consumer: Final sum: %d\n", sum);
    close(read_fd);
    exit(0);
}
