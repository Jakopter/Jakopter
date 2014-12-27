#include <SDL2/SDL.h>
#include <stdio.h>

/*
* The SDL window where the video is displayed.
*/
static SDL_Window* win = NULL;
/*
* SDL renderer attached to our window
*/
static SDL_Renderer* renderer = NULL;
/*
* Texture that holds the current frame to display. It's the same size as the window.
*/
static SDL_Texture* frameTex = NULL;
/*
* Current size of the window and the frame texture.
* Mainly used to check whether it's changed.
*/
static int current_width, current_height;
/*
* Check whether or not the display has been initialized.
*/
static int initialized = 0;


/*
* Initialize SDL, create the window and the renderer
* to get ready to draw frames.
* @param w
* @param h width and height with which to create the window.
* @return 0 on success, -1 on error.
*/
static int video_display_init(int width, int height) {
	
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Display : error initializing SDL : %s\n", SDL_GetError());
		return -1;
	}
	//create a window of the given size, without options. Make it centered.
	win = SDL_CreateWindow("Drone video",
		SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,height,0);
	if(win == NULL) {
		fprintf(stderr, "Display : error creating window : %s\n", SDL_GetError());
		return -1;
	}
	
	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	if(renderer == NULL) {
		fprintf(stderr, "Display : error creating renderer : %s\n", SDL_GetError());
		return -1;
	}
	
	return 0;
}

/*
* Set the size of the window and of the texture containing the video.
* The parameters become the current size.
*/
static int video_display_set_size(int w, int h) {
	SDL_SetWindowSize(win, w, h);
	//re-create the texture, with the new size
	SDL_DestroyTexture(frameTex);
	frameTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, w, h);
	if(frameTex == NULL) {
		fprintf(stderr, "Display : failed to create frame texture : %s\n", SDL_GetError());
		return -1;
	}
	current_width = w;
	current_height = h;
	return 0;
}

/*
* Clean the display context : close the window and clean SDL structures.
*/
static void video_display_clean() {
	//no need to clean stuff if it hasn't been initialized.
	if(initialized) {
		SDL_DestroyTexture(frameTex);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(win);
		SDL_Quit();
		initialized = 0;
	}
}

/*
* "Got frame" callback.
* Fills the texture with the given frame, and displays it on the window.
*/
int video_display_frame(uint8_t* frame, int width, int height, int size) {

	//if we get a NULL frame, stop displaying.
	if(frame == NULL) {
		video_display_clean();
		return 0;
	}

	//first time called ? Initialize things.
	if(!initialized) {
		if(video_display_init(width, height) < 0) {
			fprintf(stderr, "Display : Failed initialization.\n");
			return -1;
		}
		if(video_display_set_size(width, height) < 0)
			return -1;
		
		initialized = 1;
	}
	
	//check whether the size of the video has changed
	if(width != current_width || height != current_height)
		if(video_display_set_size(width, height) < 0)
			return -1;
	
	//update the texture with our new frame
	if(SDL_UpdateTexture(frameTex, NULL, frame, width) < 0) {
		fprintf(stderr, "Display : failed to update frame texture : %s\n", SDL_GetError());
		return -1;
	}
	
	//clear the renderer, then update it so that we get the new frame displayed.
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, frameTex, NULL, NULL);
	SDL_RenderPresent(renderer);
	
	return 0;
}
