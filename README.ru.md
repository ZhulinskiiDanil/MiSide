# MiSide Menu

[English](README.md)

Мод для Geometry Dash 2.2081 (Windows и Android) на [Geode](https://geode-sdk.org), который оформляет экраны GD в стиле интерфейса MiSide: экран загрузки с интро AIHASTO, главное меню с Митой, «Создать», официальные уровни, паузу, карточку прохождения уровня, достижения, статистику и настройки. Действия и данные GD и кнопки других модов работают как прежде; то, что мод не трогает, остаётся как в GD.

## Установка

Скачай `zhulis.miside-menu.geode` из [релизов](https://github.com/ZhulinskiiDanil/MiSide/releases), положи в `Geometry Dash/geode/mods/` и перезапусти игру.

## Настройки

| Настройка | По умолчанию | |
| --- | --- | --- |
| Vanilla menu | выкл | Главное меню точно как без мода |
| Parallax strength | 1.0 | Насколько фон следует за мышью, от 0 до 2 |
| Menu language | RU | RU или EN |
| Logo | вкл | Логотип и «ГЛАВНОЕ МЕНЮ» над пунктами |
| Studio intro | вкл | Интро AIHASTO при запуске игры |
| Confirm leaving a level | вкл | Пауза спрашивает «Выйти из уровня?» перед выходом |

## Сборка

Нужны [Geode SDK](https://docs.geode-sdk.org/getting-started/sdk) и CLI:

```
geode build
```

CI собирает Windows и Android на каждый push (`.github/workflows`). Перед релизом — [docs/CHECKLIST.ru.md](docs/CHECKLIST.ru.md).

## Авторы ресурсов

Шрифты — [M PLUS Rounded 1c](https://fonts.google.com/specimen/M+PLUS+Rounded+1c) (SIL Open Font License), звуки — [Kenney UI Audio](https://kenney.nl/assets/ui-audio) (CC0). Все лицензии — в [LICENSES.md](LICENSES.md), все ассеты — в [ASSETS.md](ASSETS.md).
