#ifndef ITEM_H
#define ITEM_H

#include "common.h"

// Um item colecionável. Sem assets ainda: cada item tem uma cor
// placeholder usada para desenhar um "ícone" retangular no inventário.
typedef struct {
    int id;
    char name[64];
    char description[MAX_TEXT];
    Color placeholderColor;
} Item;

void  InitItemDatabase(void);
Item *GetItemById(int id);
int   GetItemCount(void);

#endif // ITEM_H
