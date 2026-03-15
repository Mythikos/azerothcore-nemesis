# =============================================================================
# Deploy-PatchDeploy.ps1
# Packages exported DBC files into MPQ patches and deploys to server + client.
# Handles two patch types:
#   - patch-s.MPQ: Spell.dbc (generated SQL via node-dbc-reader, deployed to DB)
#   - patch-t.MPQ: CharTitles.dbc (no SQL needed, just DBC deploy)
#
# Prerequisites:
#   - mpqcli (https://github.com/TheGrayDot/mpqcli/releases)
#     Install: irm https://raw.githubusercontent.com/thegraydot/mpqcli/main/scripts/install.ps1 | iex
#   - node-dbc-reader (https://github.com/wowgaming/node-dbc-reader)
#     Clone into .\node-dbc-reader and run: npm install
#   - Node.js LTS (https://nodejs.org)
#   - Docker (for server deployment via docker cp)
#
# Usage:
#   .\Deploy-PatchDeploy.ps1                  # Full deploy (all patches)
#   .\Deploy-PatchDeploy.ps1 -SkipServer      # Package and deploy to client only
#   .\Deploy-PatchDeploy.ps1 -SkipClient      # Package and deploy to server only
#   .\Deploy-PatchDeploy.ps1 -SkipSql         # Skip SQL generation and deployment
#   .\Deploy-PatchDeploy.ps1 -SkipSpell       # Skip Spell.dbc entirely
#   .\Deploy-PatchDeploy.ps1 -SkipTitles      # Skip CharTitles.dbc entirely
#   .\Deploy-PatchDeploy.ps1 -RestartServer   # Also restart ac-worldserver after deploy
# =============================================================================

param(
    [switch]$SkipServer,
    [switch]$SkipClient,
    [switch]$SkipSql,
    [switch]$SkipSpell,
    [switch]$SkipTitles,
    [switch]$RestartServer,
    [switch]$DryRun
)

# ---------------------------------------------------------
# Configuration
# ---------------------------------------------------------
# Where Stoneharry's Spell Editor exports DBC files
$SpellEditorExportDir = Join-Path $PSScriptRoot "WoW Spell Editor\Export"

# Where the master CharTitles.dbc lives (edited via WDBX Editor or similar)
$CharTitlesSourceDir = Join-Path $PSScriptRoot "..\.data\dbc"

# Your project's patches directory (where patch-s.mpq, patch-t.mpq, and patch-s.sql live)
$ProjectPatchesDir = Join-Path $PSScriptRoot "..\.patches"

# Your project's data directory (where the master copies of DBC files live)
$ProjectDataDir = Join-Path $PSScriptRoot "..\.data\dbc"

# Client Data folder (where the MPQ gets deployed for the game client)
# Update this path to point to your WoW 3.3.5a client Data folder
$ClientDataDir = "C:\path\to\your\wow-3.3.5a\Data"

# Path to mpqcli executable (if not on PATH)
$MpqCliPath = Join-Path $PSScriptRoot "mpqcli\mpqcli-windows-amd64.exe"

# Path to node-dbc-reader directory (relative to this script or absolute)
$NodeDbcReaderDir = Join-Path $PSScriptRoot "node-dbc-reader"

# Minimum spell ID for custom Nemesis spells (everything >= this gets exported to SQL)
$CustomSpellIdThreshold = 100000

# Docker container name for AzerothCore worldserver
$DockerContainer = "ac-worldserver"

# Server-side paths inside the container
$ServerDbcDir = "/azerothcore/env/dist/data/dbc"
$ServerSpellDbcPath = "$ServerDbcDir/Spell.dbc"
$ServerCharTitlesDbcPath = "$ServerDbcDir/CharTitles.dbc"

# MPQ file names
$SpellMpqFileName = "patch-s.MPQ"
$TitlesMpqFileName = "patch-t.MPQ"

# Name of the output SQL file
$SqlFileName = "patch-s.sql"

# Database connection — choose ONE method:
#   "docker"  = execute SQL via docker exec into a MySQL container
#   "local"   = execute SQL via a local mysql client on your host

