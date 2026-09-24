# ELIRA — Decifra.IA (protótipo em C + Raylib)

Jogo de ficção científica point-and-click sobre letramento digital e uso
crítico de IA. Elira viaja pelas eras para desativar a IA tirânica
**A.R.1.3.L**. O jogador é representado pelo próprio cursor do mouse.

Todos os visuais ainda são **placeholders** (retângulos coloridos com
texto) — a lógica do jogo está pronta e funcional para você plugar a
arte em cima depois. Veja `ASSETS_MANIFEST.md` pra saber exatamente o
que precisa ser produzido e onde entra.

## Como jogar

- Mova o mouse: é o seu personagem.
- Clique nos retângulos (hotspots) pra examinar, abrir puzzles, tomar
  decisões ou avançar de fase.
- **D**: abre o Diário de Bordo.
- **ENTER** na tela de final: reinicia o jogo.

## Estrutura narrativa (rooms_data.c)

| # | Fase | Conceito pedagógico | Mecânica | Recompensa |
|---|------|---------------------|----------|------------|
| 1 | 1990 — A Origem dos Estudos | Fundamentos de IA / dados relevantes | Jornal legível + pasta com 3 documentos + cofre de combinação | Disquete de Memória |
| 2 | 2008 — Comportamento Humano | Filtro-bolha / recomendação | Seleção de cartas | CD de Memória |
| 3 | 2026 — Câmara do Hidrogênio | Alucinação de IA | Escolha certo/errado (perde vida se errar) | SSD de Memória |
| 4 | 2048 — Investigação Digital | Viés algorítmico | Seleção de cartas | Dados em Nuvem |
| 5 | Presente — Laboratório Central | Viés em dados de treino | Seleção de cartas | Acesso à Câmara Quântica (não conta pro final) |
| 6 | Confronto Final — Câmara Quântica | — | Porta especial (`targetRoomId = -1`) | Calcula o final |

**Cálculo do final** (`Game_ComputeEnding` em `game.c`): conta quantos
dos 4 Artefatos de Memória (ids 1–4) estão no inventário.
- 0–1 artefato → **Final Ruim**
- 2–3 artefatos → **Final Normal**
- 4 artefatos → **Final Ótimo**

Perder todas as vidas na Fase 3 força o **Final Ruim** imediatamente,
representando "aceitar as respostas da IA acriticamente".

## Compilando

