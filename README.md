# MiSide Menu

[Русский](README.ru.md)

A [Geode](https://geode-sdk.org) mod for Geometry Dash that redoes **only the main menu** (`MenuLayer`) in the style of the MiSide menu: a scene in the background that follows the mouse, a column of text items on the left, soft transitions. Every other screen (levels, pause, level end, the editor, other layers and popups) is left alone.

The background art is in `resources/background/`, the font and the sounds are freely licensed files (see [LICENSES.md](LICENSES.md)); the menu's UI pieces are still placeholders. Every asset is listed in [ASSETS.md](ASSETS.md). Decisions that are the author's to make are in [OPEN_QUESTIONS.md](OPEN_QUESTIONS.md).

## What it does

- **The menu on the left**: a small logo, "MAIN MENU" under it, then a compact column of low purple-pink plates (148×14 pt, one every 16 pt): PLAY, WARDROBE, CREATE, SETTINGS, ACHIEVEMENTS, STATISTICS (in Russian or English, upper case, a condensed bold font). EXIT stands apart, shorter, near the bottom. The whole plate is clickable; hover and the arrow keys light up the same way (the plate's color, no movement), Enter chooses. When the mouse leaves, the keyboard's choice is lit again
- **Rounded type**: M PLUS Rounded 1c Bold for the items, ExtraBold for "MAIN MENU" and the logo, pink-lilac text with a soft shadow on muted translucent lilac plates; the lit row is lighter and pinker with a soft glow
- **One shine** runs through the letters of an item when the mouse comes onto it (or the arrow keys choose it): a wide soft wave of white-pink light, 0.7 of the label wide, from hidden on the left to gone on the right in 0.28 s. It has no line or core: a flat bright middle (0.6 of its half width), then a long smooth fall, so 3–4 neighbouring letters turn near white at once and fade back to their color at its edges. A white copy of the label (without its shadow, with room around it) is rendered once into a texture and drawn with the menu's own shader, so the wave is measured in the whole label's width at any DPI; each item has its own pass, by the time that passed. Under the label and its shadow a second layer glows, like MiSide's bloom: the letters' alpha blurred once into a tight bright halo (about 2.5 pt) and a wide soft one (about 16 pt), laid together, times a wave twice as wide, white, added to the plate. It's white right by the lit letters and fades far out, their halos run together into one cloud; the sharp letters and their shadow are drawn over it
- **EXIT leans away** from the mouse as if held to its place by a spring: the closer the mouse, the further (up to 11 pt), stretched and tilted a little, back with a damped spring when the mouse leaves. It always stays under a mouse that aims at it and holds still while pressed, so it's always clickable. Keyboard only: it doesn't move
- **The logo**: an image (`resources/logo/miside-like-gd-logo.png`) shown as it is over "MAIN MENU", its drawn part's left edge with the items, 220 pt wide; its transparent margins are measured from the file, so they don't push it off
- **Each item presses its vanilla button** through `activate()`, so every mod hooking `onPlay`, `onGarage`, `onCreator`, `onOptions` and the others keeps working. No vanilla button, no item
- **The tray**: one row in the bottom right corner, every button scaled to the same small slot: the buttons other mods add (the Geode button too) and the vanilla ones that do something and have no item (the daily chest, Newgrounds). At its end, **"…"** opens a popup with RobTop, the social links and More Games
- **The profile card** in the bottom left corner, small and quiet: a round avatar with the player's cube in their colors, and their name. It opens the profile like the vanilla button (which it presses with `activate()`)
- **Nothing covers the menu**: any other node in its way (a mod's too) is moved to the right of it
- **The background** is dark and pink: the room's layers tinted and dimmed, a darkening, a faint repeating pattern of hearts, stars and flowers, a soft crimson glow on the right, a few slow pink motes, a shade on the left under the menu, a vignette. The layers follow the mouse a little (2 pt)
- **Mita** on the right, as a close portrait (face and shoulders, the top of her hair past the top edge) with a thin pink line around her outline: an animated idle loop (127 frames at 24 fps, from a CC BY 4.0 model, see LICENSES.md) over the background and under every button. She plays from the time that passed, so at the same speed at any frame rate. Her 22 atlas pages stream: at most two on the GPU (~21 MiB), decoded ahead on a worker thread with Windows' PNG decoder, uploaded on the main thread, freed when the menu closes. Missing or broken files: the menu works without her, the log says why once
- **The transition**: a short darkening, then the game's own transition. No transition is hooked, other screens change exactly as before

## Settings

| Setting | Default | |
| --- | --- | --- |
| Vanilla menu | off | On: the main menu is exactly the vanilla one (with the other mods' buttons where they put them). Applies the next time the main menu opens |
| Parallax strength | 1.0 | 0 to 2. How far the background follows the mouse, 0 keeps it still |
| Menu language | RU | RU or EN, the items' language |
| Logo | on | The logo and "MAIN MENU" above the items |

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

Mita's keys: `mita-enabled` (true), `mita-x` / `mita-y` (414.5, 0: her bottom middle), `mita-scale` (0.4444444444 points per pixel of her 640×720 canvas, so as high as the screen), `mita-fps` (24; 0 takes the manifest's), `mita-parallax-enabled` (false), `mita-parallax-max-x` / `mita-parallax-max-y` (3, 2; down only, so the cut at the bottom never shows a gap), `mita-freeze-frame` (-1 plays; 0…126 holds that frame still, to check frames one at a time). On a narrower window she stays against the right edge and is cut there rather than covering the buttons.

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

Mita's playback decisions (`src/ui/MitaPlan.hpp`) simulated without the game: the frame for the time at 30, 60, 144 and 240 fps, stutters, decoders from 60 to 400 ms per page, the 126→0 seam, never more than two pages on the GPU.

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
- [ ] Every item opens what its vanilla button opens: Play → the level select, Wardrobe → the icon kit, Create → the creator menu, Settings, Achievements, Statistics, Quit → the "quit?" popup
- [ ] Hover each item: its whole plate lights up (no movement), the hover sound plays; clicking anywhere on the plate chooses it
- [ ] Arrows move the same highlight over the six rows and EXIT; with the mouse over a row, the arrows start from it; when the mouse leaves the rows, the keyboard's row is lit again; never two lit rows
- [ ] Enter (and a click) press the vanilla button once: no double screen, no double popup, even when pressed quickly twice
- [ ] RU and EN: "ДОСТИЖЕНИЯ" / "ACHIEVEMENTS" fit their plate with room to spare; all items are the same size in both languages
- [ ] Choose Settings and close it: the darkening fades back and the column works again
- [ ] With a popup over the menu (the quit popup), moving the mouse over the column and pressing the arrows / Enter does nothing behind it
- [ ] Esc on the menu still shows the quit popup
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
- [ ] Click EXIT from every side, slowly and quickly: every click lands and the quit popup opens; pressing and dragging off it cancels as usual
- [ ] At 30, 60 and 144 fps EXIT moves the same way
- [ ] Use only the keyboard to reach EXIT: it doesn't move; Enter quits
- [ ] A popup over the menu: EXIT springs back home and doesn't follow the mouse under the popup

**Mita**

- [ ] She's a close portrait: face and shoulders, the top of her hair cut by the top edge, her chest by the bottom one; her face never cut during the whole loop; her drawing right of the menu
- [ ] A thin pink line around her outline, no rectangle around her, no glow over her face; her details as sharp as before
- [ ] Still frames first: `"mita-freeze-frame": 0`, then 5, 6 (the first frame of the second atlas page) and 126, with parallax off. Each one is the whole portrait, as in `preview.gif`, the same size and place each time. The log (debug) says `texture rect 640x720 px`, `on screen 284.444x320.000 at (414.50, 0.00)` on a 16:9 window, with no "she's drawn" warning
- [ ] Then `-1` to play: the same portrait moving, no jumps between frames 5 and 6 or 126 and 0
- [ ] She stands on the right, her feet cut by the bottom edge, over the room and under the tray, the card, the column and every mod's button; nothing she covers stops being clickable
- [ ] Her loop is smooth at 30, 60 and 144+ fps (cap the frame rate in GD's settings) and takes the same 5.3 s each time; no jump at the loop's seam
- [ ] On a dark background her edges have no light or dark fringe
- [ ] Texture quality Low / Medium / High: the same size on screen every time (as high as the screen)
- [ ] A narrower window (4:3) and resizing it: she keeps to the right edge, never over the buttons
- [ ] The log (debug level) shows each page's decode and upload time and "2 pages on the GPU" at most; check the times on your machine
- [ ] Open and leave the menu many times (and leave it right as it opens): no crash, no growth in the game's memory in Task Manager; the log says the pages were freed
- [ ] Rename `mita-manifest.json` or one atlas in the installed mod's resources: the menu works without her, one error in the log
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
