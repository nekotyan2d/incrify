# Демонстрация incrify в Docker-контейнере (Windows / PowerShell)
# Запуск: .\demo_docker.ps1
# Требования: Docker Desktop запущен

$IMAGE  = "incrify:demo"
$SRC    = "$env:TEMP\incrify_demo_src"
$BACKUP = "$env:TEMP\incrify_demo_backup"

function Run-Incrify {
    docker run --rm `
        -v "${SRC}:/data/src" `
        -v "${BACKUP}:/data/backup" `
        $IMAGE @args
}

Write-Host "=== 1. Сборка образа (включает запуск тестов Catch2) ===" -ForegroundColor Cyan
docker build -t $IMAGE (Join-Path $PSScriptRoot "..")
if ($LASTEXITCODE -ne 0) { Write-Host "Сборка провалена." -ForegroundColor Red; exit 1 }

Write-Host "`n=== 2. Подготовка тестовых файлов ===" -ForegroundColor Cyan
Remove-Item -Recurse -Force -Path $SRC, $BACKUP -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $SRC, $BACKUP | Out-Null
Set-Content "$SRC\hello.txt"          "Hello, World!"
Set-Content "$SRC\data.txt"           "Version 1"
New-Item -ItemType Directory -Force -Path "$SRC\subdir" | Out-Null
Set-Content "$SRC\subdir\nested.txt"  "nested file"
Write-Host "Файлы созданы в $SRC"

Write-Host "`n=== 3. Полный бэкап ===" -ForegroundColor Cyan
Run-Incrify full /data/src /data/backup

Write-Host "`n=== 4. Список снапшотов ===" -ForegroundColor Cyan
Run-Incrify list /data/backup

Write-Host "`n=== 5. Инкрементальный бэкап без изменений ===" -ForegroundColor Cyan
Run-Incrify incremental /data/src /data/backup

Write-Host "`n=== 6. Изменяем файлы - инкрементальный бэкап ===" -ForegroundColor Cyan
Set-Content "$SRC\data.txt"    "Version 2 - modified"
Set-Content "$SRC\newfile.txt" "brand new"
Remove-Item "$SRC\hello.txt"
Run-Incrify incremental /data/src /data/backup

Write-Host "`n=== 7. Список снапшотов после инкремента ===" -ForegroundColor Cyan
Run-Incrify list /data/backup

Write-Host "`n=== 8. Восстановление последнего снапшота ===" -ForegroundColor Cyan
$listOutput = docker run --rm -v "${BACKUP}:/data/backup" $IMAGE list /data/backup
$snapshotId = ($listOutput | Select-String '\d{8}_\d{6}_\d{6}' | Select-Object -Last 1).Matches.Value
if ($snapshotId) {
    Write-Host "Восстанавливаем снапшот: $snapshotId"
    Run-Incrify restore $snapshotId /data/backup
} else {
    Write-Host "Снапшоты не найдены." -ForegroundColor Red
}

Write-Host "`n=== 9. Неизвестная команда - вывод справки ===" -ForegroundColor Cyan
docker run --rm $IMAGE unknown

Write-Host "`n=== Готово. Очистка ===" -ForegroundColor DarkGray
Remove-Item -Recurse -Force -Path $SRC, $BACKUP -Confirm:$false
Write-Host "Выполнено." -ForegroundColor Green
