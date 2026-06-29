#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <stdint.h>

#define SCOREBOARD_MAX  10
#define SCOREBOARD_FILE "scoreboard.bin"

/* Uma entrada do ranking */
typedef struct {
    char     p1_name[32];
    char     p2_name[32];
    int      score_p1;
    int      score_p2;
    char     winner[32];
    char     date[20];    /* "YYYY-MM-DD HH:MM" */
    int      mode;        /* 0=1P vs CPU, 1=2 jogadores */
} ScoreEntry;

/* Estrutura persistida em disco */
typedef struct {
    ScoreEntry entries[SCOREBOARD_MAX];
    int        count;
    uint32_t   checksum;
} Scoreboard;

/* Calcula checksum XOR simples */
uint32_t scoreboard_checksum(const Scoreboard *sb);

/* Carrega do arquivo; retorna 0=ok, -1=não existe/inválido (inicializa vazio) */
int  scoreboard_load(Scoreboard *sb, const char *path);

/* Salva no arquivo; retorna 0=ok, -1=erro */
int  scoreboard_save(const Scoreboard *sb, const char *path);

/* Insere entrada mantendo ranking; descarta a 11ª se cheio */
void scoreboard_insert(Scoreboard *sb, const ScoreEntry *entry);

#endif
