#ifndef COMMON_H
#define COMMON_H

#include "raylib.h"
#include <stdbool.h>

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

#define MAX_HOTSPOTS_PER_ROOM 12
#define MAX_ROOMS             8
#define MAX_ITEMS             32
#define MAX_DIARY_ENTRIES     64
#define MAX_PUZZLES           16
#define MAX_TEXT              400 // textos de item/diario/puzzle/documento podem ficar longos
#define MAX_SELECTION_OPTIONS 6
#define MAX_DRAG_FILES         6
#define MAX_DOCS_PER_FOLDER    3  // documentos legiveis dentro de uma HS_DOCFOLDER
#define MAX_DECORATIONS_PER_ROOM 4 // elementos so-visuais (ex: mesa), nao clicaveis

#endif // COMMON_H
