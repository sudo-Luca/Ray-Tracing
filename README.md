# Ray Tracing Demo — 2D + 3D

A real-time C++17 / SDL2 application combining an interactive 2D optics sandbox with a 3D ray tracer, all in a single ~1800-line source file with no external dependencies beyond SDL2.

---

## Table of Contents

- [Features](#features)
- [Requirements & Build](#requirements--build)
- [Running](#running)
- [2D Mode — Optics Editor](#2d-mode--optics-editor)
  - [Scene objects](#scene-objects)
  - [Keyboard & mouse reference](#keyboard--mouse-reference)
  - [U Menu — shape placement](#u-menu--shape-placement)
  - [Built-in presets](#built-in-presets)
- [3D Mode — Ray Tracer](#3d-mode--ray-tracer)
  - [Keyboard & mouse reference](#keyboard--mouse-reference-1)
- [Project Architecture](#project-architecture)
  - [Global constants](#global-constants)
  - [Colour helpers & bitmap font](#colour-helpers--bitmap-font)
  - [Geometry generators](#geometry-generators)
  - [Scene2D](#scene2d)
  - [Scene3D](#scene3d)
  - [Main loop](#main-loop)
- [Rendering Pipeline — 2D](#rendering-pipeline--2d)
- [Rendering Pipeline — 3D](#rendering-pipeline--3d)
- [Extension Ideas](#extension-ideas)

---

## Features

**2D optics sandbox**
- Point light source emitting 4–720 rays in all directions
- Three cell types on a 40 × 22 grid: Wall, Mirror, Prism
- Free-form continuous segments (walls and mirrors) drawn with a pencil or line tool
- Parametric geometric shapes (arc, ellipse, parabola, rectangle, circle) placed via click-and-drag with real-time preview
- Prism dispersion into 7 spectral rays (violet → red) using a wavelength-to-RGB lookup
- Up to 10 reflection bounces per ray
- Interactive light animation (orbiting light source)
- 6 built-in demonstration presets
- Radial U Menu for mode selection

**3D ray tracer**
- Renders at 640 × 360 and upscales to 1280 × 720
- Sphere and infinite-plane intersection
- Multi-bounce reflections (1–8 configurable bounces)
- Hard shadows via a shadow ray toward a directional sun
- Procedural sky gradient with sun disc
- Checkerboard floor material
- Reflective back wall
- Animated orbiting light orb (emissive sphere)
- Animated ring of 8 coloured spheres
- Tone mapping: Reinhard × 0.9 + gamma 2.2

---

## Requirements & Build

### Dependencies

| Dependency | Version | Notes |
|---|---|---|
| C++ compiler | C++17 | GCC ≥ 7, Clang ≥ 5 |
| SDL2 | 2.0+ | display, events, renderer |

### Linux / macOS

```bash
# Install SDL2
sudo apt install libsdl2-dev        # Debian / Ubuntu
brew install sdl2                   # macOS (Homebrew)

# Build
g++ -O2 -std=c++17 -o raytracer src/main.cpp -lSDL2 -lm
```

### Windows — MSYS2 MinGW UCRT64

This is the primary development environment. The provided `Makefile` targets it directly.

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2

make
# or manually:
g++ -O2 -std=c++17 -o raytracer.exe src/main.cpp \
    -lmingw32 -lSDL2main -lSDL2 -lm -mwindows \
    -static-libgcc -static-libstdc++ \
    -Wl,-Bstatic -lSDL2 -lSDL2main -Wl,-Bdynamic
```

The three DLLs shipped alongside the executable (`SDL2.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`) cover the runtime dependencies for distribution on bare Windows machines.

---

## Running

```bash
./raytracer          # Linux / macOS
raytracer.exe        # Windows
```

The application opens a 1280 × 720 window and starts in **2D mode**.

| Key | Action |
|---|---|
| `Tab` | Toggle between 2D and 3D modes |
| `Esc` | Cancel current action / quit |

---

## 2D Mode — Optics Editor

The 2D mode simulates geometric optics on a 1280 × 720 canvas. A point light source casts *N* rays uniformly distributed in 360°. Each ray is traced until it hits a wall (absorbed), a mirror (reflected), a prism (dispersed into 7 spectral sub-rays), or leaves the screen.

### Scene objects

There are two distinct ways to populate the scene:

**Grid cells** (40 × 22 grid, 32 px per cell)

| Type | Colour | Behaviour |
|---|---|---|
| Wall | Dark blue-grey | Absorbs all rays |
| Mirror | Cyan | Reflects using per-cell orientation angle |
| Prism | Purple | Disperses into 7 spectral rays (±0.175 rad spread) |

**Free segments** (pixel-accurate, stored as a list)

| Type | Colour | Behaviour |
|---|---|---|
| Wall segment | Dark blue-grey | Absorbs all rays |
| Mirror segment | Cyan | Reflects; normal computed from segment direction |

Segments are generated either by the pencil/line tools or by the geometric shape tools. Their normals are automatically computed from the segment direction vector, with correct side-selection at intersection time.

### Keyboard & mouse reference

#### Light source

| Input | Action |
|---|---|
| Left-click + drag | Move the light source |
| `Space` | Toggle light orbit animation |

#### Drawing tools (legacy shortcuts)

| Key | Tool selected |
|---|---|
| `1` | Wall block |
| `2` | Mirror block |
| `3` | Prism block |
| `4` | Arc (places a half-circle arc of mirror segments) |
| `5` | Ellipse (places a full ellipse of mirror segments) |
| `6` | Parabola (places a parabolic mirror) |
| `7` | Straight wall segment |
| `8` | Straight mirror segment |

| Key / Input | Action |
|---|---|
| `D` (hold) + left-click / drag | Place / paint cells at cursor |
| `R` (hold) + mouse left-right | Rotate placement angle interactively |
| Mouse wheel (while `R` held) | Rotate in 5° steps |
| Mouse wheel (normal) | Adjust ray count (4 – 720, step 10) |
| Right-click | Place current tool on empty cell, or erase if occupied |
| `E` | Erase cell / nearby segment under cursor |
| `L` | Toggle freehand pencil (Wall or Mirror depending on tool 7/8) |

#### Other

| Key | Action |
|---|---|
| `U` | Open / close the U Menu (or exit current U mode) |
| `P` | Cycle to the next built-in preset |
| `C` | Clear all cells and segments |
| `I` | Toggle in-app help overlay |

### U Menu — shape placement

Press `U` to open the radial menu. Five branches are arranged in a circle around the screen centre. Hover a branch to reveal its sub-options in a side panel; click a sub-option to activate that mode. Press `U` again at any time to exit the active mode.

```
          BLOCS
         /     \
   LIGNES       LUMIERE
         \     /
       COURBES
          |
        FORMES
```

#### BLOCS — grid block placement

Click repeatedly to paint blocks. Hold `R` and drag the mouse horizontally to rotate the placement angle (affects Mirror and Prism normals).

| Sub-option | Cell type |
|---|---|
| Mur | Wall |
| Miroir | Mirror |
| Prisme | Prism |

#### LIGNES — straight segment tools

Click and drag from point A to point B, then release to commit a single straight segment.

| Sub-option | Segment type |
|---|---|
| Mur droit | Wall segment |
| Miroir droit | Mirror segment |

#### COURBES — freehand curve tools

Click and drag freely to paint a continuous chain of micro-segments. Each mouse move step ≥ 2 px adds one segment.

| Sub-option | Segment type |
|---|---|
| Courbe Mur | Wall segments |
| Courbe Miroir | Mirror segments |

#### FORMES — parametric geometric shapes

Click and drag to define the bounding box or radius of the shape. A real-time cyan preview is rendered during the drag. Release to commit the final geometry. Use `R` + drag (or mouse wheel while `R` held) to pre-rotate before placing.

| Sub-option | Geometry | Parameters |
|---|---|---|
| Arc | Half-circle arc of mirror segments (40 pts) | Centre = drag start, radius = drag length, open angle = `placeAngle` |
| Ellipse | Full ellipse of mirror segments (80 pts) | Centre = drag midpoint, semi-axes = half drag width/height, rotation = `placeAngle` |
| Parabole | Parabolic mirror (40 pts) | Vertex = drag start, span = max(Δx, Δy), focal length = span / 2, rotation = `placeAngle` |
| Rectangle | 4-sided closed wall (4 segments) | Corners = drag start / end |
| Cercle | Full circle of wall segments (64 pts) | Centre = drag midpoint, radius = diagonal / 2 |

#### LUMIERE — light placement

Single click to teleport the light source to that position.

### Built-in presets

Cycle through them with `P`.

| # | Name | Description |
|---|---|---|
| 0 | Default | Wall column + two angled mirrors + central prism |
| 1 | Full Ellipse (foci) | Complete ellipse of 120 mirror segments; light at left focus → converges to right focus |
| 2 | Ellipse + Shadow | Same ellipse + vertical wall from centre downward → creates a shadow half-plane |
| 3 | Laser Cavity | Two parallel horizontal mirror corridors with angled end mirrors, 32 rays |
| 4 | Parabola + Focus | Parabolic mirror; light placed at exact focal point → parallel ray beam, 90 rays |
| 5 | Rainbow | Central prism + concave arc mirror; 80 rays produce a dispersed spectral fan |

---

## 3D Mode — Ray Tracer

Press `Tab` to switch to 3D mode. A full frame is rendered to a 640 × 360 CPU buffer every frame and then upscaled to fill the window.

### Scene contents

- **Checkerboard floor** — slightly reflective (15%), 2 × 2 unit tiles
- **Reflective back wall** — high reflectivity (85%), near-white
- **Emissive orb** — animated; orbits and bobs above the scene (emission 1.5×)
- **3 large spheres** — red (diffuse), blue (70% reflective), green (diffuse)
- **2 small spheres** — gold (90% reflective), purple (40% reflective)
- **Ring of 8 small spheres** — animated rotation, mixed reflectivities and hues
- **Directional sun** — direction animates slowly when `F` is active

### Keyboard & mouse reference

| Input | Action |
|---|---|
| `W / A / S / D` | Move camera forward / left / backward / right |
| `Q / E` | Move camera down / up |
| Right-click | Toggle mouse capture (free-look) |
| Mouse (captured) | Rotate camera (yaw / pitch, clamped ± 81°) |
| `F` | Toggle scene animation (orb, ring, sun) |
| `+` / `=` | Increase max bounces (up to 8) |
| `-` | Decrease max bounces (down to 1) |
| `Tab` | Return to 2D mode |

---

## Project Architecture

Everything lives in `src/main.cpp`. There is intentionally no build system beyond a single `Makefile` line.

```
src/main.cpp
│
├── Global constants            W=1280, H=720, CELL=32, MAX_BOUNCES=10
├── Colour helpers              Col, lerp(), line(), rect_fill(), rect_draw()
├── Bitmap font 5×7             96-character ASCII, no external dependency
│
├── CellType enum               Empty, Wall, Mirror, Prism,
│                               CurveArc, CurveEllipse, CurveParabola,
│                               LineWall, LineMirror
├── Segment struct              x0,y0,x1,y1 + type + calcNormal()
├── Cell struct                 type + angle
├── wavelengthColor()           t ∈ [0,1] → Col  (7-band spectral)
│
├── genArc()                    produces N polyline segments on a circular arc
├── genEllipse()                produces N polyline segments on a rotated ellipse
├── genParabola()               produces N polyline segments on y=x²/(4f)
│
├── Scene2D                     2D optics simulation
│   ├── grid[ROWS][COLS]        cell array (40×22)
│   ├── segments[]              free segment list
│   ├── UMode enum              13 placement modes
│   ├── castRay()               hybrid grid-march + segment intersection
│   ├── commitShapeDrag()       converts drag bounds to geometry
│   ├── render()                draws grid, segments, rays, HUD, U menu
│   ├── menuUpdateHover()       hover detection with sub-panel sticky zone
│   ├── menuClick()             mode activation
│   ├── loadPreset()            6 built-in scenes
│   └── handleKey/Wheel/…       input dispatch
│
├── Vec3 / Material3D           3D math primitives
├── Sphere / Plane3D / HitInfo  3D scene primitives
│
└── Scene3D                     3D ray tracer
    ├── spheres[] / planes[]    scene geometry
    ├── intersectScene()        sphere + plane intersection, checkerboard
    ├── shadowRay()             hard shadow test toward sun
    ├── skyColor()              gradient + sun disc (power 64)
    ├── trace()                 iterative multi-bounce accumulator
    ├── renderFrame()           per-pixel loop → pixels[] → SDL_Surface
    └── render()                SDL_Surface → SDL_Texture → renderer
```

### Global constants

| Constant | Value | Meaning |
|---|---|---|
| `W` | 1280 | Window / canvas width (px) |
| `H` | 720 | Window / canvas height (px) |
| `CELL` | 32 | Grid cell size (px) |
| `COLS` | 40 | Grid column count |
| `ROWS` | 22 | Grid row count |
| `MAX_BOUNCES` | 10 | Maximum ray bounces in 2D |
| `RW / RH` | 640 / 360 | 3D render resolution |

### Colour helpers & bitmap font

`Col` is a simple `{r,g,b,a}` struct. `lerp(Col,Col,float)` linearly interpolates two colours. `line()`, `rect_fill()`, and `rect_draw()` are thin SDL2 wrappers that set blend mode automatically.

The bitmap font is a hand-crafted 5 × 7 pixel array covering the full printable ASCII range (96 characters). `drawText()` renders it at any integer scale with no SDL_ttf dependency.

### Geometry generators

Three static functions convert mathematical curves into lists of `Segment` objects:

`genArc(segs, cx, cy, radius, startAngle, spanAngle, type, nSeg)` — samples a circular arc uniformly and appends `nSeg` line segments.

`genEllipse(segs, cx, cy, a, b, rot, startAngle, spanAngle, type, nSeg)` — samples a rotated ellipse using the parametric form `(a·cos t, b·sin t)` rotated by `rot`.

`genParabola(segs, cx, cy, focalLen, rot, halfSpan, type, nSeg)` — samples the curve `y = −x² / (4f)` from `−halfSpan` to `+halfSpan`, then rotates by `rot` around the vertex. The negative sign makes the parabola open upward in screen coordinates before rotation, so a light placed at `(cx, cy − focalLen)` produces a collimated output beam after rotation by `−π/2`.

### Scene2D

**Grid vs. segments.** The scene maintains two parallel representations. The `grid[ROWS][COLS]` array holds discrete `Cell` objects used for block placement modes. The `segments` vector holds precise pixel-coordinate `Segment` objects used by all line, curve, and shape tools. Both are tested on every `castRay()` call; the closest hit wins.

**Ray casting (`castRay`).** Rays are cast with a unit direction `(dx,dy)`. Two searches run in parallel:

1. **Segment intersection** — Möller–Trumbore–style 2D cross-product test against every segment in the list. Returns the parametric distance `t` along the ray.
2. **Grid march** — steps along the ray at 1-pixel intervals, reading `grid[row][col]` until a non-empty cell is found or the screen edge is reached.

The closer of the two hits is selected. On a Wall hit the ray stops. On a Mirror hit the specular reflection direction `r = d − 2(d·n)n` is computed; the normal is derived from either the cell angle or the segment direction, with automatic side-selection. On a Prism hit, 7 sub-rays are spawned with angles spread ±0.175 rad around the incoming direction, each coloured by `wavelengthColor()`. Recursion depth is capped at `MAX_BOUNCES = 10`.

**U Menu.** The radial menu consists of 5 branches arranged in a pentagon at radius 180 px from the screen centre. Each branch shows a side panel of sub-options when hovered. The hover region is the convex hull of the branch button and its sub-panel (with 4 px margin), preventing the sub-panel from disappearing when the mouse crosses the gap between them. `menuClick()` maps the selected sub-option to one of 13 `UMode` enum values and activates it.

**Shape drag system.** When a FORMES mode is active, `shapeDragging`, `shapeDragX0/Y0`, and `shapeDragX1/Y1` track the ongoing drag. `render()` draws a live cyan preview (arc fan, ellipse polyline, parabola curve, rectangle outline, or circle) each frame. On mouse-button-up, `commitShapeDrag()` calls the appropriate generator and appends the resulting segments to the list.

**Rotation.** Holding `R` and dragging the mouse horizontally adjusts `placeAngle` at 0.02 rad per pixel. The mouse wheel in R-mode rotates by 5° per notch. `applyRotateToCell()` additionally rotates the Mirror or Prism cell under the cursor if one exists. This applies in all block and shape U modes.

### Scene3D

**Intersection.** `intersectScene()` tests the ray against all spheres (quadratic formula) and planes (dot-product formula), returning the closest `HitInfo`. Checkerboard floors are implemented by XOR-ing the floor indices `⌊x/cs⌋ + ⌊z/cs⌋` and darkening the albedo by 70% on odd tiles.

**Shading.** Each bounce accumulates direct lighting: `albedo × (diffuse + 0.08 ambient)` weighted by `(1 − reflectivity)`. A shadow ray toward `sunDir` zeroes the diffuse term when occluded. After direct lighting, the `mask` is multiplied by `albedo × reflectivity` for the next bounce. The loop terminates early when reflectivity ≤ 0.001 or the bounce count is exhausted.

**Tone mapping.** Each channel passes through `c = 0.9c / (1 + 0.9c)` (modified Reinhard), then gamma-corrected with `c^(1/2.2)`.

**Camera.** Free-look camera with yaw/pitch Euler angles. `camPitch` is clamped to ±81° to prevent gimbal wrap. The view matrix is built each frame from `forward`, `right`, and `up` unit vectors.

### Main loop

`main()` maintains two state flags — `mode3d` and `running` — plus modifier-key booleans `keyD` and `keyR`. The event loop dispatches SDL events to `Scene2D` or `Scene3D` depending on the current mode. The 2D update calls `scene2d.update(dt)` for light animation and `scene2d.render(ren)` for the full draw pass. The 3D path calls `scene3d.update(dt)` then `scene3d.render(ren)`, which re-renders the full CPU frame every loop iteration.

---

## Rendering Pipeline — 2D

```
scene2d.update(dt)
  └── animate light position (orbit)

scene2d.render(ren)
  ├── clear screen (dark background)
  ├── draw grid cells
  │   ├── Wall   → dark blue-grey fill + border
  │   ├── Mirror → cyan fill + normal direction tick + angle arc
  │   └── Prism  → purple fill + triangle icon
  ├── draw free segments (wall=grey, mirror=cyan)
  ├── cast numRays rays from (lightX, lightY)
  │   └── castRay() → vector<Seg>  (additive SDL_BLENDMODE_ADD)
  ├── draw shape drag preview (cyan polyline, yellow origin dot)
  ├── draw line-in-progress preview (thick coloured line)
  ├── draw light source (yellow dot + glow)
  ├── draw U Menu (if menuOpen)
  │   ├── darkened overlay
  │   ├── branch buttons (pentagon)
  │   └── sub-branch panel (side panel, sticky hover)
  ├── HUD status bar (bottom strip)
  └── help overlay (if showHelp)
```

Rays are drawn with `SDL_BLENDMODE_ADD`: overlapping rays accumulate brightness, producing natural glare at focal points.

---

## Rendering Pipeline — 3D

```
scene3d.update(dt)
  ├── animate emissive orb position
  ├── animate ring of 8 spheres
  └── animate sun direction

scene3d.renderFrame()
  └── for each pixel (x, y) in 640×360:
      ├── compute ray direction from camera matrix + FOV 45°
      └── trace(ro, rd, maxBounces)
          ├── intersectScene → HitInfo
          ├── if miss       → skyColor (gradient + sun disc)
          ├── if emissive   → add emission, stop
          ├── shadowRay     → hard shadow
          ├── accumulate    → direct diffuse + ambient
          ├── update mask   → albedo × reflectivity
          └── reflect ray   → next bounce

scene3d.render(ren)
  ├── upload pixels[] to SDL_Surface
  ├── SDL_Surface → SDL_Texture → SDL_RenderCopy (640×360 → 1280×720)
  └── draw HUD strip
```

---

## Extension Ideas

**2D mode**
- Directional laser source (single ray at configurable angle)
- Transparent / refractive cells (Snell's law with configurable index)
- Absorbing fog medium (ray energy attenuation along path length)
- Lens cells (converging / diverging) using two curved surface segments
- Scene save / load (serialize grid + segments to JSON or binary)

**3D mode**
- Monte-Carlo path tracing for global illumination (one random bounce per pixel, accumulated over frames)
- Dielectric materials — glass and water with Fresnel and Snell refraction
- Depth of field via thin-lens sampling
- BVH (Bounding Volume Hierarchy) to accelerate sphere intersection at higher object counts
- Triangle mesh support
- Normal maps / procedural textures