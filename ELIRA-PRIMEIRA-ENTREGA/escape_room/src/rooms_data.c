#include "room.h"
#include <string.h>

// -----------------------------------------------------------------
// Cada "Room" aqui é uma Fase/Era do roteiro do ELIRA. Os helpers
// abaixo evitam boilerplate ao popular uma fase. Todos retornam o
// índice (== id) do hotspot criado, útil quando um hotspot precisa
// referenciar outro (ex: o jornal que revela o cofre escondido).
//
// REGRA DE OURO PRA NÃO REPETIR O BUG DO COFRE/QUADRO:
// os retângulos de hotspots de uma MESMA fase nunca podem se
// sobrepor. Se dois rects colidem, o clique sempre acerta o
// primeiro hotspot adicionado (o que estiver "embaixo" na lista),
// escondendo o outro. Layout em grade, com espaçamento generoso
// entre os elementos, resolve isso. Game_Init roda uma checagem
// automática (Game_ValidateRoomHotspots) que avisa no console via
// TraceLog se alguma fase tiver hotspots sobrepostos (hotspots que
// nascem com hidden=true são ignorados nessa checagem de propósito
// — ver hotspot.h).
// -----------------------------------------------------------------

static int AddExamine(Room *r, Rectangle rect, const char *label, const char *text) {
    Hotspot *h = &r->hotspots[r->hotspotCount];
    memset(h, 0, sizeof(Hotspot));
    h->rect = rect;
    h->type = HS_EXAMINE;
    h->id = r->hotspotCount;
    strncpy(h->label, label, sizeof(h->label) - 1);
    strncpy(h->examineText, text, sizeof(h->examineText) - 1);
    r->hotspotCount++;
    return h->id;
}

// Igual a AddExamine, mas marca o hotspot como colecionavel opcional: a 1a
// vez que o jogador o examina, uma entrada (laranja) e adicionada ao diario
// automaticamente, alem da mensagem normal de exame.
static int AddCollectibleExamine(Room *r, Rectangle rect, const char *label, const char *text) {
    int id = AddExamine(r, rect, label, text);
    r->hotspots[id].isOptionalCollectible = true;
    return id;
}

static int AddItemHotspot(Room *r, Rectangle rect, const char *label,
                           const char *text, int itemId) {
    Hotspot *h = &r->hotspots[r->hotspotCount];
    memset(h, 0, sizeof(Hotspot));
    h->rect = rect;
    h->type = HS_ITEM;
    h->id = r->hotspotCount;
    strncpy(h->label, label, sizeof(h->label) - 1);
    strncpy(h->examineText, text, sizeof(h->examineText) - 1);
    h->itemId = itemId;
    r->hotspotCount++;
    return h->id;
}

// hidden=true faz o hotspot nascer invisível e não-clicável — usado pro
// cofre da Fase 1, que só aparece depois que o jornal é retirado da parede.
static int AddPuzzleHotspot(Room *r, Rectangle rect, const char *label, int puzzleId, bool hidden) {
    Hotspot *h = &r->hotspots[r->hotspotCount];
    memset(h, 0, sizeof(Hotspot));
    h->rect = rect;
    h->type = HS_PUZZLE;
    h->id = r->hotspotCount;
    strncpy(h->label, label, sizeof(h->label) - 1);
    h->puzzleId = puzzleId;
    h->hidden = hidden;
    r->hotspotCount++;
    return h->id;
}

static int AddDoorHotspot(Room *r, Rectangle rect, const char *label, int targetRoomId,
                           bool requiresItem, int requiredItemId) {
    Hotspot *h = &r->hotspots[r->hotspotCount];
    memset(h, 0, sizeof(Hotspot));
    h->rect = rect;
    h->type = HS_DOOR;
    h->id = r->hotspotCount;
    strncpy(h->label, label, sizeof(h->label) - 1);
    h->targetRoomId = targetRoomId;
    h->requiresItem = requiresItem;
    h->requiredItemId = requiredItemId;
    r->hotspotCount++;
    return h->id;
}

