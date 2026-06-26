/*
 * ============================================================
 *  RAY TRACING DEMO — 2D + 3D modes  (v2)
 *  Build:  g++ -O2 -std=c++17 -o raytracer src/main.cpp -lSDL2 -lm
 *
 *  ── 2D mode ──
 *    Left-click (drag)   → déplacer la source lumineuse
 *    D (maintenu) + drag → dessiner des objets (outil sélectionné)
 *    R (maintenu) + drag → rotation interactive (gauche/droite)
 *    Mouse wheel         → densité de rayons
 *    E                   → effacer la cellule sous la souris
 *    1/2/3/4/5/6         → Wall / Mirror / Prism / Arc / Ellipse / Parabole
 *    Space               → animation lumière
 *    P                   → preset suivant
 *    C                   → tout effacer
 *    TAB                 → mode 3D
 *  ── 3D mode ──
 *    WASD+souris → caméra
 *    F           → animation
 *    +/-         → rebonds
 * ============================================================
 */

#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>
#include <cstdio>
#include <random>
#include <functional>
#include <array>

static constexpr int   W            = 1280;
static constexpr int   H            = 720;
static constexpr int   CELL         = 32;
static constexpr int   COLS         = W / CELL;
static constexpr int   ROWS         = H / CELL;
static constexpr float PI           = 3.14159265358979f;
static constexpr int   MAX_BOUNCES  = 10;

// ─── colour helpers ──────────────────────────────────────────
struct Col { uint8_t r,g,b,a; };
static Col lerp(Col a, Col b, float t) {
    return { uint8_t(a.r+(b.r-a.r)*t), uint8_t(a.g+(b.g-a.g)*t),
             uint8_t(a.b+(b.b-a.b)*t), 255 };
}
static void line(SDL_Renderer* r, int x0,int y0,int x1,int y1,
                 uint8_t R,uint8_t G,uint8_t B,uint8_t A=255) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r,R,G,B,A);
    SDL_RenderDrawLine(r,x0,y0,x1,y1);
}
static void rect_fill(SDL_Renderer* r, int x,int y,int w,int h,
                      uint8_t R,uint8_t G,uint8_t B,uint8_t A=255) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r,R,G,B,A);
    SDL_Rect rc{x,y,w,h};
    SDL_RenderFillRect(r,&rc);
}
static void rect_draw(SDL_Renderer* r, int x,int y,int w,int h,
                      uint8_t R,uint8_t G,uint8_t B) {
    SDL_SetRenderDrawColor(r,R,G,B,255);
    SDL_Rect rc{x,y,w,h};
    SDL_RenderDrawRect(r,&rc);
}

