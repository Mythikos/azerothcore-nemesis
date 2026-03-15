# =============================================================================
# Build-Server.ps1
# Copies modules to server directory and rebuilds the worldserver container.
#
# Usage:
#   .\Build-Server.ps1                  # Copy modules + rebuild
#   .\Build-Server.ps1 -RestartOnly     # Skip rebuild, just restart the container
# =============================================================================

param(
    [switch]$RestartOnly
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------
# Configuration
# ---------------------------------------------------------
$ModulesSource = Join-Path $PSScriptRoot "..\modules"
$ModulesDestination = Join-Path $PSScriptRoot "..\server\modules"
$ServerDir = Join-Path $PSScriptRoot "..\server"
$DockerContainer = "ac-worldserver"

# ---------------------------------------------------------
# Helpers
# ---------------------------------------------------------
function Write-Status($message) { Write-Host "[*] $message" -ForegroundColor Cyan }
function Write-Success($message) { Write-Host "[+] $message" -ForegroundColor Green }
function Write-Warning($message) { Write-Host "[!] $message" -ForegroundColor Yellow }
function Write-Failure($message) { Write-Host "[-] $message" -ForegroundColor Red }

# ---------------------------------------------------------
# Copy modules
# ---------------------------------------------------------
Write-Status "Copying modules to server directory..."

if (-not (Test-Path $ModulesSource)) {
    Write-Failure "Source modules directory not found: $ModulesSource"
    exit 1
}

if (-not (Test-Path $ModulesDestination)) {
    Write-Failure "Server modules directory not found: $ModulesDestination"
    exit 1
}

# Clean existing module directories before copying
Get-ChildItem -Path $ModulesSource -Directory | ForEach-Object {
    $targetDir = Join-Path $ModulesDestination $_.Name
    if (Test-Path $targetDir) {
        Remove-Item $targetDir -Recurse -Force
        Write-Status "Removed existing $($_.Name) from server modules"
    }
}

# Copy all contents from modules source to server modules destination
Copy-Item -Path "$ModulesSource\*" -Destination $ModulesDestination -Recurse -Force
Write-Success "Modules copied to $ModulesDestination"

# ---------------------------------------------------------
# Build / Restart
# ---------------------------------------------------------
if ($RestartOnly) {
    Write-Status "Restarting $DockerContainer..."
    docker restart $DockerContainer
    if ($LASTEXITCODE -ne 0) {
        Write-Failure "Failed to restart $DockerContainer."
        exit 1
    }
    Write-Success "$DockerContainer restarted."
}
else {
    Write-Status "Rebuilding $DockerContainer (this may take a while)..."
    $originalDir = Get-Location
    Set-Location $ServerDir

    try {
        docker compose up -d --build $DockerContainer
        if ($LASTEXITCODE -ne 0) {
            Write-Failure "Docker build failed."
            exit 1
        }
        Write-Success "$DockerContainer rebuilt and started."
    }
    finally {
        Set-Location $originalDir
    }
}

Write-Host ""
Write-Success "Done."
