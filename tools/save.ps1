<#
.SYNOPSIS
    Commita tudo, registra o trabalho em docs/DEVLOG.md e faz push.

.DESCRIPTION
    Fecha o ciclo de uma tarefa: monta uma entrada no diario de desenvolvimento
    (o que foi feito, dificuldades e arquivos tocados), insere no topo do
    historico, adiciona todos os arquivos, commita e empurra para o remoto.

    A entrada do DEVLOG faz parte do proprio commit, para que o historico de
    desenvolvimento nunca dependa da mensagem de commit.

    Este script e mantido em ASCII puro de proposito: o PowerShell 5.1 le .ps1
    sem BOM como ANSI, e acentos quebram o parser.

.PARAMETER Message
    Mensagem do commit (mesmo estilo do repositorio: "tipo(escopo): resumo").

.PARAMETER Summary
    O que foi feito. Use ';' para separar itens: -Summary "item 1; item 2".

.PARAMETER Difficulty
    Dificuldades encontradas e como foram resolvidas. Use ';' para separar itens.

.PARAMETER Area
    Area/firmware afetado (ex.: "irobot", "windfollowblind", "tooling").
    Padrao: inferido dos arquivos alterados.

.PARAMETER NoPush
    Commit local sem push (use so se o remoto estiver indisponivel).

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/save.ps1 -Message "feat(irobot): ajusta PWM" -Summary "PWM minimo 60; zona morta 30" -Difficulty "motor nao partia abaixo de 40 -> min 60"

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/save.ps1 -Message "docs: atualiza README" -Summary "secao nova" -NoPush
#>

param(
    [Parameter(Mandatory = $true)]
    [string]$Message,

    [string[]]$Summary = @(),

    [string[]]$Difficulty = @(),
    [string]$Area = '',

    [switch]$NoPush
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot 'firmware'))) {
    $RepoRoot = (Get-Location).Path
}

$utf8NoBom = New-Object System.Text.UTF8Encoding $false

# powershell.exe -File entrega "a","b" como uma unica string "a,b": aceitamos
# tanto vetor (chamada direta no PS) quanto itens separados por ';'.
function Expand-List {
    param([string[]]$Items)
    $out = @()
    foreach ($item in $Items) {
        if ($null -eq $item) { continue }
        foreach ($piece in ($item -split ';')) {
            if ($piece.Trim().Length -gt 0) { $out += $piece.Trim() }
        }
    }
    return $out
}

$Summary = Expand-List -Items $Summary
$Difficulty = Expand-List -Items $Difficulty

