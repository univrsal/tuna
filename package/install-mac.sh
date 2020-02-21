#!/bin/sh
project=tuna
echo "Checking for brew.."
if ! [ -x "$(command -v brew)" ]; then
	echo 'Error: brew is not installed!'
	echo 'Install brew from https://brew.sh'
	exit 1
fi

function check_pkg() {
	echo "Checking for $1..."
	if brew ls --versions $1 > /dev/null; then
		echo "$1 is already installed"
	else
		echo "Installing $1"
		brew install $1 
	fi
}

check_pkg libmpdclient
check_pkg taglib

echo "Uninstalling old version"
rm -rf "/Users/$USER/Library/Application Support/obs-studio/plugins/$project"
echo "Creating plugin folder"
mkdir -p "/Users/$USER/Library/Application Support/obs-studio/plugins/$project"
echo "Moving plugin over"
mv plugin/* "/Users/$USER/Library/Application Support/obs-studio/plugins/$project"
echo "Done!"
