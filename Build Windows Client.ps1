param(
    [Parameter(Mandatory = $true)]
    [string]$RuntimeDirectory,
    [string]$ToolchainBin,
    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$originalPath = $env:Path
$buildDirectory = $PSScriptRoot
$buildDrive = $null
$runtime = (Resolve-Path -LiteralPath $RuntimeDirectory).Path

function Write-RuntimeBuildInputs([string]$OutputDirectory) {
    $libraries = [ordered]@{
        'SDL2.dll'                  = 'libSDL2.dll.a'
        'SDL2_mixer_ext.dll'         = 'libSDL2_mixer_ext.dll.a'
        'libcurl-x64.dll'            = 'libcurl.dll.a'
        'libgme.dll'                 = 'libgme.dll.a'
        'libopenmpt.dll'              = 'libopenmpt.dll.a'
        'libfluidsynth-2.dll'         = $null
        'libgcc_s_sjlj-1.dll'        = $null
        'libstdc++-6.dll'             = $null
        'discord_partner_sdk.dll'   = $null
    }

    $gendef = (Get-Command -Name gendef.exe -CommandType Application).Source
    $dlltool = (Get-Command -Name dlltool.exe -CommandType Application).Source
    $windres = (Get-Command -Name windres.exe -CommandType Application).Source
    foreach ($name in $libraries.Keys) {
        if (!(Test-Path -LiteralPath (Join-Path $runtime $name) -PathType Leaf)) {
            throw "Missing Windows release library $name in $runtime."
        }
    }

    $scratch = Join-Path ([IO.Path]::GetTempPath()) ('srb2-runtime-' + [Guid]::NewGuid().ToString('N'))
    New-Item -Path $scratch -ItemType Directory | Out-Null
    try {
        Push-Location -LiteralPath $scratch
        try {
            $resourceId = 100
            $resources = @()
            $filenames = @()
            foreach ($name in $libraries.Keys) {
                Copy-Item -LiteralPath (Join-Path $runtime $name) -Destination $scratch
                $resources += ('{0} RCDATA "{1}"' -f $resourceId, $name)
                $filenames += ('{{ {0}, "{1}" }},' -f $resourceId, $name)
                ++$resourceId

                $importLibrary = $libraries[$name]
                if (!$importLibrary) { continue }
                $definition = [IO.Path]::ChangeExtension($name, 'def')
                & $gendef --no-include-current-dir -f $name
                if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $definition -PathType Leaf)) {
                    throw "Could not read the exports in $name."
                }
                & $dlltool --machine i386:x86-64 --input-def $definition --output-delaylib $importLibrary
                if ($LASTEXITCODE -ne 0) { throw "Could not build the delay imports for $name." }
            }

            [IO.File]::WriteAllLines((Join-Path $scratch 'runtime.rc'), $resources, [Text.Encoding]::ASCII)
            [IO.File]::WriteAllLines((Join-Path $scratch 'windows-runtime-files.inc'), $filenames, [Text.Encoding]::ASCII)
            & $windres --input runtime.rc --output windows-runtime.res --output-format coff
            if ($LASTEXITCODE -ne 0) { throw 'Could not embed the runtime libraries.' }

            New-Item -Path $OutputDirectory -ItemType Directory -Force | Out-Null
            foreach ($name in @($libraries.Values | Where-Object { $_ }) + @('windows-runtime-files.inc', 'windows-runtime.res')) {
                Copy-Item -LiteralPath (Join-Path $scratch $name) -Destination $OutputDirectory -Force
            }
        } finally {
            Pop-Location
        }
    } finally {
        Get-ChildItem -LiteralPath $scratch -File -Force | Remove-Item -Force
        [IO.Directory]::Delete($scratch)
    }
}

try {
    if ($ToolchainBin) {
        $env:Path = (Resolve-Path -LiteralPath $ToolchainBin).Path + ';' + $env:Path
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

    $runtimeInputs = 'make/windows-runtime-repair'
    Write-RuntimeBuildInputs (Join-Path $buildDirectory $runtimeInputs)

    $makeName = [IO.Path]::GetFileNameWithoutExtension($make.Source)
    $makeArguments = @(
        'all', "MAKE=$makeName", 'MINGW64=1', 'SDL=1', 'WINDOWSHELL=1', 'CC=gcc',
        'EXENAME=srb2win.exe', "WIN_RUNTIME_DIR=../$runtimeInputs",
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
