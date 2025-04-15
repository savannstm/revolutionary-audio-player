#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

try {
    Stop-Process -Name "rap" -Force
} catch {}

# maybe remove that later
Start-Process "cmake" -ArgumentList " -DVCPKG_TARGET_TRIPLET=x64-windows-static -DWMF_STRMIIDS_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/strmiids.lib`" -DWMF_AMSTRMID_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/amstrmid.lib`" -B build -DWMF_DMOGUIDS_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/dmoguids.lib`" -DWMF_UUID_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/uuid.lib`" -DWMF_MSDMO_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/msdmo.lib`" -DWMF_OLE32_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/ole32.lib`" -DWMF_OLEAUT32_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/oleaut32.lib`" -DWMF_MF_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/mf.lib`" -DWMF_MFUUID_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/mfuuid.lib`" -DWMF_MFPLAT_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/mfplat.lib`" -DWMF_MFCORE_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/mfcore.lib`" -DWMF_PROPSYS_LIBRARY=`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/propsys.lib`"" -NoNewWindow -Wait

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