#include "diary.h"
#include <string.h>

void Diary_Init(Diary *d) {
    d->count = 0;
}

void Diary_AddEntry(Diary *d, const char *title, const char *text, bool optional) {
    if (d->count >= MAX_DIARY_ENTRIES) return;
    DiaryEntry *e = &d->entries[d->count++];
    strncpy(e->title, title, sizeof(e->title) - 1);
    e->title[sizeof(e->title) - 1] = '\0';
    strncpy(e->text, text, sizeof(e->text) - 1);
    e->text[sizeof(e->text) - 1] = '\0';
    e->optional = optional;
}
