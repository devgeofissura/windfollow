#Requires -Version 5.1
<#
.SYNOPSIS
    Instala a inicializacao automatica do broker MQTT no logon do usuario.

.DESCRIPTION
    Cria um atalho .cmd na pasta Startup do usuario atual. Nao exige
    administrador e nao abre o tunel automaticamente: o tunel publico
    deve ser iniciado sob demanda com tools\mqtt\start.ps1.

.PARAMETER Remove
    Remove a inicializacao automatica.
#>
[CmdletBinding()]
param([switch]$Remove)

$ErrorActionPreference = 'Stop'

$startup = [Environment]::GetFolderPath('Startup')
$entry = Join-Path $startup 'opencode-mqtt.cmd'
$startScript = Join-Path $PSScriptRoot 'start.ps1'

if ($Remove) {
    if (Test-Path -LiteralPath $entry) {
        Remove-Item -LiteralPath $entry -Force
        Write-Host "[mqtt] autostart removido: $entry"
    } else {
        Write-Host '[mqtt] autostart nao estava instalado'
    }
    return
}

if (-not (Test-Path -LiteralPath $startScript)) {
    throw "start.ps1 nao encontrado em $startScript"
}

$content = @"
@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$startScript" -NoTunnel > "%TEMP%\opencode\mqtt-autostart.log" 2>&1
"@
Set-Content -LiteralPath $entry -Value $content -Encoding ASCII
Write-Host "[mqtt] autostart instalado: $entry"
Write-Host '[mqtt] O broker sobe no logon; o tunel continua manual (start.ps1).'
