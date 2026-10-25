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
  (os BMPs não abrem direto para você; converto para PNG e mostro no Review pane; o script usa
  `param($BmpPath, $PngPath)` posicional e já considera o padding de 4 bytes por linha)
- Build: rodar `build.bat` **de dentro de `code\`** (`code> build.bat`). Ele chama o vcvarsall x64
  e compila `..\code\main.cpp` → `..\build\mc.exe` (`cl -FC -Zi -Od -W3 ... user32.lib gdi32.lib`).
  `code> build.bat release` compila com `/O2 /fp:fast` (benchmark); o padrão continua debug.
- `mc.exe --nocap` desliga o cap de 60 fps (só para medir; o jogo normal roda com cap).

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

Ainda em aberto da auditoria (não corrigidos, são território de outros passos): `WM_PAINT` não tratado (corrigido depois no Step 6.1); ESC via `GetAsyncKeyState` (global, fecha mesmo sem foco) — o Step 4 trata o foco; desvio do vermelho `red = x*255/(SCREEN_W-1)`.

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

## Step 6.1 — Auditoria (pedido seu) — code review dos Steps 1–6 + correções aplicadas

Pergunta: os Steps 1–6 estão bons? O que refazer? Serve para um modelo local numa máquina de jogo?

Veredito: **~8/10, nada precisa ser reescrito**. O que estava errado era periférico (janela, ferramenta de dump, build), não o núcleo (matemática, input, rasterização, depth buffer). Para modelo local rodando a ~30 tok/s o resultado é bom: o código segue as convenções (sem STL/CRT, tipos `u8/u16/u32/i32/f32`, literais `f`, structs/funções globais), compila limpo e os checks dos Steps 1–6 passaram. Os erros que apareceram foram todos "de borda de API do Win32" (parâmetros de `StretchDIBits`, offsets de BMP, `DrawState` como macro), justamente o que um modelo pequeno erra e que uma auditoria pega.

Problemas encontrados e o que cada um custaria depois:
1. **ESC não passava por `WM_DESTROY`**: o laço fechava com `PostMessageA(WM_CLOSE)` + `Running = FALSE` e saía direto. Quando o Step 35 salvar o mundo no fechamento, ESC não salvaria — bug silencioso e caro de achar. Agora ESC chama `DestroyWindow` e `RunOneFrame` retorna na hora.
2. **O cap de 60 fps escondia a performance real**: as checagens dos Steps 14/19/34 (muitos chunks, muitas luzes) iam parecer sempre "60 FPS". Agora existe `--nocap` (benchmark) e o título mostra o custo medido por frame em microssegundos.
3. **Mouse continuava capturado no Alt+Tab**: cursor ficava invisível e preso no centro do jogo até clicar de volta. Agora libera em `WM_ACTIVATEAPP`, quando `PollInput` vê a janela sem foco e em `WM_DESTROY`.
4. **`WM_PAINT`/`WM_ERASEBKGND` não tratados**: minimizar/restaurar ou arrastar outra janela por cima podia deixar o client em branco até o próximo frame; o Windows também apagava o fundo antes de cada paint. Agora `WM_PAINT` faz `BeginPaint` + `PresentBackbuffer`, e `WM_ERASEBKGND` devolve 1 (o backbuffer cobre tudo).
5. **Janela minimizada continuava renderizando** (trabalho jogado fora). Agora `IsIconic` pula só o desenho; mensagens, input, `Dt` e pacing continuam rodando.
6. **Ferramenta de dump**: o BMP ignorava o padding de 4 bytes por linha (quebrava em qualquer largura que não fosse múltipla de 4) e o bitmap da captura era deletado ainda selecionado no DC (e vazava o DC quando `CreateDIBSection` falhava). Corrigidos: `RowPitch` com padding, `SelectObject` de volta no bitmap original antes do `DeleteObject`, DC liberado no caminho de falha, `SaveBmpFromU32` recusa tamanho acima de `BMP_MAX_*`.
7. **Não havia build release**: `build.bat release` agora usa `/O2 /fp:fast` (o debug fica `/Od /Zi`), e o `build.bat` devolve o exit code real do `cl` (antes devolvia sempre 0).
8. **Armadilha do Visual Studio**: `code\handmade.vcxproj` compila o `win32_handmade.cpp` antigo, não o `main.cpp`. Se você apertar F5 no VS, roda o projeto velho. Build oficial é `code> build.bat`.
9. **Sem git**: todo o trabalho está sem snapshot. Se algo der errado num passo futuro não existe `git checkout`. (Não criei repo; fica sua decisão.)
10. **Rasterizador**: 3 chamadas de edge + 3 divisões por pixel, em `/Od`. É o gargalo natural dos Steps 14/19, mas otimizar agora seria refatorar antes da hora; fica para quando o profiling pedir (`invArea` + edges incrementais).

Correções aplicadas (`base.cpp`, `main.cpp`, `build.bat`, nada de arquivo novo, nenhum `#include` novo):
- `base.cpp`: `ReleaseMouseCapture()` (um único lugar que devolve o `ShowCursor(TRUE)` correspondente ao `FALSE` da captura); `PollInput` libera o mouse e zera os `Prev*` quando a janela perde o foco; o edge do ESC usa o helper.
- `main.cpp`: `WM_ERASEBKGND`, `WM_PAINT`, `WM_ACTIVATEAPP`; `WM_DESTROY` libera mouse + DC + `PostQuitMessage`; `PresentBackbuffer` ignora `WM_PAINT` que chega antes do `BITMAPINFO` estar preenchido; `RunOneFrame` mede o trabalho com `QueryPerformanceCounter` e publica `US` no slot 7; `--nocap` pula o `PacingWait`; `CommandLineHas()` substitui a busca manual de `--dump`; BMP com padding.
- `build.bat`: modo `release` e exit code correto.

