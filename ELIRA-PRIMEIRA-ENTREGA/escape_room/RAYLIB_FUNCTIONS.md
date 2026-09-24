# Funções da Raylib usadas no projeto

Documento separado, como pedido — reúne toda função/tipo/constante da
raylib chamada em algum lugar do código, com o que ela faz e onde é
usada aqui.

## Janela e game loop (`main.c`)

| Função | O que faz |
|--------|-----------|
| `InitWindow(w, h, titulo)` | Cria a janela e o contexto gráfico (OpenGL). Precisa ser a primeira chamada da raylib no programa — nada de textura/som pode ser carregado antes dela. |
| `SetTargetFPS(60)` | Limita o loop a rodar no máximo 60 vezes por segundo, controlando a velocidade do jogo. |
| `HideCursor()` | Esconde o cursor padrão do sistema operacional. Usamos isso porque desenhamos nosso próprio cursor (o "personagem") com `DrawCircleV`/`DrawCircleLines`. |
| `WindowShouldClose()` | Retorna `true` quando o usuário pediu pra fechar a janela (botão de fechar ou ESC por padrão). Condição do loop principal `while`. |
| `BeginDrawing()` / `EndDrawing()` | Marcam o início e o fim de um frame. Tudo que desenha na tela (`DrawX`) precisa estar entre essas duas chamadas. |
| `CloseWindow()` | Libera a janela e o contexto gráfico ao final do programa. |

## Entrada (mouse e teclado)

| Função | O que faz |
|--------|-----------|
| `GetMousePosition()` | Retorna a posição atual do cursor como um `Vector2 {x, y}`. É a base de toda a interação point-and-click: usamos essa posição pra saber onde o "personagem" (mouse) está e testar colisão com os hotspots. |
| `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)` | `true` só no frame exato em que o botão esquerdo foi clicado (não fica repetindo enquanto o botão fica pressionado). Usado pra todo clique em hotspot/carta/botão. |
| `IsMouseButtonDown(MOUSE_LEFT_BUTTON)` | `true` em TODO frame enquanto o botão está pressionado (diferente de `IsMouseButtonPressed`, que só dispara uma vez). Usado no puzzle de arrastar arquivos do Confronto Final, pra mover o arquivo junto com o mouse enquanto o jogador segura o clique. |
| `IsMouseButtonReleased(MOUSE_LEFT_BUTTON)` | `true` só no frame em que o botão foi solto. Usado no mesmo puzzle, pra checar em cima de qual pasta o arquivo foi largado. |
| `IsKeyPressed(tecla)` | Igual a `IsMouseButtonPressed`, mas pra teclado — `true` só no frame em que a tecla foi apertada. Usado com `KEY_D` (abrir/fechar diário), `KEY_ENTER`/`KEY_SPACE` (fechar mensagem, reiniciar) e `KEY_ESCAPE` (fechar diário). |

Constantes de entrada usadas: `MOUSE_LEFT_BUTTON`, `KEY_D`, `KEY_ENTER`,
`KEY_SPACE`, `KEY_ESCAPE`.

## Colisão

| Função | O que faz |
|--------|-----------|
| `CheckCollisionPointRec(ponto, retangulo)` | Testa se um ponto (a posição do mouse) está dentro de um retângulo. É o teste central de todo o jogo: "o jogador clicou dentro deste hotspot/botão/carta?" |
| `CheckCollisionRecs(retA, retB)` | Testa se dois retângulos se sobrepõem. Usada só em `Game_ValidateRoomHotspots` (dentro de `Game_Init`), pra avisar automaticamente no console se dois hotspots da mesma fase foram desenhados um em cima do outro — exatamente o tipo de bug do cofre atrás do quadro. |

## Desenho

| Função | O que faz |
|--------|-----------|
| `ClearBackground(cor)` | Pinta a tela inteira com uma cor sólida antes do resto ser desenhado por cima. Hoje é o "fundo" placeholder de cada fase/menu/tela de final (ver `ASSETS_MANIFEST.md` pra saber onde isso vira uma imagem depois). |
| `DrawRectangle(x, y, w, h, cor)` | Desenha um retângulo preenchido a partir de 4 números soltos. Usado nas barras de HUD (topo e inventário) e no overlay escuro atrás dos painéis. |
| `DrawRectangleRec(retangulo, cor)` | Igual ao de cima, mas recebendo um `Rectangle` pronto em vez de 4 números — usamos essa versão pra desenhar o próprio hotspot, já que ele guarda o retângulo como struct. |
| `DrawRectangleLinesEx(retangulo, espessura, cor)` | Desenha só o contorno (borda) de um retângulo, com espessura configurável. Usado pra destacar hotspots ao passar o mouse por cima (hover) e pra molduras de painéis/botões. |
| `DrawText(texto, x, y, tamanhoFonte, cor)` | Desenha texto usando a fonte padrão da raylib. Usado em praticamente toda a UI: rótulos de hotspot, HUD, painéis de puzzle, diário, telas de menu/final. |
| `MeasureText(texto, tamanhoFonte)` | Retorna a largura em pixels que um texto vai ocupar com aquele tamanho de fonte, sem desenhar nada. Usado pra centralizar títulos (menu, telas de final) e pra decidir quando uma linha de texto passou da largura máxima em `DrawWrappedText` (nossa função de quebra de linha manual). |
| `Fade(cor, alpha)` | Recebe uma cor e devolve a mesma cor com transparência ajustada (0.0 = invisível, 1.0 = opaca). Usado bastante pra escurecer a tela atrás de overlays (`Fade(BLACK, 0.5f)`) e pra dar feedback visual de hover/seleção nos hotspots e cartas. |
| `DrawCircleV(centro, raio, cor)` | Desenha um círculo preenchido a partir de um `Vector2`. Usado pra desenhar o "ponto" central do cursor customizado (o personagem). |
| `DrawCircleLines(x, y, raio, cor)` | Desenha só o contorno de um círculo. Usado junto com `DrawCircleV` pra formar o cursor customizado (ponto sólido + anel ao redor). |

## Log/Debug

| Função | O que faz |
|--------|-----------|
| `TraceLog(nivel, formato, ...)` | Imprime uma mensagem no console de saída da raylib, igual um `printf` com nível de severidade. Usado com `LOG_WARNING` em `Game_ValidateRoomHotspots` pra avisar sobre hotspots sobrepostos assim que o jogo inicia. |

## Tipos e cores (não são funções, mas vêm da raylib)

- **`Vector2 { float x, y; }`** — usado pra posição do mouse.
- **`Rectangle { float x, y, width, height; }`** — usado pra todo hotspot, botão e painel de UI.
- **`Color { unsigned char r, g, b, a; }`** — usado pras cores placeholder de fases e itens.
- Cores nomeadas prontas da raylib usadas no código: `BLACK`, `RAYWHITE`, `WHITE`, `GRAY`, `DARKGRAY`, `LIGHTGRAY`, `YELLOW`, `GREEN`, `RED`, `SKYBLUE`, `BROWN`, `DARKBROWN`, `MAROON`, `GOLD`. Todas já vêm definidas em `raylib.h`, não precisam ser criadas manualmente.
