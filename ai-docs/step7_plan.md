# Step 7 — Triângulo 3D (plano, ainda não implementado)

Entrada: `progresso.md` (Steps 1–6 + 6.1 aprovados) e `qwen_prompt_pack.md` (Step 7, seção B).
Arquivos que o passo nomeia: **`raster.cpp`** (código novo) e **`main.cpp`** (testes).
`base.cpp`, `build.bat`, `win32_handmade.cpp`: **intactos**. Nenhum `#include` novo.

---

## 1. O que o spec pede (literal, `qwen_prompt_pack.md` linhas 209–221)

- `raster.cpp`: `struct Camera { v3 pos; f32 yaw; f32 pitch; }`, `Camera Cam = { {0,1,-6}, 0, 0 }`, `m4 GViewProj`,
  `SetupCamera()` = `M4Mul(M4Perspective(1.2217f, SCREEN_W/SCREEN_H), M4View(Cam.pos, Cam.yaw, Cam.pitch))`,
  `ProjectToScreen(v4 clip)` (invW = 1/clip.w, NDC → screen), `DrawTriangle3D(v3 a,b,c, u32 color)`
  (transforma os 3, **retorna se qualquer w < NEAR_W**, sets `DS.FlatColor`, rasteriza).
- `main.cpp`: remover o código de teste 2D antigo; setas mudam `Cam.yaw`/`Cam.pitch` a `1.5 rad/s * Dt`
  (pitch clamp ±1.5); por frame: `SetupCamera()`, `ClearScreen(MakeColor(120,170,230))`, `ClearDepth()`,
  `DrawTriangle3D((-2,0,0),(2,0,0),(0,3,0), laranja)` e `DrawTriangle3D((-1,0,-1),(1,0,1),(0,2,0), ciano)`;
  `Cam.yaw*1000` no slot 1 do título.
- Check: dois triângulos visíveis; seta direita gira a câmera para a direita (objetos deslizam para a esquerda).

---

## 2. Remoções em `main.cpp` (tudo é código de teste de passos anteriores)

| Item | Onde está hoje | Motivo |
|---|---|---|
| `FillGradient()` (linhas 104–115) e `FrameCounter` (linha 10) | teste do Step 1 | o fundo passa a ser `ClearScreen(MakeColor(120,170,230))` |
| `SquareX`, `SquareY`, `UpdateAndDrawTestSquare()` (linhas 117–144) | teste do Step 4 | o Step 8 move a câmera com WASD; o quadrado não tem mais função |
| `RotateScreenVert()`, `DrawTestTriangles()` (linhas 146–192) | teste do Step 6 | o Step 7 é o teste novo; o depth buffer continua exercido pelos triângulos 3D |

**Efeito colateral obrigatório:** os slots 1 e 2 do título foram usados pelo teste do Step 4
(`MOUSEX`/`MOUSEY`). `DebugSlots[i].label` continua não-zero depois de remover o teste, então o título
mostraria `MOUSEX: 0` para sempre. Correção mínima: `DebugSet(2, 0, 0)` uma vez em `WinMain`
(depois de `GlobalWindow = MainWindow`), que apaga o label e faz `DebugFlush` ignorar o slot.

---

## 3. `raster.cpp` — adicionar no fim do arquivo (depois de `RasterTriangle`)

Ordem correta: `ScreenVert`, `DS`, `RasterTriangle` já existem acima; `base.cpp` já fornece `v3`, `v4`, `m4`,
`M4Perspective`, `M4View`, `M4Mul`, `M4MulV4`, `NEAR_W`.

```cpp
// --- Camera and 3D projection (step 7) ---
struct Camera
{
    v3 pos;
    f32 yaw;
    f32 pitch;
};

Camera Cam = { {0.0f, 1.0f, -6.0f}, 0.0f, 0.0f };
m4 GViewProj;

void SetupCamera()
{
    m4 projection = M4Perspective(1.2217f, (f32)SCREEN_W / (f32)SCREEN_H);
    m4 view = M4View(Cam.pos, Cam.yaw, Cam.pitch);
    GViewProj = M4Mul(projection, view);
}

ScreenVert ProjectToScreen(v4 clip)
{
    f32 invW = 1.0f / clip.w;
    f32 ndcX = clip.x * invW;
    f32 ndcY = clip.y * invW;

    ScreenVert result;
    result.x = (ndcX * 0.5f + 0.5f) * (f32)SCREEN_W;
    result.y = (0.5f - ndcY * 0.5f) * (f32)SCREEN_H;
    result.invW = invW;
    return result;
}

void DrawTriangle3D(v3 a, v3 b, v3 c, u32 color)
{
    v4 clipA = M4MulV4(GViewProj, V4(a.x, a.y, a.z, 1.0f));
    v4 clipB = M4MulV4(GViewProj, V4(b.x, b.y, b.z, 1.0f));
    v4 clipC = M4MulV4(GViewProj, V4(c.x, c.y, c.z, 1.0f));

    // Any vertex behind the near plane rejects the whole triangle. Real clipping is step 9,
    // so here the scene stays entirely in front of the camera.
    if (clipA.w < NEAR_W || clipB.w < NEAR_W || clipC.w < NEAR_W) return;

    DS.FlatColor = color;
    RasterTriangle(ProjectToScreen(clipA), ProjectToScreen(clipB), ProjectToScreen(clipC));
}
```

