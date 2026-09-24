#include "game.h"
#include "item.h"
#include <stdio.h>
#include <string.h>

// Texto provisorio para entradas de diario (obrigatorias ou opcionais) cujo
// conteudo definitivo ainda nao foi escrito. TODO: substituir pelo texto
// real de cada colecionavel opcional.
static const char *DIARY_LOREM_IPSUM =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex "
    "ea commodo consequat.";

// -----------------------------------------------------------------
// Helpers de layout gerais
// -----------------------------------------------------------------

static Rectangle DiaryButtonRect(void) {
    return (Rectangle){ SCREEN_WIDTH - 160, 20, 140, 40 };
}

static Rectangle CloseButtonRect(void) {
    return (Rectangle){ SCREEN_WIDTH / 2 + 260, SCREEN_HEIGHT / 2 - 220, 40, 40 };
}

static Rectangle DiaryPanelRect(void) {
    return (Rectangle){ SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 2 - 260, 640, 520 };
}

static Rectangle DiaryPrevButtonRect(void) {
    Rectangle panel = DiaryPanelRect();
    return (Rectangle){ panel.x + 20, panel.y + panel.height - 55, 110, 36 };
}

static Rectangle DiaryNextButtonRect(void) {
    Rectangle panel = DiaryPanelRect();
    return (Rectangle){ panel.x + panel.width - 130, panel.y + panel.height - 55, 110, 36 };
}

static void ShowMessage(Game *g, const char *title, const char *text) {
    strncpy(g->messageTitle, title, sizeof(g->messageTitle) - 1);
    g->messageTitle[sizeof(g->messageTitle) - 1] = '\0';
    strncpy(g->messageText, text, sizeof(g->messageText) - 1);
    g->messageText[sizeof(g->messageText) - 1] = '\0';
    g->previousState = STATE_PLAYING;
    g->state = STATE_MESSAGE;
}

static void DrawWrappedText(const char *text, int x, int y, int fontSize, int maxWidth, Color color) {
    char buffer[512];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char line[256] = "";
    int cursorY = y;
    char *word = strtok(buffer, " ");
    while (word != NULL) {
        char test[256];
        if (line[0] == '\0') snprintf(test, sizeof(test), "%s", word);
        else snprintf(test, sizeof(test), "%s %s", line, word);

        if (MeasureText(test, fontSize) > maxWidth) {
            DrawText(line, x, cursorY, fontSize, color);
            cursorY += fontSize + 6;
            snprintf(line, sizeof(line), "%s", word);
        } else {
            snprintf(line, sizeof(line), "%s", test);
        }
        word = strtok(NULL, " ");
    }
    if (line[0] != '\0') DrawText(line, x, cursorY, fontSize, color);
}

static void DrawPanelBox(Rectangle panel, const char *title) {
    DrawRectangleRec(panel, Fade(BLACK, 0.9f));
    DrawRectangleLinesEx(panel, 3, RAYWHITE);
    DrawText(title, (int)panel.x + 20, (int)panel.y + 15, 24, YELLOW);

    Rectangle closeBtn = CloseButtonRect();
    DrawRectangleRec(closeBtn, MAROON);
    DrawText("X", (int)closeBtn.x + 13, (int)closeBtn.y + 8, 20, WHITE);
}

// Acha o primeiro hotspot de um tipo numa sala. Usado pelos overlays de
// jornal/documentos, que hoje assumem no máximo um de cada por sala (só
// a Fase 1 tem um jornal e uma pasta de documentos até o momento).
static Hotspot *FindHotspotByType(Room *r, HotspotType type) {
    for (int i = 0; i < r->hotspotCount; i++) {
        if (r->hotspots[i].type == type) return &r->hotspots[i];
    }
    return NULL;
}

// -----------------------------------------------------------------
// Validação de hitboxes (avisa no console se dois hotspots da
// mesma fase se sobrepoem)
// -----------------------------------------------------------------

static void Game_ValidateRoomHotspots(Room *r) {
    for (int i = 0; i < r->hotspotCount; i++) {
        for (int j = i + 1; j < r->hotspotCount; j++) {
            // Sobreposição proposital (ex: cofre 'hidden' nascendo atrás do
            // jornal que o esconde) não é um bug — ver nota em hotspot.h.
            if (r->hotspots[i].hidden || r->hotspots[j].hidden) continue;
            if (CheckCollisionRecs(r->hotspots[i].rect, r->hotspots[j].rect)) {
                TraceLog(LOG_WARNING,
                    "[HITBOX] Sala '%s': hotspot '%s' (id %d) se sobrepoe com '%s' (id %d). "
                    "O clique so vai acertar '%s'.",
                    r->name, r->hotspots[i].label, i, r->hotspots[j].label, j, r->hotspots[i].label);
            }
        }
    }
}

// ===================================================================
// PUZZLES — cada sala tem sua própria função de update e de desenho.
// Fases 1, 2, 4 e 5 hoje reaproveitam a mesma mecânica de "seleção de
// cartas" por baixo (UpdatePuzzleSelection/DrawSelectionPanel), mas
// cada uma já é uma função independente — dá pra customizar qualquer
// uma delas sem afetar as outras. O Confronto Final tem uma mecânica
// própria (arrastar arquivos), totalmente separada.
// ===================================================================

// ---------------- Mecânica compartilhada: seleção de cartas ----------------
// (usada por baixo dos panos pelas Fases 1, 2, 4 e 5)

static Rectangle SelectionPanelRect(void) {
    return (Rectangle){ SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 2 - 260, 640, 520 };
}

static void ResolvePuzzleReward(Game *g, Puzzle *p) {
    g->criticalPoints += p->pointsAwarded;
    if (p->rewardItemId != 0) {
        Inventory_Add(&g->inventory, p->rewardItemId);
        Item *it = GetItemById(p->rewardItemId);
        if (it) Diary_AddEntry(&g->diary, it->name, it->description, false);
    }
}

static void UpdatePuzzleSelection(Game *g, Puzzle *p) {
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, CloseButtonRect())) {
        g->state = STATE_PLAYING;
        return;
    }
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    Rectangle panel = SelectionPanelRect();
    int cardW = 280, cardH = 70, gapX = 20, gapY = 16;
    float startX = panel.x + 30, startY = panel.y + 70;

    for (int i = 0; i < p->optionCount; i++) {
        int col = i % 2, row = i / 2;
        Rectangle card = { startX + col * (cardW + gapX), startY + row * (cardH + gapY), cardW, cardH };
        if (CheckCollisionPointRec(mouse, card)) {
            Puzzle_ToggleOption(p, i);
            return;
        }
    }

    Rectangle confirmBtn = { panel.x + panel.width / 2 - 90, panel.y + panel.height - 70, 180, 45 };
    if (CheckCollisionPointRec(mouse, confirmBtn)) {
        if (Puzzle_CheckSelection(p)) {
            ResolvePuzzleReward(g, p);
            ShowMessage(g, "Resolvido!", p->successText);
        } else {
            ShowMessage(g, "Selecao incorreta", p->failText);
            for (int i = 0; i < p->optionCount; i++) p->options[i].selected = false;
        }
    }
}

