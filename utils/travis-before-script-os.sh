#!/bin/sh

brew list
brew info libpng
brew info freetype
brew remove libpng jpeg libxml2 libtiff libtool
brew install --universal freetype jpeg libtiff little-cms2 sane-backends libgphoto2 libgsm
brew install gettext
brew link gettext --force

