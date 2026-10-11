# LICENSES — откуда взят каждый файл ресурсов

Только файлы с явной свободной лицензией или свои. Ничего из MiSide, фан-сайтов, Pinterest и прочих мест
без лицензии. Код мода — свой.

## Шрифт

| Файл в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/fonts/MPLUSRounded1c-Bold.ttf` | M PLUS Rounded 1c Bold (латиница + кириллица) | Google Fonts, [google/fonts](https://github.com/google/fonts/tree/main/ofl/mplusrounded1c): `https://github.com/google/fonts/raw/main/ofl/mplusrounded1c/MPLUSRounded1c-Bold.ttf` (скачан 2026-10-11, последний коммит каталога `84efd8ad78c3710ad14bd909e3bc407151885628`, SHA-256 `c358630584e8e2d8fbd6121d0f4693255ffef6d1e6d4f3441fd6e5a963a11f9e`) | Coji Morishita, M+ Fonts Project | [SIL OFL 1.1](https://openfontlicense.org) (`license: "OFL"` в `METADATA.pb`) | Нет. Geode CLI делает из него BMFont `miside-font` |
| `resources/fonts/MPLUSRounded1c-ExtraBold.ttf` | M PLUS Rounded 1c ExtraBold | то же, `MPLUSRounded1c-ExtraBold.ttf` (SHA-256 `8e7c15901dca87f1451b356dda594f7d092ba252a5dcc47da74523a242493c36`) | то же | SIL OFL 1.1 | Нет. BMFont `miside-title` |
| `resources/fonts/OFL.txt` | Текст лицензии | upstream M+ Fonts: `https://raw.githubusercontent.com/coz-m/MPLUS_FONTS/master/OFL.txt` (SHA-256 `04971e3fcee60b247395150d93b3616f6a0b092572332c96187b472976553abc`) | The M+ FONTS Project Authors | SIL OFL 1.1 | Нет |

Раньше здесь были Roboto (изменённый инстанс) и до него M PLUS Rounded 1c Medium; оба удалены из репозитория.
OFL разрешает встраивать и распространять шрифт с модом; рядом должен лежать текст лицензии, сам шрифт нельзя
продавать отдельно.

## Звуки

| Файл в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/sounds/hover.ogg` | Звук наведения на пункт | Kenney «UI Audio» (UI SFX Set), файл `Audio/rollover2.ogg` из `https://kenney.nl/media/pages/assets/ui-audio/490d233f68-1677590494/kenney_ui-audio.zip`, страница [kenney.nl/assets/ui-audio](https://kenney.nl/assets/ui-audio) (скачан 2026-10-11, SHA-256 `d0bc0fcba496d73df1da8c145e5330df75f87ab5a4f33a9b71461b006b5c338e`) | Kenney Vleugels (Kenney.nl) | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) | Только переименован |
| `resources/sounds/click.ogg` | Звук выбора пункта | Тот же архив, файл `Audio/click1.ogg` (SHA-256 `59175ac17cd49a68dd736285738441287636112a84a6f7ce0d89921bda5a5360`) | Kenney Vleugels (Kenney.nl) | CC0 1.0 | Только переименован |
| `resources/sounds/License-Kenney.txt` | Лицензия из архива | Тот же архив, `License.txt` | Kenney | CC0 1.0 | Нет |

CC0 не требует указывать автора; Kenney просит об этом по желанию — упоминание здесь и в README.

## Музыка