# Docker method settings
$MySqlContainer = "ac-database"

# Override via environment variables MYSQL_USER and MYSQL_PASSWORD, or edit defaults below.
# Shared database settings (used by both methods)
$SqlDeployMethod = "docker"
$MySqlUser = if ($env:MYSQL_USER) { $env:MYSQL_USER } else { "root" }
$MySqlPassword = if ($env:MYSQL_PASSWORD) { $env:MYSQL_PASSWORD } else { "password" }
$MySqlDatabase = "acore_world"
$MySqlHost = "127.0.0.1" # Local method settings (only used when SqlDeployMethod = "local")
$MySqlPort = "3306" # Local method settings (only used when SqlDeployMethod = "local")

# SpellScript bindings — maps custom spell IDs to their C++ SpellScript class names.
# These are inserted into `spell_script_names` so the server knows which SpellScript
# to attach to each spell ID. Only spells that have a corresponding RegisterSpellScript
# in C++ need an entry here.
$SpellScriptBindings = @{
    100001 = "spell_nemesis_coward_heal"
    100009 = "spell_nemesis_umbral_burst"
    100010 = "spell_nemesis_deathblow_strike"
}

# ---------------------------------------------------------
# Helper functions
# ---------------------------------------------------------
$ErrorActionPreference = "Stop"

function Write-Status($message) { Write-Host "[*] $message" -ForegroundColor Cyan }
function Write-Success($message) { Write-Host "[+] $message" -ForegroundColor Green }
function Write-Warning($message) { Write-Host "[!] $message" -ForegroundColor Yellow }
function Write-Failure($message) { Write-Host "[-] $message" -ForegroundColor Red }

function Get-MpqCli {
    if ($MpqCliPath -and (Test-Path $MpqCliPath)) { return $MpqCliPath }
    $found = Get-Command "mpqcli" -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    $found = Get-Command "mpqcli.exe" -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    return $null
}

# Creates an MPQ from a staging directory and returns the output path (or $null on failure).
function New-MpqArchive($stagingDir, $mpqFileName) {
    $mpqOutputPath = Join-Path $env:TEMP $mpqFileName
    if (Test-Path $mpqOutputPath) { Remove-Item $mpqOutputPath -Force }

    if ($DryRun) {
        Write-Warning "[DRY RUN] Would run: $mpqcli create -g wow-wotlk -o `"$mpqOutputPath`" `"$stagingDir`""
        return $mpqOutputPath
    }

    & $mpqcli create -g wow-wotlk -o "$mpqOutputPath" "$stagingDir" | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Failure "mpqcli failed to create $mpqFileName (exit code: $LASTEXITCODE)"
        return $null
    }

    if (-not (Test-Path $mpqOutputPath)) {
        $searchLocations = @($env:TEMP, (Get-Location).Path, $stagingDir)
        $generatedMpq = $null
        foreach ($searchDir in $searchLocations) {
            $generatedMpq = Get-ChildItem -Path $searchDir -Filter "*.mpq" -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
            if ($generatedMpq) { break }
        }
        if ($generatedMpq) {
            Move-Item $generatedMpq.FullName $mpqOutputPath -Force
        }
        else {
            Write-Failure "$mpqFileName was not created. Check mpqcli output above."
            return $null
        }
    }

    $mpqSize = [math]::Round((Get-Item $mpqOutputPath).Length / 1KB)
    Write-Success "Created $mpqFileName ($mpqSize KB)"
    return $mpqOutputPath
}

# ---------------------------------------------------------
# Validate environment
# ---------------------------------------------------------
Write-Status "Validating environment..."

$mpqcli = Get-MpqCli
if (-not $mpqcli) {
    Write-Failure "mpqcli not found. Install it from: https://github.com/TheGrayDot/mpqcli/releases"
    Write-Failure "  Windows install: irm https://raw.githubusercontent.com/thegraydot/mpqcli/main/scripts/install.ps1 | iex"
    exit 1
}
Write-Success "Found mpqcli: $mpqcli"

