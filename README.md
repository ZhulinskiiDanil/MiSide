# MiSide Menu

[Русский](README.ru.md)

A [Geode](https://geode-sdk.org) mod for Geometry Dash that redoes **only the main menu** (`MenuLayer`) in the style of the MiSide menu: a scene in the background that follows the mouse, a column of text items on the left, soft transitions. Every other screen (levels, pause, level end, the editor, other layers and popups) is left alone.

The background art is in `resources/background/`, the font and the sounds are freely licensed files (see [LICENSES.md](LICENSES.md)); the menu's UI pieces are still placeholders. Every asset is listed in [ASSETS.md](ASSETS.md). Decisions that are the author's to make are in [OPEN_QUESTIONS.md](OPEN_QUESTIONS.md).

## What it does

- **The menu on the left**: a small logo, "MAIN MENU" under it, then a compact column of low purple-pink plates (148×14 pt, one every 16 pt): PLAY, WARDROBE, CREATE, SETTINGS, ACHIEVEMENTS, STATISTICS (in Russian or English, upper case, a condensed bold font). EXIT stands apart, shorter, near the bottom. The whole plate is clickable; hover and the arrow keys light up the same way (the plate's color, no movement), Enter chooses. When the mouse leaves, the keyboard's choice is lit again
- **Rounded type**: M PLUS Rounded 1c Bold for the items, ExtraBold for "MAIN MENU" and the logo, pink-lilac text with a soft shadow on muted translucent lilac plates; the lit row is lighter and pinker with a soft glow
- **One shine** runs through the letters of an item when the mouse comes onto it (or the arrow keys choose it): a wide soft wave of white-pink light, 0.7 of the label wide, from hidden on the left to gone on the right in 0.28 s. It has no line or core: a flat bright middle (0.6 of its half width), then a long smooth fall, so 3–4 neighbouring letters turn near white at once and fade back to their color at its edges. A white copy of the label (without its shadow, with room around it) is rendered once into a texture and drawn with the menu's own shader, so the wave is measured in the whole label's width at any DPI; each item has its own pass, by the time that passed. Under the label and its shadow a second layer glows, like MiSide's bloom: the letters' alpha blurred once into a tight bright halo (about 2.5 pt) and a wide soft one (about 16 pt), laid together, times a wave twice as wide, white, added to the plate. It's white right by the lit letters and fades far out, their halos run together into one cloud; the sharp letters and their shadow are drawn over it
- **Quitting asks in the column**, as in MiSide, not in a popup in the middle: EXIT (and Esc) fade the list, EXIT and "MAIN MENU" out in 0.2 s and "QUIT THE GAME?" in with two wide buttons, YES and NO, under it; the logo, the background and Mita stay (her loop goes on). NO is chosen first; nothing is chosen until it has faded in, and the hidden list takes no click or key. NO, Esc and a controller's B bring the menu back as it was. YES does what GD's quit popup does on Yes: it saves, then ends the game half a second later (no popup, no killed process), once only. In the pause menu the same confirmation asks "QUIT THE LEVEL?" in place of GD's "Exit Level" popup: the pause menu's buttons hide, its darkened (and maybe blurred) level stays; YES leaves the level as the popup's Exit does, NO and Esc go back to the pause menu with the level still paused. The setting "Confirm leaving a level" (on) makes it ask even with GD's own exit confirmation off
- **The pause menu as MiSide's**: the level as it stopped, blurred and tinted pink-lilac behind (a gradient from the left for the menu to read on); "PAUSE" and "MENU" over a column of see-through rows — RESUME, RESTART, PRACTICE / NORMAL MODE, MUSIC and SFX (dark track, pink fill, a knob with the value), SETTINGS, MORE, and QUIT THE LEVEL apart under them; the level's name in a long bar on the right with its numbers under it (now, normal best, practice best; a platformer's time). Only what's available gets a row; every row presses its vanilla button with `activate()` (the volume rows set the vanilla sliders and call their own handlers), so GD's handlers and every mod's hooks run as usual. The same shine as the main menu; no Mita, no logo. MORE shows the mods' pause buttons where they put them, untouched (with their real states), and EDIT / FULL RESTART when GD has them, and can bring back the vanilla pause (a small MISIDE row goes back). Quitting asks in the column ("QUIT THE LEVEL?"). The blur is the mod's own (no BlurAPI): the level is drawn once into a framebuffer when the pause opens (never the pause menu itself), halved to a quarter, blurred lightly with a two-pass Gaussian (the blocks, spikes and ground stay as soft shapes), and kept until the window's size changes; with no blur possible, a plain darkening. The pause's own ambient is ready but off: it needs a file (see below)
- **EXIT leans away** from the mouse as if held to its place by a spring: the closer the mouse, the further (up to 11 pt), stretched and tilted a little, back with a damped spring when the mouse leaves. It always stays under a mouse that aims at it and holds still while pressed, so it's always clickable. Keyboard only: it doesn't move
- **The logo**: an image (`resources/logo/miside-like-gd-logo.png`) shown as it is over "MAIN MENU", its drawn part's left edge with the items, 220 pt wide; its transparent margins are measured from the file, so they don't push it off
- **Each item presses its vanilla button** through `activate()`, so every mod hooking `onPlay`, `onGarage`, `onCreator`, `onOptions` and the others keeps working. No vanilla button, no item
- **The tray**: one row in the bottom right corner, every button scaled to the same small slot: the buttons other mods add (the Geode button too) and the vanilla ones that do something and have no item (the daily chest, Newgrounds). At its end, **"…"** opens a popup with RobTop, the social links and More Games
- **The profile card** in the bottom left corner, small and quiet: a round avatar with the player's cube in their colors, and their name. It opens the profile like the vanilla button (which it presses with `activate()`)
- **Nothing covers the menu**: any other node in its way (a mod's too) is moved to the right of it
- **The background** is dark and pink: the room's layers tinted and dimmed, a darkening, a faint repeating pattern of hearts, stars and flowers, a soft crimson glow on the right, a few slow pink motes, a shade on the left under the menu, a vignette. The layers follow the mouse a little (2 pt)
- **Mita** on the right, as a close-up (head to chest, rendered so by the camera: 960×1080 frames shown at their own scale, the canvas as high as the screen) with a thin pink line around her outline: an animated idle loop (127 frames at 24 fps, from a CC BY 4.0 model, see LICENSES.md) over the background and under every button. She plays from the time that passed, so at the same speed at any frame rate; her clock starts with her first frame shown. Her 64 atlas pages (two frames, 83 ms of her, each) stream: at most three on the GPU (the one for now and the next two, ~24 MiB), decoded ahead one after another on a worker thread with Windows' PNG decoder (at most two finished pages wait, ~16 MiB, plus one being decoded), uploaded on the main thread into three texture slots made once (filled again, never deleted and made anew while she plays: that stalled the driver), freed when the menu closes. Missing or broken files: the menu works without her, the log says why once
- **The transition**: a short darkening, then the game's own transition. No transition is hooked, other screens change exactly as before

## Settings

| Setting | Default | |
| --- | --- | --- |
| Vanilla menu | off | On: the main menu is exactly the vanilla one (with the other mods' buttons where they put them). Applies the next time the main menu opens |
| Parallax strength | 1.0 | 0 to 2. How far the background follows the mouse, 0 keeps it still |
| Menu language | RU | RU or EN, the items' language |
| Logo | on | The logo and "MAIN MENU" above the items |
| Confirm leaving a level | on | The pause menu asks "Quit the level?" before leaving, even with GD's exit confirmation off. Off: GD decides (with its option on, the question still comes in this style) |

## The theme

Colors, fonts, sprites, sounds, spacing and timings all come from the theme (`src/Theme.cpp`). Any of them can be changed without rebuilding: put a `theme.json` into `geode/config/zhulis.miside-menu/` with the keys to change. `theme.defaults.json` next to it lists every key with its default (the mod writes it at the main menu). Colors are hex strings, for example:

```json
{
	"row-color": "#762475",
	"row-hover-color": "#ba168e",
	"item-scale": 0.75,
	"background-darken-opacity": 125,
	"layers": [
		{ "sprite": "zhulis.miside-menu/bg-far.png", "depth": 0.2, "tint": "#8c527e" },
		{ "sprite": "zhulis.miside-menu/bg-near.png", "depth": 1.0, "opacity": 110 },
		{ "sprite": "zhulis.miside-menu/bg-light.png", "depth": 0, "blend": "add", "opacity": 35 }
	]
}
```

New keys (all with defaults; a missing or wrong one keeps its default): `title-font`; `shine-enabled`, `shine-duration` (0.28 s), `shine-band-width` (0.7 of the label's width), `shine-intensity` (0.95), `shine-tint` (its edges), `shine-core-color` (its middle), `shine-core` (the flat middle's share of the half width, 0.6), `shine-slant` (0), `shine-fade-out`, `shine-glow-radius` (the wide halo, 16 pt), `shine-glow-intensity` (3), `shine-glow-inner-radius` (the tight one, 2.5 pt), `shine-glow-inner-intensity` (1.1), `shine-glow-spread` (2: the glow's wave against the letters'), `shine-glow-color` (white), `shine-freeze` (-1; 0..1 holds the wave at that point of the pass while an item is lit, to look at it, set it back to -1 after); `row-glow-color`, `row-glow-opacity`, `row-glow-spread`; `exit-dodge-enabled`, `exit-dodge-radius` (55), `exit-dodge-max` (11), `exit-dodge-vertical`, `exit-dodge-stretch` (0.04), `exit-dodge-tilt` (3°), `exit-dodge-stiffness`, `exit-dodge-damping`; `pattern-sprite`, `pattern-color`, `pattern-opacity` (16), `pattern-tile`; `vignette-sprite`, `vignette-color`, `vignette-opacity`; `particle-count` (14), `particle-color`, `particle-opacity`, `particle-size-min/max`, `particle-speed`, `particle-life`; `logo-sprite`, `logo-left` (59), `logo-bottom` (236), `logo-width` (220: the drawn part's, on screen); `mita-outline-enabled`, `mita-outline-color`, `mita-outline-opacity`, `mita-outline-width`.

The quit confirmation's keys (MiSide's screen measured, moved down 5.6 pt to sit under our logo): `confirm-title-left` (60.2), `confirm-title-cap-top` (227.5), `confirm-title-scale` (0.5: capitals 8.7 pt), `confirm-title-color`, `confirm-title-shadow-color` / `-opacity` / `-offset` (#05050C, 255, 0.65), `confirm-row-left` (59), `confirm-first-row-top` (213.7), `confirm-row-pitch` (16.4), `confirm-row-width` (148.4), `confirm-row-height` (14.9), `confirm-row-color` / `-opacity` (#9F60A1, 100), `confirm-text-color` (#F6A0FD), `confirm-text-font` (the title's), `confirm-text-scale` (0.38: capitals 6.7 pt), `confirm-text-offset`, `confirm-text-padding` (3.6), `confirm-duration` (0.2 s).

The pause menu's keys (`pause-*`, MiSide's pause measured): `pause-enabled`; layout `pause-left` (44.4), `pause-title-left` / `-cap-top` / `-scale` / `-color`, `pause-subtitle-cap-top` / `-scale` / `-color`, `pause-first-row-top` (225.3), `pause-row-pitch` (16.2), `pause-row-width` (148), `pause-row-height` (14.9), `pause-exit-gap` (7.3); colours `pause-row-color` / `-opacity`, `pause-row-hover-color` / `-opacity`, `pause-row-pressed-color` / `-opacity`, `pause-text-color`, `pause-text-hover-color`, `pause-text-scale` / `-offset` / `-padding`; `pause-shine-enabled`; sliders `pause-slider-left` / `-width` / `-height`, `pause-slider-track-color`, `-fill-color`, `-knob-color`, `-knob-width`, `-value-color`, `-value-scale`, `-step` (0.05 for Left/Right), `pause-music-reset` / `pause-sfx-reset` (-1: no reset button; 0..100 adds one that sets that value — GD's own defaults aren't known for sure, so none by default), `pause-reset-size` / `-color`; the bar `pause-bar-left`, `-right-margin`, `-color` / `-opacity`, `-border-color` / `-opacity`, `-text-scale` / `-color`, `pause-stats-scale` / `-height` / `-gap` / `-label-color` / `-value-color` / `-opacity`; blur `pause-blur-enabled`, `pause-blur-downsample` (4: a quarter; 2 or 8), `pause-blur-radius` (3 pt, the Gaussian's sigma on the screen: the level's shapes stay), `pause-blur-debug` (false: saves `pause-capture.png` and `pause-blurred.png` in the config folder to check the capture); `pause-tint-color` / `-opacity` (#B48CC4, 55: a muted lilac), `pause-gradient-color` / `-opacity` / `-width` (#6E4580, 100, 0.5), `pause-dim-color` / `-opacity` (without the blur); ambient `pause-ambient-enabled` (false), `pause-ambient-path` (a path, or a file name in `geode/config/zhulis.miside-menu/`), `pause-ambient-volume` (0.5, times GD's music volume), `pause-ambient-fade` (1 s); `pause-extra-enabled`, `pause-extra-menus` (the menus with mods' buttons shown under MORE, by Geode ID, comma-separated); `pause-duration` (0.2 s).

**The pause's ambient is off: no sound for it was provided.** Put a file (ogg/mp3/wav) in `geode/config/zhulis.miside-menu/`, set its name in `pause-ambient-path` and turn on `pause-ambient-enabled`.

Mita's keys: `mita-enabled` (true), `mita-x` / `mita-y` (414.5, 0: her bottom middle), `mita-scale` (0.2962962963 = 320/1080 points per pixel of her 960×1080 canvas, so as high as the screen; no zoom on top), `mita-fps` (24; 0 takes the manifest's), `mita-parallax-enabled` (false), `mita-parallax-max-x` / `mita-parallax-max-y` (3, 2; down only, so the cut at the bottom never shows a gap), `mita-freeze-frame` (-1 plays; 0…126 holds that frame still, to check frames one at a time). On a narrower window she stays against the right edge and is cut there rather than covering the buttons.

Positions are in points on the 569×320 screen from its bottom left corner (`column-left`, `first-row-y`, `row-pitch` — from one row's middle to the next one's, `exit-y`, `logo-bottom`/`logo-top`, `subtitle-top`, `card-left`/`card-bottom`...). Missing keys keep their defaults, a value of the wrong type is ignored with a warning in the log. The theme is read again every time the main menu opens.

## How it stays compatible

- **Nothing is removed** from `MenuLayer`. The vanilla background, the title, the hints, the buttons that got an item, the profile button and the player's name (the card shows them), and the RobTop / social / More Games menus (the popup shows them) are hidden with `setVisible(false)`: an invisible button can't be clicked or laid out, but `activate()` still runs it
- **No callback is called directly**: the items, the profile card and the popup's buttons press the vanilla buttons with `activate()`
- **The menus stay alive** with their node IDs and layouts (`bottom-menu`, `right-side-menu`, `main-menu`, `side-menu`, `social-media-menu`, `more-games-menu`, `profile-menu`, `close-menu`, `top-right-menu`...). The tray's buttons stay in their menus and keep their callbacks: the tray only scales each one to the slot and places it there. Mods can keep adding buttons and calling `updateLayout()`: when a layout moves a button, the tray puts it back in its slot before the frame is drawn (or the next one)
- **Moved, never hidden**: other mods' nodes in the column's way are moved, not hidden
- **Node IDs are Geode's** (`loader/src/ids/MenuLayer.cpp`). A node that isn't found is skipped with a warning in the log
- **Late**: the `MenuLayer::init` hook runs after the other mods' (its priority), and the tray and the column's space are arranged on the first frame, after every `init`, then again whenever something changes
- Only `MenuLayer` is hooked (`init` and `keyDown`). `keyDown` passes every key on
- Mita takes no touches or keys and goes under every node that shows (her z is one under the lowest visible one that isn't the background, found when the menu is made). Her textures and frames are her own, never in cocos' shared caches; nothing global is purged

## Tests

```
cmake -S tests -B build-tests -G Ninja && cmake --build build-tests && ctest --test-dir build-tests
```

Mita's playback decisions (`src/ui/MitaPlan.hpp`) simulated without the game: the frame for the time at 30, 60, 144 and 240 fps, stutters, decoders from 17 to 150 ms per page against the close-up's 83 ms pages, the 126→0 seam, never more than three pages on the GPU or two finished pages waiting.

## Building

```
geode build --ninja
```

Or `cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build --config RelWithDebInfo`. Needs the [Geode SDK](https://docs.geode-sdk.org/getting-started/sdk) (`GEODE_SDK`) and Geode CLI. Geode 5.10.1, GD 2.2081, Windows.

The UI placeholders are drawn by `python tools/make_placeholders.py` (Pillow). The fonts are [M PLUS Rounded 1c](https://fonts.google.com/specimen/M+PLUS+Rounded+1c) Bold and ExtraBold (SIL Open Font License), the sounds are from [Kenney's UI Audio](https://kenney.nl/assets/ui-audio) (CC0): see [LICENSES.md](LICENSES.md).

## Compatibility checklist

To go through by hand in the game before a release. Each line: what to do, then what should happen.

**The basics**

- [ ] Open the game: a dark scene with a crimson glow on the right, the logo and "MAIN MENU" over a compact column of purple plates on the left, EXIT apart near the bottom, the profile card in the bottom left, the tray's row in the bottom right; the vanilla title, icons background, big buttons, profile button, name and social links aren't visible
- [ ] Every item opens what its vanilla button opens: Play → the level select, Wardrobe → the icon kit, Create → the creator menu, Settings, Achievements, Statistics, Quit → the quit confirmation in the column
- [ ] Hover each item: its whole plate lights up (no movement), the hover sound plays; clicking anywhere on the plate chooses it
- [ ] Arrows move the same highlight over the six rows and EXIT; with the mouse over a row, the arrows start from it; when the mouse leaves the rows, the keyboard's row is lit again; never two lit rows
- [ ] Enter (and a click) press the vanilla button once: no double screen, no double popup, even when pressed quickly twice
- [ ] RU and EN: "ДОСТИЖЕНИЯ" / "ACHIEVEMENTS" fit their plate with room to spare; all items are the same size in both languages
- [ ] Choose Settings and close it: the darkening fades back and the column works again
- [ ] With a popup over the menu (the Geode button's, the "…" one), moving the mouse over the column and pressing the arrows / Enter does nothing behind it
- [ ] Esc on the menu opens the quit confirmation (no popup)
- [ ] Back to the menu from every screen: the menu is the new one each time, with no leftover darkening
- [ ] The log has no warnings from MiSide Menu (`Node ... not found`) on a clean install

**The Geode button and the tray**

- [ ] The Geode button is in the tray and opens Geode's mods list; with mod updates, its badge shows
- [ ] The tray is one row; the daily chest, Newgrounds and the mods' buttons are in it and work; Settings, Achievements and Statistics aren't (they're in the column)
- [ ] Every button in the row is the same size, centered in its slot; pressing one plays its bounce and it returns to the slot's size
- [ ] "…" at the end of the row opens a popup with RobTop, the social links and More Games, each with its usual sprite (a texture pack's too); each opens what it opens without the mod; Esc and the close button close the popup
- [ ] Many mods' buttons: the row stays one row, its slots get smaller instead of running off the screen
- [ ] Clicking empty space where the vanilla Play button was does nothing

**Nothing over the column**

- [ ] The profile card is in the bottom left corner, under EXIT, not touching it: the player's cube in their colors (with the glow if it's on) in a small circle, their name next to it
- [ ] Clicking the card opens your profile, like the vanilla button; with an account name changed, the card shows the new one at the next menu
- [ ] Nothing (a vanilla menu, a mod's button or label) is over the column's items or right next to them; the log says (debug level) which nodes were moved
- [ ] Behind the plates: only the dark background, no stripes; the hearts/stars/flowers pattern barely visible, never over the text or her face; a few motes drifting slowly up
- [ ] If the log says the shine shader doesn't compile: no shine, nothing else changes
- [ ] The window and the curtain on the right aren't washed out; the glow tints them, it doesn't whiten them
- [ ] The logo image over "MAIN MENU": its drawn left edge in line with the items, not touching the subtitle or Mita, its proportions kept, no grid, shadow or sparkles over it; as sharp at Low, Medium and High

**Mods that add buttons or popups to the main menu** (each alone, then all together)

- [ ] **Globed**: its main menu button is in the tray (or where it put it) and opens Globed; its popups block the column
- [ ] **Texture Loader**: a pack that changes the menu's sprites doesn't bring the hidden vanilla pieces back; its button (if it adds one) works
- [ ] **GDPS Switcher** (or another server switcher): its main menu button is in the tray and works
- [ ] **Custom Keybinds**: its keybinds work on the menu; the arrows and Enter still move and choose in the column
- [ ] **Eclipse Menu**, **Mega Hack** or **QOLMod**: their overlay and floating button stay on top and clickable; their menu keys work on the main menu
- [ ] **AskDash**: its button (it adds it to `bottom-menu`) is in the tray and opens its chat
- [ ] **Icon Mayhem**: its accessories are on the vanilla profile button, which is hidden: the card shows the cube without them (see OPEN_QUESTIONS.md); with *Vanilla menu* on they're back
- [ ] **Crash Doctor**: after a crash, its report over the main menu blocks the column; closing it gives the column back
- [ ] **Zhulis Mods Manager**: its update popup over the menu blocks the column
- [ ] **Improved Logger**: its console key works on the menu
- [ ] Any mod hooking `MenuLayer::onPlay`, `onGarage`, `onCreator` or `onOptions`: its hook runs when the item is chosen, exactly as with the vanilla button
- [ ] A mod that moves the vanilla buttons to another menu: the items still find them (the log names any that weren't found)
- [ ] A mod that redoes the main menu too: no crash; with *Vanilla menu* on, that mod's menu is back

**Hover and EXIT**

- [ ] Mid-pass, a soft pink halo glows around the letters the band is on, past their edges, only near the band; the label stays sharp; no halo before or after the pass
- [ ] Move onto each item: one white-and-pink band runs through its letters, left to right, in 0.28 s, clearly visible the first time, and stops; inside it the letters turn white; no light outside the letters, no rectangle; short (PLAY) and long (ACHIEVEMENTS) labels alike, at Low, Medium and High
- [ ] Keep the mouse still on an item: no second pass; move off and back: a new one; two items quickly: each its own pass, no light left on either
- [ ] Run the mouse quickly up and down over all the items: no shine left glowing, nothing piling up; moving back onto an item starts a new one
- [ ] The arrow keys: the same highlight and one shine per move
- [ ] `shine-freeze` 0.25 / 0.5 / 0.75 on «ГАРДЕРОБ» and «ИГРАТЬ»: no line or stroke, several near-white letters in one soft cloud of light, the letters sharp over it; back to -1 and it moves again
- [ ] Bring the mouse near EXIT from each side: it leans away, more the closer the mouse is, stretched and tilted a little, then springs back with a little bounce when the mouse leaves; no jitter with the mouse resting on it, no jumps
- [ ] Click EXIT from every side, slowly and quickly: every click lands and the confirmation opens; pressing and dragging off it cancels as usual
- [ ] At 30, 60 and 144 fps EXIT moves the same way
- [ ] Use only the keyboard to reach EXIT: it doesn't move; Enter opens the confirmation
- [ ] A popup over the menu: EXIT springs back home and doesn't follow the mouse under the popup

**Quitting**

- [ ] EXIT: in about 0.2 s the list, EXIT and "MAIN MENU" fade out and "ВЫЙТИ ИЗ ИГРЫ?" with ДА and НЕТ fades in where they were, as in MiSide's screenshot (white title with a dark shadow, two wide see-through plates, light pink text); the logo, the background and Mita stay, her loop doesn't restart
- [ ] НЕТ is lit (with its shine) when it opens; the click that opened it chooses nothing; Enter held from opening it doesn't quit
- [ ] The mouse over ДА and НЕТ lights them with the shine; the hidden rows and EXIT take no click (click where they were); EXIT doesn't lean away
- [ ] Up/Down (and a controller's d-pad or stick) move between ДА and НЕТ; Enter, Space or A chooses; a controller's A, X, Y do nothing else (no level select, no icon kit)
- [ ] НЕТ, Esc, a controller's B: the menu comes back as it was, the rows work, EXIT leans away again; reached by the keys, EXIT is chosen again. Open and cancel it many times, quickly too
- [ ] ДА: the game saves and closes as with GD's own popup (no second popup, no crash); clicking ДА twice or pressing Enter right after does nothing more. Reopen the game: what changed before quitting is kept
- [ ] In a level, pause, EXIT: the pause buttons hide, "ВЫЙТИ ИЗ УРОВНЯ?" with ДА and НЕТ shows over the darkened level (no Mita, no logo); the level doesn't move on
- [ ] НЕТ and Esc: the pause menu comes back, the level still paused; Esc again resumes as usual
- [ ] ДА: back to the level's page as with GD's Exit, once; with "Confirm leaving a level" off and GD's exit confirmation off it leaves at once, as without the mod
- [ ] With a mod that blurs the pause background, the blur stays while the confirmation is open

**The pause menu**

- [ ] Pause a level (normal mode, practice, a platformer, a level with a start position): the blurred, pink-lilac level behind, no sharp frame flashing first; "ПАУЗА", "МЕНЮ", the rows and the bar as in MiSide's screenshot; no huge dark panel, no green buttons, no vanilla sliders
- [ ] The rows and text are sharp (only the background is blurred); the pause menu itself isn't in the blurred picture; light and dark levels: the menu reads, the level's big colours still show on the right
- [ ] The log says once a pause: `Pause blur: … ms to submit, … ms with the GPU done; render targets … MiB at most, … MiB kept` — note the numbers; with a mod that blurs the pause too, no double blur or darkening shows (ours covers it; the other one still runs)
- [ ] Resize the window while paused, change texture quality: the picture is made again, no stretched or dark band at the edges, not upside down
- [ ] Hover each row: it gets lighter with one shine pass; still mouse: no second pass; keyboard Up/Down and a controller's d-pad / stick move the highlight, Enter / Space / A run it, Left/Right move a slider
- [ ] RESUME, RESTART, PRACTICE / NORMAL MODE do what GD's buttons do; the music stays in sync after resuming
- [ ] MUSIC and SFX: drag the knob, click the track, Left/Right: the value changes, GD's volume changes (hear it), and it's kept after the pause; SETTINGS opened over the pause and its volume changed: the sliders show the new value
- [ ] MORE: the mods' pause buttons (Geode, Globed, others) show where they were and work, toggles keep their state; EDIT / FULL RESTART when GD has them; BACK and Esc return; VANILLA PAUSE shows GD's pause as it was, MISIDE (bottom left) brings ours back
- [ ] QUIT THE LEVEL: "ВЫЙТИ ИЗ УРОВНЯ?", ДА, НЕТ under "ПАУЗА", the blur stays; НЕТ and Esc go back to the pause (the level stays paused); ДА leaves the level once
- [ ] Open and close the pause many times, quickly too: no leak (memory in Task Manager), no stuck sound, no doubled nodes
- [ ] `"pause-enabled": false`: the vanilla pause (with the quit confirmation in this style)

**Mita**

- [ ] She's a close-up as in the set's `preview.png`: head to chest, the whole head on screen, her hands and lower chest cut by the bottom edge; her eyes, face and chin never cut during the whole loop; her drawing right of the menu
- [ ] A thin pink line around her outline, no rectangle around her, no glow over her face; her details as sharp as before
- [ ] Still frames first: `"mita-freeze-frame": 0`, then 1, 2 (the first frame of the second atlas page), 63 and 126 (the last page's only frame), with parallax off. Each one is the whole portrait, as in `preview.gif`, the same size and place each time. The log (debug) says `texture rect 960x1080 px`, `on screen 284.444x320.000 at (414.50, 0.00)` on a 16:9 window, with no "she's drawn" warning
- [ ] Then `-1` to play: the same portrait moving, no jumps between frames 1 and 2 or 126 and 0
- [ ] She stands on the right, her feet cut by the bottom edge, over the room and under the tray, the card, the column and every mod's button; nothing she covers stops being clickable
- [ ] Her loop is smooth at 30, 60 and 144+ fps (cap the frame rate in GD's settings) and takes the same 5.3 s each time; no jump at the loop's seam
- [ ] On a dark background her edges have no light or dark fringe
- [ ] Texture quality Low / Medium / High: the same size on screen every time (as high as the screen)
- [ ] A narrower window (4:3) and resizing it: she keeps to the right edge, never over the buttons
- [ ] The log (debug level) shows each page's decode and upload time and "3 texture slots" at most; decode well under 83 ms (17 ms measured outside the game), upload a few ms; no "Slow frame" in `MSMita::freePages` or `MSMita::upload` from the mod profiler
- [ ] Open and leave the menu many times (and leave it right as it opens): no crash, no growth in the game's memory in Task Manager; the log says the pages were freed
- [ ] Rename `mita-closeup-manifest.json` or one atlas in the installed mod's resources: the menu works without her, one error in the log
- [ ] `"mita-enabled": false` in theme.json: no Mita, nothing loaded

**The Vanilla menu setting**

- [ ] Turn it on, open another screen and come back: the menu is exactly the vanilla one (title, icons background, big buttons, profile button and name, social links, the other mods' buttons where they put them), no column, no tray, no card, no background
- [ ] Turn it off and come back: the new menu returns

**Window and quality**

- [ ] Aspect ratios 16:9, 16:10, 4:3, 21:9 and a small window: the background covers the screen with no edges showing at full parallax strength; the column, the profile and the tray stay on screen and don't overlap (a short window makes the column smaller)
- [ ] The light overlay (`bg-light.png`) barely brightens the scene and doesn't cover it
- [ ] A narrow window (4:3): the menu, the card and the tray keep their size and don't overlap; the background is cropped around its middle, not stretched
- [ ] Parallax strength 0: the background is still; 2: it moves more, still no edges
- [ ] Texture quality Low, Medium and High (restart after each): the background, the plates, the tray, the card and the font are sharp and the right size
