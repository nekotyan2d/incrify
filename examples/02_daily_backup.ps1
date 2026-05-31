# Сценарий ежедневного бэкапа:
#   - Понедельник: полный бэкап
#   - Остальные дни: инкрементальный
#
# Запуск: .\02_daily_backup.ps1 -Source C:\MyDocs -BackupDir D:\Backups
# Для автоматизации добавьте в Планировщик заданий Windows (один раз в день).

param(
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$BackupDir
)

$INCRIFY = "$PSScriptRoot\..\build\incrify.exe"

if (-not (Test-Path $INCRIFY)) {
    Write-Error "incrify.exe не найден: $INCRIFY"
    exit 1
}

if (-not (Test-Path $Source)) {
    Write-Error "Исходная директория не существует: $Source"
    exit 1
}

$dayOfWeek = (Get-Date).DayOfWeek

if ($dayOfWeek -eq "Monday") {
    Write-Host "[$(Get-Date -Format 'yyyy-MM-dd HH:mm')] Понедельник — запуск полного бэкапа" -ForegroundColor Cyan
    & $INCRIFY full $Source $BackupDir
} else {
    Write-Host "[$(Get-Date -Format 'yyyy-MM-dd HH:mm')] $dayOfWeek — запуск инкрементального бэкапа" -ForegroundColor Cyan
    & $INCRIFY incremental $Source $BackupDir
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "Бэкап завершился с ошибкой (код $LASTEXITCODE)"
    exit $LASTEXITCODE
}

Write-Host "Бэкап завершён успешно." -ForegroundColor Green
