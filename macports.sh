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

if [ ! -d "$MACPORT_PREFIX/include/SDL2" && ! -e "$MACPORT_PREFIX/include/SDL2/SDL.h" ]; then
	sudo port install libsdl2
fi

if [ ! -d "$MACPORT_PREFIX/include/SDL2" && ! -e "$MACPORT_PREFIX/include/SDL2/SDL_ttf.h" ]; then
	sudo port install libsdl2_ttf
fi

if [ ! -d "$MACPORT_PREFIX/include/SDL2" && ! -e "$MACPORT_PREFIX/include/SDL2/SDL_image.h" ]; then
	sudo port install libsdl2_image
fi

cd `port dir ffmpeg`
sudo curl -O https://trac.macports.org/raw-attachment/ticket/43100/patch-sdl-variant.diff
sudo patch -p0 < patch-sdl-variant.diff

if [ -z `which ffmpeg` ]; then
	sudo port install ffmpeg -sdl
fi

if [ ! -e $MACPORT_PREFIX/include/curses.h ] ; then
	sudo port install ncurses
fi