static int AddChoiceHotspot(Room *r, Rectangle rect, const char *label, bool isCorrect,
                             const char *resultText, int rewardItemId) {
    Hotspot *h = &r->hotspots[r->hotspotCount];
    memset(h, 0, sizeof(Hotspot));
    h->rect = rect;
    h->type = HS_CHOICE;
    h->id = r->hotspotCount;
    strncpy(h->label, label, sizeof(h->label) - 1);
    h->isCorrectChoice = isCorrect;
    if (isCorrect) {
        strncpy(h->examineText, resultText, sizeof(h->examineText) - 1);
        h->choiceRewardItemId = rewardItemId;
    } else {
        strncpy(h->wrongText, resultText, sizeof(h->wrongText) - 1);
    }
    r->hotspotCount++;
    return h->id;
}

// Jornal emoldurado: legível a qualquer momento, com duas ações possíveis
// (fotografar / tirar da parede). wallRect é onde ele começa (pendurado);
// sideRect é pra onde ele se MOVE quando o jogador escolhe tirá-lo da
// parede (continua no mesmo Room, ainda clicável/fotografável dali, só
// não revela mais nada de novo). revealsHotspotId é o índice de outro
// hotspot da MESMA sala que deixa de ficar 'hidden' nesse momento (passe
// -1 se não houver nenhum).
static int AddNewspaperHotspot(Room *r, Rectangle wallRect, Rectangle sideRect,
                                const char *label, const char *articleText,
                                int revealsHotspotId) {
    Hotspot *h = &r->hotspots[r->hotspotCount];
    memset(h, 0, sizeof(Hotspot));
    h->rect = wallRect;
    h->sideRect = sideRect;
    h->type = HS_NEWSPAPER;
    h->id = r->hotspotCount;
    strncpy(h->label, label, sizeof(h->label) - 1);
    strncpy(h->articleText, articleText, sizeof(h->articleText) - 1);
    h->revealsHotspotId = revealsHotspotId;
    r->hotspotCount++;
    return h->id;
}

// Pasta com documentos legíveis (sem certo/errado por si só). Retorna o
// ponteiro do hotspot pra que DocFolder_AddDocument possa preencher os
// documentos logo em seguida.
static Hotspot *AddDocFolderHotspot(Room *r, Rectangle rect, const char *label) {
    Hotspot *h = &r->hotspots[r->hotspotCount];
    memset(h, 0, sizeof(Hotspot));
    h->rect = rect;
    h->type = HS_DOCFOLDER;
    h->id = r->hotspotCount;
    strncpy(h->label, label, sizeof(h->label) - 1);
    h->documentCount = 0;
    r->hotspotCount++;
    return h;
}

static void DocFolder_AddDocument(Hotspot *h, const char *title, const char *body, const char *code) {
    if (h->documentCount >= MAX_DOCS_PER_FOLDER) return;
    DocumentPage *d = &h->documents[h->documentCount++];
    strncpy(d->title, title, sizeof(d->title) - 1);
    strncpy(d->body, body, sizeof(d->body) - 1);
    strncpy(d->code, code, sizeof(d->code) - 1);
}

// Elemento decorativo (mesa, tapete, estante...) sem hitbox nem clique —
// desenhado antes dos hotspots, nunca entra na checagem de sobreposição.
static void AddDecoration(Room *r, Rectangle rect, Color color, const char *label) {
    if (r->decorationCount >= MAX_DECORATIONS_PER_ROOM) return;
    Decoration *d = &r->decorations[r->decorationCount++];
    d->rect = rect;
    d->color = color;
    strncpy(d->label, label, sizeof(d->label) - 1);
}

