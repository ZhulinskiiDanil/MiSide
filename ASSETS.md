# ASSETS — манифест ассетов MiSide Menu

Все ассеты, на которые ссылается код. Откуда взят каждый файл и под какой лицензией — в
[LICENSES.md](LICENSES.md). Ничего из файлов MiSide (логотип, персонаж, интерфейс) в мод не входит и входить не
должно.

Все имена в коде заданы в одном месте — `src/Theme.cpp` (функция `defaults()`), с префиксом мода через `_spr`
(в игре файл называется `zhulis.miside-menu/<имя>`). Любое имя можно переопределить без пересборки в
`<папка игры>/geode/config/zhulis.miside-menu/theme.json` по ключу из колонки «Ключ темы»; полный список ключей
со значениями по умолчанию мод пишет рядом, в `theme.defaults.json`.

| Папка | Что там | Как попадает в мод |
| --- | --- | --- |
| `resources/background/` | Слои фона, будущий персонаж и логотип | `mod.json` → `resources.sprites`: `resources/background/*.png` — любой PNG из папки, без правки `mod.json` |
| `resources/fonts/` | TTF шрифта и его лицензия | `mod.json` → `resources.fonts.miside-font` |
| `resources/sounds/` | Звуки | `mod.json` → `resources.files` |
| `resources/placeholders/` | Собственные белые формы интерфейса (рисует `tools/make_placeholders.py`) | `mod.json` → `resources.sprites` |

Спрайты из `resources.sprites` Geode CLI считает UHD и сам делает из них `-hd` (½) и обычную (¼) версии, поэтому
в репозиторий кладётся **только UHD-файл**. Размеры обычной и `-hd` версий в таблицах — то, что получится.

## Фон

Экран GD — 569×320 точек при 16:9 (UHD = 4 px на точку). Все слои масштабируются одинаково («cover» с общим
запасом под параллакс) и кадрируются вокруг точки `background-focus-x/y` (по умолчанию центр), пропорции
сохраняются. Нет файла — слой пропускается, вместо самого дальнего рисуется градиент `backdrop-top` →
`backdrop-bottom`.

| Файл | Назначение | Рекомендуемый размер: обычный / `-hd` / `-uhd`, px | Сейчас | Наложение | Прозрачность | Тонировка | Глубина | Ключ темы |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `bg-far.png` | Задняя стена с окном | 683×384 / 1366×768 / 2732×1536 | 1536×1024 | обычное | 255 | `background-tint` #8C527E | 0.2 | `layers[0]` |
| `bg-mid.png` | Мебель | то же | 1536×1024 | обычное | 220 | `background-tint` | 0.5 | `layers[1]` |
| `bg-near.png` | Передний план | то же | 1536×1024 | обычное | 110 | `background-tint` | 1.0 | `layers[2]` |
| `bg-light.png` | Световой оверлей, свет на **чёрном** | то же | 1536×1024 | **add** | 35 | нет | 0 | `layers[3]` |

