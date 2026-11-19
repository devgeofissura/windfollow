#Requires -Version 5.1
<#
.SYNOPSIS
    Inicia o broker MQTT local (Mosquitto) e, opcionalmente, o Quick Tunnel.

.DESCRIPTION
    O Mosquitto e o cloudflared ficam fora do repositorio, em
    %LOCALAPPDATA%\Programs. As credenciais ficam em
    %USERPROFILE%\.config\opencode\secrets\mqtt-local.txt e nunca sao
    gravadas no repositorio.

.PARAMETER NoTunnel
    Sobe somente o broker, sem abrir o tunel publico.

.PARAMETER Force
    Reinicia processos que ja estiverem rodando.
#>
[CmdletBinding()]
param(
    [switch]$NoTunnel,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

$MosquittoDir = Join-Path $env:LOCALAPPDATA 'Programs\mosquitto'
$MosquittoExe = Join-Path $MosquittoDir 'mosquitto.exe'
$MosquittoConf = Join-Path $MosquittoDir 'config\mosquitto.conf'
$CloudflaredExe = Join-Path $env:LOCALAPPDATA 'Programs\cloudflared\cloudflared.exe'
$SecretFile = Join-Path $env:USERPROFILE '.config\opencode\secrets\mqtt-local.txt'
$StateDir = Join-Path $env:TEMP 'opencode\mqtt'
$TunnelLog = Join-Path $StateDir 'cloudflared.log'
$TunnelErr = Join-Path $StateDir 'cloudflared.err'
$UrlFile = Join-Path $StateDir 'tunnel-url.txt'

New-Item -ItemType Directory -Force -Path $StateDir | Out-Null

function Test-Mosquitto {
    param([int]$TimeoutSeconds = 30)
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $conn = Get-NetTCPConnection -LocalPort 1883 -State Listen -ErrorAction SilentlyContinue
        if ($conn) { return $true }
        Start-Sleep -Milliseconds 500
    }
    return $false
}

if (-not (Test-Path -LiteralPath $MosquittoExe)) {
    throw "mosquitto.exe nao encontrado em $MosquittoExe"
}
if (-not (Test-Path -LiteralPath $MosquittoConf)) {
    throw "mosquitto.conf nao encontrado em $MosquittoConf"
}
if (-not (Test-Path -LiteralPath $SecretFile)) {
    throw "credenciais nao encontradas em $SecretFile"
}

$running = Get-Process mosquitto -ErrorAction SilentlyContinue
if ($Force -and $running) {
    $running | Stop-Process -Force
    Start-Sleep -Seconds 2
    $running = $null
}
if (-not $running) {
    Start-Process -FilePath $MosquittoExe -ArgumentList @('-c', $MosquittoConf) `
        -WindowStyle Hidden | Out-Null
    if (-not (Test-Mosquitto)) {
        throw 'mosquitto nao abriu a porta 1883. Veja o log em config\..\log\mosquitto.log'
    }
    Write-Host '[mqtt] broker iniciado em 127.0.0.1:1883 (TCP) e 127.0.0.1:9001 (WebSocket)'
} else {
    Write-Host '[mqtt] broker ja estava rodando'
}

if ($NoTunnel) {
    Write-Host '[mqtt] tunel ignorado (-NoTunnel)'
    return
}

if (-not (Test-Path -LiteralPath $CloudflaredExe)) {
    throw "cloudflared.exe nao encontrado em $CloudflaredExe"
}

$cf = Get-Process cloudflared -ErrorAction SilentlyContinue
if ($Force -and $cf) {
    $cf | Stop-Process -Force
    Start-Sleep -Seconds 1
    $cf = $null
}

if (-not $cf) {
    Remove-Item $TunnelLog, $TunnelErr -ErrorAction SilentlyContinue
    Start-Process -FilePath $CloudflaredExe `
        -ArgumentList @('tunnel', '--no-autoupdate', '--url', 'http://127.0.0.1:9001') `
        -RedirectStandardOutput $TunnelLog -RedirectStandardError $TunnelErr `
        -WindowStyle Hidden | Out-Null

    $url = $null
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Seconds 2
        $text = ''
        if (Test-Path $TunnelLog) { $text += Get-Content $TunnelLog -Raw }
        if (Test-Path $TunnelErr) { $text += Get-Content $TunnelErr -Raw }
        $m = [regex]::Match($text, 'https://[a-z0-9-]+\.trycloudflare\.com')
        if ($m.Success) { $url = $m.Value; break }
    }
    if (-not $url) {
        throw "nao foi possivel obter a URL do tunel. Veja $TunnelErr"
    }
} else {
    $url = if (Test-Path $UrlFile) { (Get-Content $UrlFile -Raw).Trim() } else { $null }
    if (-not $url) {
        throw 'cloudflared ja estava rodando, mas a URL nao esta registrada. Use -Force para reiniciar.'
    }
}

Set-Content -LiteralPath $UrlFile -Value $url -Encoding ASCII

Write-Host ''
Write-Host '[mqtt] tunel publico (WebSocket):' -ForegroundColor Cyan
Write-Host "       wss://$($url -replace '^https://', '')/mqtt"
Write-Host ''
Write-Host '[mqtt] credenciais:'
Write-Host "       $SecretFile"
Write-Host ''
Write-Host '[mqtt] O Quick Tunnel nao tem garantia de uptime e a URL muda a cada reinicio.'
Write-Host '[mqtt] Para producao, use um Named Tunnel com dominio proprio.'
