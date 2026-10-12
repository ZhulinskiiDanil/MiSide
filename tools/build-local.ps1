[CmdletBinding()]
param([ValidateRange(1, 32)][int]$Jobs = 4, [switch]$ConfigureOnly)
$ErrorActionPreference = 'Stop'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$metadataPath = Join-Path $workspaceRoot '.workspace-zone.json'
if (-not (Test-Path -LiteralPath $metadataPath)) { throw 'Use a prepared feature worktree, not the main checkout.' }
$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
if ($metadata.zone -notin @('3d', 'ui')) { throw 'Invalid workspace zone.' }
$buildRoot = Join-Path $workspaceRoot ("build-" + $metadata.zone)
if (-not $env:GEODE_SDK -or -not (Test-Path -LiteralPath $env:GEODE_SDK)) { throw 'Set GEODE_SDK to the existing SDK directory.' }
foreach ($tool in @('cmake', 'ninja', 'clang', 'clang++', 'geode')) { $null = Get-Command $tool -ErrorAction Stop }
$configureArgs = @('-S', $workspaceRoot, '-B', $buildRoot, '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++',
    '-DGEODE_DONT_INSTALL_MODS=ON')
Write-Output "Building zone $($metadata.zone) in $buildRoot. Automatic game installation is disabled."
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
if (-not $ConfigureOnly) {
    & cmake --build $buildRoot --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
}
