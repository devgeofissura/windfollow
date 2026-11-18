<#
.SYNOPSIS
    Compila e grava um firmware do projeto, sempre detectando a porta COM na hora.

.DESCRIPTION
    A porta COM de um ESP32 muda a cada vez que o cabo e replugado (as vezes muda
    ate entre duas gravacoes seguidas). Por isso este script NUNCA assume uma porta
    fixa: ele consulta o arduino-cli, confirma qual placa esta respondendo e compara
    o chip real com o chip que o firmware espera.

    Se houver mais de um dispositivo serial conectado, o script para e lista as
    portas; use -Port para escolher explicitamente.

.PARAMETER Sketch
    Qual firmware compilar/gravar: irobot, msa, windfollow ou windfollowblind.
    Opcional apenas com -DetectOnly (ai lista todas as portas sem checar perfil).

.PARAMETER Port
    Porta COM explicita (ex.: COM5). Normalmente nao e necessario.

.PARAMETER DetectOnly
    Apenas mostra as portas encontradas e o chip de cada uma. Nao compila nem grava.

.PARAMETER SkipVerify
    Nao executa o esptool para conferir o chip antes de compilar.

.PARAMETER Force
    Grava mesmo se o chip detectado nao corresponder ao esperado.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -DetectOnly

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollow -DetectOnly
#>

param(
    [Parameter(Position = 0)]
    [ValidateSet('irobot', 'msa', 'windfollow', 'windfollowblind')]
    [string]$Sketch = '',

    [string]$Port,

    [switch]$DetectOnly,

    [switch]$SkipVerify,

    [switch]$Force
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot 'firmware'))) {
    $RepoRoot = (Get-Location).Path
}

# --- Perfis de placa por firmware -------------------------------------------
# expectedChip: ESP32 (DevKit classico) ou ESP32-S3 (LILYGO T-SIMCAM).
$Profiles = @{
    'irobot' = @{
        Dir          = 'firmware\irobot'
        BuildPath    = Join-Path $env:TEMP 'irobot-build'
        Fqbn         = 'esp32:esp32:esp32'
        ExpectedChip = 'ESP32'
        BoardName    = 'ESP32 DevKit (classico, 4 MB)'
    }
    'msa' = @{
        Dir          = 'firmware\msa'
        BuildPath    = Join-Path $env:TEMP 'msa-build'
        Fqbn         = 'esp32:esp32:esp32'
        ExpectedChip = 'ESP32'
        BoardName    = 'ESP32 DevKit (classico, 4 MB)'
    }
    'windfollowblind' = @{
        Dir          = 'firmware\windfollowblind'
        BuildPath    = Join-Path $env:TEMP 'wf-blind-build'
        Fqbn         = 'esp32:esp32:esp32'
        ExpectedChip = 'ESP32'
        BoardName    = 'ESP32 DevKit (classico, 4 MB)'
    }
    'windfollow' = @{
        Dir          = 'firmware\windfollow'
        BuildPath    = Join-Path $env:TEMP 'windfollow-build'
        Fqbn         = 'esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadSpeed=921600,FlashSize=16M,PartitionScheme=huge_app,PSRAM=opi'
        ExpectedChip = 'ESP32-S3'
        BoardName    = 'LILYGO T-SIMCAM ESP32-S3 (16 MB flash, OPI PSRAM)'
    }
}

function Get-ArduinoCli {
    $cmd = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if (-not $cmd) {
        throw 'arduino-cli nao encontrado no PATH. Instale/abra um novo terminal e tente de novo.'
    }
    return $cmd.Source
}

function Get-Esptool {
    $base = Join-Path $env:LOCALAPPDATA 'Arduino15\packages\esp32\tools\esptool_py'
    if (-not (Test-Path -LiteralPath $base)) { return $null }
    $candidates = @(Get-ChildItem -LiteralPath $base -Recurse -Filter 'esptool.exe' -ErrorAction SilentlyContinue)
    if ($candidates.Count -eq 0) { return $null }

    $sorted = $candidates | Sort-Object { try { [version]$_.Directory.Name } catch { [version]'0.0.0' } } -Descending
    $exe = @($sorted)[0]
    return $exe.FullName
}

function Get-SerialPorts {
    param([string]$ArduinoCli)

    $json = & $ArduinoCli board list --format json 2>$null | Out-String
    $ports = @()

    try {
        $data = $json | ConvertFrom-Json
    } catch {
        return $ports
    }

    foreach ($d in @($data.detected_ports)) {
        if (-not $d -or -not $d.port) { continue }
        if ($d.port.protocol -ne 'serial') { continue }
        if ($d.port.address -notmatch '^COM\d+$') { continue }
        $ports += [pscustomobject]@{
            Address = $d.port.address
            Label   = $d.port.label
            Vid     = $d.port.properties.vid
            Pid     = $d.port.properties.pid
        }
    }

    return @($ports | Sort-Object Address)
}

function Get-ChipInfo {
    param([string]$Esptool, [string]$ComPort)

    if (-not $Esptool) { return $null }

    # O esptool escreve partes do diagnostic no stderr; com ErrorActionPreference
    # = Stop o PowerShell 5.1 transforma isso em erro fatal e aborta a deteccao.
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $out = & $Esptool --port $ComPort --before default-reset --after hard-reset flash-id 2>&1 | Out-String
    } finally {
        $ErrorActionPreference = $previousEap
    }
    $chip = $null
    $flash = $null
    $mac = $null

    if ($out -match '(?m)^Chip type:\s+(\S+)') { $chip = $Matches[1].Trim() }
    if ($out -match '(?m)Detected flash size:\s+(\S+)') { $flash = $Matches[1] }
    if ($out -match '(?m)^MAC:\s+([0-9a-fA-F:]{17})') { $mac = $Matches[1] }

    if (-not $chip) { return $null }

    return [pscustomobject]@{
        Chip  = $chip
        Flash = $flash
        Mac   = $mac
        Raw   = $out
    }
}

