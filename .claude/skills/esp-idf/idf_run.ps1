<#
.SYNOPSIS
    Wrapper script to run idf.py commands in ESP-IDF environment

.DESCRIPTION
    Uses idf-env to initialize ESP-IDF environment and run idf.py commands.
    This approach matches the official ESP-IDF Windows installer initialization.

    Required environment variables:
    - IDF_ID: ESP-IDF installation ID (from idf-env)

    Optional environment variables:
    - IDF_PROJECT_PATH: Project directory (defaults to current directory)
    - IDF_CMD: Command to run (used when calling from Bash)

.EXAMPLE
    # Run directly from PowerShell
    .\idf_run.ps1 build
    .\idf_run.ps1 --version

    # Run from Bash/Git Bash
    powershell.exe -Command '$env:IDF_ID="esp-idf-xxx"; $env:IDF_CMD="build"; & ".\idf_run.ps1"'
#>

# Set UTF-8 encoding to avoid cp932 codec errors
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$env:PYTHONIOENCODING = "utf-8"
chcp 65001 | Out-Null

# Clear MSYS/Mingw environment variables (ESP-IDF detects and fails)
Remove-Item Env:MSYSTEM -ErrorAction SilentlyContinue
Remove-Item Env:MINGW_PREFIX -ErrorAction SilentlyContinue
Remove-Item Env:MSYSTEM_PREFIX -ErrorAction SilentlyContinue
Remove-Item Env:MSYSTEM_CARCH -ErrorAction SilentlyContinue
Remove-Item Env:MSYSTEM_CHOST -ErrorAction SilentlyContinue

# Check required environment variables
if (-not $env:IDF_ID) {
    Write-Host "Error: IDF_ID environment variable not set" -ForegroundColor Red
    Write-Host "Example: esp-idf-b29c58f93b4ca0f49cdfc4c3ef43b562" -ForegroundColor Yellow
    exit 1
}

# Get command: $args > $env:IDF_CMD > error
$command = @()
if ($args.Count -gt 0) {
    $command = $args
} elseif ($env:IDF_CMD) {
    $command = @($env:IDF_CMD -split '\s+')
}

if ($command.Count -eq 0) {
    Write-Host "Usage: idf_run.ps1 <command> [args...]" -ForegroundColor Red
    Write-Host "Or set `$env:IDF_CMD before calling" -ForegroundColor Yellow
    exit 1
}

# Set up IDF_TOOLS_PATH
$IdfToolsPath = "C:\Espressif"
if ($null -eq $env:IDF_TOOLS_PATH) {
    $env:IDF_TOOLS_PATH = $IdfToolsPath
}
$env:PATH = "$env:IDF_TOOLS_PATH;$env:PATH"

# Use idf-env to get Python and IDF_PATH for the specified IdfId
$IdfId = $env:IDF_ID
$PythonCommand = idf-env config get --property python --idf-id $IdfId
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Failed to get Python path from idf-env for IdfId: $IdfId" -ForegroundColor Red
    exit 1
}

$IDF_PATH = idf-env config get --property path --idf-id $IdfId
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Failed to get IDF_PATH from idf-env for IdfId: $IdfId" -ForegroundColor Red
    exit 1
}

# Verify ESP-IDF path
$isEspIdfRoot = (Test-Path "$IDF_PATH/tools/idf.py")
if (-not $isEspIdfRoot) {
    Write-Host "Error: Unable to find ESP-IDF at: $IDF_PATH" -ForegroundColor Red
    exit 1
}

# Clear PYTHONPATH and PYTHONHOME (may conflict with virtualenv)
if ($null -ne $env:PYTHONPATH) {
    $env:PYTHONPATH = $null
}
if ($null -ne $env:PYTHONHOME) {
    $env:PYTHONHOME = $null
}
$env:PYTHONNOUSERSITE = "True"

# Get Git path
$IdfGit = idf-env config get --property gitPath
$IdfGitDir = (Get-Item $IdfGit).Directory.FullName
$IdfPythonDir = (Get-Item $PythonCommand).Directory.FullName

# Add Python and Git to PATH
$env:PATH = "$IdfGitDir;$IdfPythonDir;$env:PATH"

# Source export.ps1 to set up all ESP-IDF environment variables
$env:IDF_PATH = $IDF_PATH
. "$IDF_PATH/export.ps1"

# Change to project directory if set
if ($env:IDF_PROJECT_PATH) {
    Set-Location $env:IDF_PROJECT_PATH
}

# Run idf.py
$commandString = $command -join " "
Write-Host "Running: idf.py $commandString" -ForegroundColor Cyan

# Create log directory and file
$logDir = "$env:IDF_PROJECT_PATH\build\log"
if (-not (Test-Path $logDir)) {
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
}
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = "$logDir\build_$timestamp.log"

# Run command and capture output to file
$idfPyArgs = @("$IDF_PATH\tools\idf.py") + $command
$argString = $idfPyArgs | ForEach-Object { "`"$_`"" } | Join-String -Separator " "
$process = Start-Process -FilePath $PythonCommand -ArgumentList $argString -NoNewWindow -PassThru -Wait -RedirectStandardOutput "$logFile.stdout" -RedirectStandardError "$logFile.stderr"

# Merge stdout and stderr into single log file
$stdoutContent = if (Test-Path "$logFile.stdout") { Get-Content "$logFile.stdout" -Raw } else { "" }
$stderrContent = if (Test-Path "$logFile.stderr") { Get-Content "$logFile.stderr" -Raw } else { "" }
$fullLog = "$stdoutContent`n$stderrContent"
$fullLog | Out-File -FilePath $logFile -Encoding UTF8

# Clean up temp files
Remove-Item "$logFile.stdout" -ErrorAction SilentlyContinue
Remove-Item "$logFile.stderr" -ErrorAction SilentlyContinue

# Show last 20 lines summary
Write-Host "`n========== Build Log (last 20 lines) ==========" -ForegroundColor Yellow
$lines = Get-Content $logFile
$totalLines = $lines.Count
if ($totalLines -gt 20) {
    $lines | Select-Object -Last 20
} else {
    $lines
}
Write-Host "================================================" -ForegroundColor Yellow
Write-Host "Full log: $logFile" -ForegroundColor Cyan
Write-Host "Exit code: $($process.ExitCode)" -ForegroundColor $(if ($process.ExitCode -eq 0) { "Green" } else { "Red" })

# Clear IDF_CMD to avoid affecting next run
Remove-Item Env:IDF_CMD -ErrorAction SilentlyContinue

exit $process.ExitCode
