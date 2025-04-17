#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

try {
    Stop-Process -Name "rap" -Force
} catch {}

Start-Process "cmake" -ArgumentList "-DVCPKG_TARGET_TRIPLET=x64-windows-static -B build" -NoNewWindow -Wait

if ($args[0] -eq "-c") {
    exit
}

Set-Location "build"

if ($args[0] -eq "-r") {
    Start-Process "cmake" -ArgumentList "--build . --config Release" -NoNewWindow -Wait
} elseif ($args[0] -eq "-m") {
    Start-Process "cmake" -ArgumentList "--build . --config MinSizeRel" -NoNewWindow -Wait
} else {
    Start-Process "cmake" -ArgumentList "--build . --config Debug" -NoNewWindow -Wait
}

Set-Location ".."