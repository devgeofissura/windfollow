#Requires -Version 5.1
<#
.SYNOPSIS
    Mostra o estado do broker MQTT e do tunel.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'SilentlyContinue'

$StateDir = Join-Path $env:TEMP 'opencode\mqtt'
$UrlFile = Join-Path $StateDir 'tunnel-url.txt'
$MosquittoDir = Join-Path $env:LOCALAPPDATA 'Programs\mosquitto'

Write-Host '=== processos ==='
foreach ($name in @('mosquitto', 'cloudflared')) {
    $p = Get-Process $name -ErrorAction SilentlyContinue
    if ($p) {
        Write-Host ("{0,-12} PID {1}" -f $name, ($p.Id -join ','))
    } else {
        Write-Host ("{0,-12} parado" -f $name)
    }
}

Write-Host ''
Write-Host '=== portas ==='
$ports = Get-NetTCPConnection -LocalPort 1883, 9001 -State Listen -ErrorAction SilentlyContinue |
    Select-Object LocalAddress, LocalPort, State
if ($ports) {
    $ports | Format-Table -AutoSize | Out-String | Write-Host
} else {
    Write-Host 'nenhuma porta 1883/9001 escutando'
}

Write-Host '=== tunel ==='
if (Test-Path $UrlFile) {
    $url = (Get-Content $UrlFile -Raw).Trim()
    Write-Host "wss://$($url -replace '^https://', '')/mqtt"
} else {
    Write-Host 'sem URL registrada'
}

Write-Host ''
Write-Host '=== log recente ==='
$log = Join-Path $MosquittoDir 'log\mosquitto.log'
if (Test-Path $log) {
    Get-Content $log -Tail 8
}
