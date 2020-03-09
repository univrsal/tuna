tuna v1.4.1
This program is licensed under the GPL v2.0 See LICENSE.txt
github.com/univrsal/tuna

INSTALLATION

macOS:
------
Either run install-mac.sh or follow these steps:

0. Install libmpdclient and taglib over brew
   $ brew install taglib libmpdclient
1. Create a folder for the plugin:
   $ mkdir -p "/Users/$USER/Library/Application Support/obs-studio/plugins/tuna"
2. Copy over the folders "bin" and "data" from the folder "plugin" from this zip file:
   $ mv plugin/* "/Users/$USER/Library/Application Support/obs-studio/plugins/tuna"

Linux:
------
1. Create a plugin folder in your home directory:
  $ mkdir -p ~/.config/obs-studio/plugins/tuna
2. Extract the bolder bin and data into the newly created folder
  $ mv plugin/* ~/.config/obs-studio/plugins/tuna

Windows:
--------
1. Extract the archive
2. Move the contents of plugin into your obs installation directory
