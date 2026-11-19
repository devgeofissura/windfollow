#Requires -Version 5.1
<#
.SYNOPSIS
    Roda a suite de testes do contrato do parser UDP do laserseg.

.DESCRIPTION
    Usa sempre o Python LTS instalado pelo usuario, nunca o stub da
    Microsoft Store nem a versao mais recente.

.PARAMETER Pattern
    Filtro opcional de teste, repassado para unittest -k.
#>
[CmdletBinding()]
param([string]$Pattern)

$ErrorActionPreference = 'Stop'

# Nao gerar __pycache__ dentro do repositorio.
$env:PYTHONDONTWRITEBYTECODE = '1'

$python = Join-Path $env:LOCALAPPDATA 'Programs\Python\Python314\python.exe'
if (-not (Test-Path -LiteralPath $python)) {
    throw "Python LTS nao encontrado em $python"
}

$tests = Join-Path $PSScriptRoot 'test_protocol.py'
if (-not (Test-Path -LiteralPath $tests)) {
    throw "suite nao encontrada em $tests"
}

Write-Host "[tests] $python $tests" -ForegroundColor Cyan

$args = @('-m', 'unittest', 'discover', '-s', $PSScriptRoot, '-p', 'test_*.py', '-v')
if ($Pattern) {
    $args += @('-k', $Pattern)
}

& $python @args
exit $LASTEXITCODE