function Test-ChipMatch {
    param([string]$DetectedChip, [string]$ExpectedChip)

    if (-not $DetectedChip) { return $true }
    if ($ExpectedChip -eq 'ESP32-S3') { return ($DetectedChip -match 'ESP32-S3') }
    return ($DetectedChip -notmatch 'ESP32-S\d|ESP32-C\d|ESP32-H\d|ESP32-P\d')
}

# --- Execucao ---------------------------------------------------------------
$arduinoCli = Get-ArduinoCli
$esptool = Get-Esptool
$profile = if ($Sketch) { $Profiles[$Sketch] } else { $null }

if (-not $Sketch -and -not $DetectOnly) {
    throw "Informe -Sketch (irobot, msa, windfollow ou windfollowblind). Para apenas detectar portas, use -DetectOnly."
}

Write-Host ''
if ($profile) {
    Write-Host "Firmware : $Sketch"
    Write-Host "Placa    : $($profile.BoardName)"
    Write-Host "FQBN     : $($profile.Fqbn)"
} else {
    Write-Host 'Firmware : (nao informado - apenas deteccao de portas)'
}
Write-Host ''

# @() obrigatorio: PowerShell desembrulha array de 1 elemento e .Count fica vazio
$ports = @(Get-SerialPorts -ArduinoCli $arduinoCli)

if ($ports.Count -eq 0) {
    throw 'Nenhuma porta serial (COMx) encontrada. Conecte o ESP32 e confira o cabo/driver CH340.'
}

Write-Host 'Portas seriais detectadas:'
foreach ($p in $ports) {
    $chipInfo = Get-ChipInfo -Esptool $esptool -ComPort $p.Address
    $chipTxt = if ($chipInfo) { "$($chipInfo.Chip) / flash $($chipInfo.Flash) / MAC $($chipInfo.Mac)" } else { '(chip nao verificado)' }
    Write-Host ("  {0}  {1}  VID:PID {2}:{3}  ->  {4}" -f $p.Address, $p.Label, $p.Vid, $p.Pid, $chipTxt)
    if ($chipInfo) {
        if (-not $profile) {
            Write-Host '       (informe -Sketch para checar compatibilidade)'
        } elseif (Test-ChipMatch -DetectedChip $chipInfo.Chip -ExpectedChip $profile.ExpectedChip) {
            Write-Host ("       compativel com '{0}' ({1})" -f $Sketch, $profile.ExpectedChip)
        } else {
            Write-Host ("       INCOMPATIVEL com '{0}' (espera {1})" -f $Sketch, $profile.ExpectedChip) -ForegroundColor Yellow
        }
    }
}
Write-Host ''

if ($DetectOnly) {
    return
}

if ($Port) {
    $chosen = $ports | Where-Object { $_.Address -eq $Port }
    if (-not $chosen) {
        throw "A porta $Port nao esta na lista de portas seriais detectadas agora. Replugue o dispositivo ou rode com -DetectOnly para ver as portas."
    }
} elseif ($ports.Count -eq 1) {
    $chosen = $ports[0]
} else {
    throw "Ha $($ports.Count) portas seriais conectadas. Escolha uma com -Port (ex.: -Port $($ports[0].Address)) apos confirmar qual e o ESP32."
}

Write-Host "Usando porta $($chosen.Address) (detectada agora, nao fixa no codigo)."

if (-not $SkipVerify) {
    $chipInfo = Get-ChipInfo -Esptool $esptool -ComPort $chosen.Address
    if (-not $chipInfo) {
        Write-Warning "Nao foi possivel identificar o chip em $($chosen.Address). Seguindo apenas com o FQBN do perfil."
    } elseif (-not (Test-ChipMatch -DetectedChip $chipInfo.Chip -ExpectedChip $profile.ExpectedChip)) {
        $msg = "Chip em $($chosen.Address) e '$($chipInfo.Chip)', mas o firmware '$Sketch' espera '$($profile.ExpectedChip)'."
        if (-not $Force) { throw "$msg Use -Force se tiver certeza, ou escolha outro firmware/perfil." }
        Write-Warning $msg
    } else {
        Write-Host "Chip confirmado: $($chipInfo.Chip) (flash $($chipInfo.Flash)) - compativel com $($profile.ExpectedChip)."
    }
}

$sketchDir = Join-Path $RepoRoot $profile.Dir
if (-not (Test-Path -LiteralPath (Join-Path $sketchDir ($Sketch + '.ino')))) {
    throw "Sketch nao encontrado em $sketchDir"
}

Write-Host ''
Write-Host "Compilando $Sketch ..."
& $arduinoCli compile --fqbn $profile.Fqbn --build-path $profile.BuildPath $sketchDir
if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao de $Sketch." }

Write-Host ''
Write-Host "Gravando em $($chosen.Address) ..."
& $arduinoCli upload -p $chosen.Address --fqbn $profile.Fqbn --input-dir $profile.BuildPath
if ($LASTEXITCODE -ne 0) { throw "Falha no upload para $($chosen.Address)." }

Write-Host ''
Write-Host "OK: $Sketch gravado em $($chosen.Address)."
Write-Host "Se a porta mudar na proxima vez, rode o script de novo: ele detecta sozinho."
