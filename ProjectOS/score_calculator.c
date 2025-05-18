// score_calculator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_USERS 100
#define USERNAME_LEN 32
#define BUFFER_SIZE 256

typedef struct {
    int treasure_id;
    char username[USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[128];
    int value;
} Treasure;

typedef struct {
    char username[USERNAME_LEN];
    int total_score;
} ScoreEntry;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "./%s/treasures.dat", argv[1]);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    ScoreEntry scores[MAX_USERS];
    int score_count = 0;
    Treasure t;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        int found = 0;
        for (int i = 0; i < score_count; i++) {
            if (strcmp(scores[i].username, t.username) == 0) {
                scores[i].total_score += t.value;
                found = 1;
                break;
            }
        }
        if (!found && score_count < MAX_USERS) {
            strncpy(scores[score_count].username, t.username, USERNAME_LEN);
            scores[score_count].total_score = t.value;
            score_count++;
        }
    }
    close(fd);

    for (int i = 0; i < score_count; i++) {
        printf("%s: %d\n", scores[i].username, scores[i].total_score);
    }

    return 0;
}
