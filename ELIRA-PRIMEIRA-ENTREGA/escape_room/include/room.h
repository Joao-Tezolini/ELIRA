#ifndef ROOM_H
#define ROOM_H

#include "common.h"
#include "hotspot.h"

// Elemento puramente visual (sem clique, sem hitbox), desenhado antes dos
// hotspots. Serve pra cenário/mobília que não é interativa por si só —
// ex: a mesa embaixo da pasta clicável na Fase 1. Como não entra no array
// de hotspots, nunca é considerado pela checagem de sobreposição.
typedef struct {
    Rectangle rect;
    Color color;
    char label[64]; // aparece só como legenda do placeholder (ex: nome do asset)
} Decoration;

typedef struct {
    int id;
    char name[64];
    Color bgColor;         // placeholder de cenário (sem asset ainda)
    Hotspot hotspots[MAX_HOTSPOTS_PER_ROOM];
    int hotspotCount;
    Decoration decorations[MAX_DECORATIONS_PER_ROOM];
    int decorationCount;
} Room;

// Preenche o array de salas e escreve a contagem em roomCount.
// Toda a "história" do escape room (pistas, senhas, itens) é
// definida aqui de forma orientada a dados.
void InitRooms(Room rooms[], int *roomCount);

#endif // ROOM_H