Push-Location $RepoRoot
try {
    $git = (Get-Command git -ErrorAction Stop).Source
    $devlog = Join-Path $RepoRoot 'docs\DEVLOG.md'

    # --- Estado atual -------------------------------------------------------
    $status = & $git status --porcelain
    if (-not $status) {
        Write-Host 'Nada para commitar: a arvore de trabalho esta limpa.'
        return
    }

    $changed = @($status | ForEach-Object { $_.Substring(3).Trim() })
    $branch = (& $git branch --show-current).Trim()

    if (-not (Test-Path -LiteralPath $devlog)) {
        $header = "# Diario de desenvolvimento (DEVLOG)`r`n`r`n" +
            "Registro do que foi feito em cada commit, com dificuldades e solucoes.`r`n" +
            "Entradas novas sao inseridas no topo da secao Historico por tools/save.ps1.`r`n`r`n" +
            "---`r`n`r`n## Historico`r`n"
        [System.IO.File]::WriteAllText($devlog, $header, $utf8NoBom)
    }

    if (-not $Area) {
        $parts = $changed | ForEach-Object {
            if ($_ -match '^firmware/([^/]+)/') { $Matches[1] }
            elseif ($_ -match '^tools/') { 'tooling' }
            elseif ($_ -match '^docs/') { 'docs' }
            elseif ($_ -match '^simulator/') { 'simulator' }
            else { 'raiz' }
        } | Sort-Object -Unique
        $Area = ($parts -join ', ')
    }

    # --- Guarda contra segredos --------------------------------------------
    $secretPatterns = @(
        'BEGIN (RSA|OPENSSH|EC|PGP) PRIVATE KEY',
        'AKIA[0-9A-Z]{16}',
        'ghp_[A-Za-z0-9]{30,}',
        'xox[baprs]-[A-Za-z0-9-]{10,}',
        'sk-[A-Za-z0-9]{20,}'
    )
    $diff = [string]((& $git diff) -join "`n") + "`n" + [string]((& $git diff --cached) -join "`n")
    $untracked = @(& $git ls-files --others --exclude-standard)
    foreach ($file in $untracked) {
        $full = Join-Path $RepoRoot $file
        if (Test-Path -LiteralPath $full -PathType Leaf) {
            $info = Get-Item -LiteralPath $full
            if ($info.Length -le 2MB) {
                $diff += "`n" + [System.IO.File]::ReadAllText($full, [System.Text.Encoding]::UTF8)
            }
        }
    }
    foreach ($pattern in $secretPatterns) {
        if ($diff -match $pattern) {
            throw "Possivel segredo detectado no diff (padrao: $pattern). Commit abortado; revise os arquivos."
        }
    }

    # --- Entrada do DEVLOG -------------------------------------------------
    $stamp = Get-Date -Format 'yyyy-MM-dd HH:mm'
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine("### $stamp - $Message")
    [void]$sb.AppendLine('')
    [void]$sb.AppendLine("**Area:** $Area")
    [void]$sb.AppendLine('')
    [void]$sb.AppendLine('**Feito:**')
    if ($Summary.Count -gt 0) {
        foreach ($line in $Summary) { [void]$sb.AppendLine("- $line") }
    } else {
        [void]$sb.AppendLine('- (sem detalhes informados)')
    }
    [void]$sb.AppendLine('')
    [void]$sb.AppendLine('**Dificuldades e solucoes:**')
    if ($Difficulty.Count -gt 0) {
        foreach ($line in $Difficulty) { [void]$sb.AppendLine("- $line") }
    } else {
        [void]$sb.AppendLine('- nenhuma')
    }
    [void]$sb.AppendLine('')
    [void]$sb.AppendLine('**Arquivos:**')
    foreach ($file in $changed) { [void]$sb.AppendLine("- ``$file``") }
    [void]$sb.AppendLine('')
    $entry = $sb.ToString()

    $log = [System.IO.File]::ReadAllText($devlog, $utf8NoBom)
    $entryMd = $entry.TrimEnd().Replace("`r`n", "`n").Replace("`n", "`r`n")
    $heading = [regex]::Match($log, '(?m)^## Hist[^\r\n]*\r?\n')
    if ($heading.Success) {
        $pos = $heading.Index + $heading.Length
        $log = $log.Substring(0, $pos) + "`r`n" + $entryMd + "`r`n" + $log.Substring($pos)
    } else {
        $log = $log.TrimEnd() + "`r`n`r`n## Hist" + [char]0x00F3 + "rico`r`n`r`n" + $entryMd + "`r`n"
    }
    [System.IO.File]::WriteAllText($devlog, $log, $utf8NoBom)

    # --- Commit + push -----------------------------------------------------
    & $git add -A
    if ($LASTEXITCODE -ne 0) { throw 'git add falhou.' }

    & $git commit -m $Message
    if ($LASTEXITCODE -ne 0) { throw 'git commit falhou (hook ou conflito?).' }

    $hash = (& $git rev-parse --short HEAD).Trim()
    Write-Host "Commit $hash criado em '$branch'."

    if ($NoPush) {
        Write-Host 'Push pulado (-NoPush). Rode git push quando o remoto estiver acessivel.'
        return
    }

    & $git push origin $branch
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Push falhou. O commit $hash esta salvo localmente; resolva a credencial/rede e rode: git push origin $branch"
        return
    }

    Write-Host "Push concluido: origin/$branch ($hash)."
}
finally {
    Pop-Location
}
