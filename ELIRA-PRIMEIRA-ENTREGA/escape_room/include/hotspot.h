#ifndef HOTSPOT_H
#define HOTSPOT_H

#include "common.h"

// Todo elemento clicável de uma sala/fase é um Hotspot.
// HS_EXAMINE   -> só mostra um texto (pista, descrição, cenário)
// HS_ITEM      -> dá um item pro inventário na primeira vez que clica
// HS_PUZZLE    -> abre um puzzle (combinação, seleção de cartas, etc)
// HS_DOOR      -> tenta trocar de fase (pode exigir um item no inventário).
//                 targetRoomId == -1 é a porta especial que dispara o
//                 cálculo do final do jogo (Confronto Final).
// HS_CHOICE    -> decisão binária certo/errado (ex: seguir a IA vs verificar)
// HS_NEWSPAPER -> objeto legível com duas ações possíveis (fotografar /
//                 tirar da parede). Usado pelo jornal emoldurado da Fase 1;
//                 tirar da parede pode revelar outro hotspot escondido
//                 atrás dele (ver campo revealsHotspotId).
// HS_DOCFOLDER -> pasta com até MAX_DOCS_PER_FOLDER documentos legíveis,
//                 navegáveis por abas. Não tem "certo/errado" por si só —
//                 é uma pista para o jogador decifrar outro puzzle.
typedef enum {
    HS_EXAMINE,
    HS_ITEM,
    HS_PUZZLE,
    HS_DOOR,
    HS_CHOICE,
    HS_NEWSPAPER,
    HS_DOCFOLDER
} HotspotType;

// Uma página de documento dentro de uma HS_DOCFOLDER.
typedef struct {
    char title[64];
    char body[MAX_TEXT];
    char code[8]; // 4 dígitos exibidos no rodapé da página (nem todo código é válido)
} DocumentPage;

typedef struct {
    Rectangle rect;       // IMPORTANTE: nunca deixe rects de hotspots
                           // diferentes se sobreporem na mesma fase —
                           // o clique sempre atinge o primeiro da lista
                           // que colide, então uma sobreposição "esconde"
                           // o hotspot de baixo. Ver Game_ValidateRoomHotspots.
                           // EXCEÇÃO PROPOSITAL: um hotspot com hidden=true
                           // pode ocupar o mesmo rect de outro (ex: o cofre
                           // da Fase 1 nasce hidden no mesmo lugar do jornal
                           // que o esconde) — o validador ignora esses casos.
    HotspotType type;
    int id;
    char label[64];
    char examineText[MAX_TEXT]; // HS_EXAMINE, e texto de acerto em HS_CHOICE

    bool hidden; // true = não desenha e não recebe clique (ver nota acima)

    // HS_EXAMINE (colecionável opcional) -- ex: "Feed Recomendado" na Fase 2.
    // Igual ao jornal fotografável da Fase 1, mas sem UI própria: a 1ª vez
    // que o hotspot é examinado já gera a entrada (laranja) no diário.
    bool isOptionalCollectible;
    bool collectibleFound;

    // HS_ITEM
    int  itemId;
    bool itemCollected;

    // HS_PUZZLE
    int puzzleId;

    // HS_DOOR
    int  targetRoomId;
    bool requiresItem;
    int  requiredItemId;

    // HS_CHOICE
    bool isCorrectChoice;
    bool choiceResolved;      // já foi respondida corretamente?
    char wrongText[MAX_TEXT];
    int  choiceRewardItemId;  // 0 = nenhum item dado ao acertar

    // HS_NEWSPAPER
    char articleText[MAX_TEXT]; // conteúdo legível do jornal (imersão)
    bool photographed;
    bool removedFromWall;
    Rectangle sideRect;         // pra onde o quadro se move ao ser tirado da
                                 // parede — continua no mesmo Room, ainda
                                 // clicável (dá pra fotografar depois de mudar
                                 // de lugar), só não revela mais nada de novo
    int  revealsHotspotId;      // índice, no mesmo Room, do hotspot que
                                 // deixa de ficar 'hidden' quando este jornal
                                 // é retirado da parede (-1 = nenhum)

    // HS_DOCFOLDER
    DocumentPage documents[MAX_DOCS_PER_FOLDER];
    int documentCount;
} Hotspot;

#endif // HOTSPOT_H
