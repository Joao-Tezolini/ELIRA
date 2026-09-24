#ifndef PUZZLE_H
#define PUZZLE_H

#include "common.h"

// PUZZLE_COMBINATION -> teclado numérico (disponível pra uso futuro,
//                        nenhuma sala usa no momento).
// PUZZLE_SELECTION   -> várias "cartas" clicáveis; o jogador marca quais
//                        pertencem ao conjunto correto e confirma.
//                        Usado (por baixo dos panos) nas Fases 1, 2, 4 e 5.
// PUZZLE_DRAGDROP    -> arrastar arquivos até a pasta correta.
//                        Usado no Confronto Final.
typedef enum {
    PUZZLE_COMBINATION,
    PUZZLE_SELECTION,
    PUZZLE_DRAGDROP
} PuzzleType;

typedef struct {
    char label[80];
    bool isCorrect; // faz parte do conjunto certo?
    bool selected;  // jogador marcou essa carta?
} SelectionOption;

// Um "arquivo" arrastável do puzzle da Confronto Final.
typedef struct {
    char label[64];
    bool isCompromising; // true = deveria ir pra "Proteger da IA"
    bool sorted;          // já foi arquivado corretamente?
    float homeX, homeY;   // posição original (pra onde volta se errar)
    float x, y;            // posição atual (muda enquanto está sendo arrastado)
} DragFile;

typedef struct {
    int id;
    PuzzleType type;
    bool solved;

    // --- PUZZLE_COMBINATION ---
    char correctCode[8];
    char enteredCode[8];
    int  codeLength;

    // --- PUZZLE_SELECTION ---
    SelectionOption options[MAX_SELECTION_OPTIONS];
    int optionCount;

    // --- PUZZLE_DRAGDROP ---
    DragFile files[MAX_DRAG_FILES];
    int fileCount;
    int draggingIndex;     // -1 = nenhum arquivo sendo arrastado no momento
    float dragOffsetX, dragOffsetY;
    int  feedbackFolder;   // -1 = nenhum; 0/1 = índice da pasta que recebeu o último drop
    bool feedbackCorrect;
    int  feedbackTimer;    // frames restantes pro destaque visual da pasta

    // --- comum a todos os tipos ---
    int  rewardItemId;      // 0 = nenhum item
    int  pointsAwarded;     // Pontos de Consciência Crítica ganhos ao resolver
    char successText[MAX_TEXT];
    char failText[MAX_TEXT]; // PUZZLE_SELECTION: mostrado quando erra a seleção
} Puzzle;

void Puzzle_InitCombination(Puzzle *p, int id, const char *code,
                             int rewardItemId, int points, const char *successText);
void Puzzle_PressDigit(Puzzle *p, char digit);
void Puzzle_Clear(Puzzle *p);
bool Puzzle_CheckCombination(Puzzle *p);

void Puzzle_InitSelection(Puzzle *p, int id, int rewardItemId, int points,
                           const char *successText, const char *failText);
void Puzzle_AddOption(Puzzle *p, const char *label, bool isCorrect);
void Puzzle_ToggleOption(Puzzle *p, int index);
bool Puzzle_CheckSelection(Puzzle *p); // true só se selecionadas == corretas, exatamente

void Puzzle_InitDragDrop(Puzzle *p, int id, int rewardItemId, int points,
                          const char *successText);
void Puzzle_AddFile(Puzzle *p, const char *label, bool isCompromising, float homeX, float homeY);
bool Puzzle_AllFilesSorted(Puzzle *p); // true (e marca solved) só quando todos estão 'sorted'

#endif // PUZZLE_H