void InitRooms(Room rooms[], int *roomCount) {
    int rc = 0;

    // ============================================================
    // FASE 1 — 1990: A Origem dos Estudos
    // Conceito: fundamentos de IA e importância dos dados.
    //
    // Layout: um terminal (flavor), duas pastas decorativas (pistas
    // falsas), um jornal emoldurado (legível; esconde um cofre atrás
    // dele) e uma mesa com a pasta que contém os 3 documentos reais.
    // Só o código do documento que descreve o uso CORRETO de IA abre
    // o cofre (puzzle 1, combinação numérica). Abrir o cofre dá o
    // Disquete de Memória, que a porta exige pra liberar a Fase 2.
    // ============================================================
    Room *f1 = &rooms[rc];
    f1->id = rc;
    strcpy(f1->name, "Fase 1 - 1990: A Origem dos Estudos");
    f1->bgColor = (Color){ 70, 60, 45, 255 }; // PLACEHOLDER_ART: fundo_1990.png
    f1->hotspotCount = 0;
    f1->decorationCount = 0;

    AddExamine(f1, (Rectangle){60, 120, 320, 200}, "Terminal de Pesquisa",
        "Um terminal empoeirado exibe os primeiros estudos sobre sistemas capazes "
        "de aprender a partir de informacoes humanas.");

    // Pastas decorativas espalhadas pelo escritorio (pistas falsas).
    AddExamine(f1, (Rectangle){420, 120, 200, 110}, "Pasta de Recibos",
        "Recibos antigos de compras do escritorio. Nada de util aqui, aparentemente.");
    AddExamine(f1, (Rectangle){420, 250, 200, 100}, "Pasta de Correspondencias",
        "Cartas administrativas trocadas entre pesquisadores. Nao parecem ter "
        "relacao nenhuma com o cofre.");

    // Cofre embutido na parede: nasce escondido (hidden=true) atras do
    // jornal emoldurado, no MESMO retangulo do jornal. So fica visivel e
    // clicavel depois que o jogador escolhe "Tirar da Parede".
    int safeId = AddPuzzleHotspot(f1, (Rectangle){60, 380, 260, 180}, "Cofre Embutido na Parede", 1, true);

    // Jornal emoldurado: legivel a qualquer momento (imersao). Ao escolher
    // "Tirar da Parede", o quadro se MOVE pro lado (sideRect, ao lado do
    // cofre) em vez de sumir -- continua clicavel e fotografavel dali, so
    // que o cofre (safeId) e revelado no lugar onde o quadro estava.
    // Tambem pode ser fotografado antes disso, sem tirar da parede.
    AddNewspaperHotspot(f1, (Rectangle){60, 380, 260, 180}, (Rectangle){350, 380, 90, 180},
        "Jornal Emoldurado",
        "MAQUINAS QUE APRENDEM COM DADOS -- Pesquisadores universitarios anunciam "
        "avancos em sistemas capazes de identificar padroes em grandes volumes de "
        "informacao sem seguir regras fixas escritas manualmente por programadores. "
        "Batizados de algoritmos de aprendizado de maquina, esses sistemas dependem "
        "inteiramente da qualidade dos dados que recebem: informacoes tendenciosas "
        "ou incompletas tendem a gerar decisoes igualmente tendenciosas. Segundo os "
        "cientistas envolvidos, o maior desafio nao e ensinar a maquina a calcular, "
        "e sim garantir que ela aprenda a partir de exemplos confiaveis e diversos.",
        safeId);

    // Mesa (decorativa) com a pasta de verdade em cima dela.
    AddDecoration(f1, (Rectangle){480, 380, 340, 180}, (Color){90, 60, 30, 255},
        "PLACEHOLDER_ART: mesa_escritorio.png");
    Hotspot *studyFolder = AddDocFolderHotspot(f1, (Rectangle){540, 420, 200, 90},
        "Pasta de Estudos sobre IA"); // PLACEHOLDER_ART: pasta_documentos.png

    // Dois usos errados de IA (documentos-isca, com codigos que NAO abrem
    // o cofre) e um uso correto (o unico cujo codigo e a senha real).
    DocFolder_AddDocument(studyFolder, "Alimentando o Futuro",
        "Para obter respostas mais precisas, alguns pesquisadores defendem "
        "compartilhar dados pessoais completos com os sistemas de IA: nome, "
        "endereco, documentos e ate senhas. Segundo eles, quanto mais informacao "
        "privada for entregue, melhor sera o resultado obtido.",
        "1984");
    DocFolder_AddDocument(studyFolder, "Decisoes Automaticas",
        "Alguns defendem confiar plenamente nas recomendacoes de sistemas de IA "
        "para decisoes financeiras e sociais importantes, argumentando que os "
        "algoritmos processam muito mais dados do que qualquer ser humano "
        "conseguiria analisar sozinho.",
        "2001");
    DocFolder_AddDocument(studyFolder, "Verificando a Fonte",
        "Especialistas alertam: mesmo respostas geradas com total confianca por "
        "um sistema de IA podem estar erradas. Antes de tomar qualquer decisao "
        "importante, e essencial conferir a informacao em fontes confiaveis, "
        "como especialistas da area ou documentos publicados por pessoas "
        "reconhecidas no assunto.",
        "1950");

    AddDoorHotspot(f1, (Rectangle){1000, 120, 220, 480}, "Portal Temporal (2008)", 1, true, 1);
    rc++;

    // ============================================================
    // FASE 2 — 2008: Aprendendo com o Comportamento Humano
    // Conceito: algoritmos de recomendação e bolhas de conteúdo.
    // Puzzle 2 (seleção): achar o conteúdo que o algoritmo escondeu.
    // ============================================================
    Room *f2 = &rooms[rc];
    f2->id = rc;
    strcpy(f2->name, "Fase 2 - 2008: Aprendendo com o Comportamento Humano");
    f2->bgColor = (Color){ 45, 55, 65, 255 }; // PLACEHOLDER_ART: fundo_2008.png
    f2->hotspotCount = 0;
    f2->decorationCount = 0;
    // Colecionavel opcional da Fase 2 (equivalente a fotografar o jornal na Fase 1).
    AddCollectibleExamine(f2, (Rectangle){60, 120, 320, 200}, "Feed Recomendado",
        "O sistema exibe apenas o conteudo que calcula ser do seu interesse.");
    AddPuzzleHotspot(f2, (Rectangle){460, 120, 320, 200}, "Registro Completo", 2, false);
    AddDoorHotspot(f2, (Rectangle){1000, 120, 220, 480}, "Portal Temporal (2026)", 2, true, 2);
    rc++;

    // ============================================================
    // FASE 3 — 2026: A Câmara do Hidrogênio
    // Conceito: alucinação de IA e checagem de informações.
    // Mecânica: HS_CHOICE. Seguir cegamente = perde vida.
    // ============================================================
    Room *f3 = &rooms[rc];
    f3->id = rc;
    strcpy(f3->name, "Fase 3 - 2026: A Camara do Hidrogenio");
    f3->bgColor = (Color){ 35, 45, 60, 255 }; // PLACEHOLDER_ART: fundo_2026.png
    f3->hotspotCount = 0;
    f3->decorationCount = 0;
    // Colecionavel opcional da Fase 3 (equivalente a fotografar o jornal na Fase 1).
    AddCollectibleExamine(f3, (Rectangle){60, 100, 1160, 100}, "Painel da Camara",
        "A.R.1.3.L diz, com total confianca: 'Para sair, insira a chave no simbolo azul.' "
        "A resposta parece certa... mas sera que e verdadeira?");
    AddChoiceHotspot(f3, (Rectangle){80, 320, 460, 190}, "Seguir a Orientacao da IA", false,
        "Voce seguiu a instrucao sem verificar. A porta nao abriu e um alarme dispara. "
        "[Penalidade: -1 vida]", 0);
    AddChoiceHotspot(f3, (Rectangle){700, 320, 460, 190}, "Verificar a Informacao", true,
        "Ao checar os registros, Elira percebe que a orientacao da IA estava incorreta. "
        "Ela descobre que o projeto passou a estudar formas de preservar caracteristicas "
        "humanas: voz, linguagem, preferencias e padroes de comportamento.", 3);
    AddDoorHotspot(f3, (Rectangle){1000, 545, 220, 85}, "Portal Temporal (2048)", 3, true, 3);
    rc++;

    // ============================================================
    // FASE 4 — 2048: A Investigação Digital
    // Conceito: viés algorítmico e decisões automatizadas.
    // Puzzle 4 (seleção): achar o que a IA omitiu da análise.
    // ============================================================
    Room *f4 = &rooms[rc];
    f4->id = rc;
    strcpy(f4->name, "Fase 4 - 2048: A Investigacao Digital");
    f4->bgColor = (Color){ 50, 40, 55, 255 }; // PLACEHOLDER_ART: fundo_2048.png
    f4->hotspotCount = 0;
    f4->decorationCount = 0;
    // Colecionavel opcional da Fase 4 (equivalente a fotografar o jornal na Fase 1).
    AddCollectibleExamine(f4, (Rectangle){60, 120, 320, 200}, "Analise da IA",
        "A.R.1.3.L apresenta uma versao filtrada das evidencias do caso.");
    AddPuzzleHotspot(f4, (Rectangle){460, 120, 320, 200}, "Registros Originais", 4, false);
    AddDoorHotspot(f4, (Rectangle){1000, 120, 220, 480}, "Portal Temporal (Presente)", 4, true, 4);
    rc++;

    // ============================================================
    // FASE 5 — Presente: O Laboratório Central
    // Conceito: viés em dados de treinamento.
    // Puzzle 5 (seleção): achar os datasets desbalanceados.
    // Recompensa: apenas acesso (item 90), nao conta pro final.
    // ============================================================
    Room *f5 = &rooms[rc];
    f5->id = rc;
    strcpy(f5->name, "Fase 5 - Presente: O Laboratorio Central");
    f5->bgColor = (Color){ 55, 55, 60, 255 }; // PLACEHOLDER_ART: fundo_laboratorio.png
    f5->hotspotCount = 0;
    f5->decorationCount = 0;
    AddExamine(f5, (Rectangle){60, 120, 320, 200}, "Servidores de Dados",
        "Fileiras de conjuntos de dados usados para treinar A.R.1.3.L ao longo dos anos.");
    AddPuzzleHotspot(f5, (Rectangle){460, 120, 320, 200}, "Organizar Datasets", 5, false);
    AddDoorHotspot(f5, (Rectangle){1000, 120, 220, 480}, "Camara Quantica", 5, true, 90);
    rc++;

    // ============================================================
    // CONFRONTO FINAL — Câmara Quântica
    // targetRoomId == -1 dispara o calculo do final do jogo.
    // ============================================================
    Room *ff = &rooms[rc];
    ff->id = rc;
    strcpy(ff->name, "Confronto Final - Camara Quantica");
    ff->bgColor = (Color){ 15, 15, 30, 255 }; // PLACEHOLDER_ART: fundo_camara_quantica.png
    ff->hotspotCount = 0;
    ff->decorationCount = 0;
    AddExamine(ff, (Rectangle){60, 120, 1160, 220}, "Nucleo de A.R.1.3.L",
        "'Ela nasceu das memorias de uma pessoa, mas cresceu com as memorias de "
        "toda a humanidade.' Elira usa tudo o que aprendeu para confrontar a logica da IA.");
    AddPuzzleHotspot(ff, (Rectangle){60, 380, 400, 150}, "Terminal de Classificacao de Dados", 6, false);
    AddDoorHotspot(ff, (Rectangle){500, 460, 280, 140}, "Confrontar A.R.1.3.L", -1, true, 91);
    rc++;

    *roomCount = rc;
}
