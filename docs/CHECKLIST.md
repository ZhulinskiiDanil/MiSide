# Release checklist

[Русский](CHECKLIST.ru.md)

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

**Sounds**

- [ ] Hover each button (main menu, confirmation, pause, MORE): Table Enter once as the mouse comes on, with the shine; resting there: nothing more; never two at once; the old click/hover sounds are gone
- [ ] Press: Table Click; MORE opens with Switch Open, BACK / Esc close with Switch Close
- [ ] GD's settings: each toggle Table Toggle; More Options' pages Switch Kategory; the icon kit's tabs Switch Kategory, putting on an icon Gift Poof (a locked one: nothing)
- [ ] An achievement: MiSide's Achievement, not GD's; completing a level: Win, not GD's endStart
- [ ] SFX volume 0 in GD: all of them silent; at 50%: quieter
- [ ] Pause: Startpause once, Ambientpause fading in, looping with no seam; resume / restart / quit: it fades out, Startpause stops, the song in sync
- [ ] Stress: best 73% — pause at 60%: Ambientpause; at 69%: Stress; past 73% (not dead yet): Stress a bit louder; in practice, a start position, a completed level or a best under 20%: Ambientpause

**Music**

- [ ] The main menu: «MusicMenu Update» plays, at the music volume; music volume 0 mutes it; going to Online, Create, the level select and back: it goes on, never starts again, never two at once
- [ ] The icon kit: the menu's music fades out, the Chibi Room ambient fades in; back: it fades out, the menu's comes back
- [ ] Let each play to its end: it loops with no pause or click
- [ ] `menu-music-path` set to `miside-music-menu.ogg`, then `miside-menu-alternative.ogg` (reopen the game or go through the menu): the other track plays; `menu-music-enabled` false: GD's menu loop
- [ ] Open the quit confirmation and cancel it: the music goes on without a jump
- [ ] A level: its song as always; pause: the LoopRoom ambient fades in quietly; resume, restart, quit: it fades out, the song is in sync; pausing many times: never two ambients

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

**The intro**

- [ ] Start the game: the AIHASTO intro, smooth, with its sound, then a soft fade into the loading screen; a click or a key skips it at once; the loading then goes on as usual
- [ ] "Studio intro" off, or reloading resources from GD's settings: no intro

**The loading screen**

- [ ] Start the game: from its first frame the MiSide loading screen (no GD logos, no green bar), MiSide's jokes one after another in the mod's font (Cyrillic shows), the tip and both of Geode's lines fully on screen, the bar filling smoothly to the end, a tip in the bottom middle, Geode's "Loaded N/M mods" under it in pink; then the main menu
- [ ] Texture quality Low / Medium / High, and reloading resources from GD's settings: the same screen, sharp
- [ ] `"loading-enabled": false` or Vanilla menu: GD's loading screen

**The official levels**

