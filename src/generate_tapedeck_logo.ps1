# Convert the public-domain Sinclair text logo PNG to a small 1-bit mask.
Add-Type -AssemblyName System.Drawing
$source = [Drawing.Bitmap]::FromFile((Join-Path $PSScriptRoot 'assets/sinclair-logo.png'))
$small = [Drawing.Bitmap]::new(88, 13)
$graphics = [Drawing.Graphics]::FromImage($small)
$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$graphics.DrawImage($source, 0, 0, 88, 13)
$lines = [Collections.Generic.List[string]]::new()
$lines.Add('/* Sinclair text logo mask, derived from assets/sinclair-logo.png. */')
$lines.Add('#ifndef FUSE_LIBRETRO_TAPEDECK_LOGO_H')
$lines.Add('#define FUSE_LIBRETRO_TAPEDECK_LOGO_H')
$lines.Add('static const unsigned char tapedeck_logo[13][11] = {')
for ($row = 0; $row -lt 13; $row++) {
    $bytes = [Collections.Generic.List[string]]::new()
    for ($columnByte = 0; $columnByte -lt 11; $columnByte++) {
        $value = 0
        for ($bit = 0; $bit -lt 8; $bit++) {
            if ($small.GetPixel($columnByte * 8 + $bit, $row).A -ge 80) {
                $value = $value -bor (0x80 -shr $bit)
            }
        }
        $bytes.Add(('0x{0:x2}' -f $value))
    }
    $lines.Add('   { ' + ($bytes -join ', ') + ' },')
}
$lines.Add('};')
$lines.Add('#endif')
[IO.File]::WriteAllLines((Join-Path $PSScriptRoot 'tapedeck_logo.h'), $lines)
$graphics.Dispose()
$small.Dispose()
$source.Dispose()
