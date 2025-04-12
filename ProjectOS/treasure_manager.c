#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#define USERNAME_LEN 32
#define CLUE_LEN 128
#define BUFFER_SIZE 1024

typedef struct {
    int treasure_id;
    char username[USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[CLUE_LEN];
    int value;
} Treasure;

char root_path[BUFFER_SIZE] = "./";

void log_action(const char* hunt_id, const char* action) {
    char log_path[BUFFER_SIZE];
    snprintf(log_path, BUFFER_SIZE, "%s/%s/logged_hunt", root_path, hunt_id);
    log_path[BUFFER_SIZE - 1] = '\0';

    int fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("open log file");
        return;
    }

    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strcspn(time_str, "\n")] = '\0';

    dprintf(fd, "[%s] %s\n", time_str, action);
    close(fd);
}

void create_symlink(const char* hunt_id) {
    char linkname[BUFFER_SIZE];
    snprintf(linkname, BUFFER_SIZE, "%s/logged_hunt-%s", root_path, hunt_id);
    linkname[BUFFER_SIZE - 1] = '\0';

    char target[BUFFER_SIZE];
    snprintf(target, BUFFER_SIZE, "%s/%s/logged_hunt", root_path, hunt_id);
    target[BUFFER_SIZE - 1] = '\0';

    symlink(target, linkname); 
}

void add_treasure(const char* hunt_id) {
    char dir_path[BUFFER_SIZE];
    snprintf(dir_path, BUFFER_SIZE, "%s/%s", root_path, hunt_id);
    dir_path[BUFFER_SIZE - 1] = '\0';

    umask(0);
    if (mkdir(dir_path) != 0 && errno != EEXIST) {
        perror("mkdir");
        return;
    }
    

    char file_path[BUFFER_SIZE];
    snprintf(file_path, BUFFER_SIZE, "%s/treasures.dat", dir_path);
    file_path[BUFFER_SIZE - 1] = '\0';

    int fd = open(file_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("open treasure file");
        return;
    }

    Treasure t;
    printf("Enter treasure ID: ");
    scanf("%d", &t.treasure_id);
    printf("Enter username: ");
    scanf("%s", t.username);
    printf("Enter latitude: ");
    scanf("%f", &t.latitude);
    printf("Enter longitude: ");
    scanf("%f", &t.longitude);
    printf("Enter clue: ");
    getchar();
    fgets(t.clue, CLUE_LEN, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;
    printf("Enter value: ");
    scanf("%d", &t.value);

    write(fd, &t, sizeof(Treasure));
    close(fd);

    create_symlink(hunt_id);

    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, BUFFER_SIZE, "ADD %d by %s (lat: %.4f, long: %.4f)",
             t.treasure_id, t.username, t.latitude, t.longitude);
    log_msg[BUFFER_SIZE - 1] = '\0';

    log_action(hunt_id, log_msg);

    printf("Treasure added.\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s --<command> <hunt_id> [<id>]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else {
        printf("Unknown command.\n");
    }

    return 0;
}