- [ ] PLAY in the main menu: "ВЫБРАТЬ УРОВЕНЬ", the list, the floppy, as in MiSide's screenshot; no carousel, arrows, dots, bars or corners
- [ ] All the official levels are in the list; the wheel, Up/Down and dragging the bar scroll it; rows are never cut; hover lights a row with its shine and sound
- [ ] A click on a row opens that level; PLAY and Enter play the one on the card; a demon without the coins: GD's lock message
- [ ] The card: name, difficulty, stars, coins (gold when collected), normal and practice progress — as in GD's own page
- [ ] Play a level once past 5%, come back: its picture is a frame from 1–5% of it, after your first click (no pause button or percent), its proportions kept; no stutter when it's taken
- [ ] Hover rows: the card shows each at once; move off the list: the chosen level is back; PLAY plays what the card shows
- [ ] ЕЩЁ: soundtrack, info, the tower open as GD's; ИСХОДНЫЙ ЭКРАН shows GD's screen (its secret coin and door work), MISIDE brings ours back; НАЗАД and Esc close ЕЩЁ
- [ ] НАЗАД and Esc: back to the main menu

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
- [ ] She stands on the right, her feet cut by the bottom edge, over the background and under the tray, the card, the column and every mod's button; nothing she covers stops being clickable
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
- [ ] The background: no room; nearly black on the left, burgundy on the right, a soft purple glow behind Mita; the pattern is faint thin outlines, no rows, sizes and angles vary; no seam where its tile repeats
- [ ] The official levels' screen has the same background; the pause still shows the blurred level
- [ ] ACHIEVEMENTS: a whole screen, not GD's dropdown; nothing of the menu shows through; no frame, chains, page arrows or brown plates; Mita on the right doesn't cover the list
- [ ] A click on Mita (in the menu and on the achievements screen): a white flash breaking into a few soft blobs, gone in about 0.2 s; a click on her canvas' clear sides or on a button over her does nothing of the kind, and the button still works
- [ ] The count matches GD's (earned / all); earned rows: light text and a check; the others darker with the lock and a dark icon; the rewards' icons as in GD's own list
- [ ] A click opens a row (condition, status, reward, progress bar when partway), another click closes it; opening another closes the first; the list doesn't jump
- [ ] Hundreds of achievements (another mod adding them): scrolling with the wheel, a drag and the thumb stays smooth; the rows are cut cleanly at the list's top and bottom
- [ ] Up/Down light rows, Enter opens; Esc and BACK go back to the menu through the darkening; the menu works as before after it
- [ ] STATISTICS: a whole screen; every number matches GD's own Stats window (GD STATS opens it, with other mods' additions); the rows scroll and are cut cleanly; Esc and BACK return to the menu
- [ ] A level completed (normal, practice, own level): the card shows GD's attempts, jumps, time (and points on a platformer) and coins exactly as GD would; every button does what GD's does; a mod's button on that layer still shows and works; the Win sound plays once
- [ ] APPEARANCE: every choice and slider changes the same setting Geode's page shows; MUSIC / EFFECTS match GD's own settings; RESET restores the defaults; leaving after a language or logo change shows the menu remade; the column still fits above EXIT
- [ ] Official levels: locked demons show the lock, the others their best %; leaving to the menu and coming back keeps the chosen level and the scroll; finishing or quitting a level comes back with that level chosen; Up/Down and Enter still work
- [ ] Pause: after NO (or Esc) on the quit confirmation, and back from MORE, the row chosen before is chosen again; the wheel over MUSIC / SFX moves them by a step; in MORE, BACK stands apart under the rows; the vanilla pause is still reachable
- [ ] Dialogs and layout: RESET in APPEARANCE asks first (YES / NO in the column's place, BACK off meanwhile, Esc closes the question, not the screen); in English and Russian no button or row label runs past its plate or into a slider or value; on 4:3 the level complete card ends before the screen's middle
- [ ] CREDITS opens over APPEARANCE, scrolls, and BACK returns to APPEARANCE (not the menu); every line matches LICENSES.md
- [ ] SETTINGS opens the mod's screen, not GD's dropdown; every group of OPTIONS has the toggles GD's More Options has on its pages (none missing, none twice), each ON/OFF as GD shows it, and flipping one here shows flipped in GD WINDOW; the music offset, the saved songs, FMOD DEBUG, KEYS and the parental control work; GRAPHICS: full screen / borderless / the resolution / the textures change only with APPLY, as in GD; the advanced toggles, the FPS and APPLY FPS work; the account, the soundtracks, how to play, support, rate and the vault (its guard's words over the screen) open over it; GD WINDOW opens GD's own window; long columns scroll with the wheel and the arrows, the info under them matches the lit row
- [ ] In a level, the pause's SETTINGS shows GD's in-level options in the column's place, each one acting at once on the level as in GD's window; the UI editors open; BACK and Esc go back to the pause menu; QUIT THE LEVEL from there asks as usual
- [ ] A narrow window (4:3): the menu, the card and the tray keep their size and don't overlap; the background is cropped around its middle, not stretched
- [ ] Parallax strength 0: the background is still; 2: it moves more, still no edges
- [ ] Switching to full screen and back (with and without Borderless fix): hovering the buttons shows the usual shine with no green rectangle; Mita's outline and the pause blur still work
- [ ] Texture quality Low, Medium and High (restart after each): the background, the plates, the tray, the card and the font are sharp and the right size