`ProjectToScreen` usa exatamente a fórmula das CONVENTIONS (`sx = (ndcX*0.5+0.5)*SCREEN_W`,
`sy = (0.5-ndcY*0.5)*SCREEN_H`). O bounding box já existente em `RasterTriangle` faz o clip das partes
fora da tela; o depth test (`invW > Depth[idx]`) já existe, então a ordem de desenho não importa.

---

## 4. `main.cpp` — código novo

Depois de `PresentBackbuffer` (ou no lugar do bloco de testes removido):

```cpp
// --- Step 7: camera turning and the 3D test scene ---
const f32 CAMERA_TURN_SPEED = 1.5f; // radians per second

void UpdateCameraWithArrowKeys()
{
    if (In.left) Cam.yaw -= CAMERA_TURN_SPEED * Dt;
    if (In.right) Cam.yaw += CAMERA_TURN_SPEED * Dt;
    if (In.up) Cam.pitch += CAMERA_TURN_SPEED * Dt;
    if (In.down) Cam.pitch -= CAMERA_TURN_SPEED * Dt;

    Cam.pitch = Clamp(Cam.pitch, -1.5f, 1.5f);

    DebugSet(1, "YAW", i32(Cam.yaw * 1000.0f));
}

void DrawTestTriangles3D()
{
    DrawTriangle3D(V3(-2.0f, 0.0f, 0.0f), V3(2.0f, 0.0f, 0.0f), V3(0.0f, 3.0f, 0.0f),
                   MakeColor(255, 160, 0));
    DrawTriangle3D(V3(-1.0f, 0.0f, -1.0f), V3(1.0f, 0.0f, 1.0f), V3(0.0f, 2.0f, 0.0f),
                   MakeColor(0, 200, 200));
}
```

Em `RunOneFrame`:
- `UpdateCameraWithArrowKeys()` entra **logo depois do bloco do ESC** e antes do `FlushTimer`, para o
  `YAW` do título ser do frame atual (e continuar vivo com a janela minimizada);
- o bloco de desenho (depois da guarda `IsIconic`) vira:

```cpp
    SetupCamera();
    ClearScreen(MakeColor(120, 170, 230));
    ClearDepth();
    DrawTestTriangles3D();
    PresentBackbuffer(MainWindow);
```

- em `WinMain`, depois de `GlobalWindow = MainWindow`: `DebugSet(2, 0, 0);` (limpa o label sobrando).

Slots do título depois do passo: `0 FPS`, `1 YAW`, `7 US`; 2–6 vazios. O `US` sai só no Step 35.

---

## 5. Matemática prevista (para a auditoria ter números esperados)

`fovY = 1.2217` rad → `f = cot(0.61085) ≈ 1.4176`; `aspect = 320/180 = 1.7778`; `m[0][0] ≈ 0.7974`.
Câmera `(0,1,-6)`, yaw 0, pitch 0 → view: `x = p.x`, `y = p.y - 1`, `z = p.z + 6`.

**Cena inicial (yaw 0, pitch 0)**

| Triângulo | vértices projetados | bounding box esperado |
|---|---|---|
| laranja `0xFFA000` | (117.5, 111.2), (202.5, 111.2), (160, 47.5) | x 117–202, y 47–111 |
| ciano `0x00C8C8` | (134.5, 115.5), (196.5, 115.5), (160, 68.7) | x 134–196, y 68–115 |

Áreas projetadas: laranja ≈ 2706 px, ciano ≈ 1451 px; interseção ≈ 1197 px.
Profundidade: laranja tem `invW = 1/6 = 0.1667` constante; ciano varia `0.1428 → 0.2` (vértices
`z = 5, 7, 6`). A linha onde `invW` do ciano cruza 0.1667 vai de ~(160, 69) a ~(171, 115): **ciano vence à
esquerda dessa linha, laranja à direita** (diagonal visível, como no Step 6).

Contagens esperadas (tolerância ±6%): sky `0x78AAE6` ≈ 169 840 px, laranja visível ≈ 1903 px,
ciano visível ≈ 1057 px. Paleta da tela = paleta do backbuffer (3 cores, sem interpolação).

