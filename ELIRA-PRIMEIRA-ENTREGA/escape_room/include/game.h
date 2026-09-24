#ifndef GAME_H
#define GAME_H

#include "common.h"
#include "room.h"
#include "inventory.h"
#include "diary.h"
#include "puzzle.h"

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PUZZLE,
    STATE_MESSAGE,    // overlay de texto (exame/pista/aviso/escolha)
    STATE_DIARY,      // "Diário de Bordo"
    STATE_NEWSPAPER,  // overlay do jornal emoldurado (ler / fotografar / tirar da parede)
    STATE_DOCS,       // overlay de uma pasta com documentos legíveis (HS_DOCFOLDER)
    STATE_ENDING      // Final Ruim / Normal / Ótimo
} GameState;

typedef enum {
    ENDING_NONE,
    ENDING_BAD,     // Final Ruim
    ENDING_NORMAL,  // Final Normal
    ENDING_GREAT    // Final Ótimo
} EndingType;

typedef struct {
    GameState state;
    GameState previousState; // pra saber pra onde voltar ao fechar overlay

    Room rooms[MAX_ROOMS]; // cada Room = uma Fase/Era do roteiro do ELIRA
    int  roomCount;
    int  currentRoomId;

    Inventory inventory;   // guarda os Artefatos de Memória coletados
    Diary diary;           // "Diário de Bordo"

    Puzzle puzzles[MAX_PUZZLES];
    int puzzleCount;
    int activePuzzleId; // -1 = nenhum puzzle aberto

    int lives;           // perder todas força o Final Ruim (Fase 3)
    int criticalPoints;  // Pontos de Consciência Crítica
    EndingType ending;

    int docsPageIndex;   // qual documento (0..MAX_DOCS_PER_FOLDER-1) está aberto em STATE_DOCS
    int diaryPageIndex;  // qual entrada (0..diary.count-1) está aberta em STATE_DIARY

    char messageTitle[64];
    char messageText[MAX_TEXT];
} Game;

void Game_Init(Game *g);
void Game_Update(Game *g);
void Game_Draw(Game *g);

Puzzle *Game_GetPuzzleById(Game *g, int id);
Room   *Game_GetCurrentRoom(Game *g);
EndingType Game_ComputeEnding(Game *g);

#endif // GAME_H
