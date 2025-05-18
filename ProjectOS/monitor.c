// monitor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define CMD_FILE "./command.txt"
#define TREASURE_FILE "treasures.dat"
#define BUFFER_SIZE 1024

int pipe_fd = -1;

typedef struct {
    int treasure_id;
    char username[32];
    float latitude;
    float longitude;
    char clue[128];
    int value;
} Treasure;

void send_output(const char* msg) {
    if (pipe_fd >= 0) {
        write(pipe_fd, msg, strlen(msg));
    }
}

void handle_command() {
    FILE* f = fopen(CMD_FILE, "r");
    if (!f) {
        send_output("Could not open command file.\n");
        return;
    }

    char command[64], hunt[64], treasure_id[64];
    command[0] = hunt[0] = treasure_id[0] = '\0';

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "COMMAND ", 8) == 0)
            sscanf(line + 8, "%s", command);
        else if (strncmp(line, "HUNT_ID ", 8) == 0)
            sscanf(line + 8, "%s", hunt);
        else if (strncmp(line, "TREASURE_ID ", 12) == 0)
            sscanf(line + 12, "%s", treasure_id);
    }
    fclose(f);

    char path[BUFFER_SIZE];
    if (strcmp(command, "list_hunts") == 0) {
        DIR* d = opendir(".");
        if (!d) {
            send_output("Failed to open current directory.\n");
            return;
        }
        struct dirent* entry;
        char msg[BUFFER_SIZE];
        while ((entry = readdir(d))) {
            if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
                snprintf(path, sizeof(path), "./%s/%s", entry->d_name, TREASURE_FILE);
                int fd = open(path, O_RDONLY);
                int count = 0;
                Treasure t;
                while (fd >= 0 && read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
                    count++;
                if (fd >= 0) close(fd);
                snprintf(msg, sizeof(msg), "Hunt: %s | Treasures: %d\n", entry->d_name, count);
                send_output(msg);
            }
        }
        closedir(d);
    } else if (strcmp(command, "list_treasures") == 0) {
        snprintf(path, sizeof(path), "./%s/%s", hunt, TREASURE_FILE);
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            send_output("Could not open treasure file.\n");
            return;
        }
        Treasure t;
        char msg[BUFFER_SIZE];
        while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
            snprintf(msg, sizeof(msg), "ID: %d, User: %s, Value: %d\n", t.treasure_id, t.username, t.value);
            send_output(msg);
        }
        close(fd);
    } else if (strcmp(command, "view_treasure") == 0) {
        snprintf(path, sizeof(path), "./%s/%s", hunt, TREASURE_FILE);
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            send_output("Could not open treasure file.\n");
            return;
        }
        Treasure t;
        while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
            if (t.treasure_id == atoi(treasure_id)) {
                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg),
                    "ID: %d, User: %s\nLocation: %.4f, %.4f\nClue: %s\nValue: %d\n",
                    t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
                send_output(msg);
                break;
            }
        }
        close(fd);
    }
}

void sigusr1_handler(int signo) {
    handle_command();
}

void sigterm_handler(int signo) {
    usleep(500000); // simulate delay
    exit(0);
}

int main() {
    pipe_fd = atoi(getenv("PIPE_WRITE_FD"));

    struct sigaction sa1 = { .sa_handler = sigusr1_handler };
    sigemptyset(&sa1.sa_mask);
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2 = { .sa_handler = sigterm_handler };
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGTERM, &sa2, NULL);

    while (1) pause();  // wait for signals
}
