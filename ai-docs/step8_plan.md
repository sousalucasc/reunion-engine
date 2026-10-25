# Step 8 — Câmera free-fly (plano)

Entrada: `progresso.md` (Steps 1–7 + auditoria 6.1) e `qwen_prompt_pack.md` (Step 8, seção B, linhas 223–235).
Arquivos que o passo nomeia: **`player.cpp`** (novo) e **`main.cpp`**.
`base.cpp`, `raster.cpp`, `build.bat`, `win32_handmade.cpp`: **intactos**. Nenhum `#include` novo além de
`player.cpp` (que é o último da lista, como o spec manda).

---

## 1. O que o spec pede (literal)

- `player.cpp`: `f32 MoveSpeed = 10.0f; f32 MouseSens = 0.0025f;`
- `void UpdateFlyCamera()`:
  - `MouseCaptured`: `Cam.yaw += In.mouseDX*MouseSens; Cam.pitch -= In.mouseDY*MouseSens;` clamp pitch ±1.5
  - setas continuam a girar `1.5 rad/s * Dt` (clamp pitch)
  - `forwardFlat = (Sin(yaw), 0, Cos(yaw))`, `rightFlat = (Cos(yaw), 0, -Sin(yaw))`
  - W/S ao longo de `forwardFlat`, D/A ao longo de `rightFlat`, `Space` sobe, `Ctrl` desce
  - somar as direções num vetor só, normalizar se comprimento > 0, multiplicar por
    `MoveSpeed*Dt` (dobrado com `Shift`), somar em `Cam.pos`
- `main.cpp`: trocar o código antigo de setas por `UpdateFlyCamera()` chamado todo frame **antes de
  `SetupCamera()`**; `Cam.pos` x, y, z ×1000 nos slots 1..3 do título.
- Check: clique captura o mouse, o mouse gira naturalmente (mouse à direita = girar à direita, mouse
  para cima = olhar para cima), WASD anda relativo ao que você olha, Space/Ctrl sobem/descem.

---

## 2. `player.cpp` (arquivo novo, incluído por último)

Ordem de dependência: `Cam`/`SetupCamera` vêm de `raster.cpp`, `In`/`MouseCaptured`/`Clamp`/`Sin`/`Cos`/
`V3*`/`DebugSet`/`Dt` vêm de `base.cpp`. Tudo está definido antes dele na unidade de tradução.

```cpp
// player.cpp - free-fly camera movement. Included last by main.cpp, so it may use
// Camera (raster.cpp) and In/MouseCaptured/math helpers (base.cpp).

f32 MoveSpeed = 10.0f;
f32 MouseSens = 0.0025f;

const f32 CAMERA_TURN_SPEED = 1.5f; // radians per second, arrow keys

void UpdateFlyCamera() { ... }
```

`CAMERA_TURN_SPEED` sai de `main.cpp` e entra em `player.cpp`: a regra das setas agora vive dentro de
`UpdateFlyCamera`, e o passo só pode tocar `player.cpp` e `main.cpp`.

---

## 3. `main.cpp`

| Mudança | Onde | Motivo |
|---|---|---|
| `#include "player.cpp"` depois de `raster.cpp` | topo | fim da lista de includes do RULES (world.cpp/render.cpp ainda não existem) |
| remover `const f32 CAMERA_TURN_SPEED` e `UpdateCameraWithArrowKeys()` | bloco "Step 7" | o Step 8 substitui a lógica de setas; a const vai para `player.cpp` |
| `UpdateFlyCamera()` no lugar de `UpdateCameraWithArrowKeys()` | `RunOneFrame`, mesma posição (depois do bloco do ESC, antes do `FlushTimer`) | roda inclusive com a janela minimizada, como antes; `SetupCamera()` continua depois da guarda `IsIconic` |
| remover `DebugSet(2, 0, 0)` | `WinMain` | era para limpar o label `MOUSEX` sobrando do Step 4; os slots 1..3 são escritos todo frame por `UpdateFlyCamera` |

Título depois do passo: `FPS | X | Y | Z | US` (slots 4..6 vazios; `US` sai só no Step 35).

---

## 4. Matemática prevista (para a auditoria ter números esperados)

Câmera inicial `(0, 1, -6)`, yaw 0, pitch 0 → view: `x = p.x`, `y = p.y - 1`, `z = p.z + 6`.
`f = cot(0.61085) ≈ 1.4176`, `aspect = 320/180 = 1.7778`, `m[0][0] ≈ 0.7974`.

Cena parada (medido no Step 7): laranja x 118–201 y 48–110, ciano x 134–159 y 69–115, céu 54.807 px.

