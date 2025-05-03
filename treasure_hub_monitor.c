#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define CMD_FILE "./command.txt"
#define BUFFER_SIZE 256

pid_t monitor_pid = -1;
int monitor_running = 0;

void sigchld_handler(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0);
    printf("[Monitor ended with status %d]\n", status);
    monitor_running = 0;
}

void write_command_file(const char* command, const char* hunt, const char* id) {
    int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open command file");
        return;
    }
    dprintf(fd, "COMMAND %s\n", command);
    if (hunt) dprintf(fd, "HUNT_ID %s\n", hunt);
    if (id) dprintf(fd, "TREASURE_ID %s\n", id);
    close(fd);
}

void start_monitor() {
    if (monitor_running) {
        printf("Monitor already running.\n");
        return;
    }
    monitor_pid = fork();
    if (monitor_pid == 0) {
        execl("./monitor", "monitor", NULL);
        perror("execl");
        exit(1);
    } else if (monitor_pid > 0) {
        signal(SIGCHLD, sigchld_handler);
        monitor_running = 1;
        printf("Monitor started with PID %d\n", monitor_pid);
    } else {
        perror("fork");
    }
}

void send_signal(int sig) {
    if (!monitor_running) {
        printf("Monitor is not running.\n");
        return;
    }
    kill(monitor_pid, sig);
}

int main() {
    char input[BUFFER_SIZE];
    while (1) {
        printf("hub> ");
        fflush(stdout);
        if (!fgets(input, BUFFER_SIZE, stdin)) break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        } else if (strncmp(input, "list_hunts", 10) == 0) {
            write_command_file("list_hunts", NULL, NULL);
            send_signal(SIGUSR1);
        } else if (strncmp(input, "list_treasures", 14) == 0) {
            char hunt[BUFFER_SIZE];
            sscanf(input, "%*s %s", hunt);
            write_command_file("list_treasures", hunt, NULL);
            send_signal(SIGUSR1);
        } else if (strncmp(input, "view_treasure", 13) == 0) {
            char hunt[BUFFER_SIZE], id[BUFFER_SIZE];
            sscanf(input, "%*s %s %s", hunt, id);
            write_command_file("view_treasure", hunt, id);
            send_signal(SIGUSR1);
        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
            } else {
                send_signal(SIGTERM);
                monitor_running = 0;
            }
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Cannot exit while monitor is running. Use stop_monitor first.\n");
            } else {
                break;
            }
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}
