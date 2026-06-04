# Incrify

[![CI](https://github.com/nekotyan2d/incrify/actions/workflows/ci.yml/badge.svg)](https://github.com/nekotyan2d/incrify/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/nekotyan2d/incrify)](https://github.com/nekotyan2d/incrify/releases/latest)

Консольная утилита для инкрементального резервного копирования на C++17.

Incrify создаёт полные и инкрементальные снапшоты директорий, хранит историю
копий, позволяет восстанавливать данные на любую сохранённую точку и удалять
устаревшие снапшоты. Работает с локальной файловой системой. Кроссплатформенна —
Linux и Windows.

Готовые бинарники — на странице [последнего релиза](https://github.com/nekotyan2d/incrify/releases/latest).

## Возможности

- **Полный бэкап** — копия всех файлов директории, база для инкрементов.
- **Инкрементальный бэкап** — копируются только добавленные и изменённые
  файлы; удаления фиксируются в метаданных.
- **История снапшотов** — список всех копий с типом и родительским снапшотом.
- **Восстановление** — воссоздание состояния директории по цепочке снапшотов.
- **Удаление** — освобождение места, в том числе каскадное удаление зависимых
  снапшотов.

Изменения файлов определяются по контрольной сумме содержимого (CRC32).
Метаданные каждого снапшота хранятся в человекочитаемом `meta.json`.

## Сборка

Требуется CMake ≥ 3.15 и компилятор с поддержкой C++17 (GCC, Clang или MSVC).
Сторонние зависимости (`nlohmann/json`, `Catch2`) вендоризованы в `third_party/`
и не требуют установки.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Бинарь появится в `build/incrify` (Linux) или `build/Release/incrify.exe`
(Windows).

## Использование

```
incrify full        <src> <dst>            Полный бэкап
incrify incremental <src> <dst>            Инкрементальный бэкап
incrify list        <backup_dir>           Список снапшотов
incrify restore     <snapshot_id> <dir>    Восстановление
incrify delete      <snapshot_id> <dir> [-f]   Удалить снапшот (-f — каскадно)
```

Пример рабочего цикла:

```bash
incrify full ./my_docs ./my_backup           # первый полный бэкап
incrify incremental ./my_docs ./my_backup    # последующие инкременты
incrify list ./my_backup                      # посмотреть историю
incrify restore 20260428_143022 ./my_backup   # откатить к снапшоту
```

Готовые сценарии (ежедневный бэкап, ротация и др.) — в [examples/](examples/).

## Тестирование

Юнит-тесты на [Catch2](https://github.com/catchorg/Catch2), интегрированы с CTest.
Собираются по умолчанию (опция `BUILD_TESTS=ON`).

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Docker

Многостадийный [Dockerfile](Dockerfile) собирает и тестирует проект в чистом
Linux-окружении, а в финальный образ попадает только готовый бинарь.

```bash
docker build -t incrify .

docker run --rm \
    -v /path/to/source:/data/src \
    -v /path/to/backup:/data/backup \
    incrify full /data/src /data/backup
```
