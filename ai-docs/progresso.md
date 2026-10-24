# Progresso — reunion-engine (clone de Minecraft, renderização por software)

Fluxo: um passo por vez, sempre pequeno. Eu implemento, compilo, capturo imagem para
análise própria e mostro no Review pane. Só avanço com sua aprovação; a entrada de cada
passo é escrita quando você aprova. Git ignorado por enquanto (não está no PATH), então
não há commits; este arquivo é o registro.

## Ferramentas de análise (fora do caminho normal de renderização)
- `mc.exe --dump` roda ~1.5 s, grava a cada 0.5 s e se fecha sozinho:
  - `build\dump_info.txt`: tamanho do client, tamanho da janela, dpiAware, resolução do monitor, título da janela
  - `build\dump_backbuffer.bmp`: conteúdo de `BackPixels` (320x180)
  - `build\dump_screen.bmp`: conteúdo real do DC da janela (`CreateDIBSection` + `BitBlt`), 960x540
- Conversor BMP→PNG para eu ler a imagem: `C:\Users\lucas\AppData\Local\Temp\opencode\bmp2png.ps1`
  (os BMPs não abrem direto para você; converto para PNG e mostro no Review pane)
- Build: rodar `build.bat` **de dentro de `code\`** (`code> build.bat`). Ele chama o vcvarsall x64
  e compila `..\code\main.cpp` → `..\build\mc.exe` (`cl -FC -Zi -Od -W3 ... user32.lib gdi32.lib`).
  Quando FPS começar a importar, trocar `/Od` por `/O2 /fp:fast`.

## Step 1 — Baseline (main.cpp, base.cpp) — APROVADO
Criado `code\base.cpp` e `code\main.cpp` conforme o pack. `code\win32_handmade.cpp` ficou intacto (referência do Day 4).

Código:
- `base.cpp`: typedefs `u8/u16/u32/i32/f32`, `SCREEN_W=320`, `SCREEN_H=180`, `static u32 BackPixels[SCREEN_W*SCREEN_H]`, `MakeColor`, `PutPixel` (ignora fora da tela), `ClearScreen`.
- `main.cpp`: `#include <windows.h>` + `#include "base.cpp"`; classe com `CS_HREDRAW|CS_VREDRAW|CS_OWNDC`; janela com client de 960x540 via `AdjustWindowRect`; loop com `PeekMessageA`; `WM_CLOSE` → `DestroyWindow` → `WM_DESTROY` → `PostQuitMessage(0)`; `BITMAPINFO` 320x180, 32 bits, top-down (`biHeight` negativo), `BI_RGB`; cada frame preenche `BackPixels` com gradiente via `PutPixel` e escala para o client com `StretchDIBits(SRCCOPY)`; ESC fecha.

Correções que precisaram de decisão sua:
1. **`StretchDIBits` tinha dest/src invertidos** (herdado do `win32_handmade.cpp`, que tem a mesma troca em `Win32UpdateWindow`): desenhava 320x180 num canto e o resto ficava branco. Agora dest = client rect, src = `0,0,SCREEN_W,SCREEN_H`.
2. **`SetProcessDPIAware()` no início de `WinMain`** (mantido por sua aprovação): sem ele o Windows virtualizava o processo DPI-unaware e a janela saía maior/borrada. Medido: `client 960x540 | window 976x579 | dpiAware 1 | screen 1920x1080`.
3. **Vermelho contínuo** (decisão sua, desvio do spec): o spec diz `red = x`, que dá wrap em u8 em x=256 e gera faixa vertical escura em ~80% da largura. Trocado para `red = x * 255 / (SCREEN_W - 1)`. `green = y` e `blue = frame counter` continuam como no spec.

Estado do build: compila sem erros nem warnings; `build\mc.exe` gerado; fechamento limpo (exit 0).

