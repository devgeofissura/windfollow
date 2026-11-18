param(
    [string]$WebDir = (Join-Path $PSScriptRoot "..\web")
)

Add-Type -AssemblyName System.Drawing

function New-RobotIcon {
    param(
        [int]$Size,
        [string]$Path
    )

    $bmp = New-Object System.Drawing.Bitmap($Size, $Size)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.Clear([System.Drawing.Color]::FromArgb(11, 17, 32))

    $scale = $Size / 100.0
    function S([double]$v) { return [int]($v * $scale) }

    $blue = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(143, 215, 255))
    $panel = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(29, 39, 51))
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(143, 215, 255), [Math]::Max(1, [int](3 * $scale)))

    # Antena
    $g.DrawLine($pen, (S 50), (S 24), (S 50), (S 9))
    $g.FillEllipse($blue, (S 45), (S 4), (S 10), (S 10))

    # Cabeca (retangulo + cantos arredondados)
    $g.FillRectangle($panel, (S 18), (S 24), (S 64), (S 54))
    $g.FillEllipse($panel, (S 18), (S 18), (S 16), (S 16))
    $g.FillEllipse($panel, (S 66), (S 18), (S 16), (S 16))
    $g.FillEllipse($panel, (S 18), (S 68), (S 16), (S 16))
    $g.FillEllipse($panel, (S 66), (S 68), (S 16), (S 16))
    $g.DrawRectangle($pen, (S 18), (S 24), (S 64), (S 54))

    # Olhos
    $g.FillEllipse($blue, (S 32), (S 40), (S 14), (S 14))
    $g.FillEllipse($blue, (S 54), (S 40), (S 14), (S 14))

    # Sorriso
    $g.DrawArc($pen, (S 32), (S 52), (S 36), (S 18), 0, 180)

    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $pen.Dispose()
    $panel.Dispose()
    $blue.Dispose()
    $bmp.Dispose()
    Write-Host ("Gerado: {0} ({1} bytes)" -f $Path, (Get-Item -LiteralPath $Path).Length)
}

if (!(Test-Path -LiteralPath $WebDir)) {
    Write-Error "Diretorio web/ nao encontrado: $WebDir"
    exit 1
}

New-RobotIcon -Size 32  -Path (Join-Path $WebDir "favicon.png")
New-RobotIcon -Size 192 -Path (Join-Path $WebDir "icon-192.png")
New-RobotIcon -Size 512 -Path (Join-Path $WebDir "icon-512.png")