if (-not $SkipSql -and -not $SkipSpell) {
    if (-not (Test-Path $NodeDbcReaderDir)) {
        Write-Failure "node-dbc-reader directory not found: $NodeDbcReaderDir"
        Write-Failure "  Clone it: git clone https://github.com/wowgaming/node-dbc-reader.git"
        Write-Failure "  Then run: cd node-dbc-reader && npm install"
        exit 1
    }
    $nodeCheck = Get-Command "node" -ErrorAction SilentlyContinue
    if (-not $nodeCheck) {
        Write-Failure "Node.js not found. Install from: https://nodejs.org"
        exit 1
    }
    Write-Success "Found Node.js: $($nodeCheck.Source)"
    Write-Success "Found node-dbc-reader: $NodeDbcReaderDir"
}

# Track what we actually process
$spellProcessed = $false
$titlesProcessed = $false
$sqlGenerated = $false
$sqlDeployed = $false
$spellMpqOutputPath = $null
$titlesMpqOutputPath = $null

# =============================================================================
# SPELL.DBC PIPELINE (patch-s)
# =============================================================================
if (-not $SkipSpell) {
    Write-Host ""
    Write-Host "=============================================" -ForegroundColor Magenta
    Write-Host " Spell.dbc Pipeline (patch-s)" -ForegroundColor Magenta
    Write-Host "=============================================" -ForegroundColor Magenta

    if (-not (Test-Path $SpellEditorExportDir)) {
        Write-Warning "Spell Editor export directory not found: $SpellEditorExportDir — skipping Spell.dbc."
    }
    else {
        $exportedDbcFiles = Get-ChildItem -Path $SpellEditorExportDir -Filter "Spell.dbc" -ErrorAction SilentlyContinue
        if (-not $exportedDbcFiles -or $exportedDbcFiles.Count -eq 0) {
            Write-Warning "No Spell.dbc found in export directory — skipping Spell.dbc pipeline."
        }
        else {
            Write-Success "Found Spell.dbc ($([math]::Round($exportedDbcFiles[0].Length / 1KB)) KB)"

            # Stage Spell.dbc
            $spellStagingDir = Join-Path $env:TEMP "nemesis-spell-staging"
            $spellDbFilesDir = Join-Path $spellStagingDir "DBFilesClient"
            if (Test-Path $spellStagingDir) { Remove-Item $spellStagingDir -Recurse -Force }
            New-Item -ItemType Directory -Path $spellDbFilesDir -Force | Out-Null
            Copy-Item $exportedDbcFiles[0].FullName -Destination $spellDbFilesDir
            Write-Success "Staged Spell.dbc into DBFilesClient\"

            # Generate SQL
            if (-not $SkipSql) {
                Write-Status "Generating AzerothCore SQL for custom spells (ID >= $CustomSpellIdThreshold)..."

                $spellDbcExport = $exportedDbcFiles[0].FullName
                $readerDbcDir = Join-Path $NodeDbcReaderDir "data\dbc"
                if (-not (Test-Path $readerDbcDir)) { New-Item -ItemType Directory -Path $readerDbcDir -Force | Out-Null }
                Copy-Item $spellDbcExport -Destination $readerDbcDir -Force
                Write-Success "Copied Spell.dbc into node-dbc-reader data/dbc/"

                $rawSqlOutput = Join-Path $env:TEMP "nemesis-raw-spell-export.sql"
                $finalSqlOutput = Join-Path $ProjectPatchesDir $SqlFileName

                if ($DryRun) {
                    Write-Warning "[DRY RUN] Would run node-dbc-reader to export spells with ID >= $CustomSpellIdThreshold"
                }
                else {
                    $originalDir = Get-Location
                    Set-Location $NodeDbcReaderDir

                    try {
                        & npm run start -- --search="{*} >= $CustomSpellIdThreshold" --columns=ID --out-type=sql --file="$rawSqlOutput" Spell
                        if ($LASTEXITCODE -ne 0) {
                            Write-Failure "node-dbc-reader failed (exit code: $LASTEXITCODE)"
                            Write-Warning "SQL generation skipped. You'll need to build patch-s.sql manually."
                        }
                        elseif (Test-Path $rawSqlOutput) {
                            $rawLines = Get-Content $rawSqlOutput
                            $insertLines = $rawLines | Where-Object { $_ -match "INSERT\s+IGNORE\s+INTO|INSERT\s+INTO" }
                            $spellCount = ($insertLines | Measure-Object).Count

                            $wrappedLines = @("-- =============================================================================")
                            $wrappedLines += "-- patch-s.sql - Auto-generated by Deploy-PatchDeploy.ps1"
                            $wrappedLines += "-- Custom Nemesis spells (ID >= $CustomSpellIdThreshold)"
                            $wrappedLines += "-- Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
                            $wrappedLines += "-- ============================================================================="
                            $wrappedLines += ""
                            $wrappedLines += "USE acore_world;"
                            $wrappedLines += ""
                            $wrappedLines += "-- Wipe all custom Nemesis spells before re-inserting"
                            $wrappedLines += "DELETE FROM ``spell_dbc`` WHERE ``ID`` >= $CustomSpellIdThreshold;"
                            $wrappedLines += ""

                            foreach ($line in $rawLines) {
                                if ($line.Trim().Length -gt 0) {
                                    $wrappedLines += $line
                                }
                            }

                            if ($SpellScriptBindings.Count -gt 0) {
                                $wrappedLines += ""
                                $wrappedLines += "-- -----------------------------------------------------------------------------"
                                $wrappedLines += "-- SpellScript bindings (spell_script_names)"
                                $wrappedLines += "-- -----------------------------------------------------------------------------"
                                $wrappedLines += "DELETE FROM ``spell_script_names`` WHERE ``spell_id`` >= $CustomSpellIdThreshold;"
                                $wrappedLines += ""

                                foreach ($spellId in ($SpellScriptBindings.Keys | Sort-Object)) {
                                    $scriptName = $SpellScriptBindings[$spellId]
                                    $wrappedLines += "INSERT INTO ``spell_script_names`` (``spell_id``, ``ScriptName``) VALUES ($spellId, '$scriptName');"
                                }

                                Write-Success "Included $($SpellScriptBindings.Count) spell_script_names binding(s)"
                            }

                            if (-not (Test-Path $ProjectPatchesDir)) { New-Item -ItemType Directory -Path $ProjectPatchesDir -Force | Out-Null }
                            $wrappedLines | Set-Content -Path $finalSqlOutput -Encoding UTF8
                            Remove-Item $rawSqlOutput -Force -ErrorAction SilentlyContinue

                            Write-Success "Generated $SqlFileName with $spellCount custom spell(s) -> $ProjectPatchesDir"
                            $sqlGenerated = $true
                        }
                        else {
                            Write-Warning "node-dbc-reader produced no output. Are there spells with ID >= ${CustomSpellIdThreshold}?"
                        }
                    }
                    finally {
                        Set-Location $originalDir
                    }
                }

                # Deploy SQL to database
                if ($sqlGenerated) {
                    Write-Status "Applying $SqlFileName to database ($MySqlDatabase)..."

                    if ($DryRun) {
                        Write-Warning "[DRY RUN] Would apply $SqlFileName to $MySqlDatabase via $SqlDeployMethod"
                    }
                    else {
                        if ($SqlDeployMethod -eq "docker") {
                            $containerSqlPath = "/tmp/$SqlFileName"
                            docker cp "$finalSqlOutput" "${MySqlContainer}:${containerSqlPath}" 2>&1 | Out-Null
                            if ($LASTEXITCODE -ne 0) {
                                Write-Failure "Failed to copy SQL file into MySQL container ($MySqlContainer). Is it running?"
                            }
                            else {
                                $dockerResult = docker exec $MySqlContainer mysql -u"$MySqlUser" -p"$MySqlPassword" $MySqlDatabase -e "source $containerSqlPath" 2>&1
                                if ($LASTEXITCODE -ne 0) {
                                    Write-Failure "SQL execution failed inside container:"
                                    Write-Host $dockerResult -ForegroundColor Red
                                }
                                else {
                                    Write-Success "Applied $SqlFileName to $MySqlDatabase via docker exec ($MySqlContainer)"
                                    $sqlDeployed = $true
                                }
                                docker exec $MySqlContainer rm -f $containerSqlPath 2>&1 | Out-Null
                            }
                        }
                        elseif ($SqlDeployMethod -eq "local") {
                            $mysqlExe = Get-Command "mysql" -ErrorAction SilentlyContinue
                            if (-not $mysqlExe) {
                                Write-Failure "mysql client not found on PATH."
                            }
                            else {
                                $localResult = & mysql -h"$MySqlHost" -P"$MySqlPort" -u"$MySqlUser" -p"$MySqlPassword" $MySqlDatabase -e "source $finalSqlOutput" 2>&1
                                if ($LASTEXITCODE -ne 0) {
                                    Write-Failure "SQL execution failed:"
                                    Write-Host $localResult -ForegroundColor Red
                                }
                                else {
                                    Write-Success "Applied $SqlFileName to $MySqlDatabase via local mysql client"
                                    $sqlDeployed = $true
                                }
                            }
                        }
                        else {
                            Write-Failure "Unknown SqlDeployMethod: $SqlDeployMethod"
                        }
                    }
                }
            }
            else {
                Write-Warning "SQL generation skipped (-SkipSql)."
            }

            # Create spell MPQ
            Write-Status "Creating $SpellMpqFileName..."
            $spellMpqOutputPath = New-MpqArchive $spellStagingDir $SpellMpqFileName

            if ($spellMpqOutputPath) {
                # Copy Spell.dbc to project data directory
                $spellDbcStaged = Join-Path $spellDbFilesDir "Spell.dbc"
                if (-not (Test-Path $ProjectDataDir)) { New-Item -ItemType Directory -Path $ProjectDataDir -Force | Out-Null }
                if (-not $DryRun) {
                    Copy-Item $spellDbcStaged -Destination $ProjectDataDir -Force
                    Write-Success "Copied Spell.dbc -> $ProjectDataDir"
                }

                # Copy MPQ to project patches
                if (-not (Test-Path $ProjectPatchesDir)) { New-Item -ItemType Directory -Path $ProjectPatchesDir -Force | Out-Null }
                if (-not $DryRun) {
                    Copy-Item $spellMpqOutputPath -Destination (Join-Path $ProjectPatchesDir $SpellMpqFileName) -Force
                    Write-Success "Copied $SpellMpqFileName -> $ProjectPatchesDir"
                }

                $spellProcessed = $true
            }

            # Cleanup staging
            if (-not $DryRun) { Remove-Item $spellStagingDir -Recurse -Force -ErrorAction SilentlyContinue }
        }
    }
}
else {
    Write-Warning "Spell.dbc pipeline skipped (-SkipSpell)."
}

