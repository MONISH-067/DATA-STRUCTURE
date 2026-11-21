# DDD — Datastructure Duplicate File Detection System
## Brand Identity & Logo Guidelines

---

### 1. Brand Concept & Story
**DDD** stands for **Datastructure Duplicate file Detection System**.
The identity captures both the computational data structure algorithms powering the engine and the user-facing capability of detecting redundant files:
- **Three Geometric "D" Letters**: Each layer forms a clean, architectural capital letter "D" representing the three pillars: **Data Structures**, **Duplicate Detection**, and **De-duplication**.
- **File Sheet Anatomy**: The left stems feature 45° corner chamfers and a folded dog-ear flap on the front file, instantly communicating document and file storage.
- **Layered Cascade**: The three files step diagonally in isometric depth. The back two files represent redundant duplicate copies detected across directory trees, while the front high-contrast file represents the single preserved canonical original.
- **8px Negative Space Channels**: Clean knockout channels ensure optical separation across every scale, whether rendered in full color, high-contrast dark mode, or 1-color monochrome.

---

### 2. Primary Color Palette

| Token | Name | HEX | RGB | Purpose |
|---|---|---|---|---|
| `--color-primary` | **Brand Blue** | `#2563EB` | `37, 99, 235` | Front preserved file (canonical hero), primary CTA |
| `--color-accent` | **Royal Deep** | `#1D4ED8` | `29, 78, 216` | Folded dog-ear flap shadow, active interactive states |
| `--color-slate-dark` | **Deep Slate** | `#0F172A` | `15, 23, 42` | Primary dark surface, monochrome master silhouette |
| `--color-slate-mid` | **Medium Slate** | `#475569` | `71, 85, 105` | Middle duplicate copy, secondary UI hierarchy |
| `--color-slate-light` | **Muted Slate** | `#94A3B8` | `148, 163, 184` | Rear duplicate copy, borders, metadata badges |
| `--color-surface` | **Pure White** | `#FFFFFF` | `255, 255, 255` | Reversed mark, clean page background |

---

### 3. Clear Space & Minimum Sizing
- **Clear Space**: Maintain a minimum exclusion zone equal to the width of the letter stem ($S = 20\text{ px}$ on a 256px canvas, or $\approx 20\%$ of mark height) on all four sides. No typography, icons, or interface elements may intrude into this boundary.
- **Minimum Digital Size**:
  - **Favicon / Browser Tab**: $16 \times 16\text{ px}$ (use dedicated `ddd-favicon.svg`).
  - **App Icon / System Tray**: $24 \times 24\text{ px}$ to $32 \times 32\text{ px}$.
  - **Application Header**: $48 \times 48\text{ px}$ (symbol) or $220 \times 48\text{ px}$ (horizontal lockup).
  - **High-Resolution Print / Hero**: $512\text{ px} +$.

---

### 4. Lockups & Asset Directory Map

```
logo/
├── ddd-symbol.svg               # Master brand color vector symbol (256x256)
├── ddd-symbol-mono.svg          # 1-color pure black master vector symbol
├── ddd-horizontal.svg           # Primary horizontal lockup (840x256)
├── ddd-stacked.svg              # Stacked vertical lockup (320x420)
├── ddd-favicon.svg              # Micro-optimized 32x32 favicon cut
├── presentation.html            # Interactive 5-slide identity showcase
├── presentation-spec.json       # Toolchain generation specification
├── concepts.png                 # Phase 6 concept comparison sheet
│
├── dist/                        # Full production delivery package
│   ├── ddd-symbol-black.svg     # Solid black vector
│   ├── ddd-symbol-white.svg     # Reversed solid white vector
│   ├── ddd-symbol-mono-2563eb.svg # Brand blue monochrome vector
│   ├── ddd-symbol-square.svg    # Padded square vector avatar
│   ├── ddd-symbol-app-icon.svg  # App icon tile on deep slate
│   ├── favicon.ico              # Multi-resolution ICO (16/32/48px)
│   ├── favicon.svg              # Modern SVG favicon for modern browsers
│   ├── apple-touch-icon.png     # 180x180 iOS touch icon
│   ├── icon-192.png             # Android / PWA standard icon
│   ├── icon-512.png             # Android / PWA high-res splash icon
│   ├── maskable-512.png         # PWA adaptive maskable icon
│   ├── site.webmanifest         # Ready-to-use webmanifest
│   ├── head-snippet.html        # HTML <head> snippet for copy-paste
│   └── *.png                    # Transparent PNGs at 32, 64, 128, 256, 512, 1024px
│
└── slides/                      # Rendered presentation slide PNGs
    ├── slide-01.png             # Cover slide
    ├── slide-02.png             # Project brief & adjectives
    ├── slide-03.png             # Concept breakdown & size ladder
    ├── slide-04.png             # In-context mockups (Terminal, README, App, Web)
    └── slide-05.png             # Final delivery overview
```

---

### 5. Integration with Desktop GUI
To display the new logo in the pywebview GUI application, add the following to `gui.html`'s header:
```html
<link rel="icon" type="image/svg+xml" href="logo/dist/favicon.svg">
<link rel="icon" type="image/png" sizes="32x32" href="logo/dist/favicon-32.png">
```
And replace the generic folder icon in the sidebar brand header with the master mark:
```html
<img src="logo/ddd-symbol.svg" width="32" height="32" alt="DDD Logo" class="brand-logo" />
```
