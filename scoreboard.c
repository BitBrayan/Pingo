#include "scoreboard.h"
#include <stdio.h>
#include <string.h>

uint32_t scoreboard_checksum(const Scoreboard *sb) {
    const uint8_t *p = (const uint8_t *)sb;
    size_t len = sizeof(Scoreboard) - sizeof(uint32_t);
    uint32_t csum = 0;
    for (size_t i = 0; i < len; i++) {
        csum ^= (uint32_t)p[i] << ((i % 4) * 8);
    }
    return csum;
}

int scoreboard_load(Scoreboard *sb, const char *path) {
    memset(sb, 0, sizeof(Scoreboard));
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    if (fread(sb, sizeof(Scoreboard), 1, f) != 1) {
        fclose(f);
        memset(sb, 0, sizeof(Scoreboard));
        return -1;
    }
    fclose(f);

    uint32_t expected = scoreboard_checksum(sb);
    if (sb->checksum != expected) {
        memset(sb, 0, sizeof(Scoreboard));
        return -1;
    }
    return 0;
}

int scoreboard_save(const Scoreboard *sb, const char *path) {
    Scoreboard tmp = *sb;
    tmp.checksum = scoreboard_checksum(&tmp);

    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    if (fwrite(&tmp, sizeof(Scoreboard), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

void scoreboard_insert(Scoreboard *sb, const ScoreEntry *entry) {
    int new_score = (entry->score_p1 > entry->score_p2) ? entry->score_p1 : entry->score_p2;

    int pos = sb->count;
    for (int i = 0; i < sb->count; i++) {
        int existing = (sb->entries[i].score_p1 > sb->entries[i].score_p2)
                     ? sb->entries[i].score_p1 : sb->entries[i].score_p2;
        if (new_score > existing) {
            pos = i;
            break;
        }
    }
    if (pos >= SCOREBOARD_MAX) return;

    int limit = sb->count < SCOREBOARD_MAX ? sb->count : SCOREBOARD_MAX - 1;
    for (int i = limit; i > pos; i--) {
        sb->entries[i] = sb->entries[i - 1];
    }

    sb->entries[pos] = *entry;
    if (sb->count < SCOREBOARD_MAX) sb->count++;
}