Medições depois das correções (build release):
- `mc.exe --dump`: `FPS: 59 | US: 845` → com cap, o trabalho do frame custa ~0,85 ms (inclui `StretchDIBits`)
- `mc.exe --nocap --dump`: `FPS: 1438 | US: 661` → sem cap o custo cai para ~0,66 ms; o resto do frame com cap é espera
- `dump_backbuffer.bmp`: 320x180, 172.854 bytes (cabeçalho 54 + 320*180*3), 41.428 cores distintas, quadrado branco com exatamente 100 px, vermelho 1200 px, azul 4546 px → imagem do Step 6 inalterada
- captura/ESC: clique captura (cursor flags 0), 1º ESC devolve o cursor e a janela continua viva, 2º ESC fecha com exit code 0; `WM_ACTIVATEAPP(FALSE)` devolve o cursor e ele fica livre (774,315 pedido = 774,315 lido)
- build debug e release: 0 erros, 0 warnings (`/W3`)

Nota para os próximos passos: o slot 7 do título está ocupado por `US`. O Step 8 usa 1–3 (livres) e o Step 35 usa o 7 → nesse passo o `US` sai ou muda de lugar.

## Step 7 — Triângulo 3D (`raster.cpp`, `main.cpp`) — IMPLEMENTADO, checks passaram
Plano: `ai-docs\step7_plan.md`. Spec: `qwen_prompt_pack.md` Step 7.

`raster.cpp` (adicionado no fim do arquivo, nada acima foi tocado):
- `struct Camera { v3 pos; f32 yaw; f32 pitch; }`, `Camera Cam = { {0,1,-6}, 0, 0 }`, `m4 GViewProj`
- `SetupCamera()`: `GViewProj = M4Mul(M4Perspective(1.2217f, (f32)SCREEN_W/(f32)SCREEN_H), M4View(Cam.pos, Cam.yaw, Cam.pitch))`
- `ProjectToScreen(v4 clip)`: `invW = 1/clip.w`, `ndcX = clip.x*invW`, `ndcY = clip.y*invW`,
  `x = (ndcX*0.5+0.5)*SCREEN_W`, `y = (0.5-ndcY*0.5)*SCREEN_H` (fórmula das CONVENTIONS)
- `DrawTriangle3D(v3 a,b,c, u32 color)`: transforma os 3 com `M4MulV4(GViewProj, V4(x,y,z,1))`,
  **retorna se qualquer `w < NEAR_W`** (clip chega no Step 9), sets `DS.FlatColor`, chama `RasterTriangle`