# =============================================================================
# CHARTITLES.DBC PIPELINE (patch-t)
# =============================================================================
if (-not $SkipTitles) {
    Write-Host ""
    Write-Host "=============================================" -ForegroundColor Magenta
    Write-Host " CharTitles.dbc Pipeline (patch-t)" -ForegroundColor Magenta
    Write-Host "=============================================" -ForegroundColor Magenta

    $charTitlesSource = Join-Path $CharTitlesSourceDir "CharTitles.dbc"
    if (-not (Test-Path $charTitlesSource)) {
        Write-Warning "CharTitles.dbc not found at: $charTitlesSource — skipping titles pipeline."
    }
    else {
        $charTitlesSize = [math]::Round((Get-Item $charTitlesSource).Length / 1KB)
        Write-Success "Found CharTitles.dbc ($charTitlesSize KB)"

        # Stage CharTitles.dbc
        $titlesStagingDir = Join-Path $env:TEMP "nemesis-titles-staging"
        $titlesDbFilesDir = Join-Path $titlesStagingDir "DBFilesClient"
        if (Test-Path $titlesStagingDir) { Remove-Item $titlesStagingDir -Recurse -Force }
        New-Item -ItemType Directory -Path $titlesDbFilesDir -Force | Out-Null
        Copy-Item $charTitlesSource -Destination $titlesDbFilesDir
        Write-Success "Staged CharTitles.dbc into DBFilesClient\"

        # Create titles MPQ
        Write-Status "Creating $TitlesMpqFileName..."
        $titlesMpqOutputPath = New-MpqArchive $titlesStagingDir $TitlesMpqFileName

        if ($titlesMpqOutputPath) {
            # Copy MPQ to project patches
            if (-not (Test-Path $ProjectPatchesDir)) { New-Item -ItemType Directory -Path $ProjectPatchesDir -Force | Out-Null }
            if (-not $DryRun) {
                Copy-Item $titlesMpqOutputPath -Destination (Join-Path $ProjectPatchesDir $TitlesMpqFileName) -Force
                Write-Success "Copied $TitlesMpqFileName -> $ProjectPatchesDir"
            }

            $titlesProcessed = $true
        }

        # Cleanup staging
        if (-not $DryRun) { Remove-Item $titlesStagingDir -Recurse -Force -ErrorAction SilentlyContinue }
    }
}
else {
    Write-Warning "CharTitles.dbc pipeline skipped (-SkipTitles)."
}

