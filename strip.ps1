param (
    [string]$Directory = "./build/target/bin/MinSizeRel"
)

$files = Get-ChildItem -Path $Directory -Recurse -File

foreach ($file in $files) {
    try {
        Write-Host "Stripping: $($file.FullName)"
        llvm-strip --strip-all $file.FullName
    } catch {
        Write-Warning "Failed to strip: $($file.FullName)"
    }
}
