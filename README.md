# Ray Tracing Demo  —  2D + 3D

Démo C++ / SDL2 illustrant le ray tracing sous deux angles.

---

## 🔧 Compilation

```bash
# Dépendance : SDL2
sudo apt install libsdl2-dev   # Debian/Ubuntu
brew install sdl2              # macOS

# Compiler
make
# ou directement :
g++ -O2 -std=c++17 -o raytracer src/main.cpp -lSDL2 -lm
```

---

## 🎮 Contrôles

### Global
| Touche | Action |
|--------|--------|
| `Tab`  | Basculer entre mode 2D et 3D |
| `Esc`  | Quitter |

---

### Mode 2D — Editeur de rayons

La source de lumière émet des rayons dans toutes les directions.
Ils rebondissent sur les miroirs, sont bloqués par les murs,
et sont dispersés en arc-en-ciel par les prismes.

| Entrée | Action |
|--------|--------|
| **Clic gauche** (drag) | Déplacer la source lumineuse |
| **Clic droit** | Placer / effacer un objet sur la cellule |
| **Molette** | ± densité de rayons (4 → 720) |
| `1` | Outil : Mur (absorbe les rayons) |
| `2` | Outil : Miroir (réfléchit les rayons) |
| `3` | Outil : Prisme (disperse en couleurs) |
| `R` | Faire pivoter l'objet sous la souris (+15°) |
| `E` | Effacer l'objet sous la souris |
| `Space` | Activer/désactiver l'animation (lumière qui orbite) |

> **Astuce** : positionne deux miroirs face à face pour créer une cavité laser !

---

### Mode 3D — Ray Tracer physique

Rendu en temps réel (640×360 upscalé en 1280×720) avec :
- Réflexions multi-rebonds
- Ombres portées (rayon d'ombre vers le soleil)
- Sol à damier réfléchissant
- Mur miroir en fond
- Orbe lumineux animé
- Anneau de sphères colorées
- Ciel et soleil procéduraux
- Tone mapping filmic (Reinhard) + gamma

| Entrée | Action |
|--------|--------|
| `WASD` | Déplacer la caméra |
| `Q / E` | Descendre / monter |
| **Clic droit** | Activer/désactiver la souris pour regarder |
| **Souris** (mode actif) | Orienter la caméra |
| `F` | Activer/désactiver l'animation de la scène |
| `+` / `-` | Augmenter / réduire le nombre de rebonds (1–8) |

---

## 🏗️ Architecture

```
main.cpp
├── Bitmap font 5×7 (ASCII complet, sans dépendances)
├── Scene2D
│   ├── Grille CELL×CELL, 3 types de cellules
│   ├── castRay()  — marche par pas, détection de hit par cellule
│   │   ├── Wall   → arrêt
│   │   ├── Mirror → réflexion (normal = angle de la cellule)
│   │   └── Prism  → 7 rayons dispersés (violet→rouge)
│   └── Rendu SDL (additive blending pour la lumière)
└── Scene3D
    ├── intersectScene()  — sphères + plans infinis
    ├── shadowRay()       — occlusion vers le soleil
    ├── skyColor()        — dégradé + disque solaire
    ├── trace()           — boucle de rebonds avec masque d'énergie
    └── Tone mapping Reinhard + correction gamma 2.2
```

---

## 💡 Idées d'extension

- **Mode 2D** : source directionnelle (laser), lentilles convergentes/divergentes, milieu absorbant (brouillard coloré)
- **Mode 3D** : path tracing Monte-Carlo (éclairage global), matériaux diélectriques (verre/réfraction), depth of field, light maps, BVH pour accélérer l'intersection
- **Mode bonus** : visualisation du DDA (Digital Differential Analyzer) pour voir comment chaque rayon traverse la grille