# =============================================================================
# DEPLOY TO SERVER
# =============================================================================
if (-not $SkipServer) {
    Write-Host ""
    Write-Status "Deploying DBC files to server container ($DockerContainer)..."

    if ($spellProcessed) {
        $spellDbcFile = Join-Path $ProjectDataDir "Spell.dbc"
        if (Test-Path $spellDbcFile) {
            if ($DryRun) {
                Write-Warning "[DRY RUN] Would deploy Spell.dbc to $DockerContainer"
            }
            else {
                docker cp "$spellDbcFile" "${DockerContainer}:${ServerSpellDbcPath}"
                if ($LASTEXITCODE -ne 0) {
                    Write-Failure "Failed to deploy Spell.dbc to server. Is the container running?"
                }
                else {
                    Write-Success "Deployed Spell.dbc to $DockerContainer"
                }
            }
        }
    }

    if ($titlesProcessed) {
        $charTitlesFile = Join-Path $CharTitlesSourceDir "CharTitles.dbc"
        if (Test-Path $charTitlesFile) {
            if ($DryRun) {
                Write-Warning "[DRY RUN] Would deploy CharTitles.dbc to $DockerContainer"
            }
            else {
                docker cp "$charTitlesFile" "${DockerContainer}:${ServerCharTitlesDbcPath}"
                if ($LASTEXITCODE -ne 0) {
                    Write-Failure "Failed to deploy CharTitles.dbc to server. Is the container running?"
                }
                else {
                    Write-Success "Deployed CharTitles.dbc to $DockerContainer"
                }
            }
        }
    }

    if (-not $spellProcessed -and -not $titlesProcessed) {
        Write-Warning "No DBC files to deploy to server."
    }

    if ($RestartServer) {
        Write-Status "Restarting server container..."
        if ($DryRun) {
            Write-Warning "[DRY RUN] Would run: docker restart $DockerContainer"
        }
        else {
            docker restart $DockerContainer
            if ($LASTEXITCODE -eq 0) {
                Write-Success "Server container restarted."
            }
            else {
                Write-Failure "Failed to restart server container."
            }
        }
    }
    else {
        Write-Warning "Server restart skipped. Run with -RestartServer to auto-restart, or manually: docker restart $DockerContainer"
    }
}
else {
    Write-Warning "Server deploy skipped (-SkipServer)."
}

