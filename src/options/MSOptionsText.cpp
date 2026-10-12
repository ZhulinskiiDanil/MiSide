#include "MSOptionsText.hpp"

#include <string_view>

namespace
{
  struct Text
  {
    // The game variable (empty: matched by GD's text only)
    char const* key;
    // GD's English text on one line
    char const* en;
    char const* ru;
    char const* infoRu;
  };

  // GD 2.2081's settings (MoreOptionsLayer, MoreVideoOptionsLayer, ParentalOptionsLayer, the pause's
  // GameOptionsLayer), as their windows make them
  constexpr Text TEXTS[] = {
    // Gameplay
    {"0026", "Auto-Retry", "АВТОПОВТОР", "После смерти уровень сразу начинается заново."},
    {"0052", "Enable Faster Reset", "БЫСТРЫЙ ПЕРЕЗАПУСК", "После смерти уровень начинается заново через 0,5 с, а не через 1 с."},
    {"0177", "Click Between Steps", "КЛИКИ МЕЖДУ ШАГАМИ",
     "Включает подшаги: нажатие засчитывается и между обычными шагами игры. Может влиять на производительность."},
    {"0176", "Click On Steps", "КЛИКИ НА КАЖДОМ ШАГЕ",
     "Нажатие засчитывается на каждом шаге игры, какой бы ни была частота экрана. Полезно, если она ниже 240 Гц."},
    {"0128", "Lock Cursor In-Game", "ЗАКРЕПИТЬ КУРСОР", "Во время игры курсор закреплён и скрыт."},
    {"0010", "Flip 2-Player Controls", "ПОМЕНЯТЬ СТОРОНЫ ИГРОКОВ",
     "Меняет местами половины экрана, которыми управляют игроки в режиме на двоих."},
    {"0011", "Always Limit Controls", "ВСЕГДА ДЕЛИТЬ ЭКРАН",
     "Первый игрок управляет только своей половиной экрана, даже без двойного режима."},
    {"0028", "Disable Thumbstick", "ОТКЛЮЧИТЬ СТИК", "Стик геймпада больше не двигает курсор мыши."},
    {"0163", "Enable Quick Keys", "БЫСТРЫЕ КЛАВИШИ",
     "Временные быстрые клавиши, пока нет полной настройки: R — заново, Ctrl + R — полностью заново, P — хитбоксы в "
     "режиме практики."},
    // Visual
    {"0024", "Show Cursor In-Game", "КУРСОР В ИГРЕ", "Во время игры видны курсор и кнопка паузы."},
    {"0135", "Hide Attempts", "СКРЫТЬ ПОПЫТКИ", "Прячет счётчик попыток на уровнях."},
    {"0015", "Flip Pause Button", "ПАУЗА С ДРУГОЙ СТОРОНЫ", "Переносит кнопку паузы на другую сторону экрана."},
    {"0129", "Disable Portal Guide", "БЕЗ ПОДСКАЗОК ПОРТАЛОВ", "Убирает дополнительные указатели на порталах."},
    {"0130", "Enable Orb Guide", "ПОДСКАЗКИ СФЕР", "Добавляет указатели на сферах."},
    {"0140", "Disable Orb Scale", "СФЕРЫ БЕЗ УВЕЛИЧЕНИЯ", "Сферы больше не увеличиваются."},
    {"0141", "Disable Trigger Orb Scale", "СФЕРЫ-ТРИГГЕРЫ БЕЗ УВЕЛИЧЕНИЯ", "Не увеличиваются только сферы-триггеры."},
    {"0172", "Disable Shake", "БЕЗ ТРЯСКИ", "Отключает тряску экрана."},
    {"0014", "Disable Explosion Shake", "БЕЗ ТРЯСКИ ПРИ СМЕРТИ", "Экран не трясётся, когда игрок разбивается."},
    {"0072", "Disable Gravity Effect", "БЕЗ ЭФФЕКТА ГРАВИТАЦИИ", "Убирает эффект при смене гравитации."},
    {"0060", "Default Mini Icon", "ОБЫЧНАЯ МИНИ-ИКОНКА", "В мини-режиме у игрока стандартная иконка."},
    {"0061", "Switch Spider Teleport Color", "ЦВЕТ ТЕЛЕПОРТА ПАУКА",
     "Эффект телепорта паука берёт второй цвет вместо основного (или наоборот)."},
    {"0062", "Switch Dash Fire Color", "ЦВЕТ ОГНЯ РЫВКА",
     "Огонь от сфер рывка берёт второй цвет вместо основного (или наоборот)."},
    {"0096", "Switch Wave Trail Color", "ЦВЕТ СЛЕДА ВОЛНЫ",
     "След волны берёт второй цвет вместо основного (или наоборот)."},
    {"0174", "Hide Playtest Text", "СКРЫТЬ ТЕКСТ ТЕСТА",
     "Прячет текст слева вверху при старте с позиции или с отключённым уроном."},
    {"0179", "Hitbox On Death", "ХИТБОКСЫ ПРИ СМЕРТИ", "После смерти показывает хитбоксы, в обычном режиме и в практике."},
    // Practice
    {"0071", "Hide Practice Buttons", "СКРЫТЬ КНОПКИ ПРАКТИКИ", "Прячет кнопки контрольных точек в режиме практики."},
    {"0134", "Hide Attempts", "СКРЫТЬ ПОПЫТКИ", "Прячет счётчик попыток в режиме практики."},
    {"0027", "Enable Auto-Checkpoints", "АВТОТОЧКИ", "В практике контрольные точки ставятся сами."},
    {"0068", "Enable Quick Checkpoints", "ЧАСТЫЕ ТОЧКИ", "В практике контрольные точки ставятся чаще."},
    {"0100", "Enable Death Effect", "ЭФФЕКТ СМЕРТИ", "Показывает эффект смерти в режиме практики."},
    {"0125", "Enable Normal Music In Editor", "ОБЫЧНАЯ МУЗЫКА В РЕДАКТОРЕ",
     "В практике уровней из редактора играет обычная музыка, в такт уровню."},
    {"0166", "Show Hitboxes", "ПОКАЗАТЬ ХИТБОКСЫ", "Показывает хитбоксы в режиме практики."},
    {"0171", "Disable Player Hitbox", "БЕЗ ХИТБОКСА ИГРОКА",
     "В практике не показывает хитбокс самого игрока (когда хитбоксы видны)."},
    // Performance
    {"0066", "Increase Draw Capacity", "БОЛЬШЕ ОТРИСОВКИ",
     "Заранее выделяет больше места под отрисовку на старте уровня. На некоторых уровнях быстрее, но на слабых "
     "устройствах возможны проблемы."},
    {"0108", "Enable Low Detail", "НИЗКАЯ ДЕТАЛИЗАЦИЯ", "Сам включает низкую детализацию на уровнях, где она есть."},
    {"0082", "Disable High Object Alert", "БЕЗ ПРЕДУПРЕЖДЕНИЯ ОБ ОБЪЕКТАХ",
     "Не предупреждает перед уровнями с большим числом объектов."},
    {"0136", "Enable Extra LDM", "ДОП. НИЗКАЯ ДЕТАЛИЗАЦИЯ",
     "При низкой детализации убирает свечение и эффекты появления. Уровни без неё показываются облегчёнными."},
    {"0042", "Increase Maximum Levels", "БОЛЬШЕ СОХРАНЁННЫХ УРОВНЕЙ",
     "Хранит до 100 скачанных уровней вместо 10 (сами уровни, не статистику). Сохранение может сильно вырасти, "
     "поэтому для быстрого сохранения лучше не включать."},
    {"0119", "Disable Level Saving", "НЕ СОХРАНЯТЬ УРОВНИ",
     "Статистика сохраняется как обычно, но уровни скачиваются заново после каждого запуска. Сохранение и загрузка "
     "быстрее."},
    {"0127", "Save Gauntlets", "СОХРАНЯТЬ ИСПЫТАНИЯ",
     "Уровни испытаний хранятся на устройстве, их не нужно скачивать заново. Сохранение дольше, но помогает при "
     "плохой связи."},
    {"0155", "Disable Shader Anti-Aliasing", "БЕЗ СГЛАЖИВАНИЯ ШЕЙДЕРОВ", "Отключает сглаживание у шейдерных эффектов."},
    // Audio
    {"0033", "Change Custom Songs Location", "ДРУГАЯ ПАПКА ПЕСЕН",
     "Хранит пользовательские песни в другой папке. Может помочь, если они не играют."},
    {"0083", "Disable Song Alert", "БЕЗ ПРЕДУПРЕЖДЕНИЯ О ПЕСНЕ",
     "Не предупреждает перед уровнем, если его песня не скачана."},
    {"0018", "No Song Limit", "БЕЗ ЛИМИТА ПЕСЕН",
     "Не удаляет пользовательские песни сама. Обычно они удаляются, чтобы экономить место."},
    {"0142", "Reduce Quality (Read Info)", "НИЗКОЕ КАЧЕСТВО ЗВУКА",
     "Снижает частоту звука с 44100 до 24000 Гц. Действует после перезапуска игры."},
    {"0159", "Audio Fix 01", "ИСПРАВЛЕНИЕ ЗВУКА 01",
     "Увеличивает звуковой буфер, это может исправить некоторые проблемы. Не включайте, если со звуком всё хорошо: "
     "звук станет чуть запаздывать. Действует после перезапуска игры."},
    // Other
    {"0094", "More Comments", "БОЛЬШЕ КОММЕНТАРИЕВ", "Больше комментариев на странице. Почему бы и нет?"},
    {"0090", "Load Comments", "ЗАГРУЖАТЬ КОММЕНТАРИИ", "Комментарии загружаются сами."},
    {"0073", "New Completed Filter", "НОВЫЙ ФИЛЬТР ПРОЙДЕННЫХ",
     "Фильтр пройденных уровней учитывает только процент из обновления 2.1. Полезно, чтобы перепройти уровни ради "
     "сфер маны."},
    {"0093", "Increase Local Levels Per Page", "БОЛЬШЕ УРОВНЕЙ НА СТРАНИЦЕ",
     "20 созданных и сохранённых уровней на странице вместо 10."},
    {"0084", "Manual Level Order", "РУЧНОЙ ПОРЯДОК УРОВНЕЙ",
     "Новые уровни встают в конец списка сохранённых. Удобно, если поднимаете уровни наверх вручную."},
    {"0126", "Percentage Decimals", "ДРОБНЫЕ ПРОЦЕНТЫ", "Показывает прогресс уровня с дробной частью."},
    {"0099", "Show Leaderboard Percentage", "ПРОЦЕНТ ДЛЯ ТАБЛИЦЫ ЛИДЕРОВ",
     "Показывает процент уровня, который идёт в таблицу лидеров. Чтобы он туда попал, уровни, пройденные до 2.11, "
     "нужно перепройти."},
    {"0095", "Do Not...", "НЕ НАДО...", "Ничего не делает... Ну, ничего полезного."},
    {"0167", "Confirm Exit", "ПОДТВЕРЖДАТЬ ВЫХОД", "Перед выходом с уровня спрашивает ещё раз."},
    {"0168", "Fast Menu", "БЫСТРОЕ МЕНЮ", "Переходы между страницами меню быстрее."},
    // Advanced video
    {"0030", "Vertical Sync", "ВЕРТИКАЛЬНАЯ СИНХРОНИЗАЦИЯ",
     "Убирает разрывы картинки в полноэкранном режиме. В оконном может мешать."},
    {"0023", "Smooth Fix", "SMOOTH FIX",
     "Оптимизации, которые могут уменьшить лаги. Выключите, если скорость игры начнёт плавать."},
    {"0115", "Show FPS", "ПОКАЗЫВАТЬ FPS", "Показывает число кадров в секунду во время игры."},
    {"0116", "Unlock FPS", "СВОЙ ПРЕДЕЛ FPS",
     "Позволяет задать свой предел кадров в секунду. Не действует при включённой вертикальной синхронизации."},
    // Parental control
    {"0075", "Disable Comments", "ОТКЛЮЧИТЬ КОММЕНТАРИИ", ""},
    {"0076", "Disable Account Comments", "ОТКЛЮЧИТЬ КОММЕНТАРИИ ПРОФИЛЕЙ", ""},
    {"0077", "Only Allow Featured Levels", "ТОЛЬКО ИЗБРАННЫЕ УРОВНИ", ""},
    // The editor's options (by GD's text: GJOptionsLayer keeps their variables as numbers)
    {"", "Draw Trigger Boxes", "РАМКИ ТРИГГЕРОВ", "Рисует хитбоксы объектов, срабатывающих от касания."},
    {"", "Playtest No UI", "ТЕСТ БЕЗ ИНТЕРФЕЙСА", "Прячет часть интерфейса редактора во время теста."},
    {"", "Playtest Music", "МУЗЫКА В ТЕСТЕ", "Во время теста играет музыка."},
    {"", "Effect Lines", "ЛИНИИ ЭФФЕКТОВ", "Вертикальная линия показывает, где срабатывает триггер."},
    {"", "Duration Lines", "ЛИНИИ ДЛИТЕЛЬНОСТИ", "Горизонтальная линия показывает, сколько действует триггер."},
    {"", "Grid On Top", "СЕТКА ПОВЕРХ", "Сетка рисуется поверх объектов."},
    {"", "Playtest No Grid", "ТЕСТ БЕЗ СЕТКИ", "Прячет сетку во время теста."},
    {"", "Hide Background", "СКРЫТЬ ФОН", "Прячет фон в редакторе."},
    {"", "Enable Link Controls", "СВЯЗИ ОБЪЕКТОВ",
     "Позволяет связывать объекты: связанные всегда выделяются и удаляются вместе."},
    {"", "Hold To Swipe", "ВЫДЕЛЕНИЕ УДЕРЖАНИЕМ",
     "Если немного подержать нажатие на сетке, включается выделение проведением — без отдельной кнопки."},
    {"", "Swipe Cycle Mode", "ПЕРЕБОР ПРИ ВЫДЕЛЕНИИ",
     "При выделении проведением нажатие в том же месте перебирает объекты под ним, а не добавляет их."},
    {"", "Layer Locking", "БЛОКИРОВКА СЛОЁВ", "Клик по номеру слоя редактора закрывает и открывает слой."},
    {"", "Show Clicks", "ПОКАЗЫВАТЬ НАЖАТИЯ", "Во время теста отмечает, где игрок нажал."},
    {"", "Auto-Pause", "АВТОПАУЗА",
     "При старте с позиции игра ждёт второго нажатия. Помогает с лагом на старте больших уровней."},
    {"", "Hide Path", "СКРЫТЬ ПУТЬ", "Не показывает путь игрока после теста."},
    {"", "Start Optimization", "БЫСТРЫЙ СТАРТ", "Пытается ускорить загрузку стартовых позиций. Экспериментально."},
    {"", "Hide Particle Icons", "СКРЫТЬ ИКОНКИ ЧАСТИЦ", "Прячет иконки частиц в режиме предпросмотра."},
    {"", "Increase Undo/Redo", "БОЛЬШЕ ОТМЕН", "До 1000 отмен и повторов вместо 200."},
    {"", "Playtest Smooth Fix", "SMOOTH FIX В ТЕСТЕ", "Включает smooth fix при тесте в редакторе."},
    {"", "Increase Scale", "БОЛЬШЕ МАСШТАБ",
     "Масштаб объектов от 0,25x до 4x. Объекты больше 6 клеток могут пропадать у края экрана."},
    {"", "Disable Paste State Groups", "ВСТАВКА СОСТОЯНИЯ БЕЗ ГРУПП",
     "Вставка состояния не меняет группы объектов; их можно вставить на странице групп."},
    {"", "Small Warp Buttons", "МАЛЕНЬКИЕ КНОПКИ ДЕФОРМАЦИИ",
     "Кнопки деформации вдвое меньше (после перезапуска редактора)."},
    {"", "Static Trace Arrows", "НЕПОДВИЖНЫЕ СТРЕЛКИ СВЯЗЕЙ", "Отключает стрелки направленных связей в Trace In/Out."},
    // The pause's options (by GD's text: their ids aren't game variables)
    {"", "Auto-Retry", "АВТОПОВТОР", "После смерти уровень сразу начинается заново."},
    {"", "Auto-Checkpoints", "АВТОТОЧКИ", "В практике контрольные точки ставятся сами."},
    {"", "Show Progress Bar", "ПОЛОСА ПРОГРЕССА", "Показывает полосу прогресса уровня."},
    {"", "Show Percentage", "ПРОЦЕНТ ПРОГРЕССА", "Показывает процент прохождения уровня."},
    {"", "Show Time", "ВРЕМЯ", "Показывает время на уровне."},
    {"", "Audio Visualizer", "ВИЗУАЛИЗАТОР ЗВУКА", "Показывает визуализатор музыки."},
    {"", "Show Info Label", "ИНФОРМАЦИЯ ОТЛАДКИ", "Показывает подробную информацию об уровне в углу экрана."},
    {"", "Disable Checkpoints", "БЕЗ КОНТРОЛЬНЫХ ТОЧЕК", "Отключает контрольные точки."},
    {"", "Show Hitboxes", "ПОКАЗАТЬ ХИТБОКСЫ", "Показывает хитбоксы в режиме практики."},
    {"", "Hitbox On Death", "ХИТБОКСЫ ПРИ СМЕРТИ", "После смерти показывает хитбоксы, в обычном режиме и в практике."},
  };