- O bounding box e o depth test do Step 5/6 já cuidam do clip de tela e da ordem de desenho

`main.cpp`:
- Removidos os testes antigos: `FillGradient` + `FrameCounter` (Step 1), `SquareX/SquareY` +
  `UpdateAndDrawTestSquare` (Step 4), `RotateScreenVert` + `DrawTestTriangles` (Step 6)
- Novos: `CAMERA_TURN_SPEED = 1.5f`, `UpdateCameraWithArrowKeys()` (setas: yaw ±, pitch ±,
  `Clamp(pitch, -1.5, 1.5)`, `DebugSet(1,"YAW", Cam.yaw*1000)`) e `DrawTestTriangles3D()`
  (laranja `MakeColor(255,160,0)` em (-2,0,0)(2,0,0)(0,3,0); ciano `MakeColor(0,200,200)` em
  (-1,0,-1)(1,0,1)(0,2,0))
- Ordem do frame: `UpdateCameraWithArrowKeys()` → `SetupCamera()` → `ClearScreen(MakeColor(120,170,230))`
  → `ClearDepth()` → `DrawTestTriangles3D()` → `PresentBackbuffer`
- `DebugSet(2, 0, 0)` em `WinMain`: o teste do Step 4 deixava o label `MOUSEX` sobrando no título
  (`DebugFlush` imprime todo slot com label não-zero). Título final: `FPS | YAW | US` (2..6 vazios)

Correção achada na auditoria (ferramenta, não engine): copiar os dumps com sleep cego lia os arquivos
**do run anterior**. Agora a auditoria espera o `LastWriteTime` ser posterior ao início do processo
(`step7_audit2/3.ps1`). Outro achado: o script usava `Press(VK_MENU)` para conseguir foreground —
Alt+Space abre o **menu de sistema** do Windows, cujo loop modal pausa o laço de frame do app e come
as setas (foi o menu que você viu na tela). Trocado por `AttachThreadInput` + `BringWindowToTop`,
sem Alt. Runs "live" (sem `--dump`) não têm auto-close; o script fecha com `PostMessage(WM_CLOSE)`
(caminho do botão X) e mata processos sobrantes no fim.

Medições (build debug, `--dump` da raiz, frames frescos só):
- cena parada: paleta exatamente 3 cores (`78AAE6` céu, `FFA000` laranja, `00C8C8` ciano),
  laranja 2237 px x 118–201 y 48–110, ciano 556 px x 134–159 y 69–115, céu 54807 px de 57600
  → union 2793 px, como previsto pela projeção; a linha onde o ciano atravessa o laranja é ~x=160
  (ciano vence onde `invW > 1/6`, laranja onde é mais fundo), como no Step 6
- sem tecla: os dois frames do mesmo run são **idênticos** (cena determinística)
- `setas` (`step7_audit3.ps1`, hold curto para a cena não sumir):
  RIGHT yaw 0→+195 → laranja x 118–201 → 88–175 e ciano minX 134→107 (**objetos deslizam para a
  esquerda**, o check do pack); LEFT yaw 0→−247 → laranja x 151–241 (espelho, para a direita);
  UP pitch 0→+223 → laranja y 48–110 → 78–142 (objetos descem); DOWN pitch 0→−222 → y 13–82 (sobem)
- taxa de giro: segurando RIGHT 1,4 s o yaw chegou a +1697 (~1,1 s de tecla → 1,5 rad/s ✓)
- **clamp do pitch**: `DebugSet(2,"PITCH", Cam.pitch*1000)` foi adicionado **só para a auditoria** e
  removido depois; segurando UP 1,4 s o pitch parou exatamente em 1500 e segurando DOWN exatamente em
  −1500, nunca além (`Clamp` funciona)
- captura/ESC intactos (Step 4): clique → cursor flags 0 (oculto), setas continuam girando a câmera
  com o mouse capturado, 1º ESC → flags 1 e janela viva, cursor fica livre, 2º ESC → exit code 0
