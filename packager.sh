#!/bin/dash
#

set -e

NAME="$1"
STRIP_TOOL="$3"

PKGROOT="package"
SYSROOT="$PKGROOT/files"
BIN_DEST="$SYSROOT/usr/bin"
BUILD_BIN="build/$NAME"


printf "%s" "--> Cleaning old package source..."

rm -rf "$PKGROOT"
mkdir -p "$SYSROOT"

printf "DONE\n"



printf "%s" "--> Stripping the binary..."

"$STRIP_TOOL" "$BUILD_BIN"

printf "DONE\n"



echo "%s" "--> Copying the binary..."

mkdir -p "$BIN_DEST"
cp "$BUILD_BIN" "$BIN_DEST/$NAME"

printf "DONE\n"



printf "%s" "--> Creating package structure..."

echo "Name:$NAME" > "$PKGROOT/metadata.conf"

echo "" > "$PKGROOT/preinstall" 
echo "" > "$PKGROOT/postinstall"

chmod +x "$PKGROOT/preinstall" "$PKGROOT/postinstall"

printf "DONE\n"



printf "%s" "--> Creating package archive..."

tar --zstd -cf "$NAME.tar.zst" "$PKGROOT/"

printf "DONE\n"
