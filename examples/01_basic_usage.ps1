# Базовые команды incrify
# Запуск: .\01_basic_usage.ps1

$INCRIFY = "..\build\incrify.exe"
$SRC     = ".\demo_source"
$BACKUP  = ".\demo_backup"

# Создаём тестовые файлы
New-Item -ItemType Directory -Force -Path $SRC | Out-Null
Set-Content "$SRC\readme.txt"    "Hello, incrify!"
Set-Content "$SRC\config.json"   '{"version": 1}'
Set-Content "$SRC\data.csv"      "id,name`n1,Alice`n2,Bob"

Write-Host "=== Полный бэкап ===" -ForegroundColor Cyan
& $INCRIFY full $SRC $BACKUP

Write-Host "`n=== Изменяем файл и добавляем новый ===" -ForegroundColor Cyan
Add-Content "$SRC\data.csv"   "3,Charlie"
Set-Content "$SRC\new_file.txt" "added after full backup"

Write-Host "`n=== Инкрементальный бэкап ===" -ForegroundColor Cyan
& $INCRIFY incremental $SRC $BACKUP

Write-Host "`n=== Список снапшотов ===" -ForegroundColor Cyan
& $INCRIFY list $BACKUP

Write-Host "`n=== Восстановление последнего снапшота ===" -ForegroundColor Cyan
$snapshots = & $INCRIFY list $BACKUP
$lastId = ($snapshots | Select-String -Pattern '\d{8}_\d{6}_\d{6}' | Select-Object -Last 1).Matches.Value
if ($lastId) {
    & $INCRIFY restore $lastId $BACKUP
} else {
    Write-Host "Снапшоты не найдены, пропускаем восстановление"
}

Write-Host "`n=== Очистка ===" -ForegroundColor DarkGray
Remove-Item -Recurse -Force -Path $SRC, $BACKUP -Confirm:$false
Write-Host "Готово." -ForegroundColor Green
