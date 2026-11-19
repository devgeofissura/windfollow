#Requires -Version 5.1
<#
.SYNOPSIS
    Para o broker MQTT local e o Quick Tunnel.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Continue'

$StateDir = Join-Path $env:TEMP 'opencode\mqtt'
$UrlFile = Join-Path $StateDir 'tunnel-url.txt'

$stopped = $false
foreach ($name in @('cloudflared', 'mosquitto')) {
    $procs = Get-Process $name -ErrorAction SilentlyContinue
    if ($procs) {
        $procs | Stop-Process -Force
        Write-Host "[mqtt] $name parado"
        $stopped = $true
    }
}

if (Test-Path $UrlFile) {
    Remove-Item $UrlFile -Force -ErrorAction SilentlyContinue
}

if (-not $stopped) {
    Write-Host '[mqtt] nada estava rodando'
}
