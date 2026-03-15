<#
.SYNOPSIS
    Generates a client install manifest JSON file from a directory tree.

.PARAMETER ClientPath
    Root folder to scan (e.g. the WoW client directory).

.PARAMETER BaseDownloadUrl
    CDN base URL prepended to each file's relative path.

.PARAMETER OutFile
    Where to write the manifest JSON. Defaults to "manifest.json" in the current directory.

.PARAMETER IgnorePaths
    Array of relative folder/file paths to exclude (supports wildcards).
    Defaults to common WoW throwaway paths.

.PARAMETER Version
    Manifest version string. Defaults to today's date as yyyy.MM.dd.1.

.PARAMETER UpdateOnly
    Array of relative file paths to update in an existing manifest.
    When specified, loads the existing manifest, updates only these files
    (re-hashing them from ClientPath), bumps the version, and writes back.
    Files that no longer exist on disk are removed from the manifest.

.EXAMPLE
    .\Generate-Manifest.ps1 -ClientPath "C:\Games\WoW" -BaseDownloadUrl "https://cdn.example.com/client"

.EXAMPLE
    .\Generate-Manifest.ps1 -ClientPath "C:\Games\WoW" -BaseDownloadUrl "https://cdn.example.com/client" -OutFolder "D:\out" -UpdateOnly "Wow.exe","Data/patch-N.MPQ"
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ClientPath,

    [Parameter(Mandatory)]
    [string]$BaseDownloadUrl,

    [Parameter(Mandatory)]
    [string]$OutFolder,

    [string]$OutFile = "manifest.json",

    [string[]]$IgnorePaths = @(
        "WDB\*",
        "WTF\Config.wtf",
        "WTF\Account\*",
        "Cache\*",
        "Logs\*",
        "Screenshots\*",
        "Errors\*",
        "UpdaterBackups\*",
        "Data\enUS\Interface\*",
        "Data\enUS\Documentation\*"
    ),

    [string]$Version,

    [string[]]$UpdateOnly
)

$ErrorActionPreference = "Stop"

# Resolve to an absolute path
$ClientPath = (Resolve-Path $ClientPath).Path

if (-not (Test-Path $ClientPath -PathType Container)) {
    Write-Error "ClientPath '$ClientPath' does not exist or is not a directory."
    return
}

# Build version from today's date if not supplied
if (-not $Version) {
    $Version = (Get-Date -Format "yyyy.MM.dd") + ".1"
}

# Normalise the base URL (strip trailing slash)
$BaseDownloadUrl = $BaseDownloadUrl.TrimEnd('/')

# Helper: hash a single file and return a manifest entry
function Get-FileEntry {
    param([string]$FullPath, [string]$RelativeForward)
    $stream = [System.IO.File]::OpenRead($FullPath)
    try {
        $hashBytes = $script:hasher.ComputeHash($stream)
    } finally {
        $stream.Close()
    }
    $sha256 = ([BitConverter]::ToString($hashBytes) -replace '-', '').ToLowerInvariant()
    $size = (Get-Item $FullPath).Length

    return [ordered]@{
        path        = $RelativeForward
        sha256      = $sha256
        size        = $size
        downloadUrl = "$BaseDownloadUrl/$RelativeForward"
    }
}

$script:hasher = [System.Security.Cryptography.SHA256]::Create()
$manifestPath = Join-Path $OutFolder $OutFile

# ── Update-only mode ─────────────────────────────────────────────────────────
if ($UpdateOnly) {
    if (-not (Test-Path $manifestPath)) {
        Write-Error "Cannot update: manifest '$manifestPath' does not exist. Run a full scan first."
        return
    }

    $existing = Get-Content $manifestPath -Raw | ConvertFrom-Json
    # Convert files array to a hashtable keyed by path for easy lookup
    $fileMap = [ordered]@{}
    foreach ($entry in $existing.files) {
        $fileMap[$entry.path] = [ordered]@{
            path        = $entry.path
            sha256      = $entry.sha256
            size        = $entry.size
            downloadUrl = $entry.downloadUrl
        }
    }

    $updated = 0
    $removed = 0
    foreach ($target in $UpdateOnly) {
        # Normalise to forward slashes
        $relForward = $target -replace '\\', '/'
        $absPath = Join-Path $ClientPath ($target -replace '/', '\')

        if (Test-Path $absPath -PathType Leaf) {
            $newEntry = Get-FileEntry -FullPath $absPath -RelativeForward $relForward
            $fileMap[$relForward] = $newEntry
            $updated++
            Write-Host "  ~ $relForward  ($([math]::Round($newEntry.size / 1MB, 2)) MB)"
        } else {
            if ($fileMap.Contains($relForward)) {
                $fileMap.Remove($relForward)
                $removed++
                Write-Host "  - $relForward  (removed, file no longer exists)"
            } else {
                Write-Host "  ? $relForward  (not in manifest and not on disk, skipping)"
            }
        }
    }

    $manifest = [ordered]@{
        version = $Version
        files   = @($fileMap.Values)
    }

    $json = $manifest | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText($manifestPath, $json, [System.Text.Encoding]::UTF8)

    $script:hasher.Dispose()
    Write-Host ""
    Write-Host "Manifest updated: '$manifestPath'"
    Write-Host "  Version : $Version"
    Write-Host "  Updated : $updated"
    Write-Host "  Removed : $removed"
    Write-Host "  Total   : $($manifest.files.Count)"
    return
}

# ── Full scan mode ────────────────────────────────────────────────────────────
Write-Host "Scanning '$ClientPath' ..."
Write-Host "Ignore patterns: $($IgnorePaths -join ', ')"

$allFiles = Get-ChildItem -Path $ClientPath -File -Recurse

$manifest = [ordered]@{
    version = $Version
    files   = @()
}

$skipped = 0

foreach ($file in $allFiles) {
    # Compute the path relative to the client root (forward-slash separated)
    $relativePath = $file.FullName.Substring($ClientPath.Length).TrimStart('\', '/')
    $relativePathForward = $relativePath -replace '\\', '/'

    # Check ignore patterns
    $ignored = $false
    foreach ($pattern in $IgnorePaths) {
        if ($relativePath -like $pattern) {
            $ignored = $true
            break
        }
    }
    if ($ignored) {
        $skipped++
        continue
    }

    $entry = Get-FileEntry -FullPath $file.FullName -RelativeForward $relativePathForward
    $manifest.files += $entry
    Write-Host "  + $relativePathForward  ($([math]::Round($file.Length / 1MB, 2)) MB)"
}

$script:hasher.Dispose()

# Write JSON
$json = $manifest | ConvertTo-Json -Depth 4
[System.IO.File]::WriteAllText($manifestPath, $json, [System.Text.Encoding]::UTF8)

Write-Host ""
Write-Host "Manifest written to '$OutFile'"
Write-Host "  Version : $Version"
Write-Host "  Files   : $($manifest.files.Count)"
Write-Host "  Skipped : $skipped"