| Tecla segurada | `Cam.pos` esperado | Laranja na tela |
|---|---|---|
| W (yaw 0) | z: −6 → −4 (2 unidades em 0,2 s) | cresce: x 96–224, y ~26–140 |
| S | z: −6 → −8 | encolhe: x ~129–190, y ~60–98 |
| D | x: 0 → +2 | desliza para a esquerda: x ~78–160 |
| A | x: 0 → −2 | espelho: x ~160–242 |
| Space | y: 1 → +3 | desce na tela: ápice y 48 → ~90 |
| Ctrl | y: 1 → −1 | sobe na tela: ápice y 48 → ~5 |
| Shift + W | 20 unidades/s em vez de 10 | velocidade dobra |

Com yaw ≈ 0,6 rad (seta direita ~0,4 s), segurar W deve mover **X e Z juntos**: ΔX/ΔZ ≈ tan(0,6) ≈ 0,69,
ΔZ ainda positivo (continua andando para frente na direção que você olha). É o check "WASD relativo ao
que você olha".

Mouse: `MouseSens = 0.0025` → mover 15 px dá 0,0375 rad por poll. Cursor à direita do centro
(`mouseDX > 0`) → `yaw` cresce (gira para +X, objetos deslizam para a esquerda). Cursor **acima** do
centro (`mouseDY < 0`) → `pitch -= (negativo)` → `pitch` cresce (olha para cima, objetos descem na tela).

---

## 5. Verificação (protocolo do fluxo atual)

1. **Build**: `cd code` → `build.bat` e `build.bat release`: 0 erros, 0 warnings (`/W3`), exit 0.
2. **Imagens**: `build\mc.exe --dump` da raiz; `dump_backbuffer.bmp` → PNG (`bmp2png.ps1`) e mostrar no
   Review pane: cena parada, W, D, Space. Checar paleta de 3 cores e as bounding boxes da tabela.
3. **Velocidade das teclas** (`step8_audit.ps1`, parte A): run vivo segurando W/S/A/D/Space/Ctrl/Shift+W
   ~1,4 s, lendo o título (`X/Y/Z`) a cada 120 ms; medir Δ por segundo e o eixo/-signo.
   Esperado: 10 unidades/s (20 com Shift), nenhum eixo errado.
4. **Mouse look** (parte B): click captura (`mouse_event`), `SetCursorPos` deslocado a cada poll,
   `YAW`/`PITCH` lidos no título (slots 4/5 adicionados **só para a auditoria**, removidos depois).
   Esperado: `+dx` → `YAW` cresce; cursor acima do centro → `PITCH` cresce; clamp ±1500.
5. **WASD relativo ao olhar** (parte C): girar ~0,6 rad com a seta direita, depois segurar W: ΔX>0 e ΔZ>0
   com ΔX/ΔZ ≈ tan(yaw); com yaw 0, W muda só Z.
6. **Regressão** (parte D): clique captura (cursor flags 0), WASD anda com o mouse capturado, 1º ESC
   devolve o cursor e a janela continua viva, 2º ESC fecha com exit 0; FPS ~59/60 com cap, `--nocap`
   release mede o custo do frame; nada de `mc.exe` sobrando.

---

## 6. Riscos e decisões explícitas

- **Yaw não enrolado**: `Sin`/`Cos` já fazem wrap internamente; só `X`/`Z` do título são impressos, então
  não há risco de overflow do `i32` com yaw grande. Não adicionar wrap (fora do passo).
- **Mouse só gira quando capturado** (spec). Sem clique, `mouseDX/DY` são 0 → cena não gira sozinha.
- **`V3Norm` já protege** contra vetor zero (len < 0.000001 retorna a entrada); o teste explícito
  `V3Len(wish) > 0` é o do spec e evita multiplicar vetor zero.
- **Sem colisão** neste passo: `Cam.pos` pode atravessar os triângulos de teste; física é o Step 28.
- **Triângulos somem perto do plano near** (`w < NEAR_W`, rejeição por vértice do Step 7) — voar para
  frente deles pode deixar a tela só céu. É o buraco que o Step 9 fecha; não corrigir agora.
- **Sem git**: Step 8 não tem snapshot; se der errado, editar de volta manualmente.
- Depois do check: rodar o prompt de API e reescrever `ai-docs\api.txt` (agora com `player.cpp`).

---

## 7. Critério de aprovação

Check do pack: **clique captura o mouse; mouse gira naturalmente (direita = girar à direita, cima =
olhar para cima); WASD anda relativo ao que você olha; Space/Ctrl sobem/descem**. Se passar, a entrada
do Step 8 vai para `progresso.md`, `api.txt` é atualizado, e o "Próximo passo" passa a ser Step 9
(near-plane clipping, Sutherland-Hodgman, `raster.cpp` + piso verde de teste).

DONE