  struct Pair
  {
    char const* en;
    char const* ru;
  };

  constexpr Pair CATEGORIES[] = {
    {"Gameplay", "ИГРА"},   {"Visual", "ВИЗУАЛ"}, {"Practice", "ПРАКТИКА"}, {"Performance", "ПРОИЗВОДИТЕЛЬНОСТЬ"},
    {"Audio", "ЗВУК"},      {"Other", "ПРОЧЕЕ"},  {"Options", "НАСТРОЙКИ"},
  };

  constexpr Pair ACTIONS[] = {
    {"Gameplay Keys", "ИГРА"},
    {"Editor Keys", "РЕДАКТОР"},
    {"Jump", "ПРЫЖОК"},
    {"Pause", "ПАУЗА"},
    {"Place Checkpoint", "ПОСТАВИТЬ ТОЧКУ"},
    {"Delete Checkpoint", "УБРАТЬ ТОЧКУ"},
    {"Build Mode", "РЕЖИМ СТРОЙКИ"},
    {"Edit Mode", "РЕЖИМ ПРАВКИ"},
    {"Delete Mode", "РЕЖИМ УДАЛЕНИЯ"},
    {"Swipe Modifier", "ВЫДЕЛЕНИЕ ПРОВЕДЕНИЕМ"},
    {"Move Modifier", "ПЕРЕМЕЩЕНИЕ"},
    {"Rotate CCW", "ПОВОРОТ ПРОТИВ ЧАСОВОЙ"},
    {"Rotate CW", "ПОВОРОТ ПО ЧАСОВОЙ"},
    {"Flip X", "ОТРАЗИТЬ ПО X"},
    {"Flip Y", "ОТРАЗИТЬ ПО Y"},
    {"Delete selected", "УДАЛИТЬ ВЫДЕЛЕННОЕ"},
    {"Undo", "ОТМЕНИТЬ"},
    {"Redo", "ПОВТОРИТЬ"},
    {"Deselect", "СНЯТЬ ВЫДЕЛЕНИЕ"},
    {"Copy", "КОПИРОВАТЬ"},
    {"Paste", "ВСТАВИТЬ"},
    {"Duplicate", "ДУБЛИРОВАТЬ"},
    {"Rotate", "ВРАЩЕНИЕ"},
    {"Free Move", "СВОБОДНОЕ ПЕРЕМЕЩЕНИЕ"},
    {"Swipe", "ПРОВЕДЕНИЕ"},
    {"Snap", "ПРИВЯЗКА"},
    {"Playtest", "ТЕСТ УРОВНЯ"},
    {"Playback music", "ПРОИГРАТЬ МУЗЫКУ"},
    {"Next Group", "СЛЕДУЮЩАЯ ГРУППА"},
    {"Previous Group", "ПРЕДЫДУЩАЯ ГРУППА"},
    {"Object Left", "ОБЪЕКТ ВЛЕВО"},
    {"Object Right", "ОБЪЕКТ ВПРАВО"},
    {"Object Up", "ОБЪЕКТ ВВЕРХ"},
    {"Object Down", "ОБЪЕКТ ВНИЗ"},
    {"Object Left small", "ОБЪЕКТ ЧУТЬ ВЛЕВО"},
    {"Object Right small", "ОБЪЕКТ ЧУТЬ ВПРАВО"},
    {"Object Up small", "ОБЪЕКТ ЧУТЬ ВВЕРХ"},
    {"Object Down small", "ОБЪЕКТ ЧУТЬ ВНИЗ"},
    {"Lock Preview", "ЗАКРЕПИТЬ ПРЕДПРОСМОТР"},
    {"Unlock Preview", "ОТКРЕПИТЬ ПРЕДПРОСМОТР"},
    {"Preview Mode On/Off", "ПРЕДПРОСМОТР ВКЛ/ВЫКЛ"},
    {"Particle Icons On/Off", "ИКОНКИ ЧАСТИЦ ВКЛ/ВЫКЛ"},
    {"Debug Draw On/Off", "ОТЛАДОЧНАЯ ОТРИСОВКА ВКЛ/ВЫКЛ"},
    {"Hide Invisible On/Off", "СКРЫВАТЬ НЕВИДИМОЕ ВКЛ/ВЫКЛ"},
    {"Save Editor Pos", "ЗАПОМНИТЬ ПОЗИЦИЮ"},
    {"Load Editor Pos", "ВЕРНУТЬСЯ К ПОЗИЦИИ"},
    {"Zoom", "МАСШТАБ"},
    {"Transform", "ТРАНСФОРМАЦИЯ"},
  };

