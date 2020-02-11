# Automatic packaging for windows builds
param([string]$version)
[System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")

$base_dir = "..\..\..\..\"
$data_dir = "../data"
$project = "tuna"
$arch_both = "win32.64"
$arch_64 = "win64"
$msvc = "2017"
$qt = "5_10_1"
$build = "RelWithDebInfo"

$build_location_x32 = $base_dir + "build32\rundir\" + $build + "\obs-plugins\32bit"
$qtc_build_location_x32 = $base_dir + "build-obs-studio-Desktop_Qt_" + $qt + "_MSVC" + $msvc + "_32bit-" + $build +"\rundir\" + $build + "\obs-plugins\32bit"

$build_location_x64 = $base_dir + "build64\rundir\" + $build + "\obs-plugins\64bit"
$qtc_build_location_x64 = $base_dir + "build-obs-studio-Desktop_Qt_" + $qt + "_MSVC" + $msvc + "_64bit-" + $build + "\rundir\" + $build + "\obs-plugins\64bit"


$zip = "C:/Program Files/7-Zip/7z.exe"
Set-Alias sz $zip

function replace($file, $what, $with)
{
    ((Get-Content $file) -replace $what, $with) | Set-Content -Path $file
}

$Result = [System.Windows.Forms.MessageBox]::Show("Use Qt Creator builds?", "Build package script",3,[System.Windows.Forms.MessageBoxIcon]::Question) 

If ($result -eq "Yes") {
    echo("Using Qtc builds...")
    $build_location_x32 = $qtc_build_location_x32
    $build_location_x64 = $qtc_build_location_x64
} else {
    echo("Using VS builds...")
}

$Result = [System.Windows.Forms.MessageBox]::Show("64bit only build?", "Build package script",3,[System.Windows.Forms.MessageBoxIcon]::Question) 

If ($result -eq "Yes") {
    $x86 = $false
    $arch = $arch_64
    echo("64bit only build...")
} else {
    $x86 = $true
    $arch = $arch_both
    echo("Packaging 32/64bit...")
}

while ($version.length -lt 1) {
    
    $version = Read-Host -Prompt 'Enter a version string'
    $build_dir="./" + $project + ".v" + $version + "." + $arch
}
$build_dir = "./" + $project + ".v" + $version + "." + $arch

echo("Creating build directory")
New-Item $build_dir/plugin/data/obs-plugins/$project -itemtype directory
if ($x86) {
    New-Item $build_dir/plugin/obs-plugins/32bit -itemtype directory
}
New-Item $build_dir/plugin/obs-plugins/64bit -itemtype directory

if ($x86) {
    echo("Fetching build from $build_location_x32")
    Copy-Item $build_location_x32/$project.dll -Destination $build_dir/plugin/obs-plugins/32bit/
    Copy-Item $build_location_x32/$project.pdb -Destination $build_dir/plugin/obs-plugins/32bit/
}

echo("Fetching build from $build_location_x64")
Copy-Item $build_location_x64/$project.dll -Destination $build_dir/plugin/obs-plugins/64bit/
Copy-Item $build_location_x64/$project.pdb -Destination $build_dir/plugin/obs-plugins/64bit/

echo("Fetching data")
Copy-Item $data_dir/* -Destination $build_dir/plugin/data/obs-plugins/$project/ -Recurse
Copy-Item ../LICENSE -Destination $build_dir/LICENSE.txt
Copy-Item ./README.txt $build_dir/README.txt
replace $build_dir/README.txt "@VERSION" $version

echo("Making archive")
cd $build_dir
sz a -r "../$build_dir.zip" "./*"
cd ..

echo("Cleaning up")
Remove-Item $build_dir/ -Recurse