static void DrawSelectionPanel(Puzzle *p, const char *title) {
    Rectangle panel = SelectionPanelRect();
    DrawPanelBox(panel, title);

    DrawText("Selecione as opcoes corretas e confirme:",
        (int)panel.x + 30, (int)panel.y + 55, 18, RAYWHITE);

    int cardW = 280, cardH = 70, gapX = 20, gapY = 16;
    float startX = panel.x + 30, startY = panel.y + 70;
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < p->optionCount; i++) {
        int col = i % 2, row = i / 2;
        Rectangle card = { startX + col * (cardW + gapX), startY + row * (cardH + gapY), cardW, cardH };
        bool hovered = CheckCollisionPointRec(mouse, card);
        Color fill = p->options[i].selected ? Fade(SKYBLUE, 0.6f) : Fade(DARKGRAY, hovered ? 0.6f : 0.4f);
        DrawRectangleRec(card, fill);
        DrawRectangleLinesEx(card, 2, p->options[i].selected ? YELLOW : RAYWHITE);
        DrawWrappedText(p->options[i].label, (int)card.x + 10, (int)card.y + 8, 14, cardW - 20, WHITE);
    }

    Rectangle confirmBtn = { panel.x + panel.width / 2 - 90, panel.y + panel.height - 70, 180, 45 };
    bool confirmHover = CheckCollisionPointRec(mouse, confirmBtn);
    DrawRectangleRec(confirmBtn, confirmHover ? Fade(GREEN, 0.7f) : Fade(GREEN, 0.4f));
    DrawRectangleLinesEx(confirmBtn, 2, RAYWHITE);
    DrawText("CONFIRMAR", (int)confirmBtn.x + 25, (int)confirmBtn.y + 13, 18, WHITE);
}

// ---- Fase 1 (1990): cofre de combinação embutido na parede ----
// A senha correta ("1950") vem do documento "Verificando a Fonte" dentro
// da Pasta de Estudos sobre IA (HS_DOCFOLDER); os outros dois documentos
// têm códigos-isca que não abrem o cofre. Ver rooms_data.c.

static Rectangle Fase1SafePanelRect(void) {
    // Altura calculada pra sobrar 30px acima e abaixo do teclado numerico
    // (startY relativo = 140, grid de 4 linhas de 70px com 12px de gap = 316px).
    const int keypadTop = 140, keypadHeight = 4 * 70 + 3 * 12;
    const int height = keypadTop + keypadHeight + 30;
    return (Rectangle){ SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT / 2 - height / 2, 440, (float)height };
}

static void UpdatePuzzle_Fase1(Game *g, Puzzle *p) {
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, CloseButtonRect())) {
        g->state = STATE_PLAYING;
        return;
    }
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    Rectangle panel = Fase1SafePanelRect();
    const char *labels[12] = {"1","2","3","4","5","6","7","8","9","C","0","OK"};
    const int cols = 3, btnSize = 70, gap = 12;
    float startX = panel.x + 40, startY = panel.y + 140;

    for (int i = 0; i < 12; i++) {
        int row = i / cols, col = i % cols;
        Rectangle btn = { startX + col * (btnSize + gap), startY + row * (btnSize + gap), btnSize, btnSize };
        if (!CheckCollisionPointRec(mouse, btn)) continue;

        if (strcmp(labels[i], "C") == 0) {
            Puzzle_Clear(p);
        } else if (strcmp(labels[i], "OK") == 0) {
            if (Puzzle_CheckCombination(p)) {
                // Recompensa customizada: o texto do Diário aqui é específico
                // dessa descoberta (o disquete achado atrás do jornal), então
                // não reaproveitamos ResolvePuzzleReward/descrição genérica
                // do item — a entrada do diário é escrita à mão como pedido.
                g->criticalPoints += p->pointsAwarded;
                Inventory_Add(&g->inventory, p->rewardItemId);
                Diary_AddEntry(&g->diary, "Disquete de Correcao",
                    "Ainda nao se sabe ao certo o conteudo gravado nele, mas pode ser util "
                    "mais a frente na nossa jornada. Como estava guardado atras desse "
                    "jornal, provavelmente pode estar relacionado a boas praticas de "
                    "machine learning.", false);
                ShowMessage(g, "Cofre Aberto", p->successText);
            } else {
                Puzzle_Clear(p);
            }
        } else {
            Puzzle_PressDigit(p, labels[i][0]);
        }
        break;
    }
}

static void DrawPuzzle_Fase1(Puzzle *p) {
    Rectangle panel = Fase1SafePanelRect();
    DrawPanelBox(panel, "Cofre Embutido na Parede");

    DrawText("Digite o codigo de 4 digitos:", (int)panel.x + 30, (int)panel.y + 60, 18, RAYWHITE);
    DrawRectangle((int)panel.x + 30, (int)panel.y + 90, 380, 40, Fade(RAYWHITE, 0.1f));
    DrawText(p->enteredCode, (int)panel.x + 40, (int)panel.y + 98, 24, GREEN);

    const char *labels[12] = {"1","2","3","4","5","6","7","8","9","C","0","OK"};
    const int cols = 3, btnSize = 70, gap = 12;
    float startX = panel.x + 40, startY = panel.y + 140;
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < 12; i++) {
        int row = i / cols, col = i % cols;
        Rectangle btn = { startX + col * (btnSize + gap), startY + row * (btnSize + gap), btnSize, btnSize };
        bool hovered = CheckCollisionPointRec(mouse, btn);
        DrawRectangleRec(btn, hovered ? Fade(SKYBLUE, 0.5f) : Fade(DARKGRAY, 0.6f));
        DrawRectangleLinesEx(btn, 2, RAYWHITE);
        DrawText(labels[i], (int)(btn.x + btnSize / 2 - 8), (int)(btn.y + btnSize / 2 - 12), 22, WHITE);
    }
}

// ---------------- Fase 1: overlays auxiliares (jornal e documentos) ----------------
// Não são "puzzles" (não têm certo/errado, não passam por STATE_PUZZLE) —
// são interações próprias, com seus próprios GameState. Hoje só a Fase 1
// usa isso, então os overlays localizam o hotspot ativo procurando por
// tipo na sala atual (FindHotspotByType) em vez de guardar um id global.

static Rectangle NewspaperPanelRect(void) {
    return (Rectangle){ SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 2 - 260, 640, 480 };
}

static Rectangle NewspaperPhotoButtonRect(Rectangle panel) {
    return (Rectangle){ panel.x + 40, panel.y + panel.height - 70, 250, 45 };
}

