#include "item.h"
#include <string.h>

#define ITEM_DB_SIZE 16

static Item itemDatabase[ITEM_DB_SIZE];
static int  itemCount = 0;

static void AddItem(int id, const char *name, const char *desc, Color color) {
    if (itemCount >= ITEM_DB_SIZE) return;
    Item *it = &itemDatabase[itemCount++];
    it->id = id;
    strncpy(it->name, name, sizeof(it->name) - 1);
    it->name[sizeof(it->name) - 1] = '\0';
    strncpy(it->description, desc, sizeof(it->description) - 1);
    it->description[sizeof(it->description) - 1] = '\0';
    it->placeholderColor = color;
}

// -----------------------------------------------------------------
// Os 4 Artefatos de Memória (ids 1-4) contam para o cálculo do final
// do jogo (ver Game_ComputeEnding em game.c). O item 90 é só um
// utilitário de acesso à fase final e NÃO conta para o final.
// -----------------------------------------------------------------
void InitItemDatabase(void) {
    itemCount = 0;
    AddItem(1, "Disquete de Memoria",
        "O protótipo inicial e a ideia embrionária por tras do surgimento da IA. (1990)",
        (Color){40, 40, 60, 255});
    AddItem(2, "CD de Memoria",
        "A arquitetura expandida e os primeiros algoritmos avancados. (2008)",
        LIGHTGRAY);
    AddItem(3, "SSD de Memoria",
        "Formas de preservar caracteristicas humanas: voz, linguagem, comportamento. (2026)",
        (Color){60, 200, 120, 255});
    AddItem(4, "Dados em Nuvem",
        "A otimizacao do processamento e a coleta massiva de dados. (2048)",
        SKYBLUE);
    AddItem(90, "Autorizacao da Camara Quantica",
        "Acesso liberado ao nucleo central de A.R.1.3.L.",
        GOLD);
    AddItem(91, "Discernimento Demonstrado",
        "Elira classificou corretamente quais dados podem ser compartilhados com uma IA "
        "e quais precisam ser protegidos.",
        (Color){180, 60, 200, 255});
}

Item *GetItemById(int id) {
    for (int i = 0; i < itemCount; i++) {
        if (itemDatabase[i].id == id) return &itemDatabase[i];
    }
    return NULL;
}

int GetItemCount(void) { return itemCount; }