- extremo de yaw/pitch: com yaw ≳1,2 rad ou pitch ≳1,4 os triângulos **desaparecem** (rejeição por
  vértice `w < NEAR_W`) — é a regra do Step 7 de propósito; o Sutherland-Hodgman do Step 9 conserta
- builds debug e release: 0 erros, 0 warnings (`/W3`); debug `FPS: 59 | US: ~1700`, release `--nocap`
  `FPS: 1943 | US: 492` → a cena 3D custa ~0,5 ms/frame em `/O2`
- nada de `mc.exe` sobrando depois da auditoria; `WM_CLOSE` limpo

Ferramentas de análise usadas (fora do código do engine): `step7_audit2.ps1`, `step7_audit3.ps1`,
`step7_regress.ps1`, `bmp2png.ps1`; imagens convertidas para PNG e mostradas no Review pane.
Nota de PowerShell 5.1: `[ushort]` não existe como accelerator (`[System.UInt16]`, ou campos `ushort`
no struct e atribuição normal); `[type=...]` em attribute falha; `SRCCOPY` é `0x00200021`.
A captura externa do DC deste passo (`BitBlt` do DC da janela lido de fora do processo) voltou preto;
a captura do próprio engine (`dump_screen.bmp`) continua correta — fica como limitação da ferramenta,
sem efeito no engine.

`ai-docs\api.txt` gerado com o prompt de API do pack (assinaturas de `base.cpp`, `raster.cpp`, `main.cpp`).

## Step 8 — Câmera free-fly (`player.cpp` novo, `main.cpp`) — APROVADO (checks passaram)
Plano: `ai-docs\step8_plan.md`. Spec: `qwen_prompt_pack.md` Step 8.

`player.cpp` (novo, incluído no fim da lista de includes de `main.cpp`, depois de `raster.cpp`):
- `f32 MoveSpeed = 10.0f; f32 MouseSens = 0.0025f;` e `const f32 CAMERA_TURN_SPEED = 1.5f;` (a const saiu de
  `main.cpp`, porque a regra das setas agora vive dentro de `UpdateFlyCamera`)
- `UpdateFlyCamera()`:
  - `MouseCaptured` → `Cam.yaw += In.mouseDX*MouseSens; Cam.pitch -= In.mouseDY*MouseSens;` (cursor à direita
    gira para a direita; cursor acima do centro olha para cima), depois `Clamp(Cam.pitch, -1.5f, 1.5f)`
  - setas seguem girando a `CAMERA_TURN_SPEED * Dt` (yaw ±, pitch ±), com o mesmo clamp
  - `forwardFlat = V3(Sin(yaw), 0, Cos(yaw))`, `rightFlat = V3(Cos(yaw), 0, -Sin(yaw))`; W/S ao longo de
    `forwardFlat`, D/A ao longo de `rightFlat`, `Space` soma +Y, `Ctrl` soma −Y
  - `speed = MoveSpeed`, `*= 2.0f` com `Shift`; se `V3Len(wish) > 0` → `Cam.pos += V3Scale(V3Norm(wish), speed*Dt)`
  - `DebugSet(1,"X")`, `DebugSet(2,"Y")`, `DebugSet(3,"Z")` = `Cam.pos` ×1000

`main.cpp`:
- `#include "player.cpp"` no fim da lista; removidos `UpdateCameraWithArrowKeys()` e `const CAMERA_TURN_SPEED`
- `RunOneFrame`: `UpdateFlyCamera()` no lugar das setas antigas (mesma posição: depois do bloco do ESC, antes
  do `FlushTimer`, antes de `SetupCamera()`)
- removido `DebugSet(2, 0, 0)` em `WinMain` (era para limpar o label `MOUSEX` sobrando do Step 4; os slots
  1..3 passaram a ser escritos todo frame)
- título final: `FPS | X | Y | Z | US` (slots 4..6 vazios; `US` sai só no Step 35)

Ferramenta de análise (não é código do engine): durante a auditoria `DebugSet(4,"YAW")` e `DebugSet(5,"PITCH")`
foram adicionados temporariamente em `player.cpp` para medir a direção do mouse e o clamp do pitch, e
**removidos** depois — o código final só tem os slots do spec. Scripts: `step8_audit.ps1` (teclado, mouse,
clamp, regressão), `step8_dump.ps1` (imagens e bounding boxes), `step8_final.ps1`, `step8_nocap.ps1`.