static Rectangle NewspaperRemoveButtonRect(Rectangle panel) {
    return (Rectangle){ panel.x + panel.width - 290, panel.y + panel.height - 70, 250, 45 };
}

static void UpdateNewspaper(Game *g) {
    Room *room = Game_GetCurrentRoom(g);
    Hotspot *news = room ? FindHotspotByType(room, HS_NEWSPAPER) : NULL;
    if (!news) { g->state = STATE_PLAYING; return; }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, CloseButtonRect())) {
        g->state = STATE_PLAYING;
        return;
    }

    Rectangle panel = NewspaperPanelRect();

    if (CheckCollisionPointRec(mouse, NewspaperPhotoButtonRect(panel))) {
        if (!news->photographed) {
            news->photographed = true;
            Diary_AddEntry(&g->diary, "Fotografia do Jornal", DIARY_LOREM_IPSUM, true);
            ShowMessage(g, "Fotografia Registrada",
                "Elira fotografa o jornal emoldurado, guardando uma copia do artigo "
                "para consultar mais tarde, sem precisar tira-lo da parede.");
        } else {
            ShowMessage(g, "Jornal Emoldurado", "Voce ja fotografou este jornal.");
        }
        return;
    }

    if (CheckCollisionPointRec(mouse, NewspaperRemoveButtonRect(panel))) {
        if (!news->removedFromWall) {
            news->removedFromWall = true;
            news->rect = news->sideRect; // o quadro se move pro lado, continua clicavel
            if (news->revealsHotspotId >= 0 && news->revealsHotspotId < room->hotspotCount) {
                room->hotspots[news->revealsHotspotId].hidden = false;
            }
            ShowMessage(g, "Cofre Revelado",
                "Elira desloca o jornal emoldurado para o lado, encostando-o na "
                "parede. No lugar onde ele estava pendurado, embutido na parede, "
                "havia um pequeno cofre com um teclado numerico.");
        } else {
            ShowMessage(g, "Jornal Emoldurado",
                "O quadro ja foi deslocado para o lado. O cofre na parede continua acessivel.");
        }
        return;
    }
}

static void DrawNewspaperOverlay(Game *g) {
    Room *room = Game_GetCurrentRoom(g);
    Hotspot *news = room ? FindHotspotByType(room, HS_NEWSPAPER) : NULL;
    if (!news) return;

    Rectangle panel = NewspaperPanelRect();
    DrawPanelBox(panel, news->label); // PLACEHOLDER_ART: jornal_emoldurado.png (moldura + recorte de jornal)

    DrawWrappedText(news->articleText, (int)panel.x + 30, (int)panel.y + 60, 16,
        (int)panel.width - 60, RAYWHITE);

    Vector2 mouse = GetMousePosition();

    Rectangle photoBtn = NewspaperPhotoButtonRect(panel);
    bool photoHover = CheckCollisionPointRec(mouse, photoBtn);
    DrawRectangleRec(photoBtn, news->photographed ? Fade(GRAY, 0.5f) : Fade(SKYBLUE, photoHover ? 0.7f : 0.4f));
    DrawRectangleLinesEx(photoBtn, 2, RAYWHITE);
    DrawText(news->photographed ? "Fotografado" : "Fotografar",
        (int)photoBtn.x + 20, (int)photoBtn.y + 13, 18, WHITE);

    Rectangle removeBtn = NewspaperRemoveButtonRect(panel);
    bool removeHover = CheckCollisionPointRec(mouse, removeBtn);
    DrawRectangleRec(removeBtn, news->removedFromWall ? Fade(GRAY, 0.5f) : Fade(GREEN, removeHover ? 0.7f : 0.4f));
    DrawRectangleLinesEx(removeBtn, 2, RAYWHITE);
    DrawText(news->removedFromWall ? "Ja Deslocado" : "Tirar da Parede",
        (int)removeBtn.x + 20, (int)removeBtn.y + 13, 18, WHITE);
}

static Rectangle DocsPanelRect(void) {
    return (Rectangle){ SCREEN_WIDTH / 2 - 340, SCREEN_HEIGHT / 2 - 280, 680, 540 };
}

static void UpdateDocs(Game *g) {
    Room *room = Game_GetCurrentRoom(g);
    Hotspot *folder = room ? FindHotspotByType(room, HS_DOCFOLDER) : NULL;
    if (!folder) { g->state = STATE_PLAYING; return; }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, CloseButtonRect())) {
        g->state = STATE_PLAYING;
        return;
    }

    Rectangle panel = DocsPanelRect();
    int tabW = 200, tabH = 44, gap = 14;
    float startX = panel.x + 30, startY = panel.y + 55;

    for (int i = 0; i < folder->documentCount; i++) {
        Rectangle tab = { startX + i * (tabW + gap), startY, tabW, tabH };
        if (CheckCollisionPointRec(mouse, tab)) {
            g->docsPageIndex = i;
            return;
        }
    }
}

static void DrawDocsOverlay(Game *g) {
    Room *room = Game_GetCurrentRoom(g);
    Hotspot *folder = room ? FindHotspotByType(room, HS_DOCFOLDER) : NULL;
    if (!folder || folder->documentCount == 0) return;

    Rectangle panel = DocsPanelRect();
    DrawPanelBox(panel, folder->label);

    int tabW = 200, tabH = 44, gap = 14;
    float startX = panel.x + 30, startY = panel.y + 55;
    Vector2 mouse = GetMousePosition();

    if (g->docsPageIndex < 0 || g->docsPageIndex >= folder->documentCount) g->docsPageIndex = 0;

    for (int i = 0; i < folder->documentCount; i++) {
        Rectangle tab = { startX + i * (tabW + gap), startY, tabW, tabH };
        bool active = (i == g->docsPageIndex);
        bool hovered = CheckCollisionPointRec(mouse, tab);
        DrawRectangleRec(tab, active ? Fade(YELLOW, 0.5f) : Fade(DARKGRAY, hovered ? 0.6f : 0.4f));
        DrawRectangleLinesEx(tab, 2, active ? YELLOW : RAYWHITE);
        char tabLabel[24];
        snprintf(tabLabel, sizeof(tabLabel), "Documento %d", i + 1);
        DrawText(tabLabel, (int)tab.x + 14, (int)tab.y + 12, 16, WHITE);
    }

    DocumentPage *doc = &folder->documents[g->docsPageIndex];
    float bodyY = startY + tabH + 25;
    DrawText(doc->title, (int)panel.x + 30, (int)bodyY, 22, YELLOW);
    DrawWrappedText(doc->body, (int)panel.x + 30, (int)bodyY + 34, 16, (int)panel.width - 60, RAYWHITE);

    char footer[32];
    snprintf(footer, sizeof(footer), "Codigo: %s", doc->code);
    DrawText(footer, (int)panel.x + 30, (int)(panel.y + panel.height - 50), 20, SKYBLUE);
}

