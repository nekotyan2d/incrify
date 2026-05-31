# Сценарий бэкапа с ротацией:
#   1. Делает инкрементальный бэкап
#   2. Оставляет только последние N полных цепочек, остальные удаляет с -f
#
# Запуск: .\03_backup_with_rotation.ps1 -Source C:\MyDocs -BackupDir D:\Backups -KeepFull 3

param(
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$BackupDir,
    [int]$KeepFull = 3
)

$INCRIFY = "$PSScriptRoot\..\build\incrify.exe"

if (-not (Test-Path $INCRIFY)) {
    Write-Error "incrify.exe не найден: $INCRIFY"
    exit 1
}

Write-Host "=== Инкрементальный бэкап ===" -ForegroundColor Cyan
& $INCRIFY incremental $Source $BackupDir

# Получаем список снапшотов и находим полные (FULL)
$output    = & $INCRIFY list $BackupDir
$fullLines = $output | Select-String -Pattern '(\S+)\s+FULL\s+'
$fullIds   = $fullLines | ForEach-Object { $_.Matches[0].Groups[1].Value } | Sort-Object

$toDelete = $fullIds.Count - $KeepFull
if ($toDelete -gt 0) {
    Write-Host "`n=== Ротация: удаляем $toDelete старых полных цепочек ===" -ForegroundColor Yellow
    $fullIds | Select-Object -First $toDelete | ForEach-Object {
        Write-Host "  Удаляю: $_"
        & $INCRIFY delete $_ $BackupDir -f
    }
} else {
    Write-Host "`nРотация не нужна (полных снапшотов: $($fullIds.Count), лимит: $KeepFull)" -ForegroundColor Green
}

Write-Host "`n=== Актуальные снапшоты ===" -ForegroundColor Cyan
& $INCRIFY list $BackupDir
