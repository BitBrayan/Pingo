#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <stdint.h>

#define SCOREBOARD_MAX  10
#define SCOREBOARD_FILE "scoreboard.bin"


typedef struct {
    char     p1_name[32];
    char     p2_name[32];
    int      score_p1;
    int      score_p2;
    char     winner[32];
    char     date[20];  
    int      mode;      
} ScoreEntry;

typedef struct {
    ScoreEntry entries[SCOREBOARD_MAX];
    int        count;
    uint32_t   checksum;
} Scoreboard;

uint32_t scoreboard_checksum(const Scoreboard *sb);

int  scoreboard_load(Scoreboard *sb, const char *path);

int  scoreboard_save(const Scoreboard *sb, const char *path);

void scoreboard_insert(Scoreboard *sb, const ScoreEntry *entry);

#endif