Medições (runs vivos lendo o título a cada 120 ms; segurar 1,4 s = Δ medido sobre a janela de flush de 0,25 s):
- velocidade: W `dZ=+15320` (10,94 unidades/s), S `dZ=-14156` (10,11), D `dX=+15154` (10,82),
  A `dX=-15310` (10,94), Space `dY=+14187` (10,13), Ctrl `dY=-15343` (10,96) → `MoveSpeed` 10 nos 6 sentidos,
  sinais corretos; sem tecla `|d| = 0,00` unidades (a cena não anda sozinha)
- `Shift+W` = 20,20 unidades/s (dobrado ✓); `W+D` = 10,89 unidades/s com `dX=dZ=10779` → diagonal
  normalizada, não 14,1 ✓
- relativo ao olhar: segurando RIGHT 400 ms o yaw ficou 0,784 rad; depois W deu `dX=5406 dZ=5412` →
  `dX/dZ = 1,00 = tan(yaw)` exatamente ✓ (anda na direção que você olha, não no eixo do mundo)
- mouse look (click captura): nudge de 40 px por 400 ms → `YAW 0 → 1300` (0,1 rad por poll = 40×0,0025 ✓),
  pitch 0; nudge vertical −40 → `PITCH 0 → 1300` (+ = olhar para cima ✓); +40 → `PITCH 1300 → 99` (− ✓)
- clamp do pitch: mouse −400 px segurando 600 ms → **máximo exato 1500**, nunca acima; +400 → **mínimo
  exato −1500**; setas em processos novos: UP 2 s → máximo 1500, DOWN 2 s → mínimo −1500
- captura continua viva com WASD/setas: segurando W com o mouse capturado `dX=7465 dZ=2073` com yaw 1,3 rad
  (mesma regra do olhar); RIGHT com o mouse capturado → `YAW 1300 → 2075` (setas seguem funcionando)
- 1º ESC devolve o cursor (flags 1), janela viva, cursor livre e a câmera **para** de girar (yaw stayed 2075);
  2º ESC fecha com exit code 0
- imagens (`--dump`, segurar 200 ms e soltar, cena estável no 2º quadro), comparação com o Step 7
  (laranja x 118–201 y 48–110):

  | quadro | posição no título | laranja na tela |
  |---|---|---|
  | base | `X 0 Y 1000 Z -6000` | x 118–201 y 48–110 (idêntico ao Step 7) |
  | W 200 ms | z −6000 → −3860 | x 94–225 y 24–122 (cresce, objetos mais perto) |
  | S 200 ms | z −6000 → −8041 | x 128–191 y 59–105 (encolhe) |
  | D 200 ms | x 0 → 2012 | minX 118 → 75 (deslizam para a **esquerda**) |
  | A 200 ms | x 0 → −2170 | maxX 201 → 248 (espelho de D) |
  | Space 200 ms | y 1000 → 3019 | y 48–110 → 91–154 (descem na tela) |
  | Ctrl 200 ms | y 1000 → −1169 | y 48–110 → 1–64 (sobem na tela) |
  | mouse right | yaw 0 → 699 | minX 118 → 0, maxX 201 → 108 (deslizam para a esquerda) |
  | mouse up | pitch 0 → 699 | y 48–110 → 142–179 (descem na tela) |

- paleta em todos os quadros: **distinct = 3** (`78AAE6` céu, `FFA000` laranja, `00C8C8` ciano),
  57.600/57.600 px cobertos pelas 3 cores → nada de interpolação, nada de pixel fora da paleta
- builds debug e release: 0 erros, 0 warnings (`/W3`); debug `FPS: 60 | US: 1775`; release `--dump`
  `FPS: 59 | US: 474`; release `--nocap --dump` `FPS: 1873 | US: 506` → o custo da câmera é desprezível
  (Step 7 era US 492 / FPS 1943)
- todos os runs fecham com exit code 0; `lingering mc processes: 0`