| Файлы в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/music/*.ogg` (5 файлов, см. ASSETS.md) | Треки саундтрека MiSide | Предоставлены пользователем (`Downloads/miside-sounds`, 2026-10-11) | Авторы саундтрека MiSide | **Не указана; права у правообладателей MiSide** | Обрезана тишина, перекодировано в OGG Vorbis |

**Важно:** это музыка из самой игры MiSide; лицензии на распространение нет. Подключена по прямой просьбе пользователя,
для личного использования. Публиковать мод с этими файлами нельзя без разрешения правообладателей — перед публикацией
удалить `resources/music/` (мод тогда играет музыку GD; см. OPEN_QUESTIONS.md).

## Мита (анимация)

| Файлы в репозитории | Что это | Источник | Автор | Лицензия | Изменения |
| --- | --- | --- | --- | --- | --- |
| `resources/mita-closeup/mita_closeup_atlas_00.png` … `_63.png` (+ `.plist`), `resources/mita-closeup/mita-closeup-manifest.json` | Отрендеренная анимация ожидания модели «Mita», крупный план, упакованная в атласы (в пакете мода) | 3D-модель [sketchfab.com/3d-models/mita-5dc5cf2928b14fa3bd4d7c24c0cc7ff7](https://sketchfab.com/3d-models/mita-5dc5cf2928b14fa3bd4d7c24c0cc7ff7); рендер и упаковка вне репозитория (папка `outputs/mita-menu-closeup`, 2026-10-11) | Elite Big Speakerman ([sketchfab.com/EvesStudio2008](https://sketchfab.com/EvesStudio2008)) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), как указано в метаданных исходного GLB | Новый рендер исходной модели с более близкой камерой (не увеличение старых кадров), 127 кадров 960×1080 при 24 fps, прозрачные PNG и атласы. Манифест переименован в `mita-closeup-manifest.json` |
| `resources/mita-closeup/ATTRIBUTION.txt`, `resources/mita-closeup/source_metadata.json` | Атрибуция и метаданные автора/лицензии (дословно, в пакете мода) | там же | — | — | Без изменений |
| `resources/mita/mita_atlas_00.png` … `mita_atlas_21.png`, `resources/mita/mita-manifest.json` (старый комплект, только в репозитории, в пакет не входит) | Отрендеренная анимация ожидания модели «Mita», упакованная в атласы | 3D-модель [sketchfab.com/3d-models/mita-5dc5cf2928b14fa3bd4d7c24c0cc7ff7](https://sketchfab.com/3d-models/mita-5dc5cf2928b14fa3bd4d7c24c0cc7ff7); рендер и упаковка подготовлены вне этого репозитория (папка `outputs/mita-menu`, 2026-10-11) | Elite Big Speakerman ([sketchfab.com/EvesStudio2008](https://sketchfab.com/EvesStudio2008)) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), как указано в метаданных исходного GLB | Импорт в Blender, портретная камера, свет и материалы, рендер, клип Idle пересэмплирован в 127 кадров при 24 fps, прозрачные PNG и атласы. Манифест переименован в `mita-manifest.json` |
| `resources/mita/MITA_ATTRIBUTION.txt`, `resources/mita/mita-source-metadata.json` | Атрибуция и метаданные автора/лицензии из исходного архива (дословно) | там же | — | — | Только переименованы |

CC BY требует указывать автора, источник, лицензию и изменения — они в `ATTRIBUTION.txt` крупного плана, который идёт в
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

## Логотип

| Файл в репозитории | Что это | Источник | Лицензия |
| --- | --- | --- | --- |
| `resources/logo/miside-like-gd-logo.png` | Логотип «Geometry Dash» в стиле меню | Предоставлен автором мода (`Downloads/miside-like-gd-logo.png`, 2026-10-11, SHA-256 `df3862fc619c9969000e84a9f68366667701d5820db994d357060732dc967c48`), скопирован без изменений | Автора мода — уточнить в `OPEN_QUESTIONS.md` (надпись «Geometry Dash» — название игры RobTop) |

## Плейсхолдеры интерфейса

| Файл в репозитории | Что это | Источник | Лицензия |
| --- | --- | --- | --- |
| `resources/placeholders/item-accent.png` | Белая скруглённая полоска выделения | Нарисован `tools/make_placeholders.py` в этом репозитории | Как у кода мода |
| `resources/placeholders/tray-panel.png` | Белая скруглённая панель лотка (9-slice) | то же | то же |
| `resources/placeholders/tray-slot.png` | Белая скруглённая ячейка лотка (9-slice) | то же | то же |
| `resources/placeholders/avatar-circle.png` | Белый круг под аватаром в карточке профиля | то же | то же |
| `resources/placeholders/row-plate.png` | Белая плашка пункта со скруглением 3 pt (9-slice) | то же | то же |
| `resources/placeholders/glow-soft.png` | Белое мягкое круглое свечение (радиальная альфа) | то же | то же |
| `resources/placeholders/pattern-tile.png` | Бесшовная плитка из белых сердечек, звёздочек и цветочков | то же | то же |
| `resources/placeholders/vignette-soft.png` | Белая виньетка (альфа растёт к углам) | то же | то же |

Иконка игрока в карточке профиля и кнопки в попапе «…» — спрайты самой игры (текущий кадр ванильных кнопок,
кубик игрока через `SimplePlayer`), в мод они не копируются.
