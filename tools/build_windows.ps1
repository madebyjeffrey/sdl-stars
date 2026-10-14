param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$Msys2Root = 'C:\msys64',
    [int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
$pszRoot = Split-Path $PSScriptRoot -Parent
$pszBin = Join-Path $Msys2Root 'ucrt64\bin'
$szPreset = 'windows-sdl-' + $Configuration.ToLowerInvariant()
$pszBuild = Join-Path $pszRoot "dist\$szPreset"
$pszInstall = Join-Path $pszRoot "dist\$szPreset-portable"
$szPath = $env:PATH

if (!(Test-Path (Join-Path $pszBin 'gcc.exe')) -or !(Test-Path (Join-Path $pszBin 'g++.exe')) -or !(Test-Path (Join-Path $pszBin 'cmake.exe'))) {
    throw 'Install the MSYS2 UCRT64 dependencies listed in docs/WINDOWS-SDL3.md first.'
}

try {
    $env:PATH = "$pszBin;$szPath"
    Push-Location $pszRoot
    try {
        & cmake --preset $szPreset "-DPython3_EXECUTABLE=$pszBin\python.exe"
        if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
        & cmake --build --preset $szPreset --parallel $Jobs
        if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
        & cmake --install $pszBuild --prefix $pszInstall --component StarsRuntime
        if ($LASTEXITCODE -ne 0) { throw 'Runtime packaging failed.' }
    } finally { Pop-Location }
} finally { $env:PATH = $szPath }

Write-Host "Built native Windows SDL client: $pszInstall\bin\stars-sdl.exe"
