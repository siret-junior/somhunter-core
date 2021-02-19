
# Config
$lib_URL=$args[0]
$install_dir=$args[1]

# Without this PS cmdlets won't parse it as an URL
$lib_URL=$lib_URL.Replace('+', '%2B') 
$lib_URL=$lib_URL.Replace('\', '/') 

Write-Host ""
Write-Host ">>> Installing libTorch >>>"

$res = 0
try {
    if (!(Test-Path "${install_dir}/libtorch.zip")) {
        Write-Host "Downloading the library from ${lib_URL}... " -ForegroundColor "Yellow"
        New-Item -Path ${install_dir} -ItemType Directory -Force > $null
        Start-BitsTransfer -Source "${lib_URL}" -Destination "${install_dir}\libtorch.zip"
    } else {
        Write-Host "Lib file found... " -ForegroundColor "Yellow"
    }

    if (!(Test-Path "${install_dir}/libtorch")) {
        Write-Host "Unzipping it into '${install_dir}'... " -ForegroundColor "Yellow"
        Expand-Archive "${install_dir}/libtorch.zip" -DestinationPath "${install_dir}" -Force
    } else {
        Write-Host "Library already installed. " -ForegroundColor "Yellow"
    }

    Write-Host "libTorch successfully installed." -ForegroundColor "Green"
} Catch {
    Write-Host "$Error[0]" -ForegroundColor "Red"
    Write-Host "Unable to fetch the `libTorch` library!" -BackgroundColor "Red" -ForegroundColor "White"

    # Set failed return code
    $res = 1
}

Write-Host "<<< Installing libTorch <<<"
Write-Host ""

exit $res