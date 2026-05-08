param(
    [string]$WebhookUrl = "http://192.168.0.180:5678/webhook/chip8-result",
    [string]$WatchDir   = "C:\Users\Haoyi\chip8-emu"
)

$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path   = $WatchDir
$watcher.Filter = "*.cpp"
$watcher.IncludeSubdirectories = $true
$watcher.NotifyFilter = [System.IO.NotifyFilters]::LastWrite

Write-Host "Watching $WatchDir for .cpp changes -> $WebhookUrl"

while ($true) {
    $change = $watcher.WaitForChanged([System.IO.WatcherChangeTypes]::Changed, 2000)
    if (-not $change.TimedOut) {
        $body = '{"file":"' + $change.Name + '","time":"' + (Get-Date -Format o) + '"}'
        try {
            Invoke-RestMethod -Uri $WebhookUrl -Method POST -Body $body -ContentType "application/json" | Out-Null
            Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] Triggered: $($change.Name)"
        } catch {
            Write-Host "Webhook failed"
        }
    }
}
