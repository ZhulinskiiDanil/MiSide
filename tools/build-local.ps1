[CmdletBinding()]
param([ValidateRange(1, 32)][int]$Jobs = 4, [switch]$ConfigureOnly)
$ErrorActionPreference = 'Stop'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$metadataPath = Join-Path $workspaceRoot '.workspace-zone.json'
if (Test-Path -LiteralPath $metadataPath) {
    $metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
    $zone = $metadata.zone
    if ($zone -notin @('3d', 'ui', 'integration')) { throw 'Invalid workspace zone.' }
} else {
    $branch = & git -c "safe.directory=$workspaceRoot" -C $workspaceRoot branch --show-current
    if ($LASTEXITCODE -ne 0 -or $branch -ne 'main') { throw 'Expected main or a prepared worktree.' }
    $zone = 'main'
}
$buildName = if ($zone -eq 'main') { 'build' } else { "build-$zone" }
$buildRoot = Join-Path $workspaceRoot $buildName
if (-not $env:GEODE_SDK -or -not (Test-Path -LiteralPath $env:GEODE_SDK)) { throw 'Set GEODE_SDK to the existing SDK directory.' }
foreach ($tool in @('cmake', 'ninja', 'clang', 'clang++', 'geode')) { $null = Get-Command $tool -ErrorAction Stop }
$configureArgs = @('-S', $workspaceRoot, '-B', $buildRoot, '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++',
    '-DGEODE_DONT_INSTALL_MODS=ON')
Write-Output "Building zone $zone in $buildRoot. Automatic game installation is disabled."
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
if (-not $ConfigureOnly) {
    & cmake --build $buildRoot --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
}
