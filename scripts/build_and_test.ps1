param(
    [string]$ProjectDir  = "C:\Users\Haoyi\chip8-emu",
    [string]$WebhookUrl  = "http://192.168.0.180:5678/webhook/chip8-result"
)

$env:Path += ";C:\msys64\mingw64\bin"

$build = cmake --build "$ProjectDir\build" 2>&1
$buildOk = $LASTEXITCODE -eq 0

if (-not $buildOk) {
    $errors = $build | Select-String "error:" | ForEach-Object { $_.Line.Trim() }
    Write-Output (ConvertTo-Json @{
        status  = "build_failed"
        errors  = $errors
        output  = "$build"
    })
    exit 1
}

Push-Location "$ProjectDir\build"
$tests = ctest --output-on-failure 2>&1
$testOk = $LASTEXITCODE -eq 0
Pop-Location

$summary = ($tests | Select-String "% tests passed") -replace '\x1B\[[0-9;]*m',''

$payload = ConvertTo-Json @{
    status  = if ($testOk) { "pass" } else { "fail" }
    summary = "$summary"
    project = "chip8-emu"
    time    = (Get-Date -Format o)
}

Write-Output $payload

# Post result to n8n
try {
    Invoke-RestMethod -Uri $WebhookUrl -Method POST -Body $payload -ContentType "application/json" | Out-Null
    Write-Host "Result posted to n8n"
} catch {
    Write-Host "n8n unreachable — result logged locally only"
}

exit $(if ($testOk) { 0 } else { 1 })
