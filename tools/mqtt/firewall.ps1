#Requires -Version 5.1
<#
.SYNOPSIS
    Libera o MQTT na LAN e, opcionalmente, expoe o listener TCP na rede.

.DESCRIPTION
    Requer PowerShell elevado (Administrador). Cria regras de entrada para
    1883 apenas nos perfis Domain/Private. Com -EnableLan, troca o listener
    1883 de 127.0.0.1 para 0.0.0.0 (com backup da config) e reinicia o
    broker.

    O WebSocket (9001) permanece em loopback: o acesso externo e feito pelo
    Cloudflare Quick Tunnel, que nao precisa de porta aberta no firewall.

.PARAMETER EnableLan
    Tambem muda o listener TCP para 0.0.0.0. Sem isso, apenas as regras de
    firewall sao criadas.

.PARAMETER Remove
    Remove as regras de firewall criadas por este script.
#>
[CmdletBinding()]
param(
    [switch]$EnableLan,
    [switch]$Remove
)

$ErrorActionPreference = 'Stop'

$id = [Security.Principal.WindowsIdentity]::GetCurrent()
$isAdmin = ([Security.Principal.WindowsPrincipal]$id).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    throw 'Este script precisa de PowerShell como Administrador.'
}

$MosquittoDir = Join-Path $env:LOCALAPPDATA 'Programs\mosquitto'
$Conf = Join-Path $MosquittoDir 'config\mosquitto.conf'
$RuleName = 'Mosquitto 1883 - LAN'

if ($Remove) {
    $rule = Get-NetFirewallRule -DisplayName $RuleName -ErrorAction SilentlyContinue
    if ($rule) {
        Remove-NetFirewallRule -DisplayName $RuleName
        Write-Host "[mqtt] regra removida: $RuleName"
    } else {
        Write-Host '[mqtt] regra nao existia'
    }
    return
}

if (-not (Get-NetFirewallRule -DisplayName $RuleName -ErrorAction SilentlyContinue)) {
    New-NetFirewallRule -DisplayName $RuleName -Direction Inbound -Action Allow `
        -Protocol TCP -LocalPort 1883 -Profile Domain, Private | Out-Null
    Write-Host "[mqtt] regra criada: $RuleName (Domain/Private)"
} else {
    Write-Host "[mqtt] regra ja existia: $RuleName"
}

if ($EnableLan) {
    if (-not (Test-Path -LiteralPath $Conf)) {
        throw "config nao encontrada: $Conf"
    }
    $backupDir = Join-Path $env:TEMP 'opencode\backups'
    New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
    $backup = Join-Path $backupDir ("mosquitto.conf.{0}.bak" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
    Copy-Item -LiteralPath $Conf -Destination $backup -Force
    Write-Host "[mqtt] backup: $backup"

    $text = Get-Content -LiteralPath $Conf -Raw
    $new = $text -replace '(?m)^listener\s+1883\s+127\.0\.0\.1\s*$', 'listener 1883 0.0.0.0'
    if ($new -eq $text) {
        Write-Host '[mqtt] listener 1883 ja estava em 0.0.0.0 ou em formato inesperado'
    } else {
        [System.IO.File]::WriteAllText($Conf, $new, (New-Object System.Text.UTF8Encoding($false)))
        Write-Host '[mqtt] listener 1883 alterado para 0.0.0.0'
    }

    $mosq = Get-Process mosquitto -ErrorAction SilentlyContinue
    if ($mosq) {
        $mosq | Stop-Process -Force
        Start-Sleep -Seconds 2
    }
    Start-Process -FilePath (Join-Path $MosquittoDir 'mosquitto.exe') `
        -ArgumentList @('-c', $Conf) -WindowStyle Hidden | Out-Null
    Start-Sleep -Seconds 3

    $listen = Get-NetTCPConnection -LocalPort 1883 -State Listen -ErrorAction SilentlyContinue
    if ($listen) {
        $listen | Select-Object LocalAddress, LocalPort, State | Format-Table -AutoSize
    } else {
        throw 'broker nao subiu apos a alteracao; restaure o backup'
    }
}