## Step 2 — Loop timing e debug no título (base.cpp, main.cpp) — APROVADO
`base.cpp`:
- `HWND GlobalWindow = 0; f32 Dt = 0.016f; i32 CurrentFPS = 0;`
- `struct DebugSlot { const char* label; i32 value; }; static DebugSlot DebugSlots[8];`
- `DebugSet(slot, label, value)` (ignora slot fora de 0..7)
- `DebugFlush()`: monta uma string com `wsprintfA` + `lstrcatA` em `char buffer[512]`, estilo `"FPS: 60 | X: 123"`, e chama `SetWindowTextA(GlobalWindow, buffer)`
- `FrameClock()`: `QueryPerformanceFrequency`/`QueryPerformanceCounter`, segundos desde a chamada anterior; primeira chamada retorna `0.016f`

`main.cpp`:
- `GlobalWindow = MainWindow` depois de criar a janela
- cada frame: `Dt = FrameClock()` com clamp em `0.1f`; contagem de frames; a cada 0.5 s calcula `CurrentFPS` e `DebugSet(0, "FPS", CurrentFPS)`; `DebugFlush()` só a cada 0.25 s

Check na sua tela: `FPS: <número>` atualizando 2x por segundo sem flicker. Minha medição via dump do título: `FPS: 1470` (loop sem vsync, valor real desta máquina). Compila sem erros nem warnings.

Correção feita no meu código de análise durante este passo: o dump rodava na iteração em que a janela já estava destruída e gravava valores lixo em `dump_info.txt`; agora o bloco só roda enquanto `Running` é verdadeiro.

## Step 3 — Biblioteca de matemática (`base.cpp`, `main.cpp` teste temporário) — APROVADO (check passou)
`base.cpp` (adicionado após `FrameClock`):
- consts `PI = 3.14159265f`, `TAU = 6.28318531f`, `NEAR_W = 0.05f`
- `Abs`, `AbsI`, `Min`, `Max`, `MinI`, `MaxI`, `Clamp`, `Lerp`
- `Floor`, `FloorI` (i32 cast + ajuste quando x < int), `FloorDiv` (para b > 0, arredonda para -inf), `Mod` (0..b-1 via FloorDiv)
- `Sqrt` por Newton: `x <= 0` → 0; guess inicial `x*0.5` se x > 1 senão 1; 24 iterações `g = 0.5*(g + x/g)`
- `Sin`: wrap `x -= TAU*Floor(x/TAU + 0.5)` para -PI..PI; dobra para -PI/2..PI/2 (`x > PI/2 → PI - x`, `x < -PI/2 → -PI - x`); série de Taylor até x^9/362880 com potências explícitas
- `Cos(x) = Sin(x + PI*0.5)`; `MemZero`, `MemCopy` (laços de bytes, sem CRT)
- structs `v3`, `v4`, `m4`; `V3`, `V3Add`, `V3Sub`, `V3Scale`, `V3Dot`, `V3Cross`, `V3Len`, `V3Norm` (retorna a entrada se len < 0.000001)
- `V4`, `M4Identity`, `M4Mul` (triplo laço, out = a*b), `M4MulV4`
- `M4Perspective(fovY, aspect)`: `f = Cos(fovY*0.5)/Sin(fovY*0.5)`, `m[0][0]=f/aspect`, `m[1][1]=f`, `m[2][2]=1`, `m[3][2]=1` (clip.w = view z)
- `M4View(pos, yaw, pitch)`: forward/right/up pelas CONVENTIONS (up = Cross(forward,right)); linhas right/up/forward com `-Dot(axis,pos)`, última linha (0,0,0,1)

`main.cpp`: função temporária `ShowMathTest()` (chamada após o gradiente) ocupando slots 1..5 ×1000.

Check na tela: `FPS: <n> | SQRT2: 1414 | SIN: 1000 | COS: -1000 | VIEWZ: 5000 | PROJW: 5000`. Medido via título real e via `dump_info.txt`: idêntico ao esperado. Compila sem erros nem warnings.

Nota de ferramenta: `mc.exe --dump` precisa ser rodado a partir da raiz do repo (os caminhos são relativos) e o PowerShell não espera um exe GUI — por isso leituras de `dump_info.txt` logo após lançar podem ler arquivo velho.