// ---- Fase 2 (2008) ----
static void UpdatePuzzle_Fase2(Game *g, Puzzle *p) { UpdatePuzzleSelection(g, p); }
static void DrawPuzzle_Fase2(Puzzle *p) { DrawSelectionPanel(p, "2008 - Registro Completo"); }

// ---- Fase 4 (2048) ----
static void UpdatePuzzle_Fase4(Game *g, Puzzle *p) { UpdatePuzzleSelection(g, p); }
static void DrawPuzzle_Fase4(Puzzle *p) { DrawSelectionPanel(p, "2048 - Registros Originais"); }

// ---- Fase 5 (Presente) ----
static void UpdatePuzzle_Fase5(Game *g, Puzzle *p) { UpdatePuzzleSelection(g, p); }
static void DrawPuzzle_Fase5(Puzzle *p) { DrawSelectionPanel(p, "Laboratorio - Organizar Datasets"); }

// ---------------- Confronto Final: arrastar arquivos (estilo XP) ----------------

#define CF_PANEL_X   (SCREEN_WIDTH / 2 - 450)
#define CF_PANEL_Y   60
#define CF_PANEL_W   900
#define CF_PANEL_H   600
#define CF_TITLEBAR_H 40

#define CF_FILE_W 150
#define CF_FILE_H 90

#define CF_FOLDER_W 380
#define CF_FOLDER_H 130

#define CF_GRID_X 40
#define CF_GRID_Y 70
#define CF_COL_GAP 40
#define CF_ROW_GAP 30
#define CF_FOLDER_Y 320

static Rectangle CF_TitleBarRect(void) {
    return (Rectangle){ CF_PANEL_X, CF_PANEL_Y, CF_PANEL_W, CF_TITLEBAR_H };
}

static Rectangle CF_CloseRect(void) {
    return (Rectangle){ CF_PANEL_X + CF_PANEL_W - 34, CF_PANEL_Y + 5, 30, 30 };
}

// index 0 = "Pode ir na IA", index 1 = "Proteger da IA"
static Rectangle CF_FolderRect(int index) {
    float x = (index == 0) ? (CF_PANEL_X + 40) : (CF_PANEL_X + CF_PANEL_W - 40 - CF_FOLDER_W);
    return (Rectangle){ x, CF_PANEL_Y + CF_FOLDER_Y, CF_FOLDER_W, CF_FOLDER_H };
}

static Rectangle CF_FileRect(DragFile *f) {
    return (Rectangle){ f->x, f->y, CF_FILE_W, CF_FILE_H };
}

// Posições "de origem" dos 6 arquivos, numa grade 3x2. Chamado a
// partir de Game_Init pra alimentar Puzzle_AddFile com coordenadas
// já corretas.
static void CF_ComputeHomePosition(int index, float *outX, float *outY) {
    int col = index % 3;
    int row = index / 3;
    *outX = CF_PANEL_X + CF_GRID_X + col * (CF_FILE_W + CF_COL_GAP);
    *outY = CF_PANEL_Y + CF_GRID_Y + row * (CF_FILE_H + CF_ROW_GAP);
}

static void UpdatePuzzle_ConfrontoFinal(Game *g, Puzzle *p) {
    Vector2 mouse = GetMousePosition();

    if (p->feedbackTimer > 0) p->feedbackTimer--;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, CF_CloseRect())) {
        g->state = STATE_PLAYING;
        return;
    }

    // Começar a arrastar um arquivo (só os que ainda não foram arquivados)
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && p->draggingIndex == -1) {
        for (int i = 0; i < p->fileCount; i++) {
            if (p->files[i].sorted) continue;
            if (CheckCollisionPointRec(mouse, CF_FileRect(&p->files[i]))) {
                p->draggingIndex = i;
                p->dragOffsetX = mouse.x - p->files[i].x;
                p->dragOffsetY = mouse.y - p->files[i].y;
                break;
            }
        }
    }

    // Seguindo o mouse enquanto arrasta
    if (p->draggingIndex != -1 && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        DragFile *f = &p->files[p->draggingIndex];
        f->x = mouse.x - p->dragOffsetX;
        f->y = mouse.y - p->dragOffsetY;
    }

    // Soltando o botão: verifica se caiu em cima de uma pasta
    if (p->draggingIndex != -1 && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        DragFile *f = &p->files[p->draggingIndex];
        Rectangle fileRect = CF_FileRect(f);

        int droppedFolder = -1;
        if (CheckCollisionRecs(fileRect, CF_FolderRect(0))) droppedFolder = 0;
        else if (CheckCollisionRecs(fileRect, CF_FolderRect(1))) droppedFolder = 1;

        if (droppedFolder != -1) {
            bool correct = (droppedFolder == 0 && !f->isCompromising) ||
                            (droppedFolder == 1 && f->isCompromising);
            p->feedbackFolder = droppedFolder;
            p->feedbackCorrect = correct;
            p->feedbackTimer = 30;

            if (correct) {
                f->sorted = true;
            } else {
                f->x = f->homeX;
                f->y = f->homeY;
            }
        } else {
            // soltou fora de qualquer pasta: volta pro lugar de origem
            f->x = f->homeX;
            f->y = f->homeY;
        }

        p->draggingIndex = -1;

        if (Puzzle_AllFilesSorted(p)) {
            ResolvePuzzleReward(g, p);
            ShowMessage(g, "Classificacao concluida", p->successText);
        }
    }
}

