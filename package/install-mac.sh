#!/bin/sh
project_name=tuna
install_dir="/Users/$USER/Library/Application Support/obs-studio/plugins"

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
read -p "Deleting $install_dir/$project_name, is this ok? [y/N] " choice
echo
case "$choice" in
	y|Y )
	echo "Deleting..."
	rm -rf "$install_dir/$project_name";;
	* ) echo "Exiting"
	exit 1;;
esac

echo "Creating plugin folder"
mkdir -p "$install_dir"

echo "Moving plugin over"
mv "$project_name" "$install_dir/"
echo "Done!"