## Auditoria dos Steps 1 e 2 + 4 correções (pedido seu)
Pergunta: o render roda mesmo na resolução baixa? Sim, comprovado com medições:
- `dump_backbuffer.bmp` continua 320x180 mesmo com a janela em 1904x1041 (client cheio da tela)
- captura do DC da janela tem exatamente os mesmos 46.080 cores de `BackPixels`; **0** cores na tela que não existem em `BackPixels` → não há filtragem/interpolação
- cantos idênticos (`00003C` / `FF0058`→`00003C`/`FF003C` conforme frame) e pixels vizinhos iguais 86.6% na horizontal, 82.8% na vertical, batendo com o tamanho de bloco 5.95x5.78 (=1904/320, 1041/180)
- client medido no lançamento: **960x540** (janela 976x579), exatamente o spec; 960x540 é 1/4 de 1920x1080 em área

Por que o FPS caía ao maximizar: antes das correções, client 960x540 → ~1555 FPS, 86% de um núcleo; fullscreen 1904x1041 → ~674 FPS com CPU do processo quase igual (0.51 ms/frame) → o custo extra está no `StretchDIBits`/DWM (destino 4x maior), não na simulação.

Correções aprovadas por você (só `main.cpp`, sem criar arquivos, sem novos `#include`, `base.cpp` intacto):
1. **DC próprio usado**: `WindowDC` capturado uma vez após `CreateWindowExA` (classe tem `CS_OWNDC`) e liberado em `WM_DESTROY`; `PresentBackPixels` não chama mais `GetDC`/`ReleaseDC` por frame (~1500 HDC/s a menos).
2. **`ScreenBitmapInfo` zerado** com `MemZero` antes de preencher (antes `bmiColors` era memória não inicializada).
3. **Frame pacing**: cap em `TARGET_FPS = 60`. Descoberto no caminho: `Sleep(16)` e timers high-resolution não atingem 60 fps nesta máquina — o tick do timer do sistema é ~15.6 ms (`Sleep`-based media 28 ms → ~36 fps; `CreateWaitableTimerExW` falha aqui: `SetWaitableTimer` FALSE / `WaitForSingleObject` WAIT_FAILED err=5). Solução: `CreateWaitableTimerW` comum + `timeBeginPeriod(1)` carregado dinamicamente (`LoadLibraryA("winmm.dll")` + `GetProcAddress`), assim `build.bat` continua linkando só `user32.lib gdi32.lib`. Com isso a medida deu 16 ms/frame → FPS 59/60 em todos os tamanhos de janela, e CPU do processo caiu de ~86% para ~2% de um núcleo.
   Novas funções: `PacingSetup`, `PacingWait`, `PacingShutdown`; corpo do frame movido para `RunOneFrame`, laço de pacing em `WinMain` mede `QueryPerformanceCounter` antes/depois e espera o resto do orçamento.
4. **Bloco de análise movido para o fim de `main.cpp`** (abaixo de `WinMain`), com declarações antecipadas `DumpBegin`/`DumpTick`; o código do engine no laço ficou limpo.
5. **Cabeçalho BMP do dump corrigido**: os campos estavam escritos em offsets errados (lido antes: `biPlanes=24`, `biBitCount=0`, `biCompression` lixo). Agora `biPlanes` em 12 (=1), `biBitCount` em 14 (=24), `biCompression` em 16 (=0), `biSizeImage` em 20, xppm 24, yppm 28, clrUsed 32, clrImportant 36. Os BMPs agora são legíveis por leitores padrão (24bpp).

Ainda em aberto da auditoria (não corrigidos, são território de outros passos): `WM_PAINT` não tratado; ESC via `GetAsyncKeyState` (global, fecha mesmo sem foco) — o Step 4 trata o foco; desvio do vermelho `red = x*255/(SCREEN_W-1)`.

Estado do build após as correções: compila sem erros nem warnings; `WM_CLOSE` limpo (exit code 0); título `FPS: 59 | SQRT2: 1414 | SIN: 1000 | COS: -1000 | VIEWZ: 5000 | PROJW: 5000`.

