[CmdletBinding()]
param(
    [string]$EnginePath,
    [switch]$Game,
    [switch]$BuildOnly
)

$ErrorActionPreference = 'Stop'
$projectPath = Join-Path $PSScriptRoot 'BattleGround.uproject'
$project = Get-Content -Raw -LiteralPath $projectPath | ConvertFrom-Json
$requiredVersion = $project.EngineAssociation

if ($EnginePath) {
    $installLocations = @(Convert-Path -LiteralPath $EnginePath)
} else {
    $installLocations = @()
    $launcherManifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $launcherManifest -PathType Leaf) {
        $launcher = Get-Content -Raw -LiteralPath $launcherManifest | ConvertFrom-Json
        $installLocations += $launcher.InstallationList |
            Where-Object AppName -eq "UE_$requiredVersion" |
            ForEach-Object InstallLocation
    }

    $registryPath = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$requiredVersion"
    if (Test-Path -LiteralPath $registryPath) {
        $installLocations += (Get-ItemProperty -LiteralPath $registryPath).InstalledDirectory
    }
    $installLocations += Join-Path $env:ProgramFiles "Epic Games\UE_$requiredVersion"
}

$engineDirectory = $null
foreach ($location in $installLocations) {
    if (!$location) {
        continue
    }
    foreach ($candidate in @((Join-Path $location 'Engine'), $location)) {
        $versionFile = Join-Path $candidate 'Build\Build.version'
        $candidateBuild = Join-Path $candidate 'Build\BatchFiles\Build.bat'
        $candidateEditor = Join-Path $candidate 'Binaries\Win64\UnrealEditor.exe'
        if ((Test-Path -LiteralPath $versionFile -PathType Leaf) -and
            (Test-Path -LiteralPath $candidateBuild -PathType Leaf) -and
            (Test-Path -LiteralPath $candidateEditor -PathType Leaf)) {
            $version = Get-Content -Raw -LiteralPath $versionFile | ConvertFrom-Json
            if ("$($version.MajorVersion).$($version.MinorVersion)" -eq $requiredVersion) {
                $engineDirectory = $candidate
                break
            }
        }
    }
    if ($engineDirectory) {
        break
    }
}

if (!$engineDirectory) {
    throw "Unreal Engine $requiredVersion was not found. Install it or pass -EnginePath with the matching installation folder."
}

$build = Join-Path $engineDirectory 'Build\BatchFiles\Build.bat'
$editor = Join-Path $engineDirectory 'Binaries\Win64\UnrealEditor.exe'
Write-Host "Building BattleGroundEditor with $engineDirectory"
& $build BattleGroundEditor Win64 Development "-Project=$projectPath" -WaitMutex
if ($LASTEXITCODE -ne 0) {
    $buildLog = Join-Path $env:LOCALAPPDATA 'UnrealBuildTool\Log.txt'
    throw "BattleGroundEditor build failed (exit $LASTEXITCODE). See the errors above or the build log at $buildLog."
}

$module = Join-Path $PSScriptRoot 'Binaries\Win64\UnrealEditor-BattleGround.dll'
if (!(Test-Path -LiteralPath $module -PathType Leaf)) {
    throw "The build did not produce the editor module at $module."
}

if (!$BuildOnly) {
    $editorArguments = @('"{0}"' -f $projectPath)
    if ($Game) {
        $editorArguments += '-game', '-log'
    }
    Start-Process -FilePath $editor -ArgumentList $editorArguments -WorkingDirectory $PSScriptRoot
}
