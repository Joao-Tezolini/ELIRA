# Manifesto de Assets — ELIRA

Todo lugar do código marcado com `// PLACEHOLDER_ART: nome_do_arquivo.png`
é onde uma arte final deve entrar. Enquanto o arquivo não existir, o
jogo desenha um retângulo colorido no lugar (não quebra nada).

Sugestão de resolução: fundos de fase em 1280x720 (tamanho da janela).
Ícones de item/carta podem ser menores (ex: 128x128).

## Fundos de fase (Color placeholder → arquivo esperado)

| Fase | Cor placeholder atual | Arquivo esperado |
|------|-----------------------|-------------------|
| Menu | `(15,15,25)` | `fundo_menu.png` |
| Fase 1 — 1990 | `(70,60,45)` sépia | `fundo_1990.png` |
| Fase 2 — 2008 | `(45,55,65)` azulado | `fundo_2008.png` |
| Fase 3 — 2026 (Câmara do Hidrogênio) | `(35,45,60)` | `fundo_2026.png` |
| Fase 4 — 2048 (Investigação Digital) | `(50,40,55)` | `fundo_2048.png` |
| Fase 5 — Presente (Laboratório) | `(55,55,60)` | `fundo_laboratorio.png` |
| Confronto Final (Câmara Quântica) | `(15,15,30)` | `fundo_camara_quantica.png` |
| Tela de final (qualquer um dos 3) | `(10,10,15)` | `fundo_final.png` (ou 3 variações: `fundo_final_ruim.png`, `fundo_final_normal.png`, `fundo_final_otimo.png`) |

## Ícones dos Artefatos de Memória (usados no inventário)

Definidos em `src/item.c`, cor placeholder de cada:

| Item | Cor placeholder | Arquivo esperado |
|------|------------------|-------------------|
| Disquete de Memória | azul escuro | `icone_disquete.png` |
| CD de Memória | cinza claro | `icone_cd.png` |
| SSD de Memória | verde | `icone_ssd.png` |
| Dados em Nuvem | azul céu | `icone_nuvem.png` |
| Autorização da Câmara Quântica | dourado | `icone_autorizacao.png` |

## Elementos específicos da Fase 1 (jornal, mesa e pastas)

A Fase 1 tem placeholders próprios, marcados no código com
`PLACEHOLDER_ART` em `rooms_data.c`:

| Elemento | Tipo | Cor placeholder | Arquivo esperado |
|----------|------|------------------|-------------------|
| Jornal Emoldurado (`HS_NEWSPAPER`) | hotspot (retângulo marrom) | `Fade(BROWN, ...)` | `jornal_emoldurado.png` (moldura + recorte de jornal amarelado) |
| Cofre Embutido na Parede (`HS_PUZZLE`, nasce `hidden`) | hotspot, mesmo retângulo do jornal | verde quando resolvido | `cofre_parede.png` (só aparece depois que o jornal for retirado) |
| Mesa (decoração, não clicável) | `Decoration` desenhada atrás da pasta | marrom escuro | `mesa_escritorio.png` |
| Pasta de Estudos sobre IA (`HS_DOCFOLDER`, em cima da mesa) | hotspot (retângulo dourado) | `Fade(GOLD, ...)` | `pasta_documentos.png` |
| Pasta de Recibos / Pasta de Correspondências (pistas falsas) | hotspots `HS_EXAMINE` | azul padrão | `pasta_generica.png` (pode reaproveitar a mesma arte pras duas) |

O jornal e o cofre **compartilham o mesmo retângulo de tela** de
propósito (o cofre nasce escondido atrás do jornal) — ao plugar arte
de verdade, desenhe o cofre só quando `hidden == false`, exatamente
como a lógica do jogo já faz com os retângulos coloridos.

## Elementos interativos por fase (hotspots)

Cada hotspot em `src/rooms_data.c` tem um `label` — use-o como
referência do que ilustrar em cada retângulo. Nenhum precisa de arte
própria pra funcionar (o retângulo com o texto do `label` já é
suficiente pro protótipo), mas ficam mais bonitos com:

- **Fase 1**: "Terminal de Pesquisa" (um terminal antigo/CRT), ver
  tabela acima pro jornal/mesa/pastas.
- **Fase 2**: "Feed Recomendado" (tela de celular/rede social),
  "Registro Completo" (um arquivo/pasta).
- **Fase 3**: painel de controle futurista, e os dois botões de
  escolha ("Seguir a Orientação da IA" vs "Verificar a Informação").
- **Fase 4**: "Análise da IA" (holograma/relatório), "Registros
  Originais" (arquivo físico antigo).
- **Fase 5**: "Servidores de Dados" (racks de servidor),
  "Organizar Datasets" (painel de controle).
- **Confronto Final**: núcleo/orbe representando A.R.1.3.L.

## Como plugar depois de pronto

1. Adicionar um campo `Texture2D bgTexture;` em `Room` (`room.h`) e/ou
   `Texture2D icon;` em `Item`/`SelectionOption`.
2. Em `Game_Init`, carregar com `LoadTexture("assets/fundo_1990.png")`
   etc. (cuidado: `InitWindow` precisa rodar antes de qualquer
   `LoadTexture`).
3. Trocar `ClearBackground(room->bgColor)` por
   `DrawTexture(room->bgTexture, 0, 0, WHITE)` (ou `DrawTexturePro`
   se precisar redimensionar).
4. Chamar `UnloadTexture` pra cada textura antes de `CloseWindow()`
   em `main.c`, pra não vazar memória de GPU.