# =============================================================================
# DEPLOY MPQS TO CLIENT
# =============================================================================
if (-not $SkipClient) {
    Write-Host ""
    Write-Status "Deploying MPQ patches to client Data folder..."

    if (-not (Test-Path $ClientDataDir)) {
        Write-Failure "Client Data directory not found: $ClientDataDir"
    }
    else {
        if ($spellProcessed -and $spellMpqOutputPath -and (Test-Path $spellMpqOutputPath)) {
            if ($DryRun) {
                Write-Warning "[DRY RUN] Would copy $SpellMpqFileName -> $ClientDataDir"
            }
            else {
                Copy-Item $spellMpqOutputPath -Destination (Join-Path $ClientDataDir $SpellMpqFileName) -Force
                Write-Success "Deployed $SpellMpqFileName -> $ClientDataDir"
            }
        }

        if ($titlesProcessed -and $titlesMpqOutputPath -and (Test-Path $titlesMpqOutputPath)) {
            if ($DryRun) {
                Write-Warning "[DRY RUN] Would copy $TitlesMpqFileName -> $ClientDataDir"
            }
            else {
                Copy-Item $titlesMpqOutputPath -Destination (Join-Path $ClientDataDir $TitlesMpqFileName) -Force
                Write-Success "Deployed $TitlesMpqFileName -> $ClientDataDir"
            }
        }

        if (-not $spellProcessed -and -not $titlesProcessed) {
            Write-Warning "No MPQ files to deploy to client."
        }
    }
}
else {
    Write-Warning "Client deploy skipped (-SkipClient)."
}

