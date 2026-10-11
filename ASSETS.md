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
| `row-plate.png` | Плашка каждого пункта, 9-slice со скруглением 3 pt. Тонируется `row-*-color` / `row-*-opacity` по состоянию — рисовать **белой** | 12×12 / 24×24 / 48×48 | PNG, RGBA, белая | `ui/MSMenuItem.cpp` | `row-sprite` |
| `glow-soft.png` | Мягкий круглый свет (белый, альфа спадает от центра), растягивается до `glow-width` × `glow-height`, тонируется `glow-color` | 64×64 / 128×128 / 256×256 | PNG, RGBA, белый | `ui/MSBackground.cpp` | `glow-sprite` |
| `item-accent.png` | Полоска слева от выделенного пункта — по умолчанию выключена (`accent-enabled: false`) | 3×24 / 6×48 / 12×96 | PNG, RGBA, белая | `ui/MSMenuItem.cpp` | `accent-sprite` |
| `tray-panel.png` | Подложка лотка, карточки профиля и попапа «…», 9-slice — рисовать **белой** | 24×24 / 48×48 / 96×96 | PNG, RGBA, белая | `ui/MSModTray.cpp`, `ui/MSProfileCard.cpp`, `ui/MSMorePopup.cpp` | `tray-sprite` |
| `tray-slot.png` | Ячейка под кнопкой лотка, 9-slice (по умолчанию невидима: `slot-opacity: 0`) | 16×16 / 32×32 / 64×64 | PNG, RGBA, белая | `ui/MSModTray.cpp` | `slot-sprite` |
| `avatar-circle.png` | Круг под иконкой игрока в карточке профиля | 24×24 / 48×48 / 96×96 | PNG, RGBA, белый | `ui/MSProfileCard.cpp` | `avatar-sprite` |

Все — собственные простые формы из `resources/placeholders/`. Нет файла — прямоугольник того же цвета (у
свечения — без свечения). Если нет `tray-panel.png`, попап берёт ванильный `GJ_square01.png` из игры.

## Мита

| Файл | Назначение | Размер | Где используется | Ключ темы |
| --- | --- | --- | --- | --- |
| `mita_atlas_00.png` … `mita_atlas_21.png` | 22 страницы атласа анимации ожидания: по 6 кадров 640×720 (последняя — 1 кадр), extrusion 2 px, без поворота и trim | 1932×1448 (последняя 644×724), PNG RGBA, прямая альфа | `ui/MSMita.cpp` | `mita-*` |
| `mita-manifest.json` | Порядок кадров, прямоугольники, страницы, fps, canvas, размещение | — | `ui/MSMita.cpp` | — |
| `MITA_ATTRIBUTION.txt`, `mita-source-metadata.json` | Атрибуция CC BY 4.0 и метаданные исходной модели — распространяются вместе с атласами | — | — | — |

Лежат в `resources/mita/`, в пакет идут через `resources.files` (без -hd/-uhd: размер на экране считается из
размера кадра). Plist, `.blend`, исходный GLB, превью и отдельные кадры в пакет не входят: прямоугольники берутся
из манифеста. Пакет мода из-за атласов — около 49 МБ. На GPU одновременно не больше двух страниц (~21.3 МиБ), в
CPU — не больше двух декодированных страниц (~21.3 МиБ).

## Нужен арт (пока нет)

| Файл (предлагаемое имя) | Назначение | Размер | Как подключить | Сейчас |
| --- | --- | --- | --- | --- |
| `logo.png` | Собственный декоративный логотип «Geometry Dash»: насыщенный розовый, светлые блики, небольшая тёмная объёмная тень, 1–2 геометрических элемента GD. Не копировать начертание и украшения MiSide | вписывается в 148×36 pt (`logo-*`), UHD около 592×144 px, PNG RGBA | положить в `resources/background/`, в теме `"logo-sprite": "zhulis.miside-menu/logo.png"` | **текстовый плейсхолдер**: «Geometry Dash» шрифтом темы с бликом, тенью, ромбом-кубиком и шипом (`ui/MSLogo.cpp`). Это не готовый арт |
| `character.png` | Статичный персонаж справа (необязательный слой из прошлой итерации; анимированная Мита его заменяет) | высота экрана, PNG RGBA | в `resources/background/`, в теме `"character-sprite": "zhulis.miside-menu/character.png"` | выключен (`character-sprite: ""`) |

## Шрифт

| Файл | Назначение | Формат | Где используется | Ключ темы | Сейчас |
| --- | --- | --- | --- | --- | --- |
| `miside-font.fnt` (+ `.png`, `-hd`, `-uhd`) | Пункты, подпись «ГЛАВНОЕ МЕНЮ», текстовый логотип, ник, попап | BMFont, генерирует Geode CLI | `ui/MSStyle.cpp` (`createLabel`) | `font` | из `resources/fonts/RobotoMenu-CondensedBold.ttf` |

`RobotoMenu-CondensedBold.ttf` — статический инстанс Roboto (Google Fonts, SIL OFL): `wght` 700, `wdth` 80,
сделан `tools/make_font.py` из `Roboto[wdth,wght].ttf`; семейство переименовано в «Roboto Menu Condensed», как
OFL просит для изменённых шрифтов. Генерация BMFont — `mod.json` → `resources.fonts.miside-font`:

```json
"miside-font": {
	"path": "resources/fonts/RobotoMenu-CondensedBold.ttf",
	"size": 64,
	"charset": "32-126,1025,1040-1103,1105,8211,8212,8226,8230"
}
```

`charset`: ASCII, `Ё` (1025), `А–я` (1040–1103), `ё` (1105), `–` `—` `•` `…`. Метрики: заглавные 11.3 pt при
масштабе 1, строка 18.75 pt. Отсюда `item-scale` 0.75 (заглавные ≈ 8.6 pt в строке 14 pt), `subtitle-scale` 0.9
(≈ 10 pt), `name-scale` 0.5. Заглавные сидят в строке чуть ниже центра — поправка `item-text-offset` 0.6 pt.

Если шрифта нет, текст пишется системным `fallback-font` (Arial, не файл мода).

## Звуки

| Файл | Назначение | Формат | Где используется | Ключ темы | Сейчас |
| --- | --- | --- | --- | --- | --- |
| `hover.ogg` | Наведение / выбор стрелками | OGG Vorbis | `ui/MSMenuColumn.cpp` | `hover-sound` | Kenney UI Audio `rollover2.ogg` (CC0) |
| `click.ogg` | Выбор пункта | OGG Vorbis | `ui/MSMenuColumn.cpp`, `ui/MSProfileCard.cpp` | `click-sound` | Kenney UI Audio `click1.ogg` (CC0) |

## Опционально: музыка меню (кодом не используется)

| Файл | Назначение | Формат |
| --- | --- | --- |
| `menu-music.ogg` | Своя музыка главного экрана, бесшовный луп | OGG Vorbis, стерео, 44.1 кГц |

Музыку меню GD включает не `MenuLayer`, а общий код игры; подмена — выход за пределы `MenuLayer`
(`OPEN_QUESTIONS.md`).