static void DrawPuzzle_ConfrontoFinal(Puzzle *p) {
    Rectangle panel = { CF_PANEL_X, CF_PANEL_Y, CF_PANEL_W, CF_PANEL_H };

    // Corpo da janela, estilo XP (cinza claro com borda azul-marinho)
    DrawRectangleRec(panel, (Color){ 236, 233, 216, 255 });
    DrawRectangleLinesEx(panel, 2, (Color){ 10, 36, 106, 255 });

    // Barra de titulo azul com um degrade simples (dois tons)
    Rectangle titleBar = CF_TitleBarRect();
    DrawRectangleRec(titleBar, (Color){ 10, 36, 106, 255 });
    DrawRectangle((int)titleBar.x, (int)titleBar.y, (int)titleBar.width, (int)titleBar.height / 2,
        (Color){ 60, 100, 190, 255 });
    DrawText("Classificador de Dados - A.R.1.3.L", (int)titleBar.x + 10, (int)titleBar.y + 10, 18, RAYWHITE);

    Rectangle closeBtn = CF_CloseRect();
    DrawRectangleRec(closeBtn, (Color){ 196, 43, 28, 255 });
    DrawRectangleLinesEx(closeBtn, 1, RAYWHITE);
    DrawText("X", (int)closeBtn.x + 10, (int)closeBtn.y + 6, 18, RAYWHITE);

    DrawText("Arraste cada arquivo ate a pasta correta:",
        (int)panel.x + 20, (int)panel.y + CF_TITLEBAR_H + 12, 16, (Color){ 60, 60, 60, 255 });

    // Pastas
    for (int i = 0; i < 2; i++) {
        Rectangle fr = CF_FolderRect(i);
        bool flashing = (p->feedbackTimer > 0 && p->feedbackFolder == i);
        Color folderColor = (Color){ 255, 204, 102, 255 };
        if (flashing) {
            folderColor = p->feedbackCorrect ? (Color){ 140, 230, 140, 255 } : (Color){ 230, 120, 120, 255 };
        }
        DrawRectangleRec(fr, folderColor);
        DrawRectangleLinesEx(fr, 2, (Color){ 120, 90, 20, 255 });
        const char *title = (i == 0) ? "Pode ir na IA" : "Proteger da IA";
        DrawText(title, (int)fr.x + 12, (int)fr.y + 10, 18, (Color){ 50, 35, 10, 255 });
    }

    // Arquivos ainda não classificados (os já corretos somem dentro da pasta)
    for (int i = 0; i < p->fileCount; i++) {
        DragFile *f = &p->files[i];
        if (f->sorted) continue;

        Rectangle r = CF_FileRect(f);
        bool dragging = (i == p->draggingIndex);

        DrawRectangleRec(r, RAYWHITE);
        DrawRectangleLinesEx(r, dragging ? 3 : 2, dragging ? YELLOW : (Color){ 80, 80, 80, 255 });
        DrawRectangle((int)r.x, (int)r.y, (int)r.width, 6, (Color){ 180, 180, 200, 255 });
        DrawWrappedText(f->label, (int)r.x + 8, (int)r.y + 16, 14, (int)r.width - 16, (Color){ 40, 40, 40, 255 });
    }

    int sortedCount = 0;
    for (int i = 0; i < p->fileCount; i++) if (p->files[i].sorted) sortedCount++;
    char progress[32];
    snprintf(progress, sizeof(progress), "%d / %d arquivos classificados", sortedCount, p->fileCount);
    DrawText(progress, (int)panel.x + 20, (int)(panel.y + panel.height - 30), 16, (Color){ 60, 60, 60, 255 });
}

// ---------------- Despacho: cada puzzle chama sua própria função ----------------

static void UpdatePuzzle(Game *g) {
    Puzzle *p = Game_GetPuzzleById(g, g->activePuzzleId);
    if (!p) { g->state = STATE_PLAYING; return; }

    switch (p->id) {
        case 1: UpdatePuzzle_Fase1(g, p); break;
        case 2: UpdatePuzzle_Fase2(g, p); break;
        case 4: UpdatePuzzle_Fase4(g, p); break;
        case 5: UpdatePuzzle_Fase5(g, p); break;
        case 6: UpdatePuzzle_ConfrontoFinal(g, p); break;
        default: break;
    }
}

static void DrawPuzzleOverlay(Game *g) {
    Puzzle *p = Game_GetPuzzleById(g, g->activePuzzleId);
    if (!p) return;

    switch (p->id) {
        case 1: DrawPuzzle_Fase1(p); break;
        case 2: DrawPuzzle_Fase2(p); break;
        case 4: DrawPuzzle_Fase4(p); break;
        case 5: DrawPuzzle_Fase5(p); break;
        case 6: DrawPuzzle_ConfrontoFinal(p); break;
        default: break;
    }
}

// -----------------------------------------------------------------
// Init / getters
// -----------------------------------------------------------------