## Step 9 — Near-plane clipping (`raster.cpp`, `main.cpp`) — APROVADO (checks passaram)
Spec: `qwen_prompt_pack.md` Step 9. Só `raster.cpp` e `main.cpp` foram editados; nenhum `#include` novo.

`raster.cpp` (depois de `ProjectToScreen`, antes de `DrawTriangle3D`):
- `struct ClipVert { v4 pos; f32 u; f32 v; }` — `u`/`v` ainda não usados, mas o corte precisa
  interpolá-los para o Step 11
- `v4 LerpV4(v4 a, v4 b, f32 t)` — interpola x, y, z, w com `Lerp` de `base.cpp`
- `i32 ClipPolygonNear(ClipVert* in, i32 inCount, ClipVert* out)` — Sutherland-Hodgman contra
  `w = NEAR_W`: `curInside = cur.pos.w >= NEAR_W`; se `curInside` copia `cur`; se os lados diferem,
  `t = (NEAR_W - cur.pos.w) / (next.pos.w - cur.pos.w)`, interpola **todos** os campos
  (`LerpV4` + `Lerp` em `u`/`v`) e depois força `hit.pos.w = NEAR_W` exato; retorna o total
- `void DrawClipTriangle(ClipVert a, ClipVert b, ClipVert c)` — `poly[3]`, `clipped[8]`,
  `count = ClipPolygonNear(poly, 3, clipped)`, `if (count < 3) return`, fan `k = 1..count-2` a
  partir de `clipped[0]` (o polígono fica convexo depois do corte do plano near), cada fatia em
  `ProjectToScreen` + `RasterTriangle` (cor vem de `DS.FlatColor`, depth test igual)
- `DrawTriangle3D`: agora constrói 3 `ClipVert` (`pos = M4MulV4(GViewProj, V4(...,1))`, `u = v = 0`)
  e chama `DrawClipTriangle`; o teste `w < NEAR_W` que descartava o triângulo inteiro saiu

`main.cpp`:
- `DrawTestFloorTriangle()` — `DrawTriangle3D(V3(-50,-1,-50), V3(50,-1,-50), V3(0,-1,100), MakeColor(60,180,60))`
- `RunOneFrame` desenho: `ClearScreen(céu) → ClearDepth() → DrawTestFloorTriangle() → DrawTestTriangles3D()`
  (piso antes dos triângulos de teste, para os triângulos ficarem por cima quando se sobrepõem)

Regressão do Step 7 comprovada pelo próprio dump: com o descarte por vértice do Step 7, o dump base
(cam 0,1,-6) tinha **3 cores** — 2.237 px laranja + 556 px ciano e nenhum verde — porque o piso
verde tinha um vértice atrás da near plane: `(-50,-1,-50)` dá `w = -46` (câmera em z = -6), enquanto
`(0,-1,100)` dá `w = 106`, então o triângulo inteiro era jogado fora. Agora o mesmo quadro tem
**4 cores** e 25.215 px de verde `3CB43C`, com os triângulos de teste idênticos aos do Step 7
(laranja x 118–201 y 48–110 = 2.237 px, ciano x 134–159 y 69–115 = 556 px).

Referência independente em C# (`step9_ref.ps1`, `Add-Type`, dupla precisão): reimplementa
`M4Perspective`/`M4View`/`M4Mul`/`M4MulV4`/`ClipPolygonNear`/`ProjectToScreen`/`RasterTriangle`
com a mesma regra de winding, bounding box clampado e depth por `invW`, e compara o histograma de
cores do dump do engine. Câmera vem do título do dump (X/Y/Z ÷1000), só nos runs sem setas (yaw = pitch = 0):