## Decisões registradas
- Arquivos do projeto ficam em `code\` (mesmo lugar de `win32_handmade.cpp`); saída em `build\` (gitignored).
- Este registro fica em `ai-docs\progresso.md`.
- O bloco de dump fica no código (fim de `main.cpp`), ativado só por `--dump`, e os PNGs são mostrados no Review pane para você.
- Rodar `--dump` a partir da raiz do repo; converter BMP→PNG com `C:\Users\lucas\AppData\Local\Temp\opencode\bmp2png.ps1`.
- Sem git por enquanto: nada de `git commit` / `git checkout .` até você decidir.
- Loop agora tem pacing em 60 fps (`TARGET_FPS`), com `timeBeginPeriod(1)` via `winmm.dll` carregado dinamicamente.
- Você está dormindo: trabalhar sozinho, um passo por vez, sem perguntar nada, sem parar (ver `ai-docs\prompt.md` e `ai-docs\INSTRUCTIONS.txt`).

## Step 4 — Input (`base.cpp`, `main.cpp`) — APROVADO (checks passaram)
`base.cpp` (adicionado após `M4View`):
- `struct Input { bool w,a,s,d,up,down,left,right,space,ctrl,shift,escape,lmb,rmb, lmbPressed,rmbPressed, f3,f3Pressed,f5Pressed,t,f,fPressed; bool keyNum[5]; i32 mouseDX, mouseDY; }` — exatamente os campos do spec
- `Input In; bool MouseCaptured = false;` + estáticos do estado anterior: `PrevLmb, PrevRmb, PrevF3, PrevF5, PrevF` e `PrevEscape` (o spec pede estáticos para lmb/rmb/f3/f5/f; o `PrevEscape` é o "static previous-state" pedido para o edge do ESC)
- `void PollInput()`:
  - `GetForegroundWindow() != GlobalWindow` → todos os campos `false`, `mouseDX/mouseDY = 0`, retorna
  - campos preenchidos com `GetAsyncKeyState(vk) & 0x8000`: `'W','A','S','D'`, `VK_UP`, `VK_DOWN`, `VK_LEFT`, `VK_RIGHT`, `VK_SPACE`, `VK_CONTROL`, `VK_SHIFT`, `VK_ESCAPE`, `VK_LBUTTON`, `VK_RBUTTON`, `'1'..'5'` em `keyNum[0..4]`, `VK_F3`, `VK_F5`, `'T'`, `'F'`
  - campos `*Pressed` verdadeiros só no frame em que a tecla passou de solta para pressionada (edge)
  - mouse: se capturado, centro do client em coordenadas de tela (`GetClientRect` + `ClientToScreen`), `GetCursorPos`, `mouseDX/DY = cursor - centro`, depois `SetCursorPos(centro)`; senão `0`
  - não capturado + `lmbPressed` → `MouseCaptured = true; ShowCursor(FALSE)`
  - capturado + edge do ESC → `MouseCaptured = false; ShowCursor(TRUE)`

`main.cpp`:
- `PollInput()` no início de `RunOneFrame`, antes de desenhar; `wasCaptured` é lido **antes** de `PollInput` poder liberar o mouse
- ESC não fecha direto: `escapePressedForClose` (edge via `PrevEscapeForClose`) fecha só quando `!wasCaptured`; quando capturado o ESC só solta o cursor
- `ShowMathTest()` do Step 3 removida (os slots 1 e 2 agora são do Step 4)
- teste temporário `UpdateAndDrawTestSquare()`: quadrado branco 10x10 desenhado com laços `PutPixel`, andando com WASD a `100.0f * Dt` px/s; `DebugSet(1,"MOUSEX",In.mouseDX)` e `DebugSet(2,"MOUSEY",In.mouseDY)`

Medições da auditoria (modo `--dump`, janela em foreground, tecla seguradas entre os dumps de 0,5 s e 1,0 s):
- sem tecla: quadro 1 e quadro 2 idênticos (dx=0, dy=0) — a posição não deriva sozinha
- `W`: dy = −51 px (esperado −50); `D`: dx = +50; `S`: dy = +51; `A`: dx = −52 → 100 px/s em baixa resolução nos 4 sentidos
- quadrado: exatamente 100 pixels brancos, 10x10, origem em (150, 85)
- `dump_backbuffer.bmp` mostra o quadrado branco 10x10 sobre o gradiente, sem artefatos
- captura do mouse (run vivo): `GetCursorInfo` flags = **0 (cursor oculto)** após o clique; `SetCursorPos` para `centro+15,+8` é desfeita pelo app (posição volta ao centro a cada frame); título mostra `MOUSEX: 15 | MOUSEY: 8` nos frames em que houve movimento desde o último poll, `0` quando não houve
- 1º ESC: `GetCursorInfo` flags = **1 (visível)**, janela continua viva, `MOUSEX/MOUSEY` voltam a 0, cursor fica livre (movido para `centro+120,−90` e permanece lá)
- 2º ESC: fecha, exit code 0
- janela sem foco: teclas e mouse são ignorados (`GetAsyncKeyState` não vazava para o app)

Compila sem erros nem warnings. Ferramentas de auditoria usadas: `step4_audit7.ps1` (movimento nos dumps), `step4_audit9.ps1` (capture/cursor/ESC), `bmp2png.ps1`. Nota: PowerShell 5.1 não aceita `[type=...]` nem arrays em struct via `Add-Type`; `CURSORINFO.cbSize` precisa ser 24 (x64) e `BITMAPINFOHEADER` usa `ushort` para planes/bitcount (senão `CreateDIBSection` falha).

## Step 5 — Triângulo 2D (`raster.cpp` novo, `main.cpp`) — APROVADO (checks passaram)
`raster.cpp` (novo, incluído em `main.cpp` logo depois de `base.cpp`):
- `#undef DrawState` no topo: `windows.h` define `DrawState` como macro da API de desenho de botão (`DrawStateA/W`), que colide com o nome que o spec pede para a struct. Não usamos essa API, então a macro é descartada e os nomes do spec (`DrawState`, `DS`) ficam intactos. Sem novos `#include`.
- `struct ScreenVert { f32 x, y; }`
- `f32 EdgeFunction(ScreenVert a, ScreenVert b, f32 px, f32 py) = (px-a.x)*(b.y-a.y) - (py-a.y)*(b.x-a.x)`
- `struct DrawState { u32 FlatColor; }; DrawState DS;`
- `void RasterTriangle(a,b,c)`: `area = EdgeFunction(a,b,c.x,c.y)`; `area == 0` retorna; `area < 0` troca b e c e recalcula (fica positivo); bounding box dos 3 vértices em float; clamp dos floats em `[0, SCREEN_W-1] x [0, SCREEN_H-1]` **antes** de `FloorI`; laço com `px = x+0.5f`, `py = y+0.5f`, `w0 = Edge(b,c)`, `w1 = Edge(c,a)`, `w2 = Edge(a,b)`, `>= 0` nos três → `BackPixels[y*SCREEN_W + x] = DS.FlatColor`