**Seta direita segurada ~0,5 s (`yaw = 0.75`)**: laranja passa para x −73 (clip na borda 0) … 102, y 32–132;
ciano sai pela esquerda; `YAW` do título ≈ +750 por 0,5 s, ≈ +1500 por 1,0 s. Seta esquerda é o espelho
(maxX encosta em 319). Seta cima (`pitch +=`) empurra os objetos para baixo na tela (ápice laranja
47 → ~164 em pitch 0,75); seta baixo é o inverso.

---

## 6. Verificação (protocolo do fluxo atual)

1. **Build**: `cd C:\github\reunion-engine\code` → `build.bat` (debug) e `build.bat release`. Esperado:
   0 erros, 0 warnings (`/W3`), exit code 0.
2. **Cena parada**: `build\mc.exe --dump` a partir da raiz do repo → `build\dump_backbuffer.bmp` +
   `build\dump_screen.bmp`; converter com `C:\Users\lucas\AppData\Local\Temp\opencode\bmp2png.ps1`,
   ler os PNGs e mostrar no Review pane. Checar: as 3 cores, as bounding boxes da tabela, a diagonal do
   depth test, nenhuma cor fora das 3, `title: FPS: ~59 | YAW: 0 | US: <n>`.
3. **Setas**: script novo `step7_audit.ps1` no mesmo molde de `step4_audit7.ps1` (`keybd_event` segurado
   entre os dumps de 0,5 s e 1,0 s, `SetForegroundWindow`, cópia de cada BMP entre os ticks). Função
   `ColorBBox(path, r, g, b)` medindo bbox e contagem por cor. Casos: `none` (quadros idênticos),
   `VK_RIGHT` (0x27, bbox dos objetos desloca para a esquerda, `YAW` cresce), `VK_LEFT` (0x25, espelho),
   `VK_UP` (0x26, objetos descem na tela), `VK_DOWN` (0x28, objetos sobem).
   Ler `build\dump_info.txt` para o `YAW` de cada quadro.
4. **Clamp do pitch**: `Cam.pitch` não aparece no título (o spec só pede YAW). Para medir o clamp sem
   mudar o código final, adicionar temporariamente `DebugSet(2, "PITCH", i32(Cam.pitch * 1000.0f))`
   durante a auditoria (segurando DOWN ~1,2 s → deve parar em 1500) e remover depois; o slot 2 fica
   limpo de qualquer jeito. Anotar no registro que foi ferramenta de análise, não código do engine.
   Geometria: com pitch ≥ ~1,4 o triângulo laranja é rejeitado inteiro (`w < NEAR_W`) — esperado, é o
   buraco que o Step 9 fecha.
5. **Regressão**: clique captura o cursor; 1º ESC devolve o cursor e a janela continua viva; 2º ESC fecha
   com exit code 0 (`WM_DESTROY` roda); `WM_PAINT` repinta com a cena nova; minimizar/restaurar não deixa
   o client branco; FPS ~59/60 com cap e `US` ~0,7–0,9 ms (`--nocap` em build release mede o custo real).

---

## 7. Riscos e coisas que ficam explícitas

- **Rejeição por vértice** (`w < NEAR_W`) é a regra do Step 7 de propósito; o clip Sutherland-Hodgman só
  chega no Step 9. Se um triângulo encostar no plano near, ele desaparece inteiro — não corrigir agora.
- **Yaw não é enrolado**: `Sin`/`Cos` já fazem o wrap internamente; `YAW * 1000` só ultrapassa `i32` depois
  de ~1,4 milhões de segundos de giro contínuo. Não adicionar wrap (fora do passo).
- **Não mexer** em `RasterTriangle`, `ScreenVert`, `Depth`, `ShadePixel`, `#undef DrawState`: o Step 7 só
  adiciona por cima.
- **`API.txt` não existe** em `ai-docs` hoje. Depois do check passar, rodar o prompt de API do pack sobre
  `base.cpp` + `raster.cpp` + `main.cpp` e salvar `ai-docs\api.txt`, para o Step 8 entrar com contexto pequeno.
- **Sem git**: Step 7 não tem snapshot. Se der errado, o jeito é editar de volta manualmente (decisão sua).
- Nomes e literais do spec preservados: `Camera`, `Cam`, `GViewProj`, `SetupCamera`, `ProjectToScreen`,
  `DrawTriangle3D`, `DS.FlatColor`, `1.2217f`, `NEAR_W`, `1.5f`, clamp ±1.5.

---

## 8. Critério de aprovação

Check do pack: **dois triângulos visíveis; seta direita gira a câmera para a direita (objetos deslizam
para a esquerda)**. Se passar, escrevo a entrada do Step 7 em `progresso.md` (código, correções, medições)
e atualizo `ai-docs\api.txt`; o "Próximo passo" passa a ser Step 8 (`player.cpp`, câmera free-fly, slots 1–3).

DONE
