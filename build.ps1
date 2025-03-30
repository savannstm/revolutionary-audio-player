#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

if ($args[0] -eq "-r") {
    Start-Process "cmake" -ArgumentList "-DCMAKE_BUILD_TYPE=Release -B build" -NoNewWindow -Wait
} else {
    Start-Process "cmake" -ArgumentList "-DCMAKE_BUILD_TYPE=Debug -B build" -NoNewWindow -Wait
}

try {
    Stop-Process -Name "rap" -Force
} catch {}

& {
    Set-Location -Path build
    Start-Process "cmake" -ArgumentList "--build ." -NoNewWindow -Wait
}