| run | câmera | engine (verde / laranja / ciano / céu) | referência | diff máx por cor |
|---|---|---|---|---|
| start | (0, 1, -6) | 25215 / 2237 / 556 / 29592 | 25215 / 2237 / 556 / 29592 | 0 px |
| low | (0, -0.812, -6) | 28582 / 2224 / 565 / 26229 | 28582 / 2224 / 565 / 26229 | 0 px |
| plane | (0, -1.149, -6) | 28630 / 0 / 0 / 28970 | 28632 / 0 / 0 / 28968 | 2 px |
| below | (0, -3.178, -6) | 26378 / 0 / 0 / 31222 | 26378 / 0 / 0 / 31222 | 0 px |
| forward_low | (0, -0.488, -2.798) | 28206 / 6414 / 3095 / 19885 | 28204 / 6412 / 3099 / 19885 | 4 px |
| past_objects | (0, -0.49, 0.41) | 28174 / 0 / 0 / 29426 | 28190 / 0 / 0 / 29410 | 16 px |
| below_forward | (0, -2.684, -4.998) | 26880 / 0 / 0 / 30720 | 26910 / 0 / 0 / 30690 | 30 px |
| space_up | (0, 4.204, -6) | 20278 / 2344 / 386 / 34592 | 20280 / 2344 / 386 / 34590 | 2 px |

`other = 0` em todos os runs (nenhum pixel fora das 4 cores da cena). As diferenças de 0–30 px são
de borda: o engine é `f32` com as séries de `Sin`/`Cos` de `base.cpp`, a referência é `double` — nas
posições exatas (start/below) bate 0 px.

Invariantes nos 12 runs `--dump` (`step9_dump2.ps1`, teclas seguradas e **soltes** antes do tick de
dump, +240 ms de settle para o título e o quadro descreverem a mesma câmera):
- piso/scene visível em todos os runs: mínimo 14.696 px não-céu (`look_up`, a câmera olha para cima)
- paleta nunca passa das 4 cores da cena (max distinct = 4); `plane`/`below`/`past_objects`/`edge_on`
  ficam com 2 cores só (céu + verde), sem laranja/ciano quando os triângulos ficam atrás/atrasados
- nenhum pixel fora de 0..319 × 0..179 (0 runs ruins) → nenhum triângulo gigante lixo, nenhum NaN
- todos os 12 runs fecham com exit code 0; foreground em todos os runs; `lingering mc processes: 0`
- sem flicker: quadros settled byte-idênticos quando o segundo tick de dump existe (`start`,
  `below`, `edge_on`, `space_up` = True). O tick de 1,5 s às vezes não grava (`SecondsSinceDump`
  cai logo abaixo de 0,5 e o `WM_CLOSE` do self-close fecha o processo), então nesses runs só há
  1 quadro — reportado explicitamente como `2nd dump tick present: False`
- fly-through vivo (sem `--dump`), Ctrl 350 ms depois Shift+W 2,5 s: `fps min 59 max 60`,
  `us max 6450`, 20 amostras com a câmera abaixo do plano do piso (y < -1000), z de -6000 a 37872,
  16 amostras com z ≥ 0 (passando pelos triângulos de teste) → atravessou o piso e os objetos sem
  travar e sem cair o fps; exit 0
- builds debug e release: 0 erros, 0 warnings (`/W3`); debug `--dump` `FPS: 59–60 | US ~5.700–6.300`
  (Step 8 era US 1775 — o piso verde + clipping custa ~4 ms/frame no debug); release medido no Step 10

