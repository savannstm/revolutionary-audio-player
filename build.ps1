#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

if ($args[0] -eq "-r") {
    Start-Process "cmake" -ArgumentList "-DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build" -NoNewWindow -Wait
} else {
    Start-Process "cmake" -ArgumentList "-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build" -NoNewWindow -Wait
}

try {
    Stop-Process -Name "rap" -Force
} catch {}

& {
    Set-Location -Path build
    Start-Process "cmake" -ArgumentList "--build ." -NoNewWindow -Wait

    Start-Process "./target/bin/rap"
}