#include "puzzle.h"
#include <string.h>

// ---------------- Combinação (cofre / painel) ----------------

void Puzzle_InitCombination(Puzzle *p, int id, const char *code,
                             int rewardItemId, int points, const char *successText) {
    memset(p, 0, sizeof(Puzzle));
    p->id = id;
    p->type = PUZZLE_COMBINATION;
    p->solved = false;

    strncpy(p->correctCode, code, sizeof(p->correctCode) - 1);
    p->codeLength = (int)strlen(p->correctCode);
    p->rewardItemId = rewardItemId;
    p->pointsAwarded = points;
    strncpy(p->successText, successText, sizeof(p->successText) - 1);
}

void Puzzle_PressDigit(Puzzle *p, char digit) {
    int len = (int)strlen(p->enteredCode);
    if (len < p->codeLength && len < (int)sizeof(p->enteredCode) - 1) {
        p->enteredCode[len] = digit;
        p->enteredCode[len + 1] = '\0';
    }
}

void Puzzle_Clear(Puzzle *p) {
    p->enteredCode[0] = '\0';
    for (int i = 0; i < p->optionCount; i++) p->options[i].selected = false;
}

bool Puzzle_CheckCombination(Puzzle *p) {
    if (strcmp(p->enteredCode, p->correctCode) == 0) {
        p->solved = true;
        return true;
    }
    return false;
}

// ---------------- Seleção (cartas corretas/erradas) ----------------

void Puzzle_InitSelection(Puzzle *p, int id, int rewardItemId, int points,
                           const char *successText, const char *failText) {
    memset(p, 0, sizeof(Puzzle));
    p->id = id;
    p->type = PUZZLE_SELECTION;
    p->solved = false;
    p->optionCount = 0;
    p->rewardItemId = rewardItemId;
    p->pointsAwarded = points;
    strncpy(p->successText, successText, sizeof(p->successText) - 1);
    strncpy(p->failText, failText, sizeof(p->failText) - 1);
}

void Puzzle_AddOption(Puzzle *p, const char *label, bool isCorrect) {
    if (p->optionCount >= MAX_SELECTION_OPTIONS) return;
    SelectionOption *opt = &p->options[p->optionCount++];
    strncpy(opt->label, label, sizeof(opt->label) - 1);
    opt->label[sizeof(opt->label) - 1] = '\0';
    opt->isCorrect = isCorrect;
    opt->selected = false;
}

void Puzzle_ToggleOption(Puzzle *p, int index) {
    if (index < 0 || index >= p->optionCount) return;
    p->options[index].selected = !p->options[index].selected;
}

bool Puzzle_CheckSelection(Puzzle *p) {
    for (int i = 0; i < p->optionCount; i++) {
        if (p->options[i].selected != p->options[i].isCorrect) return false;
    }
    p->solved = true;
    return true;
}

// ---------------- Arrastar arquivos (Confronto Final) ----------------

void Puzzle_InitDragDrop(Puzzle *p, int id, int rewardItemId, int points,
                          const char *successText) {
    memset(p, 0, sizeof(Puzzle));
    p->id = id;
    p->type = PUZZLE_DRAGDROP;
    p->solved = false;
    p->fileCount = 0;
    p->draggingIndex = -1;
    p->feedbackFolder = -1;
    p->feedbackCorrect = false;
    p->feedbackTimer = 0;
    p->rewardItemId = rewardItemId;
    p->pointsAwarded = points;
    strncpy(p->successText, successText, sizeof(p->successText) - 1);
}

void Puzzle_AddFile(Puzzle *p, const char *label, bool isCompromising, float homeX, float homeY) {
    if (p->fileCount >= MAX_DRAG_FILES) return;
    DragFile *f = &p->files[p->fileCount++];
    strncpy(f->label, label, sizeof(f->label) - 1);
    f->label[sizeof(f->label) - 1] = '\0';
    f->isCompromising = isCompromising;
    f->sorted = false;
    f->homeX = homeX;
    f->homeY = homeY;
    f->x = homeX;
    f->y = homeY;
}

bool Puzzle_AllFilesSorted(Puzzle *p) {
    if (p->fileCount == 0) return false;
    for (int i = 0; i < p->fileCount; i++) {
        if (!p->files[i].sorted) return false;
    }
    p->solved = true;
    return true;
}
