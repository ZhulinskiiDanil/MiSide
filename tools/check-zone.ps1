[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$metadataPath = Join-Path $workspaceRoot '.workspace-zone.json'
if (-not (Test-Path -LiteralPath $metadataPath)) { throw 'Run this from a prepared UI or 3D worktree, not integration/main.' }
$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$rules = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'work-zones.json') -Raw | ConvertFrom-Json
$rule = $rules.PSObject.Properties[$metadata.zone].Value
if (-not $rule) { throw "Unknown workspace zone: $($metadata.zone)" }
function Invoke-ZoneGit {
    param([string[]]$GitArgs)
    $result = & git -c "safe.directory=$workspaceRoot" -C $workspaceRoot @GitArgs
    if ($LASTEXITCODE -ne 0) { throw "git failed: $GitArgs" }
    $result
}
$branch = Invoke-ZoneGit @('branch', '--show-current')
if ($branch -ne $metadata.branch) { throw "Expected branch $($metadata.branch), got $branch" }
Invoke-ZoneGit @('merge-base', '--is-ancestor', $metadata.base, 'HEAD')
$files = @(
    Invoke-ZoneGit @('diff', '--name-only', '--no-renames', $metadata.base, 'HEAD', '--')
    Invoke-ZoneGit @('diff', '--name-only', '--no-renames', 'HEAD', '--')
    Invoke-ZoneGit @('ls-files', '--others', '--exclude-standard')
) | Where-Object { $_ } | Sort-Object -Unique
$outside = @($files | Where-Object {
    $path = $_
    $allowed = @($rule.allow | Where-Object { $path -like $_ }).Count -gt 0
    $denied = @($rule.deny | Where-Object { $path -like $_ }).Count -gt 0
    (-not $allowed) -or $denied
})
if ($outside.Count -gt 0) {
    Write-Output "Changes outside $($metadata.zone) ownership (record integration requests instead):"
    $outside | ForEach-Object { Write-Output "  $_" }
    exit 1
}
Write-Output "Zone $($metadata.zone): OK ($(@($files).Count) changed paths, base $($metadata.base.Substring(0, 8)))."