  constexpr Pair KEY_WORDS[] = {
    {"Space", "ПРОБЕЛ"},           {"Up", "ВВЕРХ"},          {"Escape", "ESC"},
    {"Arrow Right", "СТРЕЛКА ВПРАВО"}, {"Arrow Left", "СТРЕЛКА ВЛЕВО"}, {"ScrollWheel", "КОЛЕСО"},
  };

  Text const* find(ms::options::Toggle const& toggle)
  {
    auto en = ms::options::oneLine(toggle.label);
    if (!toggle.key.empty())
    {
      for (auto const& text : TEXTS)
      {
        if (toggle.key == text.key && en == text.en)
          return &text;
      }
    }
    for (auto const& text : TEXTS)
    {
      if (std::string_view(text.key).empty() && en == text.en)
        return &text;
    }
    // A variable the mod knows under another text (GD may change its wording)
    for (auto const& text : TEXTS)
    {
      if (!toggle.key.empty() && toggle.key == text.key)
        return &text;
    }
    return nullptr;
  }

  char const* pick(Pair const* begin, Pair const* end, std::string const& en)
  {
    for (auto it = begin; it != end; ++it)
    {
      if (en == it->en)
        return it->ru;
    }
    return nullptr;
  }
} // namespace

namespace ms::options
{
  std::string upper(std::string const& text)
  {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); i++)
    {
      auto c = static_cast<unsigned char>(text[i]);
      if (c >= 'a' && c <= 'z')
      {
        out.push_back(static_cast<char>(c - 'a' + 'A'));
        continue;
      }
      // Cyrillic in UTF-8: а..п D0 B0..BF -> D0 90..9F, р..я D1 80..8F -> D0 A0..AF, ё D1 91 -> D0 81
      if (i + 1 < text.size())
      {
        auto next = static_cast<unsigned char>(text[i + 1]);
        if (c == 0xD0 && next >= 0xB0 && next <= 0xBF)
        {
          out.push_back(static_cast<char>(0xD0));
          out.push_back(static_cast<char>(next - 0x20));
          i++;
          continue;
        }
        if (c == 0xD1 && next >= 0x80 && next <= 0x8F)
        {
          out.push_back(static_cast<char>(0xD0));
          out.push_back(static_cast<char>(next + 0x20));
          i++;
          continue;
        }
        if (c == 0xD1 && next == 0x91)
        {
          out.push_back(static_cast<char>(0xD0));
          out.push_back(static_cast<char>(0x81));
          i++;
          continue;
        }
      }
      out.push_back(static_cast<char>(c));
    }
    return out;
  }

  std::string label(Toggle const& toggle, bool en)
  {
    if (!en)
    {
      if (auto text = find(toggle))
        return text->ru;
    }
    return upper(oneLine(toggle.label));
  }

  std::string info(Toggle const& toggle, bool en)
  {
    if (!en)
    {
      if (auto text = find(toggle); text && *text->infoRu)
        return text->infoRu;
    }
    return oneLine(toggle.info);
  }

  std::string category(std::string const& name, bool en)
  {
    if (!en)
    {
      if (auto ru = pick(std::begin(CATEGORIES), std::end(CATEGORIES), name))
        return ru;
    }
    return upper(name);
  }

  std::string keyAction(std::string const& action, bool en)
  {
    if (!en)
    {
      if (auto ru = pick(std::begin(ACTIONS), std::end(ACTIONS), action))
        return ru;
    }
    return upper(oneLine(action));
  }

  std::string keyNames(std::string const& keys, bool en)
  {
    if (en)
      return upper(keys);
    // Word by word between the separators GD uses (" / ", " + ")
    std::string out;
    size_t start = 0;
    while (start <= keys.size())
    {
      auto next = keys.find_first_of("/+", start);
      auto end = next == std::string::npos ? keys.size() : next;
      auto word = keys.substr(start, end - start);
      auto first = word.find_first_not_of(' ');
      auto last = word.find_last_not_of(' ');
      auto core = first == std::string::npos ? std::string() : word.substr(first, last - first + 1);
      auto ru = pick(std::begin(KEY_WORDS), std::end(KEY_WORDS), core);
      out += word.substr(0, first == std::string::npos ? word.size() : first);
      out += ru ? std::string(ru) : upper(core);
      if (last != std::string::npos)
        out += word.substr(last + 1);
      if (next == std::string::npos)
        break;
      out.push_back(keys[next]);
      start = next + 1;
    }
    return out;
  }
} // namespace ms::options
