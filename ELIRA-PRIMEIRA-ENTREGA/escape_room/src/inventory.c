#include "inventory.h"

void Inventory_Init(Inventory *inv) {
    inv->count = 0;
}

bool Inventory_Has(Inventory *inv, int itemId) {
    for (int i = 0; i < inv->count; i++) {
        if (inv->itemIds[i] == itemId) return true;
    }
    return false;
}

void Inventory_Add(Inventory *inv, int itemId) {
    if (Inventory_Has(inv, itemId)) return;
    if (inv->count < MAX_ITEMS) {
        inv->itemIds[inv->count++] = itemId;
    }
}
