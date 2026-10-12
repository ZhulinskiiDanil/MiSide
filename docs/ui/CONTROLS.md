# Controls of the mod's screens

What each owned screen takes, beyond clicking what's shown. Keys go to the top screen only; a popup over a screen
(GD's or a mod's FLAlertLayer) stops its hover and keys.

| Screen | Keys and mouse |
| --- | --- |
| Main menu (`src/main.cpp`, `ui/MSMenuColumn`) | Up/Down choose a row, Enter or A presses it; Esc / B opens the quit confirmation (YES / NO with Left/Right or Up/Down, Esc closes it); the mouse lights rows; EXIT leans away from the mouse; a click on Mita sparks (3D lane) |
| Official levels (`src/levels.cpp`) | Up/Down choose a level (scrolled into view), Enter / Space / A plays it; the wheel and the bar's thumb scroll by whole rows; a click on a row plays; Left/Right do nothing (GD's hidden pages would turn); Esc goes back, or out of MORE first |
| Pause (`src/pause.cpp`, `pause/MSPauseColumn`) | Up/Down choose (wrapping), Enter / Space / A presses, Left/Right and the wheel move MUSIC / SFX, the round button resets them; Esc goes back from the confirmation or MORE first, else GD's own; the row chosen before a confirmation or MORE is chosen again |
| Level complete (`src/results.cpp`) | The card's buttons by mouse; GD's own keys (Space replays, Esc to the menu) as GD has them |
| Achievements (`src/achievements.cpp`) | Up/Down light a row, Enter / Space / A opens or closes it; the wheel, a drag or the thumb scroll smoothly; Esc / B / BACK close |
| Statistics, credits (`src/statistics.cpp`, `src/credits.cpp`, `ui/MSScrollView`) | Up/Down and the wheel scroll, a drag or the thumb too; Esc / B / BACK close; GD STATS opens GD's own window |
| GD's settings (`src/options.cpp`) | Up/Down choose (the column scrolls to keep it in view), Enter / A or a click flips a toggle or opens a row, Left/Right change choices and sliders (the music offset by 5 ms), the wheel scrolls (or moves the slider under the mouse); on a number (music offset, FPS) a click or Enter starts typing (digits, minus or point, Backspace), Enter or another row ends it, Esc drops it; Esc / B / BACK close; GD WINDOW opens GD's own window |
| Pause settings (`src/pause.cpp`) | As the pause's column; BACK, Esc and B go back to the pause menu |
| The settings' windows (`src/options/`) | As GD's settings' screens; the tutorial also turns its pages with Left/Right; in the account forms typing goes to GD's fields; the UI editors' preview is dragged as in GD, HIDE MENU / MENU fold and bring back the column |
| Appearance (`src/settings.cpp`) | Up/Down choose, Enter / A or a click flips a choice, Left/Right change choices and sliders, the wheel moves the slider under the mouse; RESET asks first (Esc closes the question, not the screen); CREDITS opens over it |
| Loading screen (`src/loading.cpp`) | A click or any key skips the intro |

GD may send Esc both as a key and as back on one frame: every screen takes one back a frame.
