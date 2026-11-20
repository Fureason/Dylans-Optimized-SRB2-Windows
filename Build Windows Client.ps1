param(
    [string]$ToolchainBin,
    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$originalPath = $env:Path
$buildDirectory = $PSScriptRoot
$buildDrive = $null

try {
    if ($ToolchainBin) {
        $resolvedBin = (Resolve-Path -LiteralPath $ToolchainBin).Path
        $env:Path = $resolvedBin + ';' + $env:Path
    }

    $make = Get-Command -Name 'make.exe', 'mingw32-make.exe' -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    $gcc = Get-Command -Name 'gcc.exe' -CommandType Application -ErrorAction SilentlyContinue
    if (!$make -or !$gcc) {
        throw 'Install a 64-bit MinGW-w64 toolchain and add its bin directory to PATH, or pass -ToolchainBin.'
    }

    if ($buildDirectory -match '\s') {
        foreach ($letter in 'W', 'V', 'U', 'T', 'S', 'Q') {
            $drive = $letter + ':'
            if (!(Test-Path -LiteralPath ($drive + '\'))) {
                & subst.exe $drive $buildDirectory
                if ($LASTEXITCODE -ne 0) { throw "Unable to map source directory to $drive." }
                $buildDrive = $drive
                $buildDirectory = $buildDrive + '\'
                break
            }
        }
        if (!$buildDrive) { throw 'A free drive letter is needed for this source path.' }
    }

    $makeName = [IO.Path]::GetFileNameWithoutExtension($make.Source)
    $makeArguments = @(
        'all', "MAKE=$makeName", 'MINGW64=1', 'SDL=1', 'WINDOWSHELL=1', 'CC=gcc',
        ('SHELL=' + $env:ComSpec.Replace('\', '/')), '-j6'
    )
    if ($Rebuild) { $makeArguments += '-B' }

    Push-Location -LiteralPath $buildDirectory
    try {
        & $make.Source @makeArguments
        if ($LASTEXITCODE -ne 0) { throw "Windows compilation failed with exit code $LASTEXITCODE." }
    } finally {
        Pop-Location
    }
} finally {
    $env:Path = $originalPath
    if ($buildDrive) { & subst.exe $buildDrive /d }
}
