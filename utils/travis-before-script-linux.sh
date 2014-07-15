#!/bin/sh

sudo apt-add-repository -y ppa:ubuntu-wine/ppa
sudo apt-get update
sudo apt-get build-dep wine