У каждого слоя в теме: `sprite`, `depth`, `blend` (`"normal"` / `"add"`), `opacity` (0–255) и необязательный
`tint` (hex; без него — `background-tint` для обычных слоёв). Над слоями, под меню, процедурно рисуются:
затемнение (`background-darken-color` #15051A, 125), малиновое свечение справа (`glow-soft.png`, `glow-color`
#9E086F, 90) и тень слева (`left-shade-color` #08050D, 190 у края → 0 к x = `left-shade-width` 260).

Текущие слои 1536×1024 (3:2) — 384×256 точек при UHD, на экране растягиваются примерно в 1.5 раза: на высоком
качестве текстур мягче. Для резкости — 2732×1536 (16:9) или больше.

## Интерфейс

| Файл | Назначение | Обычный / `-hd` / `-uhd`, px | Формат | Где используется | Ключ темы |
| --- | --- | --- | --- | --- | --- |
| `row-plate.png` | Плашка каждого пункта, 9-slice со скруглением 4 pt. Тонируется `row-*-color` / `row-*-opacity` по состоянию — рисовать **белой** | 16×16 / 32×32 / 64×64 | PNG, RGBA, белая | `ui/MSMenuItem.cpp` | `row-sprite` |
| `glow-soft.png` | Мягкий круглый свет (белый, альфа спадает от центра), растягивается до `glow-width` × `glow-height`, тонируется `glow-color` | 64×64 / 128×128 / 256×256 | PNG, RGBA, белый | `ui/MSBackground.cpp` | `glow-sprite` |
| `item-accent.png` | Полоска слева от выделенного пункта — по умолчанию выключена (`accent-enabled: false`) | 3×24 / 6×48 / 12×96 | PNG, RGBA, белая | `ui/MSMenuItem.cpp` | `accent-sprite` |
| `tray-panel.png` | Подложка лотка, карточки профиля и попапа «…», 9-slice — рисовать **белой** | 24×24 / 48×48 / 96×96 | PNG, RGBA, белая | `ui/MSModTray.cpp`, `ui/MSProfileCard.cpp`, `ui/MSMorePopup.cpp` | `tray-sprite` |
| `tray-slot.png` | Ячейка под кнопкой лотка, 9-slice (по умолчанию невидима: `slot-opacity: 0`) | 16×16 / 32×32 / 64×64 | PNG, RGBA, белая | `ui/MSModTray.cpp` | `slot-sprite` |
| `avatar-circle.png` | Круг под иконкой игрока в карточке профиля | 24×24 / 48×48 / 96×96 | PNG, RGBA, белый | `ui/MSProfileCard.cpp` | `avatar-sprite` |
| `pattern-tile.png` | Плитка узора из сердечек, звёздочек и цветочков, бесшовная; повторяется по экрану (`GL_REPEAT`), тонируется `pattern-color`, прозрачность `pattern-opacity` (16) | 64×64 / 128×128 / 256×256 — **степень двойки** на всех качествах, иначе повтор не работает | PNG, RGBA, белая | `ui/MSBackground.cpp` | `pattern-sprite`, `pattern-tile` (размер плитки в pt) |
| `vignette-soft.png` | Виньетка: прозрачный центр, к углам непрозрачнее; растягивается на экран, тонируется `vignette-color` | 64×64 / 128×128 / 256×256 | PNG, RGBA, белая | `ui/MSBackground.cpp` | `vignette-sprite` |

Все — собственные простые формы из `resources/placeholders/`. Нет файла — прямоугольник того же цвета (у
свечения — без свечения). Если нет `tray-panel.png`, попап берёт ванильный `GJ_square01.png` из игры.

## Мита

| Файл | Назначение | Размер | Где используется | Ключ темы |
| --- | --- | --- | --- | --- |
| `mita_closeup_atlas_00.png` … `mita_closeup_atlas_63.png` | 64 страницы атласа анимации ожидания, крупный план: по 2 кадра 960×1080 (последняя — 1 кадр), extrusion 2 px, без поворота и trim | 1928×1084 (последняя 964×1084), PNG RGBA, прямая альфа | `ui/MSMita.cpp` | `mita-*` |
| `mita-closeup-manifest.json` | Порядок кадров, прямоугольники, страницы, fps, canvas, размещение (`manifest.json` комплекта, переименован) | — | `ui/MSMita.cpp` | — |
| `ATTRIBUTION.txt`, `source_metadata.json` | Атрибуция CC BY 4.0 и метаданные исходной модели (дословно; ATTRIBUTION ссылается на source_metadata) — распространяются вместе с атласами | — | — | — |
| `mita_closeup_atlas_*.plist` | Plist format 2 к страницам | — | не используются: прямоугольники из манифеста, в CCSpriteFrameCache ничего не регистрируется; в пакет не входят | — |

**Кадрирование.** Крупный план заложен камерой рендера (ортографический размер 0.48 вместо 0.65), кадры не
увеличиваются: `mita-scale` 320/1080 pt на пиксель canvas — canvas во всю высоту экрана 569×320, `mita-x` 414.5,
`mita-y` 0, anchor (0.5, 0); на экране 284.444×320 pt, canvas от x 272.3 до 556.7. Рисунок во всех кадрах — x 68–904
px canvas (`alphaBoundsUnionTopLeft`), то есть x ≈292–540 pt: голова целиком на экране, кисти обрезаны нижним
краем намеренно. Проверено вне игры: кадры 0, 1, 2, 63, 126, вырезанные из атласов мода по rect манифеста, побайтово
совпадают с кадрами комплекта, а фигура на экране стоит там же, где в `preview.png` (столбцы 296.5–539.5 pt).
На High (4 пикселя на точку) ≈1.35 пикселя экрана на пиксель кадра — детализация лица примерно вдвое выше старой.

**Контур** рисует собственный шейдер её спрайта: розовая линия (`mita-outline-color`, `-opacity`, `-width` в
пикселях текстуры, 8 ≈ 2.4 pt, как прежде) только там, где рядом с прозрачным пикселем есть нарисованный. Слева и
справа у каждого кадра не меньше 68 px прозрачного поля, поэтому выборка не задевает соседний кадр.

Лежат в `resources/mita-closeup/`, в пакет идут через `resources.files` (без -hd/-uhd: размер на экране считается
из размера кадра). `.blend`, отдельные кадры и превью в пакет не входят. Пакет мода — около 106 МБ. На GPU не больше
трёх страниц: три текстуры-слота 1928×1084 (~23.9 МиБ), созданные один раз за показ меню и перезаливаемые
`glTexSubImage2D`; в CPU — не больше двух готовых страниц и одной в декодировании (~23.9 МиБ). Декодирование
страницы WIC вне игры — 17 мс в среднем (максимум 18.6 мс) при бюджете 83 мс.

Старый комплект (`resources/mita/`, 22 страницы по 6 кадров 640×720, `mita-scale` 0.582, `mita-x` 400, `mita-y`
−75.6, контур 4 px) пока лежит в репозитории до проверки нового в игре, но в пакет не входит и не загружается.

## Логотип

| Файл | Назначение | Размер | Формат | Где используется | Ключ темы |
| --- | --- | --- | --- | --- | --- |
| `miside-like-gd-logo.png` (`resources/logo/`) | Логотип «Geometry Dash» над «ГЛАВНОЕ МЕНЮ», показывается как есть, без сетки, теней и бликов поверх | 2172×724 px — UHD; Geode делает из него `-hd` 1086×362 и обычный 543×181. Видимая часть (альфа > 8) — 2138×346 px от (25, 167) | PNG, RGBA, настоящая прозрачность | `ui/MSLogo.cpp` | `logo-sprite` |

Прозрачные поля не учитываются: видимые границы мод считает по альфе самого файла (того варианта, что
загружен для качества текстур), поэтому `logo-left` (59 — по колонке кнопок) и `logo-bottom` (236 — над
подписью, её верх на 229) задают левый нижний угол видимого рисунка, а `logo-width` (220) — его ширину на экране
с учётом масштаба родителей. Высота следует из пропорций: ≈36 pt. Справа до волос Миты на этой высоте ≥42 pt
(проверено по всем 127 кадрам). Тело букв в файле имеет альфу 252–253, а не 255 — на глаз незаметно.

Другой логотип: положить PNG в `resources/logo/`, добавить путь в `mod.json` → `resources.sprites` и указать
`logo-sprite`. Нет файла — нет логотипа (в логе предупреждение), меню работает.

## Нужен арт (пока нет)

| Файл (предлагаемое имя) | Назначение | Размер | Как подключить | Сейчас |
| --- | --- | --- | --- | --- |
| `character.png` | Статичный персонаж справа (необязательный слой из прошлой итерации; анимированная Мита его заменяет) | высота экрана, PNG RGBA | в `resources/background/`, в теме `"character-sprite": "zhulis.miside-menu/character.png"` | выключен (`character-sprite: ""`) |

## Шрифты

| Файл | Назначение | Формат | Где используется | Ключ темы | Сейчас |
| --- | --- | --- | --- | --- | --- |
| `miside-font.fnt` (+ `.png`, `-hd`, `-uhd`) | Пункты меню, ник, попап | BMFont, генерирует Geode CLI | `ui/MSStyle.cpp` | `font` | из `resources/fonts/MPLUSRounded1c-Bold.ttf` |
| `miside-title.fnt` (+ `.png`, `-hd`, `-uhd`) | «ГЛАВНОЕ МЕНЮ» и текстовый логотип | BMFont, генерирует Geode CLI | `ui/MSMenuColumn.cpp`, `ui/MSLogo.cpp` | `title-font` | из `resources/fonts/MPLUSRounded1c-ExtraBold.ttf` |

M PLUS Rounded 1c (Google Fonts, SIL OFL) — скруглённый, с полной кириллицей, статический (без инстанцирования).
Генерация — `mod.json` → `resources.fonts`: `miside-font` размером 64, `miside-title` размером 96, оба с
`charset` `32-126,1025,1040-1103,1105,8211,8212,8226,8230` (ASCII, Ё, А–я, ё, `–` `—` `•` `…`).

Метрики (UHD-атлас, в points при масштабе 1): Bold — заглавные ≈11.3, строка 23.75; ExtraBold — заглавные ≈17,
строка 35.5. Отсюда `item-scale` 0.75 (≈8.5 pt), `subtitle-scale` 0.62 (≈10.5 pt), `name-scale` 0.5. Заглавные
этого шрифта сидят выше середины строки — поправка `item-text-offset` −1.2 pt. «ACHIEVEMENTS» при 0.75 —
≈90 pt из 143 pt строки.

Если шрифта нет, текст пишется системным `fallback-font` (Arial, не файл мода).

## Звуки

| Файл | Назначение | Формат | Где используется | Ключ темы | Сейчас |
| --- | --- | --- | --- | --- | --- |
| `hover.ogg` | Наведение / выбор стрелками | OGG Vorbis | `ui/MSMenuColumn.cpp` | `hover-sound` | Kenney UI Audio `rollover2.ogg` (CC0) |
| `click.ogg` | Выбор пункта (раньше) | OGG Vorbis | — | — | Kenney UI Audio `click1.ogg` (CC0); `hover.ogg` и `click.ogg` больше не используются по умолчанию |
| `miside-table-enter.flac` | Наведение на кнопку | FLAC | все меню мода | `hover-sound` | MiSide Demo, «Table Enter» |
| `miside-table-click.flac` | Нажатие | FLAC | все меню мода | `click-sound` | «Table Click» |
| `miside-table-toggle.flac` | Переключатели настроек GD | FLAC | `Sounds.cpp` | `toggle-sound` | «Table Toggle» |
| `miside-switch-category.flac` | Страницы More Options, вкладки гардероба | FLAC | `Sounds.cpp` | `tab-sound` | «Switch Kategory» |
| `miside-switch-open.flac`, `miside-switch-close.flac` | «ДОПОЛНИТЕЛЬНО» в паузе | FLAC | `pause.cpp` | `more-open-sound`, `more-close-sound` | «Switch Open», «Switch Close» |
| `miside-gift-poof.flac` | Выбор иконки в гардеробе | FLAC | `Sounds.cpp` | `outfit-sound` | «Gift Poof» |
| `miside-achievement.flac` | Достижение (вместо achievement_01 GD) | FLAC | `Sounds.cpp` | `achievement-sound` | «Achievement» |
| `miside-win.flac` | Прохождение уровня (вместо endStart_02 GD) | FLAC | `Sounds.cpp` | `win-sound` | «Win» |
| `miside-startpause.flac` | Открытие паузы | FLAC | `pause.cpp` | `pause-start-sound` | «Startpause» |
| `miside-ambientpause.flac` | Ambient паузы, петля 11.71 с | FLAC | `pause/MSPauseAmbient.cpp` | `pause-ambient-path` | «Ambientpause» |
| `miside-stress.flac` | Ambient паузы рядом с рекордом, петля 14.43 с | FLAC | `Sounds.cpp`, `pause.cpp` | `pause-stress-path` | «Stress» |

FLAC скопированы без изменений (исходники — `Downloads/miside-sfx`): петли Ambientpause и Stress сшиты в самом
файле (скачок на стыке меньше обычного шага сигнала), FLAC сохраняет их точно до сэмпла.

## Музыка

| Файл | Исходник (Downloads/miside-sounds) | Назначение | Ключ темы |
| --- | --- | --- | --- |
| `music/miside-music-menu-update.ogg` | 02 MusicMenu Update.mp3 | Главное меню (по умолчанию) | `menu-music-path` |
| `music/miside-music-menu.ogg` | 01 MusicMenu.mp3 | Главное меню, альтернатива | `menu-music-path` |
| `music/miside-menu-alternative.ogg` | 26 MiSide Menu Alternative.mp3 | Главное меню, альтернатива | `menu-music-path` |
| `music/miside-ambient-chibi-room.ogg` | 06 Ambient - Chibi Room.mp3 | Гардероб | `garage-music-path` |
| `music/miside-ambient-loop-room.ogg` | 05 Ambient - LoopRoom.mp3 | Ambient паузы | `pause-ambient-path` |

OGG Vorbis q6, стерео, 44.1 кГц. Тишина в начале и конце обрезана (`silenceremove` до −70 dB): у 01 было 5.4 с
тишины в конце, у 02 — 0.1 с в начале и ~1 с в конце, у 26 — ~1 с в конце; у петель 05 и 06 убрана пустота
кодировщика MP3 на стыке (88.000 и 129.600 с). Остальные треки не подключены и не скопированы.
