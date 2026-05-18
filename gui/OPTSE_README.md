# OPTSE Ultra Engine Theme
## by AKRO (github.com/akro7)

### What is OPTSE?
OPTSE (Optimal Performance Touch-Screen Engine) is a revolutionary merged recovery theme  
built by combining the best features from three major Android recovery projects:

- **guitwrp** — TWRP (Team Win Recovery Project) GUI source
- **guiofox** — OrangeFox Recovery GUI
- **guipbrp** — PBRP (PitchBlack Recovery Project) GUI

---

### Revolutionary Design Features
- **Color Palette**: Deep Space Black (#060B18) + Electric Cyan (#00D4FF) + Neon Purple (#7C3AED)
- **Status Bar**: OrangeFox-inspired top status bar with version and branding
- **Dual Accent Lines**: Cyan top bar + Purple bottom signature line
- **OPTSE Build Tag**: Signature "OPTSE · ULTRA ENGINE · AKRO" displayed on main screen

---

### Merged Features by Source

#### From OrangeFox (guiofox)
- `battery.cpp` — Battery level monitoring
- `gesture.cpp` — Gesture-based navigation
- `nanosvg.cpp/h` — SVG rendering engine
- Premium fonts: GoogleSans, EuclidFlex, FiraCode, Exo2, InterDisplay, RobotoSlab
- Extra languages: Romanian (ro.xml), Ukrainian (ua.xml), Czech (cs.xml)

#### From PBRP (guipbrp)
- `readfile` page — Read any file in File Manager
- `zipname` page — Name a zip when compressing files
- `mkd` page — Create new folders in File Manager
- `choosedestinationfolderforunzip` page — Select unzip destination
- `reboot_recovery_routine` page — Dedicated reboot-to-recovery flow
- `appcheck` page — App installation check before reboot
- `rebootapp` page — Install OPTSE app prompt on reboot
- `filter_` / `filter_new` / `filter_backup` pages — Storage filter overlays
- Extra languages: Japanese (ja.xml), Korean (ko.xml), Chinese (zh_CN/zh_TW), Indonesian, Sinhala

#### From TWRP (guitwrp)
- Complete base GUI engine (all 88 original pages)
- Full settings: Screen, Vibration, Language, Timezone
- Complete backup/restore/wipe/mount/advanced flows

---

### Font Library (23 fonts)
| Font | Source |
|------|--------|
| GoogleSans Regular/Medium | OrangeFox |
| EuclidFlex Regular/Medium | OrangeFox |
| FiraCode Regular/Medium | OrangeFox |
| Exo2 Regular/Medium | OrangeFox |
| InterDisplay Regular/Medium | OrangeFox |
| RobotoSlab | OrangeFox |
| Chococooky | OrangeFox |
| RoboNoto Medium | PBRP |
| Roboto Regular/Medium | PBRP/OrangeFox |
| DroidSansMono | TWRP |
| DroidSansFallback | TWRP/PBRP |
| NotoSansCJKjp | TWRP |
| RobotoCondensed | TWRP |

---

### Language Support (28 languages)
cs, cz, de, el, en, es, fr, hu, id, in, it, ja, ko, lk, nl, pl,
pt_BR, pt_PT, ro, ru, sk, sl, sv, tr, ua, uk, zh_CN, zh_TW

---

### Total Pages: 95
(88 TWRP base + 7 PBRP exclusive)

---

### Text Changes
All instances of "Team Win Recovery Project" → **OPTSE Recovery**  
All UI display text "TWRP" → **OPTSE**  
(Internal variable names like `tw_*` are preserved for compatibility)
