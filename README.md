# MiSide Menu

[Русский](README.ru.md)

A [Geode](https://geode-sdk.org) mod for Geometry Dash 2.2081 (Windows and Android) that dresses GD's screens in the style of MiSide's interface: the loading screen with the AIHASTO intro, the main menu with Mita, CREATE, the official levels, the pause, the level complete card, achievements, statistics and settings. GD's own actions, data and other mods' buttons keep working; what the mod doesn't cover stays as it is in GD.

## Installing

Download `zhulis.miside-menu.geode` from the [releases](https://github.com/ZhulinskiiDanil/MiSide/releases) and put it in `Geometry Dash/geode/mods/`, then restart the game.

## Settings

| Setting | Default | |
| --- | --- | --- |
| Vanilla menu | off | The main menu exactly as without the mod |
| Parallax strength | 1.0 | How far the background follows the mouse, 0 to 2 |
| Menu language | RU | RU or EN |
| Logo | on | The logo and "MAIN MENU" above the items |
| Studio intro | on | The AIHASTO intro at the game's start |
| Confirm leaving a level | on | The pause asks "Quit the level?" before leaving |

## Building

Needs the [Geode SDK](https://docs.geode-sdk.org/getting-started/sdk) and CLI:

```
geode build
```

CI builds Windows and Android on every push (`.github/workflows`). Before a release, go through [docs/CHECKLIST.md](docs/CHECKLIST.md).

## Credits

The fonts are [M PLUS Rounded 1c](https://fonts.google.com/specimen/M+PLUS+Rounded+1c) (SIL Open Font License), the sounds are from [Kenney's UI Audio](https://kenney.nl/assets/ui-audio) (CC0). Every license is in [LICENSES.md](LICENSES.md), every asset in [ASSETS.md](ASSETS.md).