# ---------------------------------------------------------
# Cleanup temp MPQ files
# ---------------------------------------------------------
if (-not $DryRun) {
    if ($spellMpqOutputPath) { Remove-Item $spellMpqOutputPath -Force -ErrorAction SilentlyContinue }
    if ($titlesMpqOutputPath) { Remove-Item $titlesMpqOutputPath -Force -ErrorAction SilentlyContinue }
}

# ---------------------------------------------------------
# Summary
# ---------------------------------------------------------
Write-Host ""
Write-Host "=============================================" -ForegroundColor White
Write-Host " Deployment Summary" -ForegroundColor White
Write-Host "=============================================" -ForegroundColor White
Write-Host "  --- Spell.dbc (patch-s) ---"
Write-Host "  Spell pipeline:      $(if ($SkipSpell) { 'Skipped' } elseif ($spellProcessed) { 'OK' } else { 'No Spell.dbc found' })"
Write-Host "  SQL generated:       $(if ($sqlGenerated) { "$SqlFileName" } elseif ($SkipSql -or $SkipSpell) { 'Skipped' } else { 'Failed or empty' })"
Write-Host "  SQL applied:         $(if ($sqlDeployed) { "Applied to $MySqlDatabase" } elseif ($SkipSql -or $SkipSpell) { 'Skipped' } elseif ($sqlGenerated) { 'FAILED' } else { 'N/A' })"
Write-Host "  Script bindings:     $(if ($SpellScriptBindings.Count -gt 0 -and $sqlGenerated) { "$($SpellScriptBindings.Count) spell_script_names" } elseif ($SkipSql -or $SkipSpell) { 'Skipped' } else { 'None' })"
Write-Host ""
Write-Host "  --- CharTitles.dbc (patch-t) ---"
Write-Host "  Titles pipeline:     $(if ($SkipTitles) { 'Skipped' } elseif ($titlesProcessed) { 'OK' } else { 'No CharTitles.dbc found' })"
Write-Host ""
Write-Host "  --- Deployment ---"
Write-Host "  Server deploy:       $(if ($SkipServer) { 'Skipped' } elseif ($spellProcessed -or $titlesProcessed) { 'Done' } else { 'Nothing to deploy' })"
Write-Host "  Client deploy:       $(if ($SkipClient) { 'Skipped' } elseif ($spellProcessed -or $titlesProcessed) { 'Done' } else { 'Nothing to deploy' })"
Write-Host "  Server restart:      $(if ($RestartServer -and -not $SkipServer) { 'Done' } else { 'Manual' })"
Write-Host "=============================================" -ForegroundColor White
Write-Host ""

if (-not $RestartServer -and -not $SkipServer -and ($spellProcessed -or $titlesProcessed)) {
    Write-Warning "Remember: The server needs a restart to load the new DBC files!"
    Write-Host "  docker restart $DockerContainer" -ForegroundColor Yellow
}
