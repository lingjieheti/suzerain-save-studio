param(
    # Optional toolchain root (containing bin) or bin directory.
    [string]$Toolchain = "",
    [ValidateSet("Auto", "LLVM", "GCC")][string]$CompilerKind = "Auto"
)
$ErrorActionPreference = "Stop"
$rootDir = $PSScriptRoot
$pairs = @()
if ($CompilerKind -ne "GCC") {
    $pairs += ,@("x86_64-w64-mingw32-clang++.exe", "x86_64-w64-mingw32-windres.exe", "LLVM")
}
if ($CompilerKind -ne "LLVM") {
    $pairs += ,@("x86_64-w64-mingw32-g++.exe", "x86_64-w64-mingw32-windres.exe", "GCC")
    $pairs += ,@("g++.exe", "windres.exe", "GCC")
}
$searchDirs = @()
if ($Toolchain) {
    $searchDirs += Join-Path $Toolchain "bin"
    $searchDirs += $Toolchain
} else {
    foreach ($pair in $pairs) {
        $command = Get-Command $pair[0] -ErrorAction SilentlyContinue
        if ($command) { $searchDirs += Split-Path $command.Source }
    }
    $searchDirs += "C:\Strawberry\c\bin"
}
$compiler = $null
$resourceCompiler = $null
$kind = $null
:findCompiler foreach ($directory in ($searchDirs | Select-Object -Unique)) {
    foreach ($pair in $pairs) {
        $candidate = Join-Path $directory $pair[0]
        $resourceCandidate = Join-Path $directory $pair[1]
        if ((Test-Path -LiteralPath $candidate) -and (Test-Path -LiteralPath $resourceCandidate)) {
            $compiler = (Resolve-Path -LiteralPath $candidate).Path
            $resourceCompiler = (Resolve-Path -LiteralPath $resourceCandidate).Path
            $kind = $pair[2]
            break findCompiler
        }
    }
}
if (!$compiler) { throw "No MinGW x64 compiler and windres found. Use -Toolchain <root-or-bin> [-CompilerKind LLVM|GCC]." }
$target = & $compiler -dumpmachine
if ($LASTEXITCODE -ne 0 -or $target -notmatch '^x86_64.*(mingw|windows)') {
    throw "The selected compiler does not target Windows x64: $target"
}
$buildDir = Join-Path $rootDir "build"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$extraFlags = @()
if ($kind -eq "GCC") {
    # Stable attribute IDs: https://learn.microsoft.com/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
    # Optional calls return an unsupported result on older Windows versions.
    $compatHeader = Join-Path $buildDir "mingw-dwm-compat.hpp"
    $compatLines = @(
        '#ifndef UNICODE', '#define UNICODE', '#endif',
        '#ifndef _UNICODE', '#define _UNICODE', '#endif',
        '#ifndef WINVER', '#define WINVER 0x0A00', '#endif',
        '#ifndef _WIN32_WINNT', '#define _WIN32_WINNT 0x0A00', '#endif',
        '#ifndef WIN32_LEAN_AND_MEAN', '#define WIN32_LEAN_AND_MEAN', '#endif',
        '#ifndef NOMINMAX', '#define NOMINMAX', '#endif',
        '#include <windows.h>', '#include <dwmapi.h>',
        '#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE', '#define DWMWA_USE_IMMERSIVE_DARK_MODE 20', '#endif',
        '#ifndef DWMWA_CAPTION_COLOR', '#define DWMWA_CAPTION_COLOR 35', '#endif',
        '#ifndef DWMWA_TEXT_COLOR', '#define DWMWA_TEXT_COLOR 36', '#endif',
        '#undef NOMINMAX' # main.cpp declares this again after the forced include.
    )
    $compatLines | Set-Content -LiteralPath $compatHeader -Encoding ascii
    $extraFlags += @("-include", $compatHeader, "-static-libgcc", "-static-libstdc++")
}
Push-Location (Join-Path $rootDir "src")
try {
    & $resourceCompiler "app.rc" "-O" "coff" "-o" (Join-Path $buildDir "app.res")
    if ($LASTEXITCODE -ne 0) { throw "Resource compilation failed." }
    & $compiler "main.cpp" (Join-Path $buildDir "app.res") "-std=c++17" "-Os" "-Wall" "-Wextra" "-municode" "-mwindows" "-static" "-s" @extraFlags "-o" (Join-Path $buildDir "SuzerainSaveStudio.exe") "-luser32" "-lkernel32" "-lcomdlg32" "-lcomctl32" "-lshell32" "-lole32" "-luuid" "-lgdi32" "-luxtheme" "-ldwmapi" "-lgdiplus"
    if ($LASTEXITCODE -ne 0) { throw "C++ compilation failed." }
    Write-Host "Built ($kind, $target): $buildDir/SuzerainSaveStudio.exe"
} finally {
    Pop-Location
}
