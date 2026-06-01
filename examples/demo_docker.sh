#!/usr/bin/env bash
# Демонстрация работы incrify в Docker-контейнере.
# Запускать на Linux/macOS хосте с установленным Docker.
set -euo pipefail

IMAGE="incrify:demo"
SRC="/tmp/incrify_demo_src"
BACKUP="/tmp/incrify_demo_backup"

run() {
    # Прокидывает оба тома и передаёт произвольные аргументы контейнеру.
    docker run --rm \
        -v "$SRC":/data/src \
        -v "$BACKUP":/data/backup \
        "$IMAGE" "$@"
}

echo "=== 1. Сборка образа (включает запуск тестов Catch2) ==="
docker build -t "$IMAGE" .

echo ""
echo "=== 2. Подготовка тестовых файлов ==="
rm -rf "$SRC" "$BACKUP"
mkdir -p "$SRC" "$BACKUP"
echo "Hello, World!"  > "$SRC/hello.txt"
echo "Version 1"      > "$SRC/data.txt"
mkdir -p "$SRC/subdir"
echo "nested file"    > "$SRC/subdir/nested.txt"
echo "Файлы в $SRC:"
ls -lR "$SRC"

echo ""
echo "=== 3. Полный бэкап ==="
run full /data/src /data/backup

echo ""
echo "=== 4. Список снапшотов ==="
run list /data/backup

echo ""
echo "=== 5. Инкрементальный бэкап без изменений (снапшот создаваться не должен) ==="
run incremental /data/src /data/backup

echo ""
echo "=== 6. Изменяем файлы и создаём инкрементальный бэкап ==="
echo "Version 2 — modified"  > "$SRC/data.txt"
echo "brand new"              > "$SRC/newfile.txt"
rm "$SRC/hello.txt"
run incremental /data/src /data/backup

echo ""
echo "=== 7. Список снапшотов после инкремента ==="
run list /data/backup

echo ""
echo "=== 8. Восстановление последнего снапшота ==="
SNAPSHOT_ID=$(docker run --rm \
    -v "$BACKUP":/data/backup \
    "$IMAGE" list /data/backup \
    | awk 'NR>3 && /INCREMENTAL/ {print $1; exit}')

echo "Восстанавливаем снапшот: $SNAPSHOT_ID"
run restore "$SNAPSHOT_ID" /data/backup
echo "Файлы после восстановления:"
ls -lR "$SRC"

echo ""
echo "=== 9. Передача неизвестной команды — вывод справки ==="
docker run --rm "$IMAGE" unknown || true

echo ""
echo "=== Готово. Очистка ==="
rm -rf "$SRC" "$BACKUP"
