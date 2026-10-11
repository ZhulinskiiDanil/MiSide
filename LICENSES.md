# LICENSES — откуда взят каждый файл ресурсов

Только файлы с явной свободной лицензией или свои. Ничего из MiSide, фан-сайтов, Pinterest и прочих мест
без лицензии. Код мода — свой.

## Шрифт

| Файл в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/fonts/MPLUSRounded1c-Medium.ttf` | M PLUS Rounded 1c, Medium (латиница + кириллица) | Google Fonts, репозиторий [google/fonts](https://github.com/google/fonts/tree/main/ofl/mplusrounded1c): `https://github.com/google/fonts/raw/main/ofl/mplusrounded1c/MPLUSRounded1c-Medium.ttf` (скачан 2026-10-11, SHA-256 `adfde1b6bae58719c4e0144612a94232e72fc5ca655c4722165fe88d06521a70`) | Coji Morishita, M+ Fonts Project | [SIL Open Font License 1.1](https://openfontlicense.org) (`license: "OFL"` в `METADATA.pb` Google Fonts) | Нет. Geode CLI при сборке делает из него BMFont `miside-font` (только символы из `charset` в `mod.json`) |
| `resources/fonts/OFL.txt` | Текст лицензии шрифта | Upstream-репозиторий M+ Fonts: `https://raw.githubusercontent.com/coz-m/MPLUS_FONTS/master/OFL.txt` | The M+ FONTS Project Authors | SIL OFL 1.1 | Нет |

OFL разрешает встраивать и распространять шрифт вместе с модом; при распространении рядом должен лежать
текст лицензии (`OFL.txt`), а сам шрифт нельзя продавать отдельно.

## Звуки

| Файл в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/sounds/hover.ogg` | Звук наведения на пункт | Kenney «UI Audio» (UI SFX Set), файл `Audio/rollover2.ogg` из `https://kenney.nl/media/pages/assets/ui-audio/490d233f68-1677590494/kenney_ui-audio.zip`, страница [kenney.nl/assets/ui-audio](https://kenney.nl/assets/ui-audio) (скачан 2026-10-11, SHA-256 `d0bc0fcba496d73df1da8c145e5330df75f87ab5a4f33a9b71461b006b5c338e`) | Kenney Vleugels (Kenney.nl) | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) | Только переименован |
| `resources/sounds/click.ogg` | Звук выбора пункта | Тот же архив, файл `Audio/click1.ogg` (SHA-256 `59175ac17cd49a68dd736285738441287636112a84a6f7ce0d89921bda5a5360`) | Kenney Vleugels (Kenney.nl) | CC0 1.0 | Только переименован |
| `resources/sounds/License-Kenney.txt` | Лицензия из архива | Тот же архив, `License.txt` | Kenney | CC0 1.0 | Нет |

CC0 не требует указывать автора; Kenney просит об этом по желанию — упоминание здесь и в README.

## Фон

| Файл в репозитории | Что это | Источник | Лицензия |
| --- | --- | --- | --- |
| `resources/background/bg-far.png` | Задняя стена с окном (был `1.png`) | Предоставлен автором мода | Автора мода — уточнить в `OPEN_QUESTIONS.md` |
| `resources/background/bg-mid.png` | Мебель, прозрачный фон (был `2.png`) | Предоставлен автором мода | то же |
| `resources/background/bg-near.png` | Передний план, прозрачный фон (был `3.png`) | Предоставлен автором мода | то же |
| `resources/background/bg-light.png` | Световой оверлей, кладётся аддитивно (был `4.png`) | Предоставлен автором мода | то же |

Файлы только переименованы под `ASSETS.md`, содержимое не менялось.

## Плейсхолдеры интерфейса

| Файл в репозитории | Что это | Источник | Лицензия |
| --- | --- | --- | --- |
| `resources/placeholders/item-accent.png` | Белая скруглённая полоска выделения | Нарисован `tools/make_placeholders.py` в этом репозитории | Как у кода мода |
| `resources/placeholders/tray-panel.png` | Белая скруглённая панель лотка (9-slice) | то же | то же |
| `resources/placeholders/tray-slot.png` | Белая скруглённая ячейка лотка (9-slice) | то же | то же |
| `resources/placeholders/avatar-circle.png` | Белый круг под аватаром в карточке профиля | то же | то же |

Иконка игрока в карточке профиля и кнопки в попапе «…» — спрайты самой игры (текущий кадр ванильных кнопок,
кубик игрока через `SimplePlayer`), в мод они не копируются.
