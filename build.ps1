#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

try {
    Stop-Process -Name "revolutionary-audio-player" -Force
} catch {}

Start-Sleep(1)

Start-Process "cmake" -ArgumentList "--build ." -NoNewWindow -Wait

Start-Process "./target/bin/revolutionary-audio-player"