`main.cpp`: `#include "raster.cpp"` após `base.cpp`; `DrawTestTriangles()` depois do gradiente e antes do quadrado do Step 4 (o quadrado continua, é o teste do Step 4): triângulo horário (40,30)-(80,90)-(120,30) vermelho; o mesmo formato espelhado horizontalmente (anti-horário, area negativa) verde, 80x60 como o primeiro; um com x negativo (fora da borda esquerda) azul; um com y > 179 (fora da borda inferior) magenta.

Correção achada na auditoria: na primeira versão o triângulo "espelhado" era base 40 x altura 60 (1200 px de área), **não** o mesmo formato do primeiro. Trocado para (200,30)-(280,30)-(240,90), base 80 e altura 60, igual ao primeiro triângulo espelhado — mesmo número de pixels por simetria.

Auditoria do Step 5 (feita por mim, lendo o `dump_backbuffer.bmp` que o próprio engine grava):
- os 4 contornos aparecem preenchidos, bordas limpas (sem pixels espurhos), winding horário e anti-horário funcionam
- azul cortado na coluna 0 (41 px na borda), magenta cortado na última linha (30 px), nenhum pixel escrito fora de `BackPixels`
- verificação pixel a pixel contra o algoritmo do spec reimplementado como ferramenta descartável: 2400 px esperados e encontrados para o triângulo de área 2400; 0 pixels com cor errada; 0 pixels da cor fora da máscara
- o quadrado branco 10x10 do Step 4 continua em (150,85) com 100 px

