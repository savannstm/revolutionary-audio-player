#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

Set-Location ./src/rusty-decoder
cargo b -r

New-Item -ItemType Directory -Force -Path ../../lib
Move-Item -Force ./target/release/rusty_decoder.dll ../../lib/

Set-Location ../../

Set-Location ./lib
gendef rusty_decoder.dll
dlltool -d rusty_decoder.def -l rusty_decoder.lib -D rusty_decoder.dll

Set-Location ..

try {
    Stop-Process -Name "revolutionary-audio-player" -Force
} catch {}

Start-Sleep(1)

Copy-Item ./lib/rusty_decoder.dll ./target/bin

Start-Process "cmake" -ArgumentList "--build ." -NoNewWindow -Wait

Start-Process "./target/bin/revolutionary-audio-player"