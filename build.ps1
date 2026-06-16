<#
.SYNOPSIS
  Local build / package / release for JPEGView (replaces the GitHub Actions CI).

.DESCRIPTION
  Builds the JPEGView solution locally with the v143 toolset, optionally packages a
  portable zip (matching what the old "Publish Release on tag" workflow produced), and
  optionally creates a GitHub Release with the GitHub CLI (release uploads are free; only
  Actions *compute* minutes are billed, which is why CI was moved local).

  Requires: Visual Studio 2022 or Build Tools 2022 with the "Desktop development with C++"
  workload (v143) and ATL/WTL. -Publish additionally needs the GitHub CLI (gh) authenticated.

.EXAMPLE
  .\build.ps1                         # Release|x64 (default)
.EXAMPLE
  .\build.ps1 -Config Debug -Platform Win32
.EXAMPLE
  .\build.ps1 -All                    # Debug+Release x Win32+x64 (the four CI build jobs)
.EXAMPLE
  .\build.ps1 -Package -Tag v1.4.0    # Release x64 + portable zip JPEGView_v1.4.0_x64_portable.zip
.EXAMPLE
  .\build.ps1 -Package -Publish -Tag v1.4.0   # also: gh release create v1.4.0 with the zip (+ installer if present)
#>
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')] [string]$Config = 'Release',
    [ValidateSet('x64', 'Win32')]     [string]$Platform = 'x64',
    [switch]$All,
    [switch]$Package,
    [string]$Tag,
    [switch]$Publish
)

$ErrorActionPreference = 'Stop'
$repo = $PSScriptRoot
$sln = Join-Path $repo 'src\JPEGView.sln'

# --- Locate Visual Studio (Build Tools or full VS) with MSBuild + the C++ toolset ---
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere not found. Install Visual Studio 2022 or Build Tools 2022 with the C++ workload."
}
$vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath | Select-Object -First 1
if (-not $vsPath) { throw "No Visual Studio install with MSBuild was found." }

Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
Set-Location $repo

function Build-One([string]$cfg, [string]$plat) {
    Write-Host "==> Building $cfg|$plat (v143) ..." -ForegroundColor Cyan
    # Build the two code projects (the app + the WIC loader DLL). The Setup/MSI project is
    # skipped here because it needs the WiX toolset; build the full solution if you have WiX.
    & msbuild $sln '/t:JPEGView;WICLoader' /m /nologo /clp:Summary `
        "/property:Platform=$plat" "/property:Configuration=$cfg" '/property:PlatformToolset=v143'
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $cfg|$plat" }
}

if ($All) {
    foreach ($c in 'Debug', 'Release') { foreach ($p in 'Win32', 'x64') { Build-One $c $p } }
}
else {
    Build-One $Config $Platform
}

if ($Package -or $Publish) {
    $relDir = Join-Path $repo 'src\JPEGView\bin\x64\Release'
    if (-not (Test-Path (Join-Path $relDir 'JPEGView.exe'))) { Build-One 'Release' 'x64' }

    $ver = if ($Tag) { $Tag } else { 'local' }
    # Drop developer-only files so the portable zip matches the old CI artifact contents.
    Get-ChildItem -Path $relDir -Recurse -Include *.pdb, *.exp, *.lib -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
    $zip = Join-Path $repo "JPEGView_${ver}_x64_portable.zip"
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path "$relDir\*" -DestinationPath $zip -Force
    Write-Host "==> Portable zip: $zip" -ForegroundColor Green

    if ($Publish) {
        if (-not $Tag) { throw "-Publish requires -Tag (e.g. -Tag v1.4.0)." }
        if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
            throw "-Publish needs the GitHub CLI (gh) installed and authenticated."
        }
        $assets = @($zip)
        $installer = Get-ChildItem -Path (Join-Path $repo 'src\JPEGView.Setup\bin') -Recurse `
            -Include *.exe, *.msi -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($installer) { $assets += $installer.FullName }
        else { Write-Host "(no installer found under src\JPEGView.Setup\bin - publishing zip only)" -ForegroundColor Yellow }
        Write-Host "==> Publishing release $Tag ..." -ForegroundColor Cyan
        & gh release create $Tag @assets --title "JPEGView $Tag" --generate-notes
        if ($LASTEXITCODE -ne 0) { throw "gh release create failed." }
    }
}

Write-Host "Done." -ForegroundColor Green
