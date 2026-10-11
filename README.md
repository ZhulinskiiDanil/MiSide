# MiSide Menu

[Русский](README.ru.md)

A [Geode](https://geode-sdk.org) mod for Geometry Dash that redoes **only the main menu** (`MenuLayer`) in the style of the MiSide menu: a scene in the background that follows the mouse, a column of text items on the left, soft transitions. Every other screen (levels, pause, level end, the editor, other layers and popups) is left alone.

The background art is in `resources/background/`, the font and the sounds are freely licensed files (see [LICENSES.md](LICENSES.md)); the menu's UI pieces are still placeholders. Every asset is listed in [ASSETS.md](ASSETS.md). Decisions that are the author's to make are in [OPEN_QUESTIONS.md](OPEN_QUESTIONS.md).

## What it does

- **The column**: "Geometry Dash" above Play, Wardrobe, Create, Settings, Achievements, Statistics, Quit (in Russian or English), in the theme's font with a soft shadow. Hover with the mouse, or move with the arrow keys and choose with Enter
- **Each item presses its vanilla button** through `activate()`, so every mod hooking `onPlay`, `onGarage`, `onCreator`, `onOptions` and the others keeps working. No vanilla button, no item
- **The tray**: one row in the bottom right corner, every button scaled to the same small slot: the buttons other mods add (the Geode button too) and the vanilla ones that do something and have no item (the daily chest, Newgrounds). At its end, **"…"** opens a popup with RobTop, the social links and More Games
- **The profile card** in the bottom left corner: a round avatar with the player's cube in their colors, and their name. It opens the profile like the vanilla button (which it presses with `activate()`)
- **Nothing covers the column**: the column fits between the profile card and the top left buttons, and any other node in its way (a mod's too) is moved to the right of it
- **The background**: layers that follow the mouse by their depth, a light overlay added on top at about two thirds of its strength, a gradient when the art is missing
- **The transition**: a short darkening, then the game's own transition. No transition is hooked, other screens change exactly as before

## Settings

| Setting | Default | |
| --- | --- | --- |
| Vanilla menu | off | On: the main menu is exactly the vanilla one (with the other mods' buttons where they put them). Applies the next time the main menu opens |
| Parallax strength | 1.0 | 0 to 2. How far the background follows the mouse, 0 keeps it still |
| Menu language | RU | RU or EN, the items' language |
| Title | on | "Geometry Dash" above the items |

## The theme

Colors, fonts, sprites, sounds, spacing and timings all come from the theme (`src/Theme.cpp`). Any of them can be changed without rebuilding: put a `theme.json` into `geode/config/zhulis.miside-menu/` with the keys to change. `theme.defaults.json` next to it lists every key with its default (the mod writes it at the main menu). Colors are hex strings, for example:

```json
{
	"accent-color": "#ff6f91",
	"item-scale": 1.2,
	"layers": [
		{ "sprite": "zhulis.miside-menu/bg-far.png", "depth": 0.2 },
		{ "sprite": "zhulis.miside-menu/bg-near.png", "depth": 1.0 },
		{ "sprite": "zhulis.miside-menu/bg-light.png", "depth": 0, "blend": "add" }
	]
}
```

The theme is read again every time the main menu opens.

## How it stays compatible

- **Nothing is removed** from `MenuLayer`. The vanilla background, the title, the hints, the buttons that got an item, the profile button and the player's name (the card shows them), and the RobTop / social / More Games menus (the popup shows them) are hidden with `setVisible(false)`: an invisible button can't be clicked or laid out, but `activate()` still runs it
- **No callback is called directly**: the items, the profile card and the popup's buttons press the vanilla buttons with `activate()`
- **The menus stay alive** with their node IDs and layouts (`bottom-menu`, `right-side-menu`, `main-menu`, `side-menu`, `social-media-menu`, `more-games-menu`, `profile-menu`, `close-menu`, `top-right-menu`...). The tray's buttons stay in their menus and keep their callbacks: the tray only scales each one to the slot and places it there. Mods can keep adding buttons and calling `updateLayout()`: when a layout moves a button, the tray puts it back in its slot before the frame is drawn (or the next one)
- **Moved, never hidden**: other mods' nodes in the column's way are moved, not hidden
- **Node IDs are Geode's** (`loader/src/ids/MenuLayer.cpp`). A node that isn't found is skipped with a warning in the log
- **Late**: the `MenuLayer::init` hook runs after the other mods' (its priority), and the tray and the column's space are arranged on the first frame, after every `init`, then again whenever something changes
- Only `MenuLayer` is hooked (`init` and `keyDown`). `keyDown` passes every key on

## Building

```
geode build --ninja
```

Or `cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build --config RelWithDebInfo`. Needs the [Geode SDK](https://docs.geode-sdk.org/getting-started/sdk) (`GEODE_SDK`) and Geode CLI. Geode 5.10.1, GD 2.2081, Windows.

The UI placeholders are drawn by `python tools/make_placeholders.py` (Pillow). The font is [M PLUS Rounded 1c](https://fonts.google.com/specimen/M+PLUS+Rounded+1c) under the SIL Open Font License, the sounds are from [Kenney's UI Audio](https://kenney.nl/assets/ui-audio) (CC0): see [LICENSES.md](LICENSES.md).

## Compatibility checklist

To go through by hand in the game before a release. Each line: what to do, then what should happen.

**The basics**

- [ ] Open the game: the menu has the new background, "Geometry Dash" over the column on the left, the profile card in the bottom left, the tray's row in the bottom right; the vanilla title, icons background, big buttons, profile button, name and social links aren't visible
- [ ] Every item opens what its vanilla button opens: Play → the level select, Wardrobe → the icon kit, Create → the creator menu, Settings, Achievements, Statistics, Quit → the "quit?" popup
- [ ] Hover each item: it lights up, moves a little and plays the hover sound; arrows move the highlight, Enter chooses
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

- [ ] The profile card is in the bottom left corner, under the column, not over it: the player's cube in their colors (with the glow if it's on) in a circle, their name next to it
- [ ] Clicking the card opens your profile, like the vanilla button; with an account name changed, the card shows the new one at the next menu
- [ ] Nothing (a vanilla menu, a mod's button or label) is over the column's items or right next to them; the log says (debug level) which nodes were moved
- [ ] No stripes or panels show behind the items: only the background art
- [ ] The items, the title and the name have a soft shadow and stay readable over the light parts of the scene

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

**The Vanilla menu setting**

- [ ] Turn it on, open another screen and come back: the menu is exactly the vanilla one (title, icons background, big buttons, profile button and name, social links, the other mods' buttons where they put them), no column, no tray, no card, no background
- [ ] Turn it off and come back: the new menu returns

**Window and quality**

- [ ] Aspect ratios 16:9, 16:10, 4:3, 21:9 and a small window: the background covers the screen with no edges showing at full parallax strength; the column, the profile and the tray stay on screen and don't overlap (a short window makes the column smaller)
- [ ] The light overlay (`bg-light.png`) brightens the scene, softer than the file itself, and doesn't darken or cover it
- [ ] Parallax strength 0: the background is still; 2: it moves more, still no edges
- [ ] Texture quality Low, Medium and High (restart after each): the background, the tray's panel, the card, the font and the accent bar are sharp and the right size