Precisa da [Raylib](https://www.raylib.com/) instalada.

### Linux (Debian/Ubuntu)
```bash
sudo apt install libraylib-dev
make
./escape_room
```
Alternativa via pkg-config:
```bash
gcc -Wall -std=c99 -Iinclude src/*.c -o escape_room $(pkg-config --libs --cflags raylib)
```

### macOS
```bash
brew install raylib
gcc -Wall -std=c99 -Iinclude src/*.c -o escape_room -lraylib -framework OpenGL -framework Cocoa -framework IOKit
```

### Windows (w64devkit ou MSYS2/MinGW)
```bash
gcc -Wall -std=c99 -Iinclude src/*.c -o escape_room.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

## Arquitetura

```
include/
  common.h      constantes globais + include do raylib.h
  item.h        struct Item (Artefatos de Memória)
  inventory.h   struct Inventory
  diary.h       struct Diary ("Diário de Bordo")
  puzzle.h      struct Puzzle: PUZZLE_COMBINATION, PUZZLE_SELECTION, PUZZLE_DRAGDROP
  hotspot.h     struct Hotspot: HS_EXAMINE, HS_ITEM, HS_PUZZLE, HS_DOOR, HS_CHOICE,
                HS_NEWSPAPER, HS_DOCFOLDER
  room.h        struct Room (= uma Fase/Era do roteiro) + Decoration (cenário não clicável)
  game.h        struct Game (estado global, vidas, pontos, final)

src/
  item.c        cadastro dos 5 itens (4 artefatos + 1 acesso)
  inventory.c   lógica de inventário
  diary.c       lógica do diário
  puzzle.c      lógica dos dois tipos de puzzle
  rooms_data.c  TODA a narrativa: as 6 fases, pistas, cartas, portas
  game.c        update/draw + máquina de estados + cálculo de final
  main.c        janela raylib + game loop
```

### ⚠️ Regra de ouro: hitboxes não podem se sobrepor

Um hotspot é só um `Rectangle` clicável. Se dois hotspots da mesma fase
tiverem retângulos que se sobrepõem, o clique **sempre** acerta o
primeiro da lista (foi exatamente o bug do cofre atrás do quadro).

Pra evitar isso:
- Layout em grade, com espaçamento generoso entre elementos (é o que
  `rooms_data.c` faz agora — compare com as posições de cada fase).
- `Game_Init` roda `Game_ValidateRoomHotspots` automaticamente pra
  **toda** fase, e imprime um aviso no console (via `TraceLog`) se
  encontrar hotspots sobrepostos. Sempre olhe o console ao testar uma
  fase nova.

### Máquina de estados (`GameState`)
`STATE_MENU → STATE_PLAYING ⇄ (STATE_PUZZLE | STATE_MESSAGE | STATE_DIARY | STATE_NEWSPAPER | STATE_DOCS) → STATE_ENDING`

`STATE_NEWSPAPER` e `STATE_DOCS` são interações próprias da Fase 1 (não
passam pelo dispatch de puzzles porque não têm "certo/errado" por si só)
— ver `UpdateNewspaper`/`UpdateDocs` em `game.c`.

### Sistema de Puzzles
- `PUZZLE_COMBINATION`: teclado numérico. Usado pelo cofre da Fase 1
  (`UpdatePuzzle_Fase1`/`DrawPuzzle_Fase1`).
- `PUZZLE_SELECTION`: várias cartas (`SelectionOption`), o jogador marca
  quais pertencem ao conjunto correto e confirma. Usado nas Fases 2, 4 e
  5. O acerto exige selecionar **exatamente** o conjunto certo (nem a
  mais, nem a menos) — ver `Puzzle_CheckSelection`.
- `PUZZLE_DRAGDROP`: arrastar arquivos até a pasta correta. Usado no
  Confronto Final.

Cada fase despacha pro seu próprio par `UpdatePuzzle_FaseX`/
`DrawPuzzle_FaseX` (switch por `puzzleId` em `UpdatePuzzle`/
`DrawPuzzleOverlay`), então dá pra customizar a UI de uma fase sem
mexer nas outras, mesmo quando duas reaproveitam o mesmo `PuzzleType`
por baixo.

### Sistema de Escolhas (`HS_CHOICE`)
Usado na Fase 3. Cada hotspot de escolha é marcado como certo ou errado
na criação (`AddChoiceHotspot`). Escolher a opção certa dá o item de
recompensa e soma Pontos de Consciência Crítica; escolher a errada tira
uma vida.

### O jornal emoldurado e o cofre escondido (Fase 1)
`HS_NEWSPAPER` é um objeto sempre legível (imersão) com dois botões:
**Fotografar** (marca `photographed`, não consome nada, pode ser feito
antes ou depois do outro botão, e continua disponível mesmo depois do
quadro já ter sido deslocado) e **Tirar da Parede** (marca
`removedFromWall`, move o próprio jornal pra um novo lugar na mesma
sala e revela outro hotspot via `revealsHotspotId`).

"Tirar da Parede" não faz o jornal desaparecer — ele se desloca pro
lado (`h->rect = h->sideRect`, um retângulo vizinho definido em
`AddNewspaperHotspot`) e continua clicável dali, exatamente como a
Elira teria realmente movido o quadro. Clicar nele de novo depois
ainda abre o mesmo overlay (o botão "Tirar da Parede" fica cinza e
mostra "Já Deslocado"; "Fotografar" continua funcionando normalmente).

O cofre por trás dele usa o campo `hidden` do `Hotspot`: nasce em
`AddPuzzleHotspot(..., hidden = true)` **no mesmo retângulo** onde o
jornal começa pendurado. Isso violaria a regra de não sobrepor
hitboxes — só que aqui é proposital, então `Game_ValidateRoomHotspots`
ignora esse par porque o cofre já nasce `hidden`. No momento em que o
jornal se move pro `sideRect`, o cofre vira `hidden = false` e passa a
ocupar sozinho o retângulo que o jornal deixou na parede — então nunca
há dois hotspots "ativos" disputando o mesmo clique ao mesmo tempo.

A senha do cofre (`"1950"`) está escrita no rodapé de um dos 3
documentos dentro de `HS_DOCFOLDER` ("Pasta de Estudos sobre IA", em
cima da mesa decorativa) — só o documento que descreve um uso correto
de IA (conferir a resposta em fontes confiáveis) tem o código real; os
outros dois têm códigos-isca. Ver os textos completos em
`rooms_data.c`.

## Como adicionar/ajustar uma fase

Em `src/rooms_data.c`, copie o bloco de uma fase existente e ajuste
nome, cor de fundo (placeholder) e hotspots com `AddExamine`,
`AddItemHotspot`, `AddPuzzleHotspot`, `AddChoiceHotspot`,
`AddNewspaperHotspot`, `AddDocFolderHotspot`/`DocFolder_AddDocument`,
`AddDecoration` e `AddDoorHotspot` — sempre conferindo que os
retângulos não se tocam (exceto um par proposital com `hidden = true`,
ver seção acima).

Para editar as pistas/cartas de um puzzle de seleção, ou o código do
cofre da Fase 1, mexa em `Game_Init` (`src/game.c`), na chamada
`Puzzle_InitSelection` + `Puzzle_AddOption`, ou `Puzzle_InitCombination`,
correspondente ao `puzzleId`.

## Próximos passos sugeridos

- **Plugar as artes**: ver `ASSETS_MANIFEST.md`. O padrão sugerido é
  adicionar um campo `Texture2D bgTexture` em `Room` (e equivalente em
  `Item`/cartas), carregar com `LoadTexture` em `Game_Init`, e trocar
  `ClearBackground`/`DrawRectangleRec` por `DrawTexture`/`DrawTexturePro`
  onde hoje há um comentário `// PLACEHOLDER_ART: ...`.
- **Áudio**: `InitAudioDevice()` no `main.c` + `PlaySound()` em acertos,
  erros e transições de fase.
- **Save/load**: serializar `Inventory`, `criticalPoints`, `lives`,
  quais `HS_ITEM`/`HS_CHOICE` já foram resolvidos, e `currentRoomId`.
- **Refinar as mecânicas por fase**: as seleções (Fases 1, 2, 4, 5)
  hoje reaproveitam a mesma mecânica de cartas. Se quiser algo visual e
  mecanicamente mais específico por fase (ex: um "feed" de rede social
  de verdade na Fase 2, ou um dashboard de datasets na Fase 5), dá pra
  criar novos `PuzzleType` seguindo o mesmo padrão de
  `PUZZLE_SELECTION`.
