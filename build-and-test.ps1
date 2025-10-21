param(
    [ValidateSet('host','linux','macos','ios','android','windows')][string]$Platform = 'host',
    [string]$Arch,
    [ValidateSet('Release','Debug')][string]$BuildType = 'Release',
    [switch]$Clean,
    [switch]$NoFFI,
    [switch]$Shared,
    [switch]$Verbose,
    [switch]$RunTests,
    [switch]$SkipTests,
    [string]$Output,
    [switch]$InstallHeaders,
    [switch]$Help
)

$ErrorActionPreference = 'Stop'

function Show-Help {
@"
web-ifc build script (PowerShell)

Usage: ./build-and-test.ps1 [options]

Platform & Architecture:
    -Platform host|linux|macos|ios|android|windows
    -Arch     x86_64|arm64|armv7|x86|universal (required for some cross builds)

Build Configuration:
    -BuildType Release|Debug (default Release)
    -NoFFI     Disable FFI build
    -Shared    Build shared libraries
    -Clean     Remove build directory before building

Output:
    -Output <dir>        Custom build/output directory
    -InstallHeaders      Copy public headers into include folder
    -Verbose             CMake verbose makefiles

Testing:
    -RunTests            Force running tests
    -SkipTests           Skip tests

Other:
    -Help                Show this help and exit
"@
}

if ($Help) { Show-Help; exit 0 }

# Auto-detect arch when host and none provided
if ($Platform -eq 'host' -and -not $Arch) {
    $Arch = $(uname -m) 2>$null
}

if ($Platform -eq 'macos' -and $Arch -eq 'universal') {
    $macArchArg = 'x86_64;arm64'
} elseif ($Arch) {
    $macArchArg = $Arch
}

$sourceRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cppSource = Join-Path $sourceRoot 'src/cpp'

if (-not $Output) {
    $Output = if ($Platform -eq 'host') { 'build/native' } else { "build/$Platform" + ($Arch ? "-$Arch" : '') }
}

$buildDir = $Output

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "[CLEAN] Removing $buildDir" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}
if (-not (Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

# Decide tests auto-run
if (-not $RunTests.IsPresent -and -not $SkipTests.IsPresent) {
    $autoRun = $Platform -eq 'host'
} else {
    $autoRun = $RunTests.IsPresent -and -not $SkipTests.IsPresent
}

$ffiFlag = if ($NoFFI) { 'OFF' } else { 'ON' }
$sharedFlag = if ($Shared) { 'ON' } else { 'OFF' }

$cmakeArgs = @(
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DWEBIFC_BUILD_FFI=$ffiFlag",
    "-DBUILD_SHARED_LIBS=$sharedFlag"
)

switch ($Platform) {
    'macos' {
        if ($macArchArg) { $cmakeArgs += "-DCMAKE_OSX_ARCHITECTURES=$macArchArg" }
    }
    'ios' {
        if (-not $Arch) { throw 'Architecture (-Arch arm64|x86_64) required for iOS.' }
        $cmakeArgs += '-DCMAKE_SYSTEM_NAME=iOS'
        $cmakeArgs += '-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0'
        $cmakeArgs += "-DCMAKE_OSX_ARCHITECTURES=$Arch"
        if ($Arch -eq 'x86_64') { $cmakeArgs += '-DCMAKE_OSX_SYSROOT=iphonesimulator' }
    }
    'android' {
        if (-not $env:ANDROID_NDK) { throw 'ANDROID_NDK env var not set' }
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$($env:ANDROID_NDK)/build/cmake/android.toolchain.cmake"
        $cmakeArgs += '-DANDROID_PLATFORM=android-21'
        switch ($Arch) {
            'arm64' { $cmakeArgs += '-DANDROID_ABI=arm64-v8a' }
            'armv7' { $cmakeArgs += '-DANDROID_ABI=armeabi-v7a' }
            'x86_64' { $cmakeArgs += '-DANDROID_ABI=x86_64' }
            'x86' { $cmakeArgs += '-DANDROID_ABI=x86' }
            default { throw 'Architecture required for Android (arm64|armv7|x86_64|x86)' }
        }
    }
    'windows' {
        # If cross compiling with mingw available
        if (Get-Command x86_64-w64-mingw32-gcc -ErrorAction SilentlyContinue) {
            $cmakeArgs += '-DCMAKE_SYSTEM_NAME=Windows'
            $cmakeArgs += '-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc'
            $cmakeArgs += '-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++'
        }
    }
    'linux' {
        if ($Arch -eq 'arm64' -and (Get-Command aarch64-linux-gnu-gcc -ErrorAction SilentlyContinue)) {
            $cmakeArgs += '-DCMAKE_SYSTEM_NAME=Linux'
            $cmakeArgs += '-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc'
            $cmakeArgs += '-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++'
        }
    }
}

Write-Host "[CONFIG] Platform=$Platform Arch=$Arch BuildType=$BuildType FFI=$ffiFlag Shared=$sharedFlag" -ForegroundColor Cyan

Push-Location $buildDir

if ($Verbose) { $cmakeArgs += '-DCMAKE_VERBOSE_MAKEFILE=ON' }
cmake $cmakeArgs $cppSource
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }

# Parallel jobs
$jobs = (Get-Command nproc -ErrorAction SilentlyContinue) ? (& nproc) : ((sysctl -n hw.ncpu) 2>$null)
if (-not $jobs) { $jobs = 4 }
Write-Host "[BUILD] Parallel jobs: $jobs" -ForegroundColor Cyan
if ($IsWindows) {
    # On multi-config generators (Visual Studio) we would use --config, keep compatibility if detected later.
    cmake --build . --config $BuildType -- -j $jobs
} else {
    # Single-config generators (Unix Makefiles/Ninja) ignore --config
    cmake --build . -- -j $jobs
}
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }

# Artifact summary
Write-Host "[ARTIFACTS]" -ForegroundColor Green
$artifacts = @()
foreach ($f in 'libweb-ifc-library.a','libweb-ifc-library.so','libweb-ifc-ffi.a','libweb-ifc-ffi.so','web-ifc','web-ifc-test','web-ifc-ffi-test') {
    if (Test-Path $f) {
        $size = (Get-Item $f).Length / 1KB
        Write-Host "  - $f ($( '{0:N1}KB' -f $size ))"; $artifacts += $f
    }
}

if ($InstallHeaders) {
    Write-Host '[HEADERS] Installing...' -ForegroundColor Cyan
    $incDir = Join-Path (Get-Location) 'include'
    New-Item -ItemType Directory -Force -Path $incDir | Out-Null
    if ($ffiFlag -eq 'ON') { Copy-Item (Join-Path $cppSource 'ffi/web-ifc-ffi.h') $incDir }
    Get-ChildItem -Recurse (Join-Path $cppSource 'web-ifc') -Filter *.h | ForEach-Object { Copy-Item $_.FullName (Join-Path $incDir 'web-ifc') -Force }
}

if ($autoRun) {
    Write-Host '[TEST] Running ctest...' -ForegroundColor Cyan
    # For single-config generators, --config is invalid; only pass it when multi-config expected.
    if ($IsWindows) {
        ctest --output-on-failure --config $BuildType
    } else {
        ctest --output-on-failure
    }
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
}

Pop-Location

Write-Host "[DONE] Artifacts: $($artifacts.Count) at $buildDir" -ForegroundColor Green
