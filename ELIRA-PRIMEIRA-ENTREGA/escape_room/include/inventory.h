#ifndef INVENTORY_H
#define INVENTORY_H

#include "common.h"

typedef struct {
    int itemIds[MAX_ITEMS];
    int count;
} Inventory;

void Inventory_Init(Inventory *inv);
void Inventory_Add(Inventory *inv, int itemId);
bool Inventory_Has(Inventory *inv, int itemId);

#endif // INVENTORY_H