// ─── bitmap font 5×7 ─────────────────────────────────────────
static const uint8_t FONT[96][5] = {
  {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5f,0x00,0x00},
  {0x00,0x07,0x00,0x07,0x00},{0x14,0x7f,0x14,0x7f,0x14},
  {0x24,0x2a,0x7f,0x2a,0x12},{0x23,0x13,0x08,0x64,0x62},
  {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
  {0x00,0x1c,0x22,0x41,0x00},{0x00,0x41,0x22,0x1c,0x00},
  {0x14,0x08,0x3e,0x08,0x14},{0x08,0x08,0x3e,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},
  {0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
  {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
  {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
  {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e},
  {0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
  {0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
  {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},
  {0x32,0x49,0x79,0x41,0x3e},{0x7e,0x11,0x11,0x11,0x7e},
  {0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
  {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},
  {0x7f,0x09,0x09,0x09,0x01},{0x3e,0x41,0x49,0x49,0x7a},
  {0x7f,0x08,0x08,0x08,0x7f},{0x00,0x41,0x7f,0x41,0x00},
  {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},
  {0x7f,0x40,0x40,0x40,0x40},{0x7f,0x02,0x0c,0x02,0x7f},
  {0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
  {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},
  {0x7f,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
  {0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
  {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},
  {0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},
  {0x61,0x51,0x49,0x45,0x43},{0x00,0x7f,0x41,0x41,0x00},
  {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7f,0x00},
  {0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
  {0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
  {0x7f,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},
  {0x38,0x44,0x44,0x48,0x7f},{0x38,0x54,0x54,0x54,0x18},
  {0x08,0x7e,0x09,0x01,0x02},{0x0c,0x52,0x52,0x52,0x3e},
  {0x7f,0x08,0x04,0x04,0x78},{0x00,0x44,0x7d,0x40,0x00},
  {0x20,0x40,0x44,0x3d,0x00},{0x7f,0x10,0x28,0x44,0x00},
  {0x00,0x41,0x7f,0x40,0x00},{0x7c,0x04,0x18,0x04,0x78},
  {0x7c,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},
  {0x7c,0x14,0x14,0x14,0x08},{0x08,0x14,0x14,0x18,0x7c},
  {0x7c,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
  {0x04,0x3f,0x44,0x40,0x20},{0x3c,0x40,0x40,0x20,0x7c},
  {0x1c,0x20,0x40,0x20,0x1c},{0x3c,0x40,0x30,0x40,0x3c},
  {0x44,0x28,0x10,0x28,0x44},{0x0c,0x50,0x50,0x50,0x3c},
  {0x44,0x64,0x54,0x4c,0x44},{0x00,0x08,0x36,0x41,0x00},
  {0x00,0x00,0x7f,0x00,0x00},{0x00,0x41,0x36,0x08,0x00},
  {0x10,0x08,0x08,0x10,0x08},{0x78,0x46,0x41,0x46,0x78},
};

static void drawChar(SDL_Renderer* r, int x, int y, char c,
                     uint8_t R=255,uint8_t G=255,uint8_t B=255, int sc=1) {
    if (c < 32 || c > 127) c = '?';
    const uint8_t* col = FONT[(int)(c-32)];
    for (int cx=0;cx<5;cx++)
        for (int cy=0;cy<7;cy++)
            if (col[cx] & (1<<cy)) {
                SDL_SetRenderDrawColor(r,R,G,B,255);
                SDL_Rect rc{x+cx*sc, y+cy*sc, sc, sc};
                SDL_RenderFillRect(r,&rc);
            }
}
static void drawText(SDL_Renderer* r, int x, int y, const char* txt,
                     uint8_t R=255,uint8_t G=255,uint8_t B=255, int sc=1) {
    int ox=x;
    for (; *txt; txt++) {
        if (*txt=='\n') { y+=9*sc; x=ox; continue; }
        drawChar(r,x,y,*txt,R,G,B,sc);
        x += 6*sc;
    }
}

// ══════════════════════════════════════════════════════════════
//  2D RAY TRACING
// ══════════════════════════════════════════════════════════════

// Types d'objets supportés
enum class CellType { Empty=0, Wall, Mirror, Prism, CurveArc, CurveEllipse, CurveParabola, LineWall, LineMirror };

// Un segment réfléchissant/absorbant continu dans l'espace pixel
// (pour les courbes et les murs fins placés librement)
struct Segment {
    float x0,y0, x1,y1;    // endpoints en pixels
    CellType type;
    float angle;            // orientation de la normale (pour mirror)
    // normale calculée automatiquement depuis (x0,y0)→(x1,y1)
    void calcNormal(float& nx, float& ny) const {
        float dx = x1-x0, dy = y1-y0;
        float len = sqrtf(dx*dx+dy*dy);
        if (len < 0.001f) { nx=0; ny=-1; return; }
        nx = -dy/len;
        ny =  dx/len;
    }
};

struct Cell {
    CellType type = CellType::Empty;
    float    angle = 0.f;
};

struct Ray2D {
    float x,y,dx,dy;
    Col   color;
    int   bounces;
};

static Col wavelengthColor(float t) {
    if (t < 0.17f) return lerp({148,0,211,255},{0,0,255,255},  t/0.17f);
    if (t < 0.34f) return lerp({0,0,255,255},  {0,255,255,255},(t-0.17f)/0.17f);
    if (t < 0.50f) return lerp({0,255,255,255},{0,255,0,255},  (t-0.34f)/0.16f);
    if (t < 0.67f) return lerp({0,255,0,255},  {255,255,0,255},(t-0.50f)/0.17f);
    if (t < 0.84f) return lerp({255,255,0,255},{255,127,0,255},(t-0.67f)/0.17f);
    return           lerp({255,127,0,255},{255,0,0,255},       (t-0.84f)/0.16f);
}

// ─── Génération de courbes en segments ───────────────────────
// Transforme un outil "courbe" en une série de Segment précis

// Arc de cercle miroir (demi-cercle par défaut)
static void genArc(std::vector<Segment>& segs, float cx, float cy, float radius,
                   float startAngle, float spanAngle, CellType type, int nSeg=40) {
    for (int i=0; i<nSeg; i++) {
        float a0 = startAngle + spanAngle*(float)i/nSeg;
        float a1 = startAngle + spanAngle*(float)(i+1)/nSeg;
        float x0 = cx + cosf(a0)*radius;
        float y0 = cy + sinf(a0)*radius;
        float x1 = cx + cosf(a1)*radius;
        float y1 = cy + sinf(a1)*radius;
        Segment s;
        s.x0=x0; s.y0=y0; s.x1=x1; s.y1=y1;
        s.type=type; s.angle=0;
        segs.push_back(s);
    }
}

// Ellipse complète ou partielle
static void genEllipse(std::vector<Segment>& segs, float cx, float cy,
                       float a, float b, float rot,
                       float startAngle, float spanAngle,
                       CellType type, int nSeg=80) {
    float cr = cosf(rot), sr = sinf(rot);
    for (int i=0; i<nSeg; i++) {
        float t0 = startAngle + spanAngle*(float)i/nSeg;
        float t1 = startAngle + spanAngle*(float)(i+1)/nSeg;
        auto pt = [&](float t, float& px, float& py){
            float lx = a*cosf(t), ly = b*sinf(t);
            px = cx + lx*cr - ly*sr;
            py = cy + lx*sr + ly*cr;
        };
        float x0,y0,x1,y1;
        pt(t0,x0,y0); pt(t1,x1,y1);
        Segment s; s.x0=x0; s.y0=y0; s.x1=x1; s.y1=y1;
        s.type=type; s.angle=0;
        segs.push_back(s);
    }
}

// Parabole miroir
static void genParabola(std::vector<Segment>& segs, float cx, float cy,
                        float focalLen, float rot, float halfSpan,
                        CellType type, int nSeg=40) {
    float cr = cosf(rot), sr = sinf(rot);
    for (int i=0; i<nSeg; i++) {
        auto pt = [&](float t, float& px, float& py){
            float lx = t;
            float ly = -(t*t)/(4*focalLen);
            px = cx + lx*cr - ly*sr;
            py = cy + lx*sr + ly*cr;
        };
        float t0 = -halfSpan + 2*halfSpan*(float)i/nSeg;
        float t1 = -halfSpan + 2*halfSpan*(float)(i+1)/nSeg;
        float x0,y0,x1,y1;
        pt(t0,x0,y0); pt(t1,x1,y1);
        Segment s; s.x0=x0; s.y0=y0; s.x1=x1; s.y1=y1;
        s.type=type; s.angle=0;
        segs.push_back(s);
    }
}

// ─── Scene2D ─────────────────────────────────────────────────
class Scene2D {
public:
    Cell             grid[ROWS][COLS];
    std::vector<Segment> segments;
    float            lightX, lightY;
    int              numRays     = 240;
    bool             animLight   = false;
    float            lightAngle  = 0.f;
    CellType         placeTool   = CellType::Wall;
    float            placeAngle  = 0.f;
    int              presetIndex = -1;
    // ── Outil ligne libre ──────────────────────────────────
    bool  lineDrawing  = false;
    float lineStartX   = 0, lineStartY = 0;
    float linePreviewX = 0, linePreviewY = 0;
    // ── Crayon libre (L) ───────────────────────────────────
    bool  pencilActive = false;  // L maintenu
    bool  pencilDown   = false;  // clic gauche en cours
    float pencilPrevX  = 0, pencilPrevY = 0;
    bool  pencilMirror = false;  // false=mur, true=miroir
    // ── Aide ────────────────────────────────────────────────
    bool  showHelp     = false;

    // ══ Menu U ══════════════════════════════════════════════
    // Un "mode U" est actif : seul le clic gauche de ce mode
    // fonctionne, plus rien d'autre (sauf U pour quitter).
    bool  menuOpen     = false;   // menu visible
    int   menuBranch   = -1;      // branche survolée (-1=aucune)
    int   menuSub      = -1;      // sous-branche survolée

    // Mode U actif (mode exclusif apres confirmation)
    bool  uModeActive  = false;
    // Description du mode actif
    enum class UMode {
        None,
        BlocWall, BlocMirror, BlocPrism,       // BLOCS
        LineWallFree, LineMirrorFree,           // LIGNES
        CurveWall, CurveMirror,                 // COURBES (pencil libre)
        ShapeArc, ShapeEllipse, ShapeParabola,  // FORMES geometriques (drag)
        ShapeRect, ShapeCircle,                 // FORMES geometriques (drag)
        Light
    };
    UMode uMode = UMode::None;

    // Drag de forme geometrique
    bool  shapeDragging = false;
    float shapeDragX0=0, shapeDragY0=0;
    float shapeDragX1=0, shapeDragY1=0;

    const char* uModeName() const {
        switch(uMode) {
            case UMode::BlocWall:        return "BLOC MUR";
            case UMode::BlocMirror:      return "BLOC MIROIR";
            case UMode::BlocPrism:       return "BLOC PRISME";
            case UMode::LineWallFree:    return "LIGNE MUR";
            case UMode::LineMirrorFree:  return "LIGNE MIROIR";
            case UMode::CurveWall:       return "COURBE MUR";
            case UMode::CurveMirror:     return "COURBE MIROIR";
            case UMode::ShapeArc:        return "FORME ARC";
            case UMode::ShapeEllipse:    return "FORME ELLIPSE";
            case UMode::ShapeParabola:   return "FORME PARABOLE";
            case UMode::ShapeRect:       return "FORME RECTANGLE";
            case UMode::ShapeCircle:     return "FORME CERCLE";
            case UMode::Light:           return "LUMIERE";
            default:                     return "";
        }
    }

    // Genere la forme geometrique au commit drag
    void commitShapeDrag(float x0, float y0, float x1, float y1) {
        float cx=(x0+x1)*0.5f, cy=(y0+y1)*0.5f;
        float rw=fabsf(x1-x0)*0.5f, rh=fabsf(y1-y0)*0.5f;
        float rmoy=sqrtf(rw*rw+rh*rh)*0.5f;
        switch(uMode) {
            case UMode::ShapeArc: {
                float rad=sqrtf((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));
                if(rad<4.f) break;
                genArc(segments, x0, y0, rad, placeAngle, PI, CellType::Mirror, 40);
                break;
            }
            case UMode::ShapeEllipse: {
                if(rw<4.f||rh<4.f) break;
                genEllipse(segments, cx, cy, rw, rh, placeAngle, 0, 2*PI, CellType::Mirror, 80);
                break;
            }
            case UMode::ShapeParabola: {
                float span=std::max(rw,rh);
                float focal=span*0.5f;
                if(span<4.f) break;
                genParabola(segments, x0, y0, focal, placeAngle, span, CellType::Mirror, 40);
                break;
            }
            case UMode::ShapeRect: {
                if(rw<4.f||rh<4.f) break;
                auto addSeg=[&](float ax,float ay,float bx,float by,CellType t){
                    Segment s; s.x0=ax;s.y0=ay;s.x1=bx;s.y1=by;
                    s.type=t; s.angle=0; segments.push_back(s);
                };
                addSeg(x0,y0,x1,y0,CellType::Wall);
                addSeg(x1,y0,x1,y1,CellType::Wall);
                addSeg(x1,y1,x0,y1,CellType::Wall);
                addSeg(x0,y1,x0,y0,CellType::Wall);
                break;
            }
            case UMode::ShapeCircle: {
                float r=rmoy;
                if(r<4.f) break;
                genArc(segments, cx, cy, r, 0, 2*PI, CellType::Wall, 64);
                break;
            }
            default: break;
        }
        (void)rmoy;
    }

    // Appliquer le mode U au clic (modes sans drag)
    void uModeClick(float px, float py) {
        int col=(int)(px/CELL), row=(int)(py/CELL);
        if (col<0||row<0||col>=COLS||row>=ROWS) return;
        switch(uMode) {
            case UMode::BlocWall:   grid[row][col]={CellType::Wall,   placeAngle}; break;
            case UMode::BlocMirror: grid[row][col]={CellType::Mirror, placeAngle}; break;
            case UMode::BlocPrism:  grid[row][col]={CellType::Prism,  placeAngle}; break;
            case UMode::Light:
                lightX=px; lightY=py; animLight=false; break;
            default: break;
        }
    }

    // Debut du drag (ligne, courbe libre, forme)
    void uModeBeginDrag(float px, float py) {
        switch(uMode) {
            case UMode::LineWallFree:
                placeTool=CellType::LineWall;
                lineBegin((int)px,(int)py); break;
            case UMode::LineMirrorFree:
                placeTool=CellType::LineMirror;
                lineBegin((int)px,(int)py); break;
            case UMode::CurveWall:
                pencilActive=true; pencilMirror=false;
                pencilStart(px,py); break;
            case UMode::CurveMirror:
                pencilActive=true; pencilMirror=true;
                pencilStart(px,py); break;
            // Formes geometriques : drag debut→fin
            case UMode::ShapeArc:
            case UMode::ShapeEllipse:
            case UMode::ShapeParabola:
            case UMode::ShapeRect:
            case UMode::ShapeCircle:
                shapeDragging=true;
                shapeDragX0=px; shapeDragY0=py;
                shapeDragX1=px; shapeDragY1=py;
                break;
            default: break;
        }
    }

    // Fin du drag
    void uModeEndDrag(float px, float py) {
        switch(uMode) {
            case UMode::LineWallFree:
            case UMode::LineMirrorFree:
                lineCommit((int)px,(int)py); break;
            case UMode::CurveWall:
            case UMode::CurveMirror:
                pencilStop(); pencilActive=false; break;
            case UMode::ShapeArc:
            case UMode::ShapeEllipse:
            case UMode::ShapeParabola:
            case UMode::ShapeRect:
            case UMode::ShapeCircle:
                if(shapeDragging) {
                    commitShapeDrag(shapeDragX0,shapeDragY0,px,py);
                    shapeDragging=false;
                }
                break;
            default: break;
        }
    }

    // Mise a jour du preview pendant le drag de forme
    void uModeUpdateDrag(float px, float py) {
        if(shapeDragging) {
            shapeDragX1=px; shapeDragY1=py;
        }
        if(pencilActive && pencilDown) {
            pencilMoveTo(px,py);
        }
    }

    Scene2D() : lightX(W*0.15f), lightY(H*0.5f) {
        loadPreset(0);
    }

    // ── Presets ───────────────────────────────────────────────
    // Retourne le nom du preset
    const char* presetName(int idx) {
        switch(idx%NUM_PRESETS) {
            case 0: return "Defaut : murs + miroirs + prisme";
            case 1: return "Ellipse complete (foyers)";
            case 2: return "Ellipse + demi-mur (zone d'ombre)";
            case 3: return "Couloir de miroirs (cavite laser)";
            case 4: return "Parabole + source au foyer";
            case 5: return "Arc-en-ciel : prisme + arc miroir";
            default: return "?";
        }
    }
    static constexpr int NUM_PRESETS = 6;

    void loadPreset(int idx) {
        idx = idx % NUM_PRESETS;
        presetIndex = idx;
        clearAll();

        float cx = W*0.5f, cy = H*0.5f;

        switch(idx) {
            case 0: {
                // Defaut original
                for (int row=3;row<=7;row++) grid[row][5] = {CellType::Wall, 0};
                grid[3][10] = {CellType::Mirror, -PI/4};
                grid[8][10] = {CellType::Mirror,  PI/4};
                grid[5][15] = {CellType::Prism, 0};
                lightX = W*0.15f; lightY = H*0.5f;
                break;
            }
            case 1: {
                // Ellipse complète en miroirs
                // a=300, b=180 → foyers à (±sqrt(a²-b²)) = ±240 de cx
                float a=300, b=180;
                float c=sqrtf(a*a-b*b); // 240
                genEllipse(segments, cx, cy, a, b, 0, 0, 2*PI, CellType::Mirror, 120);
                // Lumière sur le foyer gauche
                lightX = cx - c; lightY = cy;
                break;
            }
            case 2: {
                // Ellipse en miroirs + demi-mur vertical au centre
                // → même si l'ellipse est parfaite, le mur bloque
                float a=300, b=180;
                float c=sqrtf(a*a-b*b);
                genEllipse(segments, cx, cy, a, b, 0, 0, 2*PI, CellType::Mirror, 120);
                // Demi-mur vertical du bas de l'ellipse jusqu'en bas de l'écran
                // (bloque le foyer droit → zone d'ombre)
                Segment wall;
                wall.x0 = cx; wall.y0 = cy;
                wall.x1 = cx; wall.y1 = cy + b + 20;
                wall.type = CellType::Wall; wall.angle = 0;
                segments.push_back(wall);
                // Quelques cellules mur aussi
                for (int row = (int)((cy)/CELL); row < ROWS; row++)
                    grid[row][(int)(cx/CELL)] = {CellType::Wall, 0};
                lightX = cx - c; lightY = cy;
                break;
            }
            case 3: {
                // Couloir de miroirs horizontal (cavité laser)
                int midR = ROWS/2;
                for (int col=2; col<COLS-2; col++) {
                    grid[midR-3][col] = {CellType::Mirror, 0};   // mur haut
                    grid[midR+3][col] = {CellType::Mirror, 0};   // mur bas
                }
                // Miroirs aux extremites
                grid[midR-2][2] = {CellType::Mirror, PI/4};
                grid[midR+2][2] = {CellType::Mirror, -PI/4};
                grid[midR-2][COLS-3] = {CellType::Mirror, -PI/4};
                grid[midR+2][COLS-3] = {CellType::Mirror, PI/4};
                lightX = CELL*3.f; lightY = cy;
                numRays = 32;
                break;
            }
            case 4: {
                // Parabole miroir — source au foyer → rayons parallèles
                float focal = 160.f;
                genParabola(segments, cx, cy, focal, -PI/2, 240.f, CellType::Mirror, 60);
                // Mur de fond pour visualiser les rayons parallèles
                Segment back;
                back.x0 = cx-280; back.y0 = cy - focal - 20;
                back.x1 = cx+280; back.y1 = cy - focal - 20;
                back.type = CellType::Wall; back.angle = 0;
                segments.push_back(back);
                lightX = cx; lightY = cy - focal; // foyer exact
                numRays = 90;
                break;
            }
            case 5: {
                // Prisme central + arc de miroir concave → arc-en-ciel concentré
                // Prisme au centre
                grid[ROWS/2][COLS/2] = {CellType::Prism, 0};
                // Arc de miroir concave à droite
                genArc(segments, cx+320, cy, 200, PI*0.55f, PI*0.9f, CellType::Mirror, 40);
                lightX = cx - 150; lightY = cy;
                numRays = 80;
                break;
            }
        }
    }

    void clearAll() {
        for (int r=0;r<ROWS;r++) for (int c=0;c<COLS;c++) grid[r][c]={CellType::Empty,0};
        segments.clear();
    }

    // ── Ray casting ───────────────────────────────────────────
    struct Seg { float x0,y0,x1,y1; Col c; };

    // Intersect ray with a Segment (returns t or -1)
    float intersectSeg(float ox, float oy, float dx, float dy,
                       const Segment& s) const {
        float ex=s.x1-s.x0, ey=s.y1-s.y0;
        float denom = dx*ey - dy*ex;
        if (fabsf(denom) < 1e-6f) return -1;
        float tx = s.x0-ox, ty=s.y0-oy;
        float t = (tx*ey - ty*ex)/denom;
        float u = (tx*dy - ty*dx)/denom;
        if (t < 0.5f || u < 0.f || u > 1.f) return -1;
        return t;
    }

    void castRay(float ox, float oy, float dx, float dy, Col c,
                 int bounces, std::vector<Seg>& out) const {
        if (bounces > MAX_BOUNCES) return;
        float maxDist = sqrtf((float)(W*W+H*H));

        // Test contre tous les segments continus
        float bestT = maxDist;
        int   bestSeg = -1;
        for (int i=0;i<(int)segments.size();i++) {
            float t = intersectSeg(ox,oy,dx,dy,segments[i]);
            if (t > 0.5f && t < bestT) { bestT=t; bestSeg=i; }
        }

        // Test contre la grille (marche par pas)
        float stepT = -1;
        int   hitCol=-1, hitRow=-1;
        float step = 1.0f;
        for (float t=step; t<bestT; t+=step) {
            float nx=ox+dx*t, ny=oy+dy*t;
            if (nx<0||ny<0||nx>=W||ny>=H) { stepT=t; hitCol=-2; break; }
            int col=(int)(nx/CELL), row=(int)(ny/CELL);
            if (col<0||row<0||col>=COLS||row>=ROWS) { stepT=t; hitCol=-2; break; }
            if (grid[row][col].type != CellType::Empty) {
                stepT=t; hitCol=col; hitRow=row; break;
            }
        }

        // Choisir le hit le plus proche
        if (bestSeg >= 0 && (stepT < 0 || bestT < stepT)) {
            // Hit un segment continu
            float hx=ox+dx*bestT, hy=oy+dy*bestT;
            out.push_back({ox,oy,hx,hy,c});

            const Segment& seg = segments[bestSeg];
            CellType typ = seg.type;

            if (typ == CellType::Wall) return;

            if (typ == CellType::Mirror) {
                float nx2,ny2;
                seg.calcNormal(nx2,ny2);
                // Choisir le côté de la normale vers lequel le rayon arrive
                if (dx*nx2+dy*ny2 > 0) { nx2=-nx2; ny2=-ny2; }
                float dot=dx*nx2+dy*ny2;
                float rdx=dx-2*dot*nx2, rdy=dy-2*dot*ny2;
                castRay(hx+rdx*2, hy+rdy*2, rdx, rdy, c, bounces+1, out);
                return;
            }
            if (typ == CellType::Prism) {
                float base=atan2f(dy,dx);
                for (int w=0;w<7;w++) {
                    float frac=(float)w/6.f;
                    float spread=(frac-0.5f)*0.35f;
                    float a=base+spread;
                    Col wc=wavelengthColor(frac); wc.a=200;
                    castRay(hx+cosf(a)*2, hy+sinf(a)*2, cosf(a), sinf(a), wc, bounces+2, out);
                }
                return;
            }
            return;
        }

        // Hit grille
        if (stepT > 0) {
            float hx=ox+dx*stepT, hy=oy+dy*stepT;
            out.push_back({ox,oy,hx,hy,c});
            if (hitCol == -2) return; // bord de l'écran
            const Cell& cell=grid[hitRow][hitCol];
            if (cell.type==CellType::Wall) return;
            if (cell.type==CellType::Mirror) {
                float nx2=cosf(cell.angle+PI/2), ny2=sinf(cell.angle+PI/2);
                if (dx*nx2+dy*ny2>0) {nx2=-nx2; ny2=-ny2;}
                float dot=dx*nx2+dy*ny2;
                float rdx=dx-2*dot*nx2, rdy=dy-2*dot*ny2;
                castRay(hx+rdx*2, hy+rdy*2, rdx, rdy, c, bounces+1, out);
                return;
            }
            if (cell.type==CellType::Prism) {
                float base=atan2f(dy,dx);
                for (int w=0;w<7;w++) {
                    float frac=(float)w/6.f;
                    float spread=(frac-0.5f)*0.35f;
                    float a=base+spread;
                    Col wc=wavelengthColor(frac); wc.a=200;
                    castRay(hx+cosf(a)*2, hy+sinf(a)*2, cosf(a), sinf(a), wc, bounces+2, out);
                }
                return;
            }
            return;
        }

        // Aucun hit
        float ex=ox+dx*maxDist, ey=oy+dy*maxDist;
        out.push_back({ox,oy,ex,ey,c});
    }

    void update(float dt) {
        if (animLight) {
            lightAngle += dt * 0.8f;
            lightX = W*0.5f + cosf(lightAngle)*200.f;
            lightY = H*0.5f + sinf(lightAngle)*200.f;
        }
    }

    // ── Draw routines ─────────────────────────────────────────
    void drawSegment(SDL_Renderer* r, const Segment& s) {
        uint8_t R,G,B;
        switch(s.type) {
            case CellType::Wall:    R=70; G=80; B=100; break;
            case CellType::Mirror:  R=100; G=200; B=255; break;
            case CellType::Prism:   R=200; G=100; B=255; break;
            default: R=200; G=200; B=200;
        }
        // Épaisseur 3
        for (int o=-1;o<=1;o++) {
            float nx2,ny2; s.calcNormal(nx2,ny2);
            int x0=(int)(s.x0+nx2*o), y0=(int)(s.y0+ny2*o);
            int x1=(int)(s.x1+nx2*o), y1=(int)(s.y1+ny2*o);
            line(r,x0,y0,x1,y1,R,G,B,255);
        }
    }

    // Dessine un cercle approximatif avec des segments SDL
    void drawCircle(SDL_Renderer* r, int cx, int cy, int rad,
                    uint8_t R, uint8_t G, uint8_t B, uint8_t A=255) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r,R,G,B,A);
        int N=48;
        for (int i=0;i<N;i++) {
            float a0=2*PI*i/N, a1=2*PI*(i+1)/N;
            SDL_RenderDrawLine(r,
                cx+(int)(cosf(a0)*rad), cy+(int)(sinf(a0)*rad),
                cx+(int)(cosf(a1)*rad), cy+(int)(sinf(a1)*rad));
        }
    }

    void render(SDL_Renderer* r, bool drawMode, bool rotMode, bool pencilOn, int mx, int my) {
        SDL_SetRenderDrawColor(r,10,10,20,255);
        SDL_RenderClear(r);

        // Grille
        for (int c=0;c<COLS;c++) { SDL_SetRenderDrawColor(r,20,20,35,255); SDL_RenderDrawLine(r,c*CELL,0,c*CELL,H); }
        for (int row=0;row<ROWS;row++) { SDL_SetRenderDrawColor(r,20,20,35,255); SDL_RenderDrawLine(r,0,row*CELL,W,row*CELL); }

        // Surbrillance cellule sous la souris (toujours visible)
        {
            int hcol=mx/CELL, hrow=my/CELL;
            if (hcol>=0&&hrow>=0&&hcol<COLS&&hrow<ROWS)
                rect_fill(r,hcol*CELL,hrow*CELL,CELL,CELL,255,255,255,18);
        }

        // Cellules grille
        for (int row=0;row<ROWS;row++) {
            for (int c=0;c<COLS;c++) {
                const Cell& cell=grid[row][c];
                int px=c*CELL, py=row*CELL;
                if (cell.type==CellType::Wall) {
                    rect_fill(r,px+1,py+1,CELL-2,CELL-2,60,70,90);
                    rect_draw(r,px,py,CELL,CELL,100,120,160);
                } else if (cell.type==CellType::Mirror) {
                    rect_fill(r,px+1,py+1,CELL-2,CELL-2,20,40,60);
                    float ccx=px+CELL/2.f, ccy=py+CELL/2.f;
                    float mx2=cosf(cell.angle)*CELL*0.45f, my2=sinf(cell.angle)*CELL*0.45f;
                    SDL_SetRenderDrawColor(r,100,200,255,255);
                    SDL_RenderDrawLine(r,(int)(ccx-mx2),(int)(ccy-my2),(int)(ccx+mx2),(int)(ccy+my2));
                    // Petite flèche normale
                    float nx2=cosf(cell.angle+PI/2)*8, ny2=sinf(cell.angle+PI/2)*8;
                    SDL_SetRenderDrawColor(r,60,160,255,160);
                    SDL_RenderDrawLine(r,(int)ccx,(int)ccy,(int)(ccx+nx2),(int)(ccy+ny2));
                    rect_draw(r,px,py,CELL,CELL,60,160,220);
                } else if (cell.type==CellType::Prism) {
                    rect_fill(r,px+1,py+1,CELL-2,CELL-2,40,20,60);
                    float ccx=px+CELL/2.f, ccy=py+CELL/2.f;
                    int hs=CELL/2-4;
                    SDL_SetRenderDrawColor(r,200,100,255,255);
                    SDL_RenderDrawLine(r,(int)ccx,(int)(ccy-hs),(int)(ccx-hs),(int)(ccy+hs));
                    SDL_RenderDrawLine(r,(int)(ccx-hs),(int)(ccy+hs),(int)(ccx+hs),(int)(ccy+hs));
                    SDL_RenderDrawLine(r,(int)(ccx+hs),(int)(ccy+hs),(int)ccx,(int)(ccy-hs));
                    rect_draw(r,px,py,CELL,CELL,160,60,255);
                }
            }
        }

        // Segments courbes
        for (auto& s: segments) drawSegment(r, s);

        // Rayons
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
        std::vector<Seg> segs;
        segs.reserve(numRays*8);
        for (int i=0;i<numRays;i++) {
            float angle=(2.f*PI*i)/numRays;
            Col c{255,230,100,180};
            castRay(lightX,lightY,cosf(angle),sinf(angle),c,0,segs);
        }
        for (auto& s: segs) {
            SDL_SetRenderDrawColor(r,s.c.r,s.c.g,s.c.b,s.c.a);
            SDL_RenderDrawLine(r,(int)s.x0,(int)s.y0,(int)s.x1,(int)s.y1);
        }
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

        // Lueur source
        for (int g=14;g>0;g--) {
            SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r,255,220,80,5*g);
            SDL_Rect gr{(int)(lightX-g),(int)(lightY-g),g*2,g*2};
            SDL_RenderFillRect(r,&gr);
        }
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r,255,255,200,255);
        SDL_Rect lr{(int)(lightX-3),(int)(lightY-3),6,6};
        SDL_RenderFillRect(r,&lr);

        // ── Visuel outil rotation ─────────────────────────────
        if (rotMode) {
            int hcol=mx/CELL, hrow=my/CELL;
            if (hcol>=0&&hrow>=0&&hcol<COLS&&hrow<ROWS) {
                int ccx=hcol*CELL+CELL/2, ccy=hrow*CELL+CELL/2;
                int rad=CELL-2;

                // Fond de la cellule orange translucide
                rect_fill(r,hcol*CELL,hrow*CELL,CELL,CELL,255,160,40,60);

                // Cercle de rotation
                drawCircle(r,ccx,ccy,rad,255,180,60,200);

                // Quel angle afficher : celui de la cellule si elle existe, sinon placeAngle
                float dispAngle = placeAngle;
                if (grid[hrow][hcol].type==CellType::Mirror ||
                    grid[hrow][hcol].type==CellType::Prism)
                    dispAngle = grid[hrow][hcol].angle;

                // Aiguille de l'angle actuel
                SDL_SetRenderDrawColor(r,255,220,80,255);
                SDL_RenderDrawLine(r,ccx,ccy,
                    ccx+(int)(cosf(dispAngle)*(rad-2)),
                    ccy+(int)(sinf(dispAngle)*(rad-2)));
                // Petite flèche perpendiculaire (normale)
                SDL_SetRenderDrawColor(r,80,200,255,200);
                SDL_RenderDrawLine(r,ccx,ccy,
                    ccx+(int)(cosf(dispAngle+PI/2)*rad*0.6f),
                    ccy+(int)(sinf(dispAngle+PI/2)*rad*0.6f));

                // Angle en degrés affiché à côté
                char abuf[32];
                float deg = dispAngle * 180.f / PI;
                // normalise 0-360
                while (deg < 0)   deg += 360;
                while (deg >= 360) deg -= 360;
                snprintf(abuf,sizeof(abuf),"%.0f deg", deg);
                rect_fill(r,ccx+rad+4,ccy-8,70,14,0,0,0,180);
                drawText(r,ccx+rad+6,ccy-6,abuf,255,220,80,1);
            }

            // Bandeau haut
            rect_fill(r,0,0,W,18,180,100,20,200);
            drawText(r,6,5,"[R] ROTATION — glisser gauche/droite  |  Molette = pas fins",255,200,80,1);
        }

        // ── Indicateur CRAYON ────────────────────────────────
        if (pencilOn) {
            const char* ptype = pencilMirror ? "MIROIR" : "MUR";
            char pbuf[64]; snprintf(pbuf,sizeof(pbuf),"[L] CRAYON LIBRE (%s) — maintenir clic gauche pour tracer",ptype);
            rect_fill(r,0,0,W,18,160,80,200,220);
            drawText(r,6,5,pbuf,220,160,255,1);
        }
        // ── Indicateur D ─────────────────────────────────────
        if (drawMode && !pencilOn) {
            rect_fill(r,0,0,W,18,20,140,40,200);
            drawText(r,6,5,"[D] DESSIN — clic gauche ou glisser pour placer",80,255,120,1);
        }

        // ── Preview drag de forme geometrique ────────────────
        if (shapeDragging) {
            float x0=shapeDragX0, y0=shapeDragY0, x1=shapeDragX1, y1=shapeDragY1;
            float cx2=(x0+x1)*0.5f, cy2=(y0+y1)*0.5f;
            float rw=fabsf(x1-x0)*0.5f, rh=fabsf(y1-y0)*0.5f;
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r,100,220,255,180);
            switch(uMode) {
                case UMode::ShapeRect: {
                    // Rectangle preview
                    SDL_RenderDrawLine(r,(int)x0,(int)y0,(int)x1,(int)y0);
                    SDL_RenderDrawLine(r,(int)x1,(int)y0,(int)x1,(int)y1);
                    SDL_RenderDrawLine(r,(int)x1,(int)y1,(int)x0,(int)y1);
                    SDL_RenderDrawLine(r,(int)x0,(int)y1,(int)x0,(int)y0);
                    break;
                }
                case UMode::ShapeCircle: {
                    float rad=sqrtf(rw*rw+rh*rh)*0.5f;
                    int N=48;
                    for(int i=0;i<N;i++){
                        float a0=2*PI*i/N, a1=2*PI*(i+1)/N;
                        SDL_RenderDrawLine(r,
                            (int)(cx2+cosf(a0)*rad),(int)(cy2+sinf(a0)*rad),
                            (int)(cx2+cosf(a1)*rad),(int)(cy2+sinf(a1)*rad));
                    }
                    break;
                }
                case UMode::ShapeEllipse: {
                    int N=60;
                    float cr=cosf(placeAngle),sr=sinf(placeAngle);
                    for(int i=0;i<N;i++){
                        float t0=2*PI*i/N, t1=2*PI*(i+1)/N;
                        float lx0=rw*cosf(t0),ly0=rh*sinf(t0);
                        float lx1=rw*cosf(t1),ly1=rh*sinf(t1);
                        SDL_RenderDrawLine(r,
                            (int)(cx2+lx0*cr-ly0*sr),(int)(cy2+lx0*sr+ly0*cr),
                            (int)(cx2+lx1*cr-ly1*sr),(int)(cy2+lx1*sr+ly1*cr));
                    }
                    break;
                }
                case UMode::ShapeArc: {
                    float rad=sqrtf((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));
                    int N=40;
                    for(int i=0;i<N;i++){
                        float a0=placeAngle+PI*i/N, a1=placeAngle+PI*(i+1)/N;
                        SDL_RenderDrawLine(r,
                            (int)(x0+cosf(a0)*rad),(int)(y0+sinf(a0)*rad),
                            (int)(x0+cosf(a1)*rad),(int)(y0+sinf(a1)*rad));
                    }
                    // Ligne centre→souris
                    SDL_SetRenderDrawColor(r,255,255,80,120);
                    SDL_RenderDrawLine(r,(int)x0,(int)y0,(int)x1,(int)y1);
                    break;
                }
                case UMode::ShapeParabola: {
                    float span=std::max(rw,rh);
                    float focal=span*0.5f;
                    float cr=cosf(placeAngle),sr=sinf(placeAngle);
                    int N=40;
                    for(int i=0;i<N;i++){
                        float t0=-span+2*span*(float)i/N;
                        float t1=-span+2*span*(float)(i+1)/N;
                        float lx0=t0, ly0=-(t0*t0)/(4*focal);
                        float lx1=t1, ly1=-(t1*t1)/(4*focal);
                        SDL_RenderDrawLine(r,
                            (int)(x0+lx0*cr-ly0*sr),(int)(y0+lx0*sr+ly0*cr),
                            (int)(x0+lx1*cr-ly1*sr),(int)(y0+lx1*sr+ly1*cr));
                    }
                    SDL_SetRenderDrawColor(r,255,255,80,120);
                    SDL_RenderDrawLine(r,(int)x0,(int)y0,(int)x1,(int)y1);
                    break;
                }
                default: break;
            }
            // Point de depart
            SDL_SetRenderDrawColor(r,255,255,80,255);
            SDL_Rect sp{(int)x0-4,(int)y0-4,8,8};
            SDL_RenderFillRect(r,&sp);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }

        // ── Preview ligne en cours ───────────────────────────
        if (lineDrawing) {
            bool isMirrorLine = (placeTool==CellType::LineMirror);
            // Ligne preview épaisse
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            for (int off=-2; off<=2; off++) {
                // direction perp
                float dx2=linePreviewX-lineStartX, dy2=linePreviewY-lineStartY;
                float len2=sqrtf(dx2*dx2+dy2*dy2); if(len2<1)len2=1;
                float nx2=-dy2/len2*off, ny2=dx2/len2*off;
                if (isMirrorLine)
                    SDL_SetRenderDrawColor(r,80,200,255, off==0?255:80);
                else
                    SDL_SetRenderDrawColor(r,100,115,145, off==0?255:80);
                SDL_RenderDrawLine(r,
                    (int)(lineStartX+nx2),(int)(lineStartY+ny2),
                    (int)(linePreviewX+nx2),(int)(linePreviewY+ny2));
            }
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
            // Point de départ
            SDL_SetRenderDrawColor(r,255,255,80,255);
            SDL_Rect sp{(int)lineStartX-4,(int)lineStartY-4,8,8};
            SDL_RenderFillRect(r,&sp);
            // Longueur en pixels
            float dx2=linePreviewX-lineStartX, dy2=linePreviewY-lineStartY;
            float len2=sqrtf(dx2*dx2+dy2*dy2);
            float midX=(lineStartX+linePreviewX)/2, midY=(lineStartY+linePreviewY)/2;
            char lbuf[32]; snprintf(lbuf,sizeof(lbuf),"%.0fpx",len2);
            rect_fill(r,(int)midX-2,(int)midY-9,50,12,0,0,0,180);
            drawText(r,(int)midX,(int)midY-8,lbuf,255,240,80,1);
        }

        // ── HUD bas ───────────────────────────────────────────
        char buf[512];
        const char* toolName = "?";
        switch(placeTool) {
            case CellType::Wall:          toolName="MUR[1]"; break;
            case CellType::Mirror:        toolName="MIROIR[2]"; break;
            case CellType::Prism:         toolName="PRISME[3]"; break;
            case CellType::CurveArc:      toolName="ARC[4]"; break;
            case CellType::CurveEllipse:  toolName="ELLIPSE[5]"; break;
            case CellType::CurveParabola: toolName="PARABOLE[6]"; break;
            case CellType::LineWall:      toolName="LIGNE-MUR[7]"; break;
            case CellType::LineMirror:    toolName="LIGNE-MIROIR[8]"; break;
            default: break;
        }
        // Angle de placement courant
        float pdeg = placeAngle*180.f/PI;
        while (pdeg<0) pdeg+=360; while (pdeg>=360) pdeg-=360;
        if(uModeActive) {
            // Bandeau mode U
            bool isShapeDrag=(uMode==UMode::ShapeArc||uMode==UMode::ShapeEllipse||
                              uMode==UMode::ShapeParabola||uMode==UMode::ShapeRect||uMode==UMode::ShapeCircle);
            bool isCurveFree=(uMode==UMode::CurveWall||uMode==UMode::CurveMirror);
            bool isLineFree =(uMode==UMode::LineWallFree||uMode==UMode::LineMirrorFree);
            if(isShapeDrag)
                snprintf(buf,sizeof(buf),"[ MODE U ] %s  |  Clic+Glisser = tracer la forme  R+glisser=angle  U=quitter",uModeName());
            else if(isCurveFree)
                snprintf(buf,sizeof(buf),"[ MODE U ] %s  |  Clic+Glisser = tracer librement  U = quitter",uModeName());
            else if(isLineFree)
                snprintf(buf,sizeof(buf),"[ MODE U ] %s  |  Clic+Glisser = tracer la ligne  U = quitter",uModeName());
            else
                snprintf(buf,sizeof(buf),"[ MODE U ] %s  |  Clic gauche = placer  R+glisser=rotation  U=quitter",uModeName());
            rect_fill(r,0,H-20,W,20,20,20,60,230);
            SDL_SetRenderDrawColor(r,100,180,255,200);
            SDL_RenderDrawLine(r,0,H-20,W,H-20);
            drawText(r,4,H-15,buf,120,200,255,1);
        } else if (placeTool==CellType::LineWall || placeTool==CellType::LineMirror) {
            snprintf(buf,sizeof(buf),
                "OUTIL:%s  |  D+clic gauche=point A  relacher=tracer  E=effacer  WHEEL=rayons(%d)  P=preset  C=vider",
                toolName, numRays);
            rect_fill(r,0,H-20,W,20,0,0,0,210);
            drawText(r,4,H-15,buf,180,185,200,1);
        } else {
            snprintf(buf,sizeof(buf),
                "OUTIL:%s  angle:%.0f  |  G=lumiere  D=dessin  R=rotation  Clic droit=placer  E=effacer  WHEEL=rayons(%d)  U=menu  I=aide  P=preset  C=vider",
                toolName, pdeg, numRays);
            rect_fill(r,0,H-20,W,20,0,0,0,210);
            drawText(r,4,H-15,buf,180,185,200,1);
        }

        // ── Overlay aide (I) ─────────────────────────────────
        if (showHelp) {
            // Fond semi-transparent
            rect_fill(r, 80, 30, 1120, 660, 0, 0, 0, 220);
            // Bordure
            SDL_SetRenderDrawColor(r,100,180,255,255);
            SDL_Rect brd{80,30,1120,660}; SDL_RenderDrawRect(r,&brd);

            int col1=100, col2=560, y=48, dy=14, sc=1;
            drawText(r,col1,y,"════ TOUCHES ════",100,200,255,2); y+=28;

            drawText(r,col1,y,"LUMIERE",255,220,80,sc);
            drawText(r,col2,y,"OBJETS (grille)",100,200,255,sc); y+=dy;

            drawText(r,col1,y,"Clic gauche    = placer la lumiere",200,200,200,sc);
            drawText(r,col2,y,"1 = Mur (bloc grille)",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"Clic gauche glisser = suivre souris",200,200,200,sc);
            drawText(r,col2,y,"2 = Miroir (bloc grille)",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"Space = animation orbitale",200,200,200,sc);
            drawText(r,col2,y,"3 = Prisme (disperse en arc-en-ciel)",200,200,200,sc); y+=dy+4;

            drawText(r,col1,y,"MODIFICATEURS",255,220,80,sc);
            drawText(r,col2,y,"COURBES",100,200,255,sc); y+=dy;

            drawText(r,col1,y,"D maintenu + clic/glisser = dessiner",200,200,200,sc);
            drawText(r,col2,y,"4 = Arc de cercle miroir",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"R maintenu = rotation (glisser gauche/droite)",200,200,200,sc);
            drawText(r,col2,y,"5 = Ellipse miroir",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"R + molette = rotation par pas de 5 deg",200,200,200,sc);
            drawText(r,col2,y,"6 = Parabole miroir",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"Clic droit = placer/effacer 1 cellule",200,200,200,sc);
            drawText(r,col2,y,"7 = Ligne droite mur (D + glisser)",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"E = effacer sous la souris",200,200,200,sc);
            drawText(r,col2,y,"8 = Ligne droite miroir (D + glisser)",200,200,200,sc); y+=dy+4;

            drawText(r,col1,y,"CRAYON LIBRE",255,220,80,sc);
            drawText(r,col2,y,"PRESETS (P pour cycler)",100,200,255,sc); y+=dy;

            drawText(r,col1,y,"L = active/desactive le crayon libre",200,200,200,sc);
            drawText(r,col2,y,"0 = Defaut",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"Avec L actif : maintenir clic gauche = tracer",200,200,200,sc);
            drawText(r,col2,y,"1 = Ellipse complete (foyers)",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"Le crayon utilise le type Mur ou Miroir",200,200,200,sc);
            drawText(r,col2,y,"2 = Ellipse + zone d ombre",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"  selon le dernier outil 7 ou 8 choisi",200,200,200,sc);
            drawText(r,col2,y,"3 = Couloir miroirs",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"",200,200,200,sc);
            drawText(r,col2,y,"4 = Parabole + foyer",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"AUTRES",255,220,80,sc);
            drawText(r,col2,y,"5 = Arc-en-ciel",200,200,200,sc); y+=dy;

            drawText(r,col1,y,"Molette = nombre de rayons",200,200,200,sc); y+=dy;
            drawText(r,col1,y,"C = tout effacer",200,200,200,sc); y+=dy;
            drawText(r,col1,y,"TAB = basculer mode 3D",200,200,200,sc); y+=dy;
            drawText(r,col1,y,"Echap = fermer / annuler trace",200,200,200,sc); y+=dy;
            drawText(r,col1,y,"I = afficher/masquer cette aide",200,200,200,sc); y+=dy+8;

            drawText(r,col1,y,"MODE 3D (TAB) : WASD=deplacement  Clic droit=capturer souris",160,160,180,sc); y+=dy;
            drawText(r,col1,y,"               F=animation  +/-=rebonds  Q/E=haut/bas",160,160,180,sc);

            // Centrer "Appuie sur I pour fermer"
            drawText(r,420,660,"[ I ] pour fermer cette aide",120,200,255,2);
        }

        // ══ Menu U ═══════════════════════════════════════════
        if (menuOpen) {
            // Assombrir l'écran
            rect_fill(r, 0, 0, W, H, 0, 0, 0, 160);

            // Donnees des branches
            struct Branch {
                const char* label;
                const char* icon;
                uint8_t cr,cg,cb;
                const char* subs[5];   // jusqu'a 5 sous-branches
                UMode       modes[5];
            };
            static const Branch branches[] = {
                { "BLOCS",   "#", 100,180,255,
                  {"Mur","Miroir","Prisme",nullptr,nullptr},
                  {UMode::BlocWall,UMode::BlocMirror,UMode::BlocPrism,UMode::None,UMode::None} },
                { "LIGNES",  "/", 100,255,160,
                  {"Mur droit","Miroir droit",nullptr,nullptr,nullptr},
                  {UMode::LineWallFree,UMode::LineMirrorFree,UMode::None,UMode::None,UMode::None} },
                { "COURBES", "~", 255,200, 80,
                  {"Courbe Mur","Courbe Miroir",nullptr,nullptr,nullptr},
                  {UMode::CurveWall,UMode::CurveMirror,UMode::None,UMode::None,UMode::None} },
                { "FORMES",  "O", 200,100,255,
                  {"Arc","Ellipse","Parabole","Rectangle","Cercle"},
                  {UMode::ShapeArc,UMode::ShapeEllipse,UMode::ShapeParabola,UMode::ShapeRect,UMode::ShapeCircle} },
                { "LUMIERE", "*", 255,240, 80,
                  {nullptr,nullptr,nullptr,nullptr,nullptr},
                  {UMode::Light,UMode::None,UMode::None,UMode::None,UMode::None} },
            };
            constexpr int NB = 5;

            // Layout : branches disposées en cercle autour du centre
            float cx = W*0.5f, cy = H*0.5f;
            float bradius = 180.f; // rayon du cercle principal
            float bw = 130, bh = 40; // taille bouton branche

            for (int b=0; b<NB; b++) {
                float angle = -PI/2 + b*(2*PI/NB);
                int bx = (int)(cx + cosf(angle)*bradius - bw/2);
                int by = (int)(cy + sinf(angle)*bradius - bh/2);

                bool hov = (menuBranch == b);
                uint8_t alpha = hov ? 240 : 180;
                uint8_t br = branches[b].cr, bg2 = branches[b].cg, bb2 = branches[b].cb;

                // Fond bouton branche
                rect_fill(r, bx, by, (int)bw, (int)bh, br/5, bg2/5, bb2/5, alpha);
                // Bordure
                SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, br, bg2, bb2, hov?255:150);
                SDL_Rect brc{bx,by,(int)bw,(int)bh};
                SDL_RenderDrawRect(r,&brc);
                // Ligne du centre vers le bouton
                SDL_SetRenderDrawColor(r, br/2, bg2/2, bb2/2, 120);
                SDL_RenderDrawLine(r,(int)cx,(int)cy, bx+(int)bw/2, by+(int)bh/2);
                SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

                // Texte branche
                drawText(r, bx+6, by+8,  branches[b].icon, br,bg2,bb2, 2);
                drawText(r, bx+22, by+14, branches[b].label, hov?255:200, hov?255:200, hov?255:200, 1);

                // Sous-branches si branche survolée
                if (hov) {
                    int nSub=0;
                    while(nSub<5 && branches[b].subs[nSub]) nSub++;
                    if (nSub == 0) {
                        rect_fill(r, bx-2, by-2, (int)bw+4, (int)bh+4, br, bg2, bb2, 40);
                    } else {
                        int px2 = (bx > W/2) ? bx - 185 : bx + (int)bw + 10;
                        int py2 = by - 10;
                        int sw=170, sh=32;
                        for (int s=0; s<nSub; s++) {
                            int sy = py2 + s*(sh+4);
                            bool shov = (menuSub == s);
                            rect_fill(r, px2, sy, sw, sh, br/6, bg2/6, bb2/6, shov?230:160);
                            SDL_SetRenderDrawColor(r, br, bg2, bb2, shov?255:120);
                            SDL_Rect src2{px2,sy,sw,sh};
                            SDL_RenderDrawRect(r,&src2);
                            // Couleur icone selon le mode
                            uint8_t mr=200,mg=200,mb=200;
                            UMode subMode = branches[b].modes[s];
                            if(subMode==UMode::BlocWall||subMode==UMode::LineWallFree||subMode==UMode::CurveWall||subMode==UMode::ShapeRect)
                                {mr=160;mg=170;mb=200;}  // gris-bleu = Mur
                            else if(subMode==UMode::BlocMirror||subMode==UMode::LineMirrorFree||subMode==UMode::CurveMirror||subMode==UMode::ShapeArc||subMode==UMode::ShapeEllipse||subMode==UMode::ShapeParabola||subMode==UMode::ShapeCircle)
                                {mr=100;mg=220;mb=255;}  // cyan = Miroir/forme
                            else if(subMode==UMode::BlocPrism)
                                {mr=220;mg=100;mb=255;}  // violet = Prisme
                            drawText(r, px2+8, sy+10, branches[b].subs[s], shov?255:mr, shov?255:mg, shov?255:mb, 1);
                            if(shov) drawText(r, px2+sw-14, sy+10, ">", 255,255,80,1);
                        }
                        SDL_SetRenderDrawColor(r,br,bg2,bb2,80);
                        SDL_RenderDrawLine(r, bx+(bx>W/2?(int)0:(int)bw), by+(int)bh/2,
                                           px2+(bx>W/2?sw:0), py2+nSub*(sh+4)/2);
                    }
                }
            }

            // Centre du menu
            rect_fill(r, (int)cx-50, (int)cy-18, 100, 36, 20,20,30, 220);
            SDL_SetRenderDrawColor(r,100,180,255,200);
            SDL_Rect crc{(int)cx-50,(int)cy-18,100,36};
            SDL_RenderDrawRect(r,&crc);
            drawText(r,(int)cx-18,(int)cy-6,"[ U ]",180,220,255,1);

            // Mode actif en bas
            if(uModeActive) {
                char ubuf[64];
                snprintf(ubuf,sizeof(ubuf),"Mode actif : %s   [ U ] pour changer",uModeName());
                rect_fill(r,0,H-38,W,18,0,0,0,200);
                drawText(r,6,H-33,ubuf,120,220,255,1);
            }
        }

    } // fin render()

    // Mise a jour survol menu (appelee depuis MOUSEMOTION)
    void menuUpdateHover(int mx2, int my2) {
        if (!menuOpen) return;
        // nSub par branche : BLOCS=3, LIGNES=2, COURBES=2, FORMES=5, LUMIERE=0
        static const int nSubByBranch[5] = {3, 2, 2, 5, 0};
        float cx=W*0.5f, cy=H*0.5f, bradius=180.f, bw=130, bh=40;
        int sw=170, sh=32;

        int hitBranch = -1;
        for (int b=0; b<5; b++) {
            float angle=-PI/2+b*(2*3.14159f/5);
            int bx=(int)(cx+cosf(angle)*bradius-bw/2);
            int by=(int)(cy+sinf(angle)*bradius-bh/2);
            if(mx2>=bx&&mx2<=bx+(int)bw&&my2>=by&&my2<=by+(int)bh) {
                hitBranch=b; break;
            }
        }

        // Garde la branche active si la souris est sur le panneau sous-branches
        if (hitBranch < 0 && menuBranch >= 0 && nSubByBranch[menuBranch] > 0) {
            float angle=-PI/2+menuBranch*(2*3.14159f/5);
            int bx=(int)(cx+cosf(angle)*bradius-bw/2);
            int by=(int)(cy+sinf(angle)*bradius-bh/2);
            int px2=(bx>W/2)?bx-185:bx+(int)bw+10;
            int py2=by-10;
            int nSub=nSubByBranch[menuBranch];
            int panelH=nSub*(sh+4);
            int xMin=std::min(bx,px2)-4, xMax=std::max(bx+(int)bw,px2+sw)+4;
            int yMin=std::min(by,py2)-4, yMax=std::max(by+(int)bh,py2+panelH)+4;
            if(mx2>=xMin&&mx2<=xMax&&my2>=yMin&&my2<=yMax)
                hitBranch=menuBranch;
        }

        menuBranch=hitBranch;
        menuSub=-1;

        if (menuBranch>=0 && nSubByBranch[menuBranch]>0) {
            float angle=-PI/2+menuBranch*(2*3.14159f/5);
            int bx=(int)(cx+cosf(angle)*bradius-bw/2);
            int by=(int)(cy+sinf(angle)*bradius-bh/2);
            int px2=(bx>W/2)?bx-185:bx+(int)bw+10;
            int py2=by-10;
            for(int s=0;s<nSubByBranch[menuBranch];s++) {
                int sy=py2+s*(sh+4);
                if(mx2>=px2&&mx2<=px2+sw&&my2>=sy&&my2<=sy+sh) {
                    menuSub=s; break;
                }
            }
        }
    }

    // Clic dans le menu
    bool menuClick(int mx2, int my2) {
        if (!menuOpen) return false;
        struct BranchData { int nSub; UMode modes[5]; };
        static const BranchData bd[] = {
            {3, {UMode::BlocWall,UMode::BlocMirror,UMode::BlocPrism,UMode::None,UMode::None}},
            {2, {UMode::LineWallFree,UMode::LineMirrorFree,UMode::None,UMode::None,UMode::None}},
            {2, {UMode::CurveWall,UMode::CurveMirror,UMode::None,UMode::None,UMode::None}},
            {5, {UMode::ShapeArc,UMode::ShapeEllipse,UMode::ShapeParabola,UMode::ShapeRect,UMode::ShapeCircle}},
            {0, {UMode::Light,UMode::None,UMode::None,UMode::None,UMode::None}},
        };
        float cx=W*0.5f, cy=H*0.5f, bradius=180.f, bw=130, bh=40;
        int sw=170, sh=32;
        for (int b=0; b<5; b++) {
            float angle=-PI/2+b*(2*3.14159f/5);
            int bx=(int)(cx+cosf(angle)*bradius-bw/2);
            int by=(int)(cy+sinf(angle)*bradius-bh/2);
            bool onBranch=(mx2>=bx&&mx2<=bx+(int)bw&&my2>=by&&my2<=by+(int)bh);
            if(onBranch && bd[b].nSub==0) {
                uMode=bd[b].modes[0]; uModeActive=true;
                menuOpen=false; pencilActive=false; lineDrawing=false; shapeDragging=false;
                return true;
            }
            int px2=(bx>W/2)?bx-185:bx+(int)bw+10;
            int py2=by-10;
            for(int s=0;s<bd[b].nSub;s++) {
                int sy=py2+s*(sh+4);
                if(mx2>=px2&&mx2<=px2+sw&&my2>=sy&&my2<=sy+sh) {
                    uMode=bd[b].modes[s]; uModeActive=true;
                    menuOpen=false; pencilActive=false; lineDrawing=false; shapeDragging=false;
                    // Sync outil legacy
                    if(uMode==UMode::BlocWall)       placeTool=CellType::Wall;
                    if(uMode==UMode::BlocMirror)     placeTool=CellType::Mirror;
                    if(uMode==UMode::BlocPrism)      placeTool=CellType::Prism;
                    if(uMode==UMode::LineWallFree)   placeTool=CellType::LineWall;
                    if(uMode==UMode::LineMirrorFree) placeTool=CellType::LineMirror;
                    return true;
                }
            }
        }
        return false;
    }

    void toggleMenu() {
        menuOpen=!menuOpen;
        if(!menuOpen) {
            // Ne désactive PAS uModeActive à la fermeture normale
            menuBranch=-1; menuSub=-1;
        }
    }
    void exitUMode() {
        uModeActive=false; uMode=UMode::None;
        menuOpen=false; pencilActive=false; lineDrawing=false;
        shapeDragging=false; pencilDown=false;
    }

    // ── Event handlers ────────────────────────────────────────
    void handleKey(SDL_Keycode k) {
        if (k==SDLK_1) { placeTool=CellType::Wall;          lineDrawing=false; pencilActive=false; }
        if (k==SDLK_2) { placeTool=CellType::Mirror;        lineDrawing=false; pencilActive=false; }
        if (k==SDLK_3) { placeTool=CellType::Prism;         lineDrawing=false; pencilActive=false; }
        if (k==SDLK_4) { placeTool=CellType::CurveArc;      lineDrawing=false; pencilActive=false; }
        if (k==SDLK_5) { placeTool=CellType::CurveEllipse;  lineDrawing=false; pencilActive=false; }
        if (k==SDLK_6) { placeTool=CellType::CurveParabola; lineDrawing=false; pencilActive=false; }
        if (k==SDLK_7) { placeTool=CellType::LineWall;      lineDrawing=false; pencilActive=false; pencilMirror=false; }
        if (k==SDLK_8) { placeTool=CellType::LineMirror;    lineDrawing=false; pencilActive=false; pencilMirror=true; }
        // L : bascule crayon libre (garde le type mur/miroir du dernier choix)
        if (k==SDLK_l) {
            pencilActive=!pencilActive;
            pencilDown=false;
            // crayon miroir si outil 8 actif, sinon mur
            pencilMirror = (placeTool==CellType::LineMirror);
        }
        if (k==SDLK_i) showHelp=!showHelp;
        if (k==SDLK_u) {
            if (uModeActive) exitUMode(); // U quand mode actif = quitter
            else toggleMenu();
        }
        if (k==SDLK_SPACE) animLight=!animLight;
        if (k==SDLK_p) loadPreset((presetIndex+1) % NUM_PRESETS);
        if (k==SDLK_c) { clearAll(); lineDrawing=false; pencilDown=false; }
    }
    void handleWheel(int dy) {
        numRays=std::clamp(numRays+dy*10,4,720);
    }

    // Placer un objet à la position souris
    void placeAt(int px, int py) {
        int col=px/CELL, row=py/CELL;
        if (col<0||row<0||col>=COLS||row>=ROWS) return;
        switch(placeTool) {
            case CellType::Wall:
            case CellType::Mirror:
            case CellType::Prism:
                grid[row][col] = {placeTool, placeAngle};
                break;
            case CellType::CurveArc: {
                // Place un arc centré sur la cellule cliquée
                float cx=col*CELL+CELL/2.f, cy=row*CELL+CELL/2.f;
                genArc(segments, cx, cy, CELL*2.5f, placeAngle, PI, CellType::Mirror, 20);
                break;
            }
            case CellType::CurveEllipse: {
                float cx=col*CELL+CELL/2.f, cy=row*CELL+CELL/2.f;
                genEllipse(segments, cx, cy, CELL*3.f, CELL*1.8f, placeAngle,
                           0, 2*PI, CellType::Mirror, 60);
                break;
            }
            case CellType::CurveParabola: {
                float cx=col*CELL+CELL/2.f, cy=row*CELL+CELL/2.f;
                genParabola(segments, cx, cy, CELL*1.5f, placeAngle, CELL*2.5f,
                            CellType::Mirror, 30);
                break;
            }
            default: break;
        }
    }

    // ── Crayon libre ─────────────────────────────────────────
    void pencilStart(float px, float py) {
        pencilDown=true;
        pencilPrevX=px; pencilPrevY=py;
    }
    void pencilMoveTo(float px, float py) {
        if (!pencilDown) return;
        float dx=px-pencilPrevX, dy=py-pencilPrevY;
        float len=sqrtf(dx*dx+dy*dy);
        if (len < 2.f) return; // evite micro-segments
        CellType t = pencilMirror ? CellType::Mirror : CellType::Wall;
        Segment s;
        s.x0=pencilPrevX; s.y0=pencilPrevY;
        s.x1=px;          s.y1=py;
        s.type=t; s.angle=0;
        segments.push_back(s);
        pencilPrevX=px; pencilPrevY=py;
    }
    void pencilStop() { pencilDown=false; }

    // ── Outil ligne libre ─────────────────────────────────────
    bool isLineTool() const {
        return placeTool==CellType::LineWall || placeTool==CellType::LineMirror;
    }
    void lineBegin(int px, int py) {
        lineDrawing=true;
        lineStartX=(float)px; lineStartY=(float)py;
        linePreviewX=(float)px; linePreviewY=(float)py;
    }
    void lineUpdatePreview(int px, int py) {
        if (!lineDrawing) return;
        linePreviewX=(float)px; linePreviewY=(float)py;
    }
    void lineCommit(int px, int py) {
        if (!lineDrawing) return;
        lineDrawing=false;
        float ex=(float)px, ey=(float)py;
        float len=sqrtf((ex-lineStartX)*(ex-lineStartX)+(ey-lineStartY)*(ey-lineStartY));
        if (len < 2.f) return; // trop court, ignore
        CellType t = (placeTool==CellType::LineWall) ? CellType::Wall : CellType::Mirror;
        Segment s;
        s.x0=lineStartX; s.y0=lineStartY;
        s.x1=ex;         s.y1=ey;
        s.type=t; s.angle=0;
        segments.push_back(s);
    }
    void lineCancel() { lineDrawing=false; }

    // ── Clic droit : place l'outil courant, ou efface si déjà occupé
    void handleRightClick(int px, int py) {
        int col=px/CELL, row=py/CELL;
        if (col<0||row<0||col>=COLS||row>=ROWS) return;
        // Si la cellule est vide → placer (types grille seulement)
        // Si occupée → effacer
        if (grid[row][col].type == CellType::Empty) {
            // Pour les outils grille, on place directement
            switch(placeTool) {
                case CellType::Wall:
                case CellType::Mirror:
                case CellType::Prism:
                    grid[row][col] = {placeTool, placeAngle};
                    break;
                // Pour les courbes, clic droit place aussi (même chose que D+clic)
                default:
                    placeAt(px, py);
                    break;
            }
        } else {
            // Effacer
            grid[row][col] = {CellType::Empty, 0};
        }
    }

    void handleErase(int mx2, int my2) {
        int col=mx2/CELL, row=my2/CELL;
        if (col>=0&&row>=0&&col<COLS&&row<ROWS) grid[row][col]={CellType::Empty,0};
        // Effacer les segments proches
        float px=(float)mx2, py=(float)my2;
        segments.erase(std::remove_if(segments.begin(), segments.end(),
            [px,py](const Segment& s){
                // Distante point-segment
                float ex=s.x1-s.x0, ey=s.y1-s.y0;
                float len2=ex*ex+ey*ey;
                if (len2<0.001f) return false;
                float t=std::clamp(((px-s.x0)*ex+(py-s.y0)*ey)/len2, 0.f,1.f);
                float dx=px-(s.x0+t*ex), dy=py-(s.y0+t*ey);
                return dx*dx+dy*dy < 20.f*20.f;
            }), segments.end());
    }

    void handleRotateMouse(float dx_mouse) {
        // dx_mouse > 0 = rotation horaire
        placeAngle += dx_mouse * 0.01f;
        // Applique aussi aux cellules sous la souris si elles existent
    }
    void applyRotateToCell(int mx2, int my2, float da) {
        int col=mx2/CELL, row=my2/CELL;
        if (col>=0&&row>=0&&col<COLS&&row<ROWS) {
            if (grid[row][col].type==CellType::Mirror||
                grid[row][col].type==CellType::Prism)
                grid[row][col].angle += da;
        }
        placeAngle += da;
    }
};

// ══════════════════════════════════════════════════════════════
//  3D RAY TRACING
// ══════════════════════════════════════════════════════════════
struct Vec3 {
    float x,y,z;
    Vec3 operator+(Vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    Vec3 operator-(Vec3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    Vec3 operator*(float t) const { return {x*t,y*t,z*t}; }
    Vec3 operator/(float t) const { return {x/t,y/t,z/t}; }
    float dot(Vec3 b) const { return x*b.x+y*b.y+z*b.z; }
    Vec3 cross(Vec3 b) const { return {y*b.z-z*b.y,z*b.x-x*b.z,x*b.y-y*b.x}; }
    float len() const { return sqrtf(x*x+y*y+z*z); }
    Vec3 norm() const { float l=len(); return l>0?(*this)/l:Vec3{0,0,0}; }
    Vec3 reflect(Vec3 n) const { return *this-n*(2.f*dot(n)); }
};
inline Vec3 operator*(float t, Vec3 v){return v*t;}

struct Material3D {
    Vec3  albedo;
    float reflectivity;
    float emission;
    Vec3  emitColor;
    float checkerboard;
};
struct Sphere { Vec3 center; float radius; Material3D mat; };
struct Plane3D { Vec3 normal,point; Material3D mat; };
struct HitInfo { float t; Vec3 pos,normal; Material3D mat; bool hit; };

class Scene3D {
public:
    std::vector<Sphere>  spheres;
    std::vector<Plane3D> planes;
    Vec3 sunDir;
    Vec3 skyTop{0.12f,0.18f,0.35f}, skyHorizon{0.4f,0.55f,0.7f};
    float time=0; bool animate=true; int maxBounces=4;
    Vec3 camPos{0,1.5f,-5}; Vec3 camDir{0,0,1};
    float camYaw=0, camPitch=0;
    static constexpr int RW=640,RH=360;
    uint32_t pixels[RW*RH];
    SDL_Surface* surf=nullptr;

    Scene3D() { sunDir=Vec3{0.6f,1,0.4f}.norm(); buildScene(); surf=SDL_CreateRGBSurfaceWithFormat(0,RW,RH,32,SDL_PIXELFORMAT_RGBA32); }
    ~Scene3D() { if(surf) SDL_FreeSurface(surf); }

    void buildScene() {
        spheres.clear(); planes.clear();
        planes.push_back({Vec3{0,1,0},Vec3{0,0,0},{Vec3{0.8f,0.8f,0.8f},0.15f,0,{},2.f}});
        planes.push_back({Vec3{0,0,-1},Vec3{0,0,12},{Vec3{0.9f,0.9f,1},0.85f,0,{},0}});
        spheres.push_back({Vec3{0,3,6},0.5f,{Vec3{1,0.8f,0.3f},0,1.5f,Vec3{1,0.8f,0.3f}}});
        spheres.push_back({Vec3{-2,0.8f,4},0.8f,{Vec3{0.9f,0.2f,0.2f},0}});
        spheres.push_back({Vec3{0,0.8f,4},0.8f,{Vec3{0.2f,0.6f,0.9f},0.7f}});
        spheres.push_back({Vec3{2,0.8f,4},0.8f,{Vec3{0.1f,0.9f,0.4f},0}});
        spheres.push_back({Vec3{-1,0.4f,2.5f},0.4f,{Vec3{1,0.85f,0.1f},0.9f}});
        spheres.push_back({Vec3{1,0.4f,2.5f},0.4f,{Vec3{0.8f,0.3f,0.9f},0.4f}});
        for (int i=0;i<8;i++) {
            float a=i*PI/4;
            spheres.push_back({Vec3{cosf(a)*3.5f,0.3f+sinf(a*2)*0.3f,sinf(a)*3.5f+4},0.25f,
                {Vec3{0.5f+0.5f*cosf(a),0.5f+0.5f*sinf(a),0.5f},0.2f+0.6f*fabsf(sinf(a*1.3f))}});
        }
    }
    void update(float dt) {
        if (!animate) return; time+=dt;
        if (!spheres.empty()) spheres[0].center={sinf(time*0.7f)*2.5f,3+sinf(time*1.1f)*0.5f,6+cosf(time*0.7f)*1.5f};
        for (int i=0;i<8&&i+5<(int)spheres.size();i++) {
            float a=i*PI/4+time*0.4f;
            spheres[5+i].center={cosf(a)*3.5f,0.3f+sinf(a*2+time)*0.3f,sinf(a)*3.5f+4};
        }
        sunDir=Vec3{0.6f+sinf(time*0.2f)*0.4f,1,0.4f+cosf(time*0.2f)*0.4f}.norm();
    }
    HitInfo intersectScene(Vec3 ro, Vec3 rd) const {
        HitInfo best{1e9f,{},{},{},false};
        for (auto& sp:spheres) {
            Vec3 oc=ro-sp.center; float b=oc.dot(rd),c=oc.dot(oc)-sp.radius*sp.radius,disc=b*b-c;
            if(disc<0)continue; float t=-b-sqrtf(disc); if(t<0.001f)t=-b+sqrtf(disc);
            if(t<0.001f||t>best.t)continue;
            best.t=t; best.pos=ro+rd*t; best.normal=(best.pos-sp.center).norm(); best.mat=sp.mat; best.hit=true;
        }
        for (auto& pl:planes) {
            float denom=pl.normal.dot(rd); if(fabsf(denom)<1e-6f)continue;
            float t=(pl.point-ro).dot(pl.normal)/denom; if(t<0.001f||t>best.t)continue;
            best.t=t; best.pos=ro+rd*t; best.normal=pl.normal; best.mat=pl.mat;
            if(pl.mat.checkerboard>0){float cs=pl.mat.checkerboard; int cx=(int)floorf(best.pos.x/cs)+(int)floorf(best.pos.z/cs); if(cx&1)best.mat.albedo=best.mat.albedo*0.3f;}
            best.hit=true;
        }
        return best;
    }
    bool shadowRay(Vec3 pos, Vec3 dir, float maxT) const {
        for(auto&sp:spheres){if(sp.mat.emission>0)continue;Vec3 oc=pos-sp.center;float b=oc.dot(dir),c=oc.dot(oc)-sp.radius*sp.radius,disc=b*b-c;if(disc<0)continue;float t=-b-sqrtf(disc);if(t>0.001f&&t<maxT)return true;}
        for(auto&pl:planes){float denom=pl.normal.dot(dir);if(fabsf(denom)<1e-6f)continue;float t=(pl.point-pos).dot(pl.normal)/denom;if(t>0.001f&&t<maxT)return true;}
        return false;
    }
    Vec3 skyColor(Vec3 rd) const {
        float t=std::clamp(rd.y*0.5f+0.5f,0.f,1.f);
        float sun=std::max(0.f,rd.dot(sunDir));
        Vec3 sky=skyHorizon*(1-t)+skyTop*t;
        return sky+Vec3{1.4f,1.1f,0.6f}*powf(sun,64)*3.f;
    }
    Vec3 trace(Vec3 ro, Vec3 rd, int depth) const {
        Vec3 accum{0,0,0},mask{1,1,1};
        for(int b=0;b<=depth;b++){
            HitInfo hit=intersectScene(ro,rd);
            if(!hit.hit){Vec3 sky=skyColor(rd);accum=accum+Vec3{mask.x*sky.x,mask.y*sky.y,mask.z*sky.z};break;}
            if(hit.mat.emission>0){Vec3 e=hit.mat.emitColor*hit.mat.emission;accum=accum+Vec3{mask.x*e.x,mask.y*e.y,mask.z*e.z};break;}
            bool inShadow=shadowRay(hit.pos,sunDir,1e6f);
            float diff=inShadow?0.f:std::max(0.f,hit.normal.dot(sunDir));
            Vec3 direct=hit.mat.albedo*(diff+0.08f);
            accum=accum+Vec3{mask.x*direct.x,mask.y*direct.y,mask.z*direct.z}*(1-hit.mat.reflectivity);
            if(hit.mat.reflectivity<=0.001f||b>=depth)break;
            mask=Vec3{mask.x*hit.mat.albedo.x*hit.mat.reflectivity,mask.y*hit.mat.albedo.y*hit.mat.reflectivity,mask.z*hit.mat.albedo.z*hit.mat.reflectivity};
            rd=rd.reflect(hit.normal).norm(); ro=hit.pos+rd*0.002f;
        }
        return accum;
    }
    void renderFrame() {
        Vec3 forward=Vec3{sinf(camYaw)*cosf(camPitch),sinf(camPitch),cosf(camYaw)*cosf(camPitch)}.norm();
        Vec3 right=forward.cross(Vec3{0,1,0}).norm();
        Vec3 up=right.cross(forward).norm();
        float fov=tanf(PI/4);
        for(int y=0;y<RH;y++) for(int x=0;x<RW;x++){
            float u=(2.f*(x+0.5f)/RW-1.f)*((float)RW/RH)*fov;
            float v=(1.f-2.f*(y+0.5f)/RH)*fov;
            Vec3 rd=(forward+right*u+up*v).norm();
            Vec3 col=trace(camPos,rd,maxBounces);
            auto tm=[](float c)->uint8_t{c=c*0.9f;c=c/(1+c);c=powf(std::clamp(c,0.f,1.f),1/2.2f);return(uint8_t)(c*255);};
            pixels[y*RW+x]=(255u<<24)|(tm(col.x)<<16)|(tm(col.y)<<8)|tm(col.z);
        }
        SDL_LockSurface(surf);
        memcpy(surf->pixels,pixels,RW*RH*4);
        SDL_UnlockSurface(surf);
    }
    void render(SDL_Renderer* r) {
        renderFrame();
        SDL_Texture* tex=SDL_CreateTextureFromSurface(r,surf);
        if(tex){SDL_Rect dst{0,0,W,H};SDL_RenderCopy(r,tex,nullptr,&dst);SDL_DestroyTexture(tex);}
        char buf[256];
        snprintf(buf,sizeof(buf),"3D  |  Rebonds:%d[+/-]  |  F=Anim[%s]  |  TAB=2D",maxBounces,animate?"ON":"OFF");
        rect_fill(r,0,H-22,W,22,0,0,0,200);
        drawText(r,6,H-16,buf,200,200,200,1);
    }
    void handleKey(SDL_Keycode k){
        if(k==SDLK_f){animate=!animate;}
        if(k==SDLK_PLUS||k==SDLK_EQUALS)maxBounces=std::min(maxBounces+1,8);
        if(k==SDLK_MINUS)maxBounces=std::max(maxBounces-1,1);
    }
};

// ══════════════════════════════════════════════════════════════
//  MAIN
// ══════════════════════════════════════════════════════════════
int main(int, char**) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window*   win=SDL_CreateWindow("Ray Tracing v2",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,0);
    SDL_Renderer* ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");

    Scene2D scene2d;
    Scene3D scene3d;

    bool mode3d=false, running=true;
    int mx=0,my=0;
    bool camActive=false;
    const uint8_t* keys=SDL_GetKeyboardState(nullptr);

    // États des touches modificatrices
    bool keyD=false;   // maintenir D → dessiner
    bool keyR=false;   // maintenir R → rotation souris
    int  rotPrevX=0;   // position souris précédente pour rotation
    // Note: L (crayon) est géré dans scene2d.pencilActive

    uint64_t prev=SDL_GetPerformanceCounter();
    uint64_t freq=SDL_GetPerformanceFrequency();

    while(running) {
        uint64_t now=SDL_GetPerformanceCounter();
        float dt=(float)(now-prev)/freq; prev=now; dt=std::min(dt,0.05f);

        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            if(ev.type==SDL_QUIT) running=false;

            if(ev.type==SDL_KEYDOWN) {
                SDL_Keycode k=ev.key.keysym.sym;
                if(k==SDLK_ESCAPE) {
                    if(!mode3d && scene2d.lineDrawing) scene2d.lineCancel();
                    else running=false;
                }
                if(k==SDLK_TAB) { mode3d=!mode3d; camActive=false; SDL_SetRelativeMouseMode(SDL_FALSE); }
                if(!mode3d) {
                    scene2d.handleKey(k);
                    if(!scene2d.uModeActive) {
                        if(k==SDLK_d) { keyD=true; }
                        if(k==SDLK_e) scene2d.handleErase(mx,my);
                    }
                    // R (rotation) autorisé aussi en mode U bloc (Mur/Miroir/Prisme)
                    if(!scene2d.uModeActive ||
                       scene2d.uMode==Scene2D::UMode::BlocWall   ||
                       scene2d.uMode==Scene2D::UMode::BlocMirror ||
                       scene2d.uMode==Scene2D::UMode::BlocPrism) {
                        if(k==SDLK_r) { keyR=true; rotPrevX=mx; }
                    }
                } else {
                    scene3d.handleKey(k);
                }
            }
            if(ev.type==SDL_KEYUP) {
                if(ev.key.keysym.sym==SDLK_d) { keyD=false; }
                if(ev.key.keysym.sym==SDLK_r) { keyR=false; }
            }

            if(ev.type==SDL_MOUSEBUTTONDOWN) {
                if(!mode3d) {
                    if(ev.button.button==1) {
                        if(scene2d.menuOpen) {
                            // Clic dans le menu → sélection
                            scene2d.menuClick(ev.button.x, ev.button.y);
                        } else if(scene2d.uModeActive) {
                            // Mode U actif : seul l'action du mode autorisee
                            using UM=Scene2D::UMode;
                            UM m=scene2d.uMode;
                            bool isDrag=(m==UM::LineWallFree||m==UM::LineMirrorFree||
                                         m==UM::CurveWall||m==UM::CurveMirror||
                                         m==UM::ShapeArc||m==UM::ShapeEllipse||m==UM::ShapeParabola||
                                         m==UM::ShapeRect||m==UM::ShapeCircle);
                            if(isDrag) scene2d.uModeBeginDrag((float)ev.button.x,(float)ev.button.y);
                            else       scene2d.uModeClick((float)ev.button.x,(float)ev.button.y);
                        } else if(scene2d.pencilActive) {
                            scene2d.pencilStart((float)ev.button.x,(float)ev.button.y);
                        } else if(scene2d.isLineTool() && keyD) {
                            scene2d.lineBegin(ev.button.x, ev.button.y);
                        } else if(keyD) {
                            scene2d.placeAt(ev.button.x, ev.button.y);
                        } else {
                            scene2d.lightX=(float)ev.button.x;
                            scene2d.lightY=(float)ev.button.y;
                            scene2d.animLight=false;
                        }
                    }
                    if(ev.button.button==3) {
                        if(scene2d.menuOpen) { scene2d.toggleMenu(); } // Clic droit ferme menu
                        else if(!scene2d.uModeActive) {
                            if(scene2d.isLineTool() && scene2d.lineDrawing)
                                scene2d.lineCancel();
                            else if(!keyD && !keyR)
                                scene2d.handleRightClick(ev.button.x, ev.button.y);
                        }
                    }
                } else {
                    if(ev.button.button==3) {
                        camActive=!camActive;
                        SDL_SetRelativeMouseMode(camActive?SDL_TRUE:SDL_FALSE);
                    }
                }
            }

            if(ev.type==SDL_MOUSEBUTTONUP) {
                if(!mode3d && ev.button.button==1) {
                    if(scene2d.uModeActive && !scene2d.menuOpen) {
                        scene2d.uModeEndDrag((float)ev.button.x,(float)ev.button.y);
                    } else if(scene2d.pencilActive) {
                        scene2d.pencilStop();
                    } else if(scene2d.isLineTool() && scene2d.lineDrawing) {
                        scene2d.lineCommit(ev.button.x, ev.button.y);
                    }
                }
            }

            if(ev.type==SDL_MOUSEMOTION) {
                mx=ev.motion.x; my=ev.motion.y;
                if(!mode3d) {
                    bool lbtn=(SDL_GetMouseState(nullptr,nullptr)&SDL_BUTTON(1))!=0;
                    scene2d.lineUpdatePreview(mx, my);
                    scene2d.menuUpdateHover(mx, my);
                    if(scene2d.menuOpen) {
                        // Menu ouvert : rien d'autre
                    } else if(scene2d.uModeActive) {
                        // Rotation R disponible en mode U bloc
                        if(keyR) {
                            float da=(float)(mx - rotPrevX) * 0.02f;
                            scene2d.applyRotateToCell(mx,my, da);
                            rotPrevX=mx;
                        }
                        if(!keyR) {
                            using UM=Scene2D::UMode;
                            UM m=scene2d.uMode;
                            // Preview drag de forme geometrique (pas besoin de clic)
                            if(scene2d.shapeDragging) {
                                scene2d.uModeUpdateDrag((float)mx,(float)my);
                            } else if(lbtn) {
                                if(scene2d.pencilActive)
                                    scene2d.pencilMoveTo((float)mx,(float)my);
                                else if(scene2d.isLineTool() && scene2d.lineDrawing)
                                    scene2d.lineUpdatePreview(mx,my);
                                else if(m==UM::Light) {
                                    scene2d.lightX=(float)mx;
                                    scene2d.lightY=(float)my;
                                } else if(m==UM::BlocWall||m==UM::BlocMirror||m==UM::BlocPrism) {
                                    scene2d.uModeClick((float)mx,(float)my);
                                }
                            }
                        }
                    } else if(scene2d.pencilActive) {
                        if(lbtn) scene2d.pencilMoveTo((float)mx,(float)my);
                    } else if(scene2d.isLineTool()) {
                        if(lbtn && !keyD && !keyR) {
                            scene2d.lightX=(float)mx; scene2d.lightY=(float)my;
                        }
                    } else {
                        if(keyD && lbtn) scene2d.placeAt(mx,my);
                        if(keyR) {
                            float da=(float)(mx - rotPrevX) * 0.02f;
                            scene2d.applyRotateToCell(mx,my, da);
                            rotPrevX=mx;
                        }
                        if(lbtn && !keyD && !keyR)
                        { scene2d.lightX=(float)mx; scene2d.lightY=(float)my; }
                    }
                } else if(camActive) {
                    scene3d.camYaw   += ev.motion.xrel*0.003f;
                    scene3d.camPitch -= ev.motion.yrel*0.003f;
                    scene3d.camPitch  = std::clamp(scene3d.camPitch,-PI*0.45f,PI*0.45f);
                }
            }
            if(ev.type==SDL_MOUSEWHEEL && !mode3d) {
                if(keyR) {
                    // Molette en mode rotation = pas de 5°
                    float da = ev.wheel.y * (5.f * PI / 180.f);
                    scene2d.applyRotateToCell(mx, my, da);
                } else {
                    scene2d.handleWheel(ev.wheel.y);
                }
            }
        }

        // 3D movement
        if(mode3d) {
            Vec3 fwd{sinf(scene3d.camYaw),0,cosf(scene3d.camYaw)};
            Vec3 right{cosf(scene3d.camYaw),0,-sinf(scene3d.camYaw)};
            float spd=4.f*dt;
            if(keys[SDL_SCANCODE_W]) scene3d.camPos=scene3d.camPos+fwd*spd;
            if(keys[SDL_SCANCODE_S]) scene3d.camPos=scene3d.camPos-fwd*spd;
            if(keys[SDL_SCANCODE_A]) scene3d.camPos=scene3d.camPos-right*spd;
            if(keys[SDL_SCANCODE_D]) scene3d.camPos=scene3d.camPos+right*spd;
            if(keys[SDL_SCANCODE_Q]) scene3d.camPos.y-=spd;
            if(keys[SDL_SCANCODE_E]) scene3d.camPos.y+=spd;
            scene3d.update(dt);
        } else {
            scene2d.update(dt);
        }

        SDL_SetRenderDrawColor(ren,0,0,0,255);
        SDL_RenderClear(ren);
        if(!mode3d) scene2d.render(ren, keyD, keyR, scene2d.pencilActive, mx, my);
        else        scene3d.render(ren);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
