#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

function Show-Help {
    Write-Output @"
Usage: ./build.ps1 [options]

Options:
    -c                      Configure only (no build)
    -r                      Build in Release configuration
    -m                      Build in MinSizeRel configuration
    --cflags <flags>        Specify flags when running `cmake .` command
    --bflags <flags>        Specify flags when running `cmake --build .` command
    --help                  Show this help message
"@
}

$buildConfig = "Debug"
$configureOnly = $false
$cflags = ""
$bflags = ""

for ($i = 0; $i -lt $args.Count; $i++) {
    switch ($args[$i]) {
        "--help" {
            Show-Help
            exit
        }
        "-c" {
            $configureOnly = $true
        }
        "-r" {
            $buildConfig = "Release"
        }
        "-m" {
            $buildConfig = "MinSizeRel"
        }
        "--cflags" {
            if ($i + 1 -ge $args.Count) {
                Write-Error "--cflags requires a value"
                exit 1
            }

            $cflags = $args[$i + 1]
            $i++
        }
        "--bflags" {
            if ($i + 1 -ge $args.Count) {
                Write-Error "--bflags requires a value"
                exit 1
            }

            $bflags = $args[$i + 1]
            $i++
        }
        default {
            Write-Error "Unknown argument: $($args[$i])"
            Show-Help
            exit 1
        }
    }
}

try {
    Stop-Process -Name "rap" -Force
} catch {}

$cmakeArgs = "-B build"
if ($cflags -ne "") {
    $cmakeArgs += " $cflags"
}

Start-Process "cmake" -ArgumentList $cmakeArgs -NoNewWindow -Wait

if ($configureOnly) {
    exit
}

Set-Location "build"

$buildArgs = "--build . --config $buildConfig -j"
if ($bflags -ne "") {
    $buildArgs += " $bflags"
}

Start-Process "cmake" -ArgumentList $buildArgs -NoNewWindow -Wait

Set-Location ".."
