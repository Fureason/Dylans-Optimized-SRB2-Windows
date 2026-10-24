$ErrorActionPreference = 'Stop'

$sourceRoot = $PSScriptRoot
$toolchainRoot = 'C:\ProgramData\SRB2TC64'
$buildDrive = 'R:'

if (-not (Test-Path -LiteralPath $sourceRoot)) {
	throw "Windows source is missing: $sourceRoot"
}

if (-not (Test-Path -LiteralPath "$toolchainRoot\bin\mingw32-make.exe")) {
	throw "Windows compiler is missing: $toolchainRoot"
}

if (Get-PSDrive -Name $buildDrive.TrimEnd(':') -ErrorAction SilentlyContinue) {
	throw "$buildDrive is already in use. Remove that mapping before building."
}

subst $buildDrive $sourceRoot

try {
	$env:Path = "$toolchainRoot\bin;" + $env:Path
	Push-Location "$buildDrive\"
	try {
		& "$toolchainRoot\bin\mingw32-make.exe" -C src MINGW64=1 SDL=1 WINDOWSHELL=1 CC=gcc 'EXENAME=srb2win64.exe' 'DBGNAME=srb2win64.debug' -j4 all
		if ($LASTEXITCODE -ne 0) {
			throw "SRB2 build failed with exit code $LASTEXITCODE"
		}
	}
	finally {
		Pop-Location
	}
}
finally {
	subst $buildDrive /D
}

Write-Host "Build complete: $sourceRoot\bin\srb2win64.exe"
