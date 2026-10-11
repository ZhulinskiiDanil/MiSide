# LICENSES — откуда взят каждый файл ресурсов

Только файлы с явной свободной лицензией или свои. Ничего из MiSide, фан-сайтов, Pinterest и прочих мест
без лицензии. Код мода — свой.

## Шрифт

| Файл в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/fonts/RobotoMenu-CondensedBold.ttf` | «Roboto Menu Condensed Bold»: статический инстанс Roboto, `wght` 700, `wdth` 80 (латиница + кириллица) | Google Fonts, репозиторий [google/fonts](https://github.com/google/fonts/tree/main/ofl/roboto): `https://github.com/google/fonts/raw/main/ofl/roboto/Roboto%5Bwdth,wght%5D.ttf` (скачан 2026-10-11, коммит `91d5d3e5b81efa04a77925cc609fdcdd7ee663d1` в `METADATA.pb`, SHA-256 исходника `d7598e12c5dbef095ff8272cfc55da0250bd07fbdecbac8a530b9b277872a134`); upstream [googlefonts/roboto-classic](https://github.com/googlefonts/roboto-classic) | The Roboto Project Authors | [SIL Open Font License 1.1](https://openfontlicense.org) (Reserved Font Name нет) | **Изменён**: оси `wght`/`wdth` зафиксированы (`tools/make_font.py`, fontTools 4.66.1), семейство переименовано в «Roboto Menu Condensed». SHA-256 результата `2e86a5d8225f6be5a1c46bcf5b1ce37c5df73e8c03768ea110dec19dde893a96`. Geode CLI при сборке делает из него BMFont `miside-font` |
| `resources/fonts/OFL.txt` | Текст лицензии шрифта | `https://github.com/google/fonts/raw/main/ofl/roboto/OFL.txt` | The Roboto Project Authors | SIL OFL 1.1 | Нет |

Раньше здесь был M PLUS Rounded 1c (OFL); он удалён из репозитория.

OFL разрешает встраивать и распространять шрифт вместе с модом; при распространении рядом должен лежать
текст лицензии (`OFL.txt`), а сам шрифт нельзя продавать отдельно.

## Звуки

| Файл в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/sounds/hover.ogg` | Звук наведения на пункт | Kenney «UI Audio» (UI SFX Set), файл `Audio/rollover2.ogg` из `https://kenney.nl/media/pages/assets/ui-audio/490d233f68-1677590494/kenney_ui-audio.zip`, страница [kenney.nl/assets/ui-audio](https://kenney.nl/assets/ui-audio) (скачан 2026-10-11, SHA-256 `d0bc0fcba496d73df1da8c145e5330df75f87ab5a4f33a9b71461b006b5c338e`) | Kenney Vleugels (Kenney.nl) | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) | Только переименован |
| `resources/sounds/click.ogg` | Звук выбора пункта | Тот же архив, файл `Audio/click1.ogg` (SHA-256 `59175ac17cd49a68dd736285738441287636112a84a6f7ce0d89921bda5a5360`) | Kenney Vleugels (Kenney.nl) | CC0 1.0 | Только переименован |
| `resources/sounds/License-Kenney.txt` | Лицензия из архива | Тот же архив, `License.txt` | Kenney | CC0 1.0 | Нет |

CC0 не требует указывать автора; Kenney просит об этом по желанию — упоминание здесь и в README.

## Мита (анимация)

| Файлы в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/mita/mita_atlas_00.png` … `mita_atlas_21.png`, `resources/mita/mita-manifest.json` | Отрендеренная анимация ожидания модели «Mita», упакованная в атласы | 3D-модель [sketchfab.com/3d-models/mita-5dc5cf2928b14fa3bd4d7c24c0cc7ff7](https://sketchfab.com/3d-models/mita-5dc5cf2928b14fa3bd4d7c24c0cc7ff7); рендер и упаковка подготовлены вне этого репозитория (папка `outputs/mita-menu`, 2026-10-11) | Elite Big Speakerman ([sketchfab.com/EvesStudio2008](https://sketchfab.com/EvesStudio2008)) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), как указано в метаданных исходного GLB | Импорт в Blender, портретная камера, свет и материалы, рендер, клип Idle пересэмплирован в 127 кадров при 24 fps, прозрачные PNG и атласы. Манифест переименован в `mita-manifest.json` |
| `resources/mita/MITA_ATTRIBUTION.txt`, `resources/mita/mita-source-metadata.json` | Атрибуция и метаданные автора/лицензии из исходного архива (дословно) | там же | — | — | Только переименованы |

CC BY требует указывать автора, источник, лицензию и изменения — они в `MITA_ATTRIBUTION.txt`, который идёт в
пакет мода вместе с атласами, и здесь. **Важно:** CC BY выдал загрузчик модели; права на самого персонажа (Мита,
игра MiSide) принадлежат её авторам, и лицензия загрузчика их не передаёт. См. `OPEN_QUESTIONS.md`.

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
| `resources/placeholders/row-plate.png` | Белая плашка пункта со скруглением 3 pt (9-slice) | то же | то же |
| `resources/placeholders/glow-soft.png` | Белое мягкое круглое свечение (радиальная альфа) | то же | то же |

Иконка игрока в карточке профиля и кнопки в попапе «…» — спрайты самой игры (текущий кадр ванильных кнопок,
кубик игрока через `SimplePlayer`), в мод они не копируются.