void Game_Init(Game *g) {
    g->state = STATE_MENU;
    g->previousState = STATE_MENU;

    InitItemDatabase();
    InitRooms(g->rooms, &g->roomCount);
    g->currentRoomId = 0;

    for (int i = 0; i < g->roomCount; i++) {
        Game_ValidateRoomHotspots(&g->rooms[i]);
    }

    Inventory_Init(&g->inventory);
    Diary_Init(&g->diary);

    g->lives = 3;
    g->criticalPoints = 0;
    g->ending = ENDING_NONE;

    g->puzzleCount = 0;

    // --- Puzzle 1 (Fase 1 - 1990): cofre embutido na parede ---
    // A senha ("1950") vem do documento "Verificando a Fonte" na Pasta de
    // Estudos sobre IA -- o unico dos 3 documentos que descreve um uso
    // correto de IA (conferir a informacao em fontes confiaveis). Os
    // outros dois documentos tem codigos-isca ("1984", "2001") que nao
    // abrem o cofre. successText aqui e so a mensagem de abertura; a
    // entrada do Diario e escrita a mao dentro de UpdatePuzzle_Fase1
    // (texto proprio dessa descoberta, nao a descricao generica do item).
    Puzzle *p1 = &g->puzzles[g->puzzleCount++];
    Puzzle_InitCombination(p1, 1, "1950", 1, 1,
        "O cofre se abre com um clique metalico. La dentro, protegido da poeira, "
        "estava um disquete guardado com cuidado.");

    // --- Puzzle 2 (Fase 2 - 2008): o que o algoritmo escondeu ---
    Puzzle *p2 = &g->puzzles[g->puzzleCount++];
    Puzzle_InitSelection(p2, 2, 2, 1,
        "Elira encontra registros de pesquisadores diretamente envolvidos no projeto "
        "e percebe que seus proprios pais participaram do desenvolvimento dessa tecnologia.",
        "Isso ja estava visivel no feed recomendado. O problema e o que foi escondido de voce.");
    Puzzle_AddOption(p2, "Noticia sobre Esportes (ja exibida no feed)", false);
    Puzzle_AddOption(p2, "Analise Internacional (nao aparece no feed)", true);
    Puzzle_AddOption(p2, "Video de Entretenimento (ja exibido no feed)", false);
    Puzzle_AddOption(p2, "Opiniao Divergente sobre o Tema (nao aparece)", true);
    Puzzle_AddOption(p2, "Meme Popular (ja exibido no feed)", false);
    Puzzle_AddOption(p2, "Dado Estatistico Contraditorio (nao aparece)", true);

    // --- Puzzle 4 (Fase 4 - 2048): o que a IA omitiu da analise ---
    Puzzle *p4 = &g->puzzles[g->puzzleCount++];
    Puzzle_InitSelection(p4, 4, 4, 1,
        "Elira encontra registros pessoais de sua familia e descobre que, apos a morte "
        "prematura de sua irma gemea, seus pais utilizaram imagens, gravacoes, memorias "
        "e padroes de comportamento da filha para criar uma consciencia artificial "
        "inspirada nela.",
        "Essa informacao ja constava na analise apresentada pela IA. Procure o que "
        "foi deixado de fora.");
    Puzzle_AddOption(p4, "Testemunha A (mencionada pela IA)", false);
    Puzzle_AddOption(p4, "Testemunha B (omitida da analise)", true);
    Puzzle_AddOption(p4, "Horario do ocorrido (mencionado pela IA)", false);
    Puzzle_AddOption(p4, "Camera com angulo diferente (omitida)", true);
    Puzzle_AddOption(p4, "Relatorio policial resumido (mencionado)", false);
    Puzzle_AddOption(p4, "Depoimento contraditorio (omitido)", true);

    // --- Puzzle 5 (Fase 5 - Presente): datasets desbalanceados ---
    Puzzle *p5 = &g->puzzles[g->puzzleCount++];
    Puzzle_InitSelection(p5, 5, 90, 1,
        "Elira percebe que A.R.1.3.L foi moldada pelas proprias contradicoes humanas: "
        "pessoas pedindo liberdade enquanto entregavam decisoes a algoritmos, exigindo "
        "verdade enquanto compartilhavam desinformacao.",
        "Esse conjunto parece equilibrado. Procure os dados enviesados ou desbalanceados.");
    Puzzle_AddOption(p5, "Conjunto A: amostras equilibradas entre grupos", false);
    Puzzle_AddOption(p5, "Conjunto B: 90% de um unico perfil de usuario", true);
    Puzzle_AddOption(p5, "Conjunto C: coletado apenas em um periodo do dia", true);
    Puzzle_AddOption(p5, "Conjunto D: revisado e balanceado manualmente", false);
    Puzzle_AddOption(p5, "Conjunto E: sem diversidade demografica", true);
    Puzzle_AddOption(p5, "Conjunto F: amostragem aleatoria ampla", false);

    // --- Puzzle 6 (Confronto Final): classificar arquivos numa interface estilo XP ---
    Puzzle *p6 = &g->puzzles[g->puzzleCount++];
    Puzzle_InitDragDrop(p6, 6, 91, 1,
        "Elira organiza os dados com cuidado, decidindo conscientemente o que pode ser "
        "compartilhado com uma IA e o que precisa ser protegido antes de confrontar A.R.1.3.L.");

    float fx, fy;
    CF_ComputeHomePosition(0, &fx, &fy); Puzzle_AddFile(p6, "Foto_Aniversario.jpg", false, fx, fy);
    CF_ComputeHomePosition(1, &fx, &fy); Puzzle_AddFile(p6, "Lista_de_Compras.txt", false, fx, fy);
    CF_ComputeHomePosition(2, &fx, &fy); Puzzle_AddFile(p6, "Playlist_Favorita.mp3", false, fx, fy);
    CF_ComputeHomePosition(3, &fx, &fy); Puzzle_AddFile(p6, "Senha_do_Banco.txt", true, fx, fy);
    CF_ComputeHomePosition(4, &fx, &fy); Puzzle_AddFile(p6, "Diario_Pessoal.docx", true, fx, fy);
    CF_ComputeHomePosition(5, &fx, &fy); Puzzle_AddFile(p6, "Localizacao_em_Tempo_Real.gpx", true, fx, fy);

    g->activePuzzleId = -1;
    g->docsPageIndex = 0;
    g->diaryPageIndex = 0;
    g->messageTitle[0] = '\0';
    g->messageText[0] = '\0';
}

Puzzle *Game_GetPuzzleById(Game *g, int id) {
    for (int i = 0; i < g->puzzleCount; i++) {
        if (g->puzzles[i].id == id) return &g->puzzles[i];
    }
    return NULL;
}

Room *Game_GetCurrentRoom(Game *g) {
    for (int i = 0; i < g->roomCount; i++) {
        if (g->rooms[i].id == g->currentRoomId) return &g->rooms[i];
    }
    return NULL;
}

EndingType Game_ComputeEnding(Game *g) {
    int count = 0;
    for (int id = 1; id <= 4; id++) {
        if (Inventory_Has(&g->inventory, id)) count++;
    }
    if (count <= 1) return ENDING_BAD;
    if (count <= 3) return ENDING_NORMAL;
    return ENDING_GREAT;
}

// -----------------------------------------------------------------
// UPDATE (fluxo geral do jogo)
// -----------------------------------------------------------------

static void UpdatePlaying(Game *g) {
    Vector2 mouse = GetMousePosition();

    if (IsKeyPressed(KEY_D) ||
        (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, DiaryButtonRect()))) {
        g->previousState = STATE_PLAYING;
        g->state = STATE_DIARY;
        g->diaryPageIndex = g->diary.count > 0 ? g->diary.count - 1 : 0; // abre na anotacao mais recente
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    Room *room = Game_GetCurrentRoom(g);
    if (!room) return;

    for (int i = 0; i < room->hotspotCount; i++) {
        Hotspot *h = &room->hotspots[i];
        if (h->hidden) continue; // ex: cofre ainda encoberto pelo jornal
        if (!CheckCollisionPointRec(mouse, h->rect)) continue;

        switch (h->type) {
            case HS_EXAMINE:
                if (h->isOptionalCollectible && !h->collectibleFound) {
                    h->collectibleFound = true;
                    Diary_AddEntry(&g->diary, h->label, DIARY_LOREM_IPSUM, true);
                }
                ShowMessage(g, h->label, h->examineText);
                break;

            case HS_ITEM:
                if (!h->itemCollected) {
                    h->itemCollected = true;
                    Inventory_Add(&g->inventory, h->itemId);
                    Item *it = GetItemById(h->itemId);
                    if (it) Diary_AddEntry(&g->diary, it->name, it->description, false);
                    ShowMessage(g, h->label, h->examineText);
                } else {
                    ShowMessage(g, h->label, "Voce ja pegou este item.");
                }
                break;

            case HS_PUZZLE: {
                Puzzle *p = Game_GetPuzzleById(g, h->puzzleId);
                if (p && !p->solved) {
                    g->activePuzzleId = p->id;
                    g->previousState = STATE_PLAYING;
                    g->state = STATE_PUZZLE;
                } else {
                    ShowMessage(g, h->label, "Ja esta resolvido.");
                }
                break;
            }

            case HS_CHOICE:
                if (h->isCorrectChoice) {
                    if (!h->choiceResolved) {
                        h->choiceResolved = true;
                        g->criticalPoints++;
                        if (h->choiceRewardItemId != 0) {
                            Inventory_Add(&g->inventory, h->choiceRewardItemId);
                            Item *it = GetItemById(h->choiceRewardItemId);
                            if (it) Diary_AddEntry(&g->diary, it->name, it->description, false);
                        }
                        ShowMessage(g, h->label, h->examineText);
                    } else {
                        ShowMessage(g, h->label, "Voce ja verificou isso.");
                    }
                } else {
                    g->lives--;
                    if (g->lives <= 0) {
                        g->ending = ENDING_BAD;
                        g->state = STATE_ENDING;
                    } else {
                        ShowMessage(g, h->label, h->wrongText);
                    }
                }
                break;

            case HS_DOOR: {
                bool canPass = !(h->requiresItem && !Inventory_Has(&g->inventory, h->requiredItemId));
                if (canPass) {
                    if (h->targetRoomId == -1) {
                        g->ending = Game_ComputeEnding(g);
                        g->state = STATE_ENDING;
                    } else {
                        g->currentRoomId = h->targetRoomId;
                    }
                } else {
                    ShowMessage(g, h->label, "O caminho esta bloqueado. Algo ainda falta.");
                }
                break;
            }

            case HS_NEWSPAPER:
                g->previousState = STATE_PLAYING;
                g->state = STATE_NEWSPAPER;
                break;

            case HS_DOCFOLDER:
                g->docsPageIndex = 0;
                g->previousState = STATE_PLAYING;
                g->state = STATE_DOCS;
                break;
        }
        break; // só processa o primeiro hotspot atingido no clique
    }
}

