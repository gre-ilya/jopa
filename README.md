# libgdbparse

Маленькая C++ библиотека, которая **читает файлы `.gdb`** (Garmin MapSource GPS Database) и отдаёт содержимое в виде обычных C++ структур.

Под капотом — официальный парсер из проекта [gpsbabel](https://www.gpsbabel.org/) 1.9.0. Мы просто завернули его в один удобный вызов.

---

## Что умеет

Открыть файл `.gdb` → получить три списка:

- **`waypoints`** — точки на карте (POI, метки)
- **`routes`** — маршруты (упорядоченные списки точек, которые надо пройти)
- **`tracks`** — треки (записанная история движения)

В каждом списке — структура с понятными полями: `latitude`, `longitude`, `altitude`, `name`, `description`, время создания и так далее. Никаких `QString`, `QVector` и других Qt-типов в API — только `std::string`, `std::vector`, `std::optional`.

---

## Самый короткий пример

```cpp
#include "gdbparse.h"
#include <cstdio>

int main() {
    gdbparse::GdbData data = gdbparse::parse_gdb("my_file.gdb");

    std::printf("Точек: %zu\n",     data.waypoints.size());
    std::printf("Маршрутов: %zu\n", data.routes.size());
    std::printf("Треков: %zu\n",    data.tracks.size());
}
```

Запускаешь — получаешь количество всего, что лежит в файле. Всё.

---

## Сборка

Нужны:

- **CMake** ≥ 3.16
- **C++17** компилятор (GCC, Clang, MSVC, MinGW — любой свежий)
- **Qt5** (>= 5.12) **или Qt6** с компонентом `Core`
- **zlib** (почти всегда стоит из коробки)
- Исходники **gpsbabel 1.9.0** (директория, в которой лежит этот README, должна быть внутри дерева исходников gpsbabel)

### Linux / macOS

```bash
# Из корня проекта gpsbabel:
cmake -S lib_gdbparse -B build_gdbparse
cmake --build build_gdbparse -j
```

После сборки:
- `build_gdbparse/libgdbparse.a` — статическая библиотека
- `build_gdbparse/gdbparse_demo` — готовый бинарь-пример

Запустить пример:

```bash
./build_gdbparse/gdbparse_demo путь/к/файлу.gdb
```

### Windows (MSVC)

Открываем `Developer Command Prompt for VS`:

```bat
cmake -S lib_gdbparse -B build_gdbparse -G "Visual Studio 17 2022" ^
      -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
cmake --build build_gdbparse --config Release
```

Подставь свой путь к Qt в `CMAKE_PREFIX_PATH`.

### Windows (MinGW)

```bat
cmake -S lib_gdbparse -B build_gdbparse -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/mingw81_64"
cmake --build build_gdbparse -j
```

### Опции CMake

| Опция | По умолчанию | Что делает |
|---|---|---|
| `BUILD_SHARED_LIBS` | `OFF` | `ON` → `.so`/`.dll` вместо `.a`/`.lib` |
| `GDBPARSE_BUILD_DEMO` | `ON` | Собирать ли `gdbparse_demo` |
| `GDBPARSE_GPSBABEL_DIR` | `..` | Путь к корню исходников gpsbabel |

Пример с shared lib:

```bash
cmake -S lib_gdbparse -B build_gdbparse -DBUILD_SHARED_LIBS=ON
cmake --build build_gdbparse -j
```

---

## Использование в своём CMake-проекте

Способ 1 — добавить как поддиректорию:

```cmake
add_subdirectory(third_party/gpsbabel/lib_gdbparse)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE gdbparse)
```

Способ 2 — слинковать как уже собранную статическую либу:

```cmake
add_executable(my_app main.cpp)
target_include_directories(my_app PRIVATE path/to/lib_gdbparse)
target_link_libraries(my_app PRIVATE
    path/to/build_gdbparse/libgdbparse.a
    Qt5::Core   # или Qt6::Core
    ZLIB::ZLIB
)
```

---

## Примеры посерьёзнее

### 1. Распечатать все точки

```cpp
#include "gdbparse.h"
#include <cstdio>

int main(int argc, char** argv) {
    auto data = gdbparse::parse_gdb(argv[1]);

    for (const auto& w : data.waypoints) {
        std::printf("%-20s  %10.6f, %10.6f\n",
                    w.shortname.c_str(),
                    w.latitude,
                    w.longitude);
    }
}
```

### 2. Найти точки выше 1000 метров

```cpp
auto data = gdbparse::parse_gdb("alps.gdb");

for (const auto& w : data.waypoints) {
    if (w.altitude.has_value() && *w.altitude > 1000.0) {
        std::printf("HIGH: %s @ %.1f m\n",
                    w.shortname.c_str(),
                    *w.altitude);
    }
}
```

`altitude` — это `std::optional<double>`, потому что в `.gdb` высота может отсутствовать. Поэтому проверяем `has_value()` перед чтением.

### 3. Посчитать длину каждого трека (в метрах, по прямым)

```cpp
#include "gdbparse.h"
#include <cmath>

static double haversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371000.0;   // радиус Земли, м
    auto rad = [](double d) { return d * M_PI / 180.0; };
    double dlat = rad(lat2 - lat1);
    double dlon = rad(lon2 - lon1);
    double a = std::sin(dlat/2) * std::sin(dlat/2)
             + std::cos(rad(lat1)) * std::cos(rad(lat2))
               * std::sin(dlon/2) * std::sin(dlon/2);
    return 2 * R * std::asin(std::sqrt(a));
}

int main() {
    auto data = gdbparse::parse_gdb("hike.gdb");

    for (const auto& trk : data.tracks) {
        double total = 0.0;
        for (std::size_t i = 1; i < trk.waypoints.size(); ++i) {
            const auto& a = trk.waypoints[i - 1];
            const auto& b = trk.waypoints[i];
            total += haversine(a.latitude, a.longitude,
                               b.latitude, b.longitude);
        }
        std::printf("%-30s %.0f m\n", trk.name.c_str(), total);
    }
}
```

### 4. Конвертация в GeoJSON

```cpp
#include "gdbparse.h"
#include <fstream>

int main() {
    auto data = gdbparse::parse_gdb("places.gdb");

    std::ofstream out("places.geojson");
    out << R"({"type":"FeatureCollection","features":[)";

    bool first = true;
    for (const auto& w : data.waypoints) {
        if (!first) out << ",";
        first = false;
        out << R"({"type":"Feature","geometry":{"type":"Point","coordinates":[)"
            << w.longitude << "," << w.latitude
            << R"(]},"properties":{"name":")" << w.shortname << R"("}})";
    }
    out << "]}";
}
```

### 5. Обработка ошибок

Если файл битый, отсутствует или это вообще не gdb — парсер кинет `gdbparse::ParseError` (это наследник `std::runtime_error`):

```cpp
#include "gdbparse.h"
#include <cstdio>

int main(int argc, char** argv) {
    try {
        auto data = gdbparse::parse_gdb(argv[1]);
        std::printf("ok: %zu waypoints\n", data.waypoints.size());
        return 0;
    } catch (const gdbparse::ParseError& e) {
        std::fprintf(stderr, "не смог распарсить: %s\n", e.what());
        return 1;
    }
}
```

---

## Что внутри `Waypoint`

```cpp
struct Waypoint {
    std::string shortname;       // короткое имя (как в навигаторе)
    std::string description;     // описание
    std::string notes;           // длинная заметка
    std::string icon_descr;      // название иконки Garmin

    double latitude;             // широта,  градусы
    double longitude;            // долгота, градусы

    std::optional<double> altitude;     // высота, м (если есть)
    std::optional<double> proximity;    // радиус срабатывания, м
    std::optional<double> depth;        // глубина, м
    std::optional<float>  temperature;  // °C
    std::optional<float>  course;       // курс, градусы
    std::optional<float>  speed;        // м/с
    std::optional<double> geoidheight;  // м

    int64_t creation_time_unix;  // unix-время (UTC). 0 если нет

    std::vector<UrlLink> urls;   // вложенные ссылки
};
```

`Route` и `Track` идентичны по форме:

```cpp
struct Route {
    std::string name;
    std::string description;
    std::vector<UrlLink> urls;

    int32_t line_color_rgb;      // 0x00RRGGBB или -1
    int32_t line_width;          // пикселей, или -1

    std::vector<Waypoint> waypoints;
};

using Track = Route;
```

---

## Важные ограничения

1. **Не потокобезопасно.** Под капотом gpsbabel использует глобальные списки. Если зовёшь `parse_gdb` из разных потоков — оберни мьютексом.

2. **В `waypoints` попадают ВСЕ точки из файла**, включая «скрытые» (которые служат via-точками для маршрутов). Если нужны только пользовательские — фильтруй на своей стороне (например, по тому, ссылается ли на точку какой-то маршрут).

3. **Зависимость от Qt** остаётся на этапе линковки (`Qt5::Core` или `Qt6::Core`) — но в `gdbparse.h` Qt-типов нет, твой код о Qt знать не обязан.

4. **`warning()` из gpsbabel пишет в `stderr`** при странностях в файле. Парс при этом не падает. Если мешает — перенаправь stderr.

---

## Структура файлов

```
lib_gdbparse/
├── gdbparse.h        ← публичный заголовок (включай его)
├── gdbparse.cc       ← обёртка над GdbFormat
├── fatal_lib.cc      ← замена fatal.cc: throw вместо exit
├── gbversion.h       ← заглушка версии для сборки
├── demo.cc           ← пример CLI-утилиты
├── CMakeLists.txt    ← сборка
└── README.md         ← этот файл
```

---

## Лицензия

Код gpsbabel распространяется под GPL v2. Эта обёртка наследует ту же лицензию (см. `COPYING` в корне gpsbabel).
