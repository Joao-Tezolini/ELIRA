#ifndef DIARY_H
#define DIARY_H

#include "common.h"

// Cada vez que o jogador acha um item ou resolve um puzzle,
// uma entrada é adicionada ao diário automaticamente.
typedef struct {
    char title[64];
    char text[MAX_TEXT];
    bool optional; // false = obrigatorio (titulo amarelo), true = colecionavel opcional (titulo laranja)
} DiaryEntry;

typedef struct {
    DiaryEntry entries[MAX_DIARY_ENTRIES];
    int count;
} Diary;

void Diary_Init(Diary *d);
void Diary_AddEntry(Diary *d, const char *title, const char *text, bool optional);

#endif // DIARY_H