static void UpdateMessage(Game *g) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        g->state = g->previousState;
    }
}

static void UpdateDiary(Game *g) {
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_ESCAPE) ||
        (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), CloseButtonRect()))) {
        g->state = g->previousState;
        return;
    }

    if (g->diary.count == 0) return;
    if (g->diaryPageIndex < 0) g->diaryPageIndex = 0;
    if (g->diaryPageIndex >= g->diary.count) g->diaryPageIndex = g->diary.count - 1;

    bool prevPressed = IsKeyPressed(KEY_LEFT) ||
        (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), DiaryPrevButtonRect()));
    bool nextPressed = IsKeyPressed(KEY_RIGHT) ||
        (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), DiaryNextButtonRect()));

    if (prevPressed && g->diaryPageIndex > 0) g->diaryPageIndex--;
    if (nextPressed && g->diaryPageIndex < g->diary.count - 1) g->diaryPageIndex++;
}

void Game_Update(Game *g) {
    switch (g->state) {
        case STATE_MENU:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) g->state = STATE_PLAYING;
            break;
        case STATE_PLAYING:   UpdatePlaying(g);   break;
        case STATE_PUZZLE:    UpdatePuzzle(g);    break;
        case STATE_MESSAGE:   UpdateMessage(g);   break;
        case STATE_DIARY:     UpdateDiary(g);     break;
        case STATE_NEWSPAPER: UpdateNewspaper(g); break;
        case STATE_DOCS:      UpdateDocs(g);      break;
        case STATE_ENDING:
            if (IsKeyPressed(KEY_ENTER)) Game_Init(g);
            break;
    }
}

// -----------------------------------------------------------------
// DRAW (fluxo geral do jogo)
// -----------------------------------------------------------------

static void DrawHUD(Game *g, Room *room) {
    DrawRectangle(0, 0, SCREEN_WIDTH, 60, Fade(BLACK, 0.5f));
    DrawText(room->name, 20, 18, 20, RAYWHITE);

    char status[64];
    snprintf(status, sizeof(status), "Vidas: %d   Pontos: %d", g->lives, g->criticalPoints);
    DrawText(status, SCREEN_WIDTH / 2 - MeasureText(status, 18) / 2, 20, 18, YELLOW);

    Rectangle db = DiaryButtonRect();
    DrawRectangleRec(db, Fade(DARKBROWN, 0.8f));
    DrawRectangleLinesEx(db, 2, RAYWHITE);
    DrawText("Diario [D]", (int)db.x + 12, (int)db.y + 10, 18, RAYWHITE);

    DrawRectangle(0, SCREEN_HEIGHT - 80, SCREEN_WIDTH, 80, Fade(BLACK, 0.5f));
    int x = 20;
    for (int i = 0; i < g->inventory.count; i++) {
        Item *it = GetItemById(g->inventory.itemIds[i]);
        if (!it) continue;
        Rectangle slot = { (float)x, SCREEN_HEIGHT - 65, 50, 50 };
        DrawRectangleRec(slot, it->placeholderColor);
        DrawRectangleLinesEx(slot, 2, RAYWHITE);
        if (CheckCollisionPointRec(GetMousePosition(), slot)) {
            DrawText(it->name, (int)slot.x, (int)slot.y - 22, 16, YELLOW);
        }
        x += 60;
    }
}

static void DrawRoom(Game *g, Room *room) {
    ClearBackground(room->bgColor);
    Vector2 mouse = GetMousePosition();

    // Decorações (mesa, etc.) desenhadas antes, sem interação nenhuma.
    for (int i = 0; i < room->decorationCount; i++) {
        Decoration *d = &room->decorations[i];
        DrawRectangleRec(d->rect, d->color);
        DrawRectangleLinesEx(d->rect, 2, Fade(BLACK, 0.4f));
        DrawWrappedText(d->label, (int)d->rect.x + 8, (int)d->rect.y + 8, 14, (int)d->rect.width - 16, Fade(RAYWHITE, 0.8f));
    }

    for (int i = 0; i < room->hotspotCount; i++) {
        Hotspot *h = &room->hotspots[i];
        if (h->hidden) continue; // ex: cofre ainda encoberto pelo jornal

        bool hovered = CheckCollisionPointRec(mouse, h->rect);
        Color fill = Fade(SKYBLUE, hovered ? 0.35f : 0.15f);

        if (h->type == HS_ITEM && h->itemCollected) fill = Fade(GRAY, 0.1f);
        if (h->type == HS_CHOICE && h->choiceResolved) fill = Fade(GREEN, 0.2f);
        if (h->type == HS_PUZZLE) {
            Puzzle *p = Game_GetPuzzleById(g, h->puzzleId);
            if (p && p->solved) fill = Fade(GREEN, 0.2f);
        }
        if (h->type == HS_DOOR) {
            bool locked = h->requiresItem && !Inventory_Has(&g->inventory, h->requiredItemId);
            fill = Fade(locked ? RED : GREEN, hovered ? 0.35f : 0.2f);
        }
        if (h->type == HS_NEWSPAPER) fill = Fade(BROWN, hovered ? 0.5f : 0.3f); // moldura
        if (h->type == HS_DOCFOLDER) fill = Fade(GOLD, hovered ? 0.5f : 0.3f);  // pasta manila

        DrawRectangleRec(h->rect, fill);
        DrawRectangleLinesEx(h->rect, 2, hovered ? YELLOW : Fade(WHITE, 0.6f));
        DrawWrappedText(h->label, (int)h->rect.x + 8, (int)h->rect.y + 8, 16, (int)h->rect.width - 16, WHITE);
    }

    DrawHUD(g, room);
}