Compila sem erros nem warnings. Nota de ferramenta: para ler a imagem eu converto o BMP do dump para PNG (`bmp2png.ps1`) e leio o PNG; nada disso faz parte do código do projeto.

## Step 6 — Depth buffer (`raster.cpp`, `main.cpp`) — APROVADO (check passou)
`raster.cpp`:
- `ScreenVert` passou a `{ f32 x, y, invW; }`
- `static f32 Depth[SCREEN_W * SCREEN_H];` + `void ClearDepth()` (laço zerando tudo, `Depth[i] = 0.0f`)
- `u32 ShadePixel(f32 invW) { return DS.FlatColor; }` (vai crescer nos passos seguintes)
- dentro do laço de `RasterTriangle`, quando o pixel está dentro das 3 edges: `index = y*SCREEN_W + x`, `l0 = w0/area`, `l1 = w1/area`, `l2 = w2/area`, `invW = l0*a.invW + l1*b.invW + l2*c.invW`, `if (invW > Depth[index]) { Depth[index] = invW; BackPixels[index] = ShadePixel(invW); }`
- a troca de `b` e `c` quando `area < 0` troca os vértices inteiros, então o `invW` viaja junto — correto

`main.cpp`:
- `ClearDepth()` cada frame, depois do gradiente e antes dos triângulos
- teste do Step 6 substitui o teste de 4 triângulos do Step 5: triângulo **A** vermelho com os três vértices `invW = 0.2f` (100,40)-(220,40)-(160,120); triângulo **B** azul com a mesma forma rotada 0.35 rad em torno do centroide (160,80) de A, `invW = 0.1f` num vértice e `0.4f` nos outros; ordem normal = B depois de A; com **SPACE** pressionado = A depois de B (usando `In.space`)
- `RotateScreenVert()` explícito com `Sin`/`Cos` de `base.cpp`, preserva `invW`
- o quadrado branco do Step 4 continua (desenhado com `PutPixel`, não passa pelo depth buffer)

Check: as duas imagens (dump sem SPACE e dump com SPACE segurado, janela em foreground) são **idênticas** — o depth buffer torna a ordem irrelevante, porque `invW > Depth` é stricto e o campo de `invW` de B varia de 0.1 (atrás de A, `0.2`) a 0.4 (na frente de A). Visível: azul cobre o centro onde B é mais perto, vermelho aparece na faixa perto do vértice fundo de B, com uma linha diagonal onde B atravessa A. Compila sem erros nem warnings; FPS continua ~60.

Ferramenta usada para ver as imagens: só o dump do próprio engine (`build\dump_backbuffer.bmp`, `build\dump_space_*.bmp`) convertido para PNG para eu ler; o script de segurar SPACE apenas dirige input (nada de auditoria por script reimplementando o algoritmo).

## Próximo passo
Step 7 — Triângulo 3D (`raster.cpp`, `main.cpp`): `Camera { v3 pos; f32 yaw, pitch; }`, `Cam = { {0,1,-6}, 0, 0 }`, `m4 GViewProj`, `SetupCamera()` com `M4Perspective(1.2217f, SCREEN_W/SCREEN_H)` × `M4View`, `ProjectToScreen(v4 clip)` (invW = 1/clip.w, NDC → screen), `DrawTriangle3D(v3 a,b,c, u32 color)` (transforma os 3 pontos, retorna se qualquer `w < NEAR_W`, sets `DS.FlatColor`, rasteriza). Em `main.cpp`: remover o código de teste 2D antigo; setas mudam `Cam.yaw`/`Cam.pitch` a 1.5 rad/s × `Dt` (pitch clamp ±1.5); cada frame `SetupCamera()`, `ClearScreen(MakeColor(120,170,230))`, `ClearDepth()`, dois `DrawTriangle3D` (laranja e ciano); `Cam.yaw*1000` no slot 1 do título. Check: dois triângulos visíveis; seta direita gira a câmera para a direita (objetos deslizam para a esquerda).
