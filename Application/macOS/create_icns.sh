#!/bin/bash
mkdir -p AppIcon.iconset
echo "Generating icon sizes..."
sips -z 16 16     Icon.png --out AppIcon.iconset/icon_16x16.png > /dev/null 2>&1
sips -z 32 32     Icon.png --out AppIcon.iconset/icon_16x16@2x.png > /dev/null 2>&1
sips -z 32 32     Icon.png --out AppIcon.iconset/icon_32x32.png > /dev/null 2>&1
sips -z 64 64     Icon.png --out AppIcon.iconset/icon_32x32@2x.png > /dev/null 2>&1
sips -z 128 128   Icon.png --out AppIcon.iconset/icon_128x128.png > /dev/null 2>&1
sips -z 256 256   Icon.png --out AppIcon.iconset/icon_128x128@2x.png > /dev/null 2>&1
sips -z 256 256   Icon.png --out AppIcon.iconset/icon_256x256.png > /dev/null 2>&1
sips -z 512 512   Icon.png --out AppIcon.iconset/icon_256x256@2x.png > /dev/null 2>&1
sips -z 512 512   Icon.png --out AppIcon.iconset/icon_512x512.png > /dev/null 2>&1
sips -z 1024 1024 Icon.png --out AppIcon.iconset/icon_512x512@2x.png > /dev/null 2>&1
echo "Creating .icns file..."
iconutil -c icns AppIcon.iconset
rm -rf AppIcon.iconset
echo "AppIcon.icns created successfully!"