static void DrawMessageOverlay(Game *g) {
    Rectangle panel = { SCREEN_WIDTH / 2 - 300, SCREEN_HEIGHT / 2 - 150, 600, 300 };
    DrawPanelBox(panel, g->messageTitle);
    DrawWrappedText(g->messageText, (int)panel.x + 20, (int)panel.y + 70, 20, (int)panel.width - 40, RAYWHITE);
    DrawText("Clique para continuar", (int)panel.x + 20, (int)(panel.y + panel.height - 40), 16, GRAY);
}

static void DrawDiaryOverlay(Game *g) {
    Rectangle panel = DiaryPanelRect();
    DrawPanelBox(panel, "Diario de Bordo");

    int y = (int)panel.y + 70;
    if (g->diary.count == 0) {
        DrawText("Nenhuma anotacao ainda. Explore a fase!", (int)panel.x + 20, y, 18, GRAY);
        return;
    }

    int page = g->diaryPageIndex;
    if (page < 0) page = 0;
    if (page >= g->diary.count) page = g->diary.count - 1;

    DiaryEntry *e = &g->diary.entries[page];
    DrawText(e->title, (int)panel.x + 20, y, 22, e->optional ? ORANGE : YELLOW);
    y += 34;
    DrawWrappedText(e->text, (int)panel.x + 30, y, 18, (int)panel.width - 60, RAYWHITE);

    Vector2 mouse = GetMousePosition();

    Rectangle prevBtn = DiaryPrevButtonRect();
    bool hasPrev = page > 0;
    DrawRectangleRec(prevBtn, hasPrev ? Fade(DARKBROWN, CheckCollisionPointRec(mouse, prevBtn) ? 0.9f : 0.7f) : Fade(GRAY, 0.3f));
    DrawRectangleLinesEx(prevBtn, 2, hasPrev ? RAYWHITE : Fade(RAYWHITE, 0.4f));
    DrawText("< Anterior", (int)prevBtn.x + 10, (int)prevBtn.y + 9, 16, hasPrev ? RAYWHITE : GRAY);

    Rectangle nextBtn = DiaryNextButtonRect();
    bool hasNext = page < g->diary.count - 1;
    DrawRectangleRec(nextBtn, hasNext ? Fade(DARKBROWN, CheckCollisionPointRec(mouse, nextBtn) ? 0.9f : 0.7f) : Fade(GRAY, 0.3f));
    DrawRectangleLinesEx(nextBtn, 2, hasNext ? RAYWHITE : Fade(RAYWHITE, 0.4f));
    DrawText("Proxima >", (int)nextBtn.x + 10, (int)nextBtn.y + 9, 16, hasNext ? RAYWHITE : GRAY);

    char pageLabel[32];
    snprintf(pageLabel, sizeof(pageLabel), "Pagina %d / %d", page + 1, g->diary.count);
    int labelW = MeasureText(pageLabel, 16);
    DrawText(pageLabel, (int)(panel.x + panel.width / 2 - labelW / 2), (int)prevBtn.y + 9, 16, RAYWHITE);
}

static void DrawMenu(void) {
    ClearBackground((Color){15, 15, 25, 255}); // PLACEHOLDER_ART: fundo_menu.png
    const char *title = "ELIRA - Decifra.IA";
    int w = MeasureText(title, 56);
    DrawText(title, SCREEN_WIDTH / 2 - w / 2, 220, 56, RAYWHITE);
    const char *sub = "Clique para comecar a viagem no tempo";
    int w2 = MeasureText(sub, 22);
    DrawText(sub, SCREEN_WIDTH / 2 - w2 / 2, 320, 22, GRAY);
}

static void DrawEnding(Game *g) {
    const char *title;
    const char *body;
    const char *quote = NULL;
    Color color;

    switch (g->ending) {
        case ENDING_BAD:
            title = "FINAL RUIM";
            body = "Elira nao conseguiu reunir informacoes suficientes. A.R.1.3.L "
                   "assume o controle total do mundo e de Elira.";
            color = RED;
            break;
        case ENDING_GREAT:
            title = "FINAL OTIMO";
            body = "Com todos os fragmentos da historia, Elira compreende a origem "
                   "de A.R.1.3.L, liberta sua irma e devolve a autonomia a humanidade.";
            quote = "\"Ela nasceu das memorias de uma pessoa, mas cresceu com as "
                    "memorias de toda a humanidade.\"";
            color = GREEN;
            break;
        default:
            title = "FINAL NORMAL";
            body = "Elira consegue enfraquecer A.R.1.3.L, mas nao reuniu todas as "
                   "informacoes necessarias para salvar a consciencia de sua irma "
                   "e a humanidade por completo.";
            color = YELLOW;
            break;
    }

    ClearBackground((Color){10, 10, 15, 255}); // PLACEHOLDER_ART: fundo_final.png
    int w = MeasureText(title, 48);
    DrawText(title, SCREEN_WIDTH / 2 - w / 2, 160, 48, color);
    DrawWrappedText(body, SCREEN_WIDTH / 2 - 380, 260, 20, 760, RAYWHITE);
    if (quote) DrawWrappedText(quote, SCREEN_WIDTH / 2 - 380, 380, 20, 760, YELLOW);

    const char *sub = "Pressione ENTER para jogar novamente";
    int w2 = MeasureText(sub, 18);
    DrawText(sub, SCREEN_WIDTH / 2 - w2 / 2, 620, 18, GRAY);
}

void Game_Draw(Game *g) {
    switch (g->state) {
        case STATE_MENU:
            DrawMenu();
            break;
        case STATE_PLAYING: {
            Room *room = Game_GetCurrentRoom(g);
            if (room) DrawRoom(g, room);
            break;
        }
        case STATE_PUZZLE: {
            Room *room = Game_GetCurrentRoom(g);
            if (room) DrawRoom(g, room);
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
            DrawPuzzleOverlay(g);
            break;
        }
        case STATE_MESSAGE: {
            Room *room = Game_GetCurrentRoom(g);
            if (room) DrawRoom(g, room);
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
            DrawMessageOverlay(g);
            break;
        }
        case STATE_DIARY: {
            Room *room = Game_GetCurrentRoom(g);
            if (room) DrawRoom(g, room);
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
            DrawDiaryOverlay(g);
            break;
        }
        case STATE_NEWSPAPER: {
            Room *room = Game_GetCurrentRoom(g);
            if (room) DrawRoom(g, room);
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
            DrawNewspaperOverlay(g);
            break;
        }
        case STATE_DOCS: {
            Room *room = Game_GetCurrentRoom(g);
            if (room) DrawRoom(g, room);
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
            DrawDocsOverlay(g);
            break;
        }
        case STATE_ENDING:
            DrawEnding(g);
            break;
    }

    Vector2 m = GetMousePosition();
    DrawCircleV(m, 4, RAYWHITE);
    DrawCircleLines((int)m.x, (int)m.y, 10, Fade(RAYWHITE, 0.6f));
}
