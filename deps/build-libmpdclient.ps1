Set-Location "libmpdclient"

Copy-Item -Path "meson.build" -Destination "meson.build.bak" -Force

(Get-Content "meson.build") -replace 'libmpdclient = library', 'libmpdclient = static_library' |
    Set-Content "meson.build"

(Get-Content "meson.build") -replace 'soversion: splitted_version.*,$', '' |
    Set-Content "meson.build"

(Get-Content "meson.build") -replace 'version: meson.project_version\(\),', '' |
    Set-Content "meson.build"

$pwdPath = (Get-Location).Path
meson setup ./build . --prefix="$pwdPath\install"
meson compile -C ./build
meson install -C ./build
