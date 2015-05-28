#!/bin/bash
# Script to install all the dependances of Jakopter with Macports

MACPORT_PREFIX="/opt/local"

if [ $# -eq 2 ] ; then
	if [ $1 = "--help" || $1 = "-h" ] ; then
		echo "./macports.sh [macport install prefix]
		Default: /opt/local"
	else
		MACPORT_PREFIX=$1
	fi
fi

if [ -z `which port` ] ; then
	echo "Port isn't installed. Please follow instructions on macports.org"
	exit
fi

if [ -z `which pkg-config` ]; then
	sudo port install pkgconfig
fi

if [ -z `which cmake` ]; then
	sudo port install cmake
fi

if [ -z `which lua` ]; then
	sudo port install lua
fi

if [ -z `find $MACPORT_PREFIX/include/SDL2 -name "SDL.h"` ]; then
	sudo port install libsdl2
fi

if [ -z `find $MACPORT_PREFIX/include/SDL2 -name "SDL_ttf.h"` ]; then
	sudo port install libsdl2_ttf
fi

if [ -z `find $MACPORT_PREFIX/include/SDL2 -name "SDL_image.h"` ]; then
	sudo port install libsdl2_image
fi

cd `port dir ffmpeg`
sudo curl -O https://trac.macports.org/raw-attachment/ticket/43100/patch-sdl-variant.diff
sudo patch -p0 < patch-sdl-variant.diff

if [ -z `which ffmpeg` ]; then
	sudo port install ffmpeg -sdl
fi

if [ -z `find $MACPORT_PREFIX/include -name "curses.h"` ] ; then
	sudo port install ncurses
fi
