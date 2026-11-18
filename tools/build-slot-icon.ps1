# Deterministic equipment glyph: white geometry, empty transparent interiors.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$scale=4
$canvas=[Drawing.Bitmap]::new(200*$scale,200*$scale,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g=[Drawing.Graphics]::FromImage($canvas)
$output=[Drawing.Bitmap]::new(200,200,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$down=[Drawing.Graphics]::FromImage($output)
$ring=[Drawing.Drawing2D.GraphicsPath]::new([Drawing.Drawing2D.FillMode]::Alternate)
try {
 $g.Clear([Drawing.Color]::Transparent)
 $g.ScaleTransform($scale,$scale)
 $g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
 $white=[Drawing.Brushes]::White
 $ring.AddEllipse(66,10,68,68)
 $ring.AddEllipse(82,26,36,36)
 $g.FillPath($white,$ring)
 $roof=[Drawing.PointF[]]@([Drawing.PointF]::new(100,62),[Drawing.PointF]::new(43,94),[Drawing.PointF]::new(43,104),[Drawing.PointF]::new(157,104),[Drawing.PointF]::new(157,94))
 $g.FillPolygon($white,$roof)
 $g.FillRectangle($white,52,100,18,72)
 $g.FillRectangle($white,130,100,18,72)
 $g.FillRectangle($white,52,168,96,22)
 $g.FillEllipse($white,41,168,22,22)
 $g.FillEllipse($white,137,168,22,22)
 $down.Clear([Drawing.Color]::Transparent)
 $down.CompositingMode=[Drawing.Drawing2D.CompositingMode]::SourceCopy
 $down.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
 $down.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::HighQuality
 $down.DrawImage($canvas,[Drawing.Rectangle]::new(0,0,200,200))
 $output.Save((Join-Path $PSScriptRoot '../assets/item/lantern-slot.png'),[Drawing.Imaging.ImageFormat]::Png)
} finally {$ring.Dispose();$down.Dispose();$output.Dispose();$g.Dispose();$canvas.Dispose()}
