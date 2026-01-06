# MyCraft build script for emscripten
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MyCraft Web Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if emcc is available
Write-Host "Checking Emscripten installation..." -ForegroundColor Yellow
$emccPath = Get-Command emcc -ErrorAction SilentlyContinue

if ($null -eq $emccPath) {
    Write-Host ""
    Write-Host "[ERROR] Emscripten not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please activate Emscripten first:" -ForegroundColor Yellow
    Write-Host "  1. Navigate to your emsdk directory:" -ForegroundColor White
    Write-Host "     cd C:\path\to\emsdk" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  2. Run the activation script:" -ForegroundColor White
    Write-Host "     .\emsdk_env.ps1" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  3. Then run this script again:" -ForegroundColor White
    Write-Host "     cd C:\Users\chenk\Documents\GitHub\mycraft" -ForegroundColor Gray
    Write-Host "     .\build-web.ps1" -ForegroundColor Gray
    Write-Host ""
    exit 1
}

Write-Host "  [OK] Emscripten found: $($emccPath.Source)" -ForegroundColor Green

# Define source files
$sources = @(
    "main.cpp",
    "shaders/shaderClass/shaderClass.cpp",
    "shaders/VAO.cpp",
    "shaders/VBO.cpp",
    "shaders/EBO.cpp",
    "player/Camera.cpp",
    "player/Player.cpp",
    "player/Ray.cpp",
    "entities/src/Block.cpp",
    "entities/src/Map.cpp",
    "entities/src/Model/Mesh.cpp",
    "entities/src/Model/Triangle.cpp",
    "entities/src/worldGen/Chunk.cpp",
    "entities/src/worldGen/compBlock.cpp",
    "gameLogic/UpdatePacket.cpp",
    "gameLogic/DestroyPacket.cpp",
    "gameLogic/AddPacket.cpp",
    "libraries/include/stb/stb.cpp",
    "Texture.cpp",
    "libraries/include/mygui/imgui.cpp",
    "libraries/include/mygui/imgui_draw.cpp",
    "libraries/include/mygui/imgui_tables.cpp",
    "libraries/include/mygui/imgui_widgets.cpp",
    "libraries/include/mygui/backends/imgui_impl_glfw.cpp",
    "libraries/include/mygui/backends/imgui_impl_opengl3.cpp"
)

# Validate source files
Write-Host ""
Write-Host "Validating source files..." -ForegroundColor Yellow
$missingFiles = @()
$foundCount = 0

foreach ($file in $sources) {
    if (!(Test-Path $file)) {
        $missingFiles += $file
        Write-Host "  [MISSING] $file" -ForegroundColor Red
    } else {
        $foundCount++
    }
}

if ($missingFiles.Count -gt 0) {
    Write-Host ""
    Write-Host "[ERROR] Missing $($missingFiles.Count) file(s). Aborting build." -ForegroundColor Red
    exit 1
}

Write-Host "  [OK] All $foundCount source files found" -ForegroundColor Green

# Check required directories
Write-Host ""
Write-Host "Checking asset directories..." -ForegroundColor Yellow
$requiredDirs = @("shaders/web", "textures", "libraries/include/mygui/misc/fonts")
$missingDirs = @()

foreach ($dir in $requiredDirs) {
    if (!(Test-Path $dir)) {
        $missingDirs += $dir
        Write-Host "  [MISSING] $dir" -ForegroundColor Red
    } else {
        Write-Host "  [OK] $dir" -ForegroundColor Green
    }
}

if ($missingDirs.Count -gt 0) {
    Write-Host ""
    Write-Host "[ERROR] Missing required directories. Aborting build." -ForegroundColor Red
    exit 1
}

# Define compilation flags
Write-Host ""
Write-Host "Starting compilation..." -ForegroundColor Green
Write-Host "This may take a few minutes..." -ForegroundColor Yellow
Write-Host ""

# Execute emcc directly with arguments
$startTime = Get-Date

# Build argument array properly for native execution
$emccArgs = @(
    $sources
    "-o"
    "mycraft.js"
    "-s"
    "USE_WEBGL2=1"
    "-s"
    "FULL_ES3=1"
    "-s"
    "USE_GLFW=3"
    "-s"
    "ALLOW_MEMORY_GROWTH=1"
    "-s"
    "INITIAL_MEMORY=268435456"
    "--preload-file"
    "shaders/web@/shaders/web"
    "--preload-file"
    "textures@/textures"
    "--preload-file"
    "libraries/include/mygui/misc/fonts@/libraries/include/mygui/misc/fonts"
    "-O3"
    "-I"
    "./libraries/include"
    "-I"
    "./libraries/include/mygui"
    "-I"
    "./libraries/include/glm"
    "-I"
    "./libraries/include/stb"
    "-std=c++14"
    "-sEXPORTED_FUNCTIONS=_main"
    "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap"
)

& emcc.bat @emccArgs

$endTime = Get-Date
$duration = ($endTime - $startTime).TotalSeconds

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan

if ($LASTEXITCODE -eq 0) {
    Write-Host "  BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Compilation time: $([math]::Round($duration, 2)) seconds" -ForegroundColor Yellow
    
    Write-Host ""
    Write-Host "Generated files:" -ForegroundColor Cyan
    
    if (Test-Path "mycraft.js") {
        $size = (Get-Item "mycraft.js").Length / 1MB
        Write-Host "  OK mycraft.js     $([math]::Round($size, 2)) MB" -ForegroundColor Green
    }
    
    if (Test-Path "mycraft.wasm") {
        $size = (Get-Item "mycraft.wasm").Length / 1MB
        Write-Host "  OK mycraft.wasm   $([math]::Round($size, 2)) MB" -ForegroundColor Green
    }
    
    if (Test-Path "mycraft.data") {
        $size = (Get-Item "mycraft.data").Length / 1MB
        Write-Host "  OK mycraft.data   $([math]::Round($size, 2)) MB" -ForegroundColor Green
    }
    
    Write-Host ""
    Write-Host "Next Steps:" -ForegroundColor Yellow
    Write-Host "  1. Test locally:" -ForegroundColor White
    Write-Host "     emrun --browser chrome mycraft.html" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  2. Deploy to Next.js:" -ForegroundColor White
    Write-Host "     Copy .js, .wasm, .data to your-nextjs-app/public/" -ForegroundColor Gray
    Write-Host ""
    
} else {
    Write-Host "  BUILD FAILED!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Exit code: $LASTEXITCODE" -ForegroundColor Red
    Write-Host ""
    Write-Host "Check the error messages above for details." -ForegroundColor Yellow
}
