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

## Decisões registradas
- Arquivos do projeto ficam em `code\` (mesmo lugar de `win32_handmade.cpp`); saída em `build\` (gitignored).
- Este registro fica em `ai-docs\progresso.md`.
- O bloco de dump fica no código, ativado só por `--dump`, e os PNGs são mostrados no Review pane para você.
- Sem git por enquanto: nada de `git commit` / `git checkout .` até você decidir.

## Próximo passo quando você pedir
Step 3 — biblioteca de matemática em `base.cpp` (`PI`, `TAU`, `NEAR_W`, `Abs`, `Min`, `Max`, `Clamp`, `Lerp`, `Floor`, `FloorI`, `FloorDiv`, `Mod`, `Sqrt` via Newton, `Sin`/`Cos` por série, `MemZero`, `MemCopy`, `v3`/`v4`/`m4` e operações, `M4Perspective`, `M4View`) + teste temporário nos slots 1..5 da linha de título. Check esperado: `SQRT2: 1414 | SIN: 1000 | COS: -1000 | VIEWZ: 5000 | PROJW: 5000`.