Ferramenta de análise (não é código do engine): `step9_dump2.ps1` (runs settled, histograma, bbox,
fly-through), `step9_ref.ps1` (rasterizador de referência em C#), `step9_sizes.ps1`,
`bmp2png.ps1` (bmp → png para o Review pane). O reference C# usa `(r << 16) | (g << 8) | b` com
parênteses completos (PS 5.1 + C# interpreta `==` antes de `|` sem parênteses).

## Step 10 — Cubes with shading (`raster.cpp`, `main.cpp`) — IMPLEMENTADO, auditoria PENDENTE (parado a pedido seu)

Feito:
- Código em `raster.cpp`: `FaceNormal[6]`, `FaceCorner[6][4][3]`, `FaceShade[6]`, `ScaleColor`,
  `DrawQuad`, `DrawBlockFace` (culta por `dot <= 0`), `DrawCube`. Em `main.cpp`: `DrawTestCubes`
  (chão 5x5 verde em x,z 0..4 y=0 + torre marrom em (2,1..3,2)), ordem de frame,
  `Cam.pos = V3(2.5, 3.0, -6.0)` em `WinMain`. Os triângulos do Step 7/9 foram substituídos pela
  cena de cubos.
- `build.bat` debug (`/Zi -Od -W3`) e release (`-O2 -fp:fast`): 0 erros, 0 warnings.
- Paleta exata do engine medida com `shade_probe.cpp` sob as mesmas flags: verde top 60,180,60 /
  side 48,144,48 / end 39,116,39 / bottom 30,90,30; marrom top 120,84,52 / side 96,67,41 /
  end 78,54,33 / bottom 60,42,26 (multiplicação em f32 + truncamento, não arredondamento).
- 11 dumps novos (`step10_dump.ps1`) passam os invariantes: px fora da paleta = 0, buracos de céu
  = 0, bbox dentro da tela, exit 0, distinct máximo 4; abaixo do chão mostra 30,90,30 e não mostra
  o top verde; ordenação de shading top > side/end > bottom confirmada. Fly-through 2.5 s:
  fps min 42, max 60, exit 0.

Faltando (é aqui que parei):
1. **Título dessincronizado do frame dumpado.** `DebugFlush` só roda a cada 0.25 s (`main.cpp`
   linha 279), então num run que segura setas o `dump_info.txt` traz yaw/pitch velhos de até 0.25 s
   enquanto o bmp é do frame atual. Prova: o reference C# bate com o geom do engine exatamente só
   nos runs com camera parado (`front`, `graze_low`, `below_floor`, `close_clip`, `past_cubes`,
   `inside_field` = 0 px de diferença); nos runs em turno a diferença é 1051..4233 px
   (`az_right`, `az_left`, `az_back`, `top_down`) e 16 px em `space_up` (movimento ainda ativo).
   Correção planejada: liberar as teclas antes de ~0.75 s (turn 1.5 rad/s, move 10 m/s,
   sprint 20 m/s) e aceitar só dumps cujo `LastWriteTime` seja ≥ liberação + 260 ms, o que garante
   um `DebugFlush` já com camera parado.
2. `step10_ref.ps1` precisa ser re-executado contra os dumps settled. A regex de `bbox` do parser
   do reference não casa com o formato do texto (só cosmético: geom e cores são lidos certos).
3. PNGs das cenas ainda não mostrados no Review pane (`s10_*.png` já gerados).
4. Entradas de auditoria ainda no código: `DebugSet(4, "YAW")` e `DebugSet(5, "PITCH")` em
   `player.cpp` (linhas 55-58, ANALYSIS ONLY) — remover depois do check como no Step 8, rebuildar
   debug + release e re-medir FPS/US com `--dump` e `--nocap --dump`.
5. Entrada do Step 10 no `progresso.md` com as tabelas finais, e `ai-docs\api.txt` ainda diz
   "Step 9" — regenerar para "Step 10" com as assinaturas novas de `raster.cpp`/`main.cpp`.

## Próximo passo
Termizar a auditoria do Step 10 (itens 1-5 acima), depois Step 11 — UVs e blocos pintados
(`base.cpp`, `raster.cpp`, `main.cpp`): enum `AIR/GRASS/DIRT/WOOD/WATER/LEAVES`;
`ScreenVert { x, y, invW, uw, vw }`; `ProjectToScreen(ClipVert)` com `uw = cv.u*invW`,
`vw = cv.v*invW`; `RasterTriangle` interpola `uw`/`vw` com os mesmos pesos de `invW` e chama
`ShadePixel(invW, u, v)`; `DrawState { FlatColor; Block; Face; Shade }` (`Block == 0` = FlatColor);
`DrawQuadUV` com UVs (0,0)(1,0)(1,1)(0,1); `Hash3`; `PaintBlock(block, face, pu, pv)` com
`noise = Hash3(pu, pv, block*8 + face) & 15`, `mul = 0.92f + noise*0.01f`; `ShadePixel` faz
`px/py = clamp((i32)(u*16), 0, 15)`; `DrawBlockFace(x, y, z, face, i32 block)`; teste em `main.cpp`:
linha de 3 cubos (grass, dirt, grass) + uma face de grama 8x8 em y = -1.

