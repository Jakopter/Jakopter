#include <SDL2/SDL.h>
#include "SDL_ttf.h"

#include "navdata.h"
#include "video_display.h"
#include "com_master.h"

//maximum size in bytes of a text to be displayed
#define TEXT_BUF_SIZE 100


/**
* Structure that defines a graphical element that can be drawn on the screen.
*/
typedef struct graphics {
	SDL_Texture* tex;
	SDL_Rect pos;
} graphics_t;
/*
* List of all overlayed graphical elements. For now, navdata text only.
*/
graphics_t graphs[VIDEO_NB_NAV_INFOS];
/*
* channels for data input and output.
* Input expects navdata in the following order :
* battery%, altitude, angles (3), speed (3)
*/
static jakopter_com_channel_t *DISPLAY_COM_IN;
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
* Rectangle drawn in the middle of the screen
*/
static SDL_Rect rectangle;
/*
* Check whether or not the display has been initialized.
*/
static int initialized = 0;
/**
* TTF-related functions (for text)
*/
static TTF_Font* font;
static SDL_Color text_color = {0, 200, 0};
static int video_init_text(char* font_path);
static void video_clean_text();
static SDL_Texture* video_make_text(char* text, int* res_w, int* res_h);
//Last saved modification timestamp from the input channel
static double prev_update = 0;
/*
* Read the input channel to update the displayed informations.
*/
static void update_infos();

/**
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
	
	//create the communication channels
	if(!jakopter_com_master_is_init())
		jakopter_com_init_master(6);
	DISPLAY_COM_IN = jakopter_com_add_channel(DISPLAY_COM_IN_ID, DISPLAY_COM_IN_SIZE);
	
	//set the overlay elements to null so that they don't get drawn
	for(int i=0 ; i<VIDEO_NB_NAV_INFOS ; i++)
		graphs[i].tex = NULL;

	//initialize SDL_ttf for font rendering
	if(video_init_text(FONT_PATH) == -1)
		return -1;

	//a red rectangle will be drawn on the screen.
	rectangle.w = 50;
	rectangle.h = 50;
	SDL_SetRenderDrawColor(renderer, 0, 250, 0, 255);

	return 0;
}

static int video_init_text(char* font_path)
{
	if(TTF_Init() == -1) {
		fprintf(stderr, "Display : TTF_Init error : %s\n", TTF_GetError());
		return -1;
	}
	font = TTF_OpenFont(font_path, 16);
	if(!font) {
		fprintf(stderr, "Display : couldn't load font : %s\n", TTF_GetError());
		return -1;
	}
	return 0;
}

/**
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
	//we want the rectangle to be centered
	rectangle.x = (w/2) - (rectangle.w/2);
	rectangle.y = (h/2) - (rectangle.h/2);
	return 0;
}

/**
* Clean the display context : close the window and clean SDL structures.
*/
static void video_display_clean() {
	//no need to clean stuff if it hasn't been initialized.
	if(initialized) {
		SDL_DestroyTexture(frameTex);
		for(int i=0 ; i<VIDEO_NB_NAV_INFOS ; i++)
			if(graphs[i].tex != NULL)
				SDL_DestroyTexture(graphs[i].tex);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(win);
		video_clean_text();
		SDL_Quit();
		jakopter_com_remove_channel(DISPLAY_COM_IN_ID);
		initialized = 0;
	}
}

void video_clean_text()
{
	TTF_CloseFont(font);
	TTF_Quit();
}

/**
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
			
	//check whether there's new stuff in the input com buffer
	double new_update = jakopter_com_get_timestamp(DISPLAY_COM_IN);
	if(new_update > prev_update) {
		update_infos();
		prev_update = new_update;
	}

	//update the texture with our new frame
	if(SDL_UpdateTexture(frameTex, NULL, frame, width) < 0) {
		fprintf(stderr, "Display : failed to update frame texture : %s\n", SDL_GetError());
		return -1;
	}

	//clear the renderer, then update it so that we get the new frame displayed.
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, frameTex, NULL, NULL);
	//SDL_RenderFillRect(renderer, &rectangle);
	//draw all overlay elements, when they exist
	for(int i=0 ; i<VIDEO_NB_NAV_INFOS ; i++)
		if(graphs[i].tex != NULL)
			SDL_RenderCopy(renderer, graphs[i].tex, NULL, &graphs[i].pos);
	SDL_RenderPresent(renderer);

	return 0;
}

SDL_Texture* video_make_text(char* text, int* res_w, int* res_h)
{
	//create a surface with the given text
	SDL_Surface* text_surf = TTF_RenderUTF8_Blended(font, text, text_color);
	//then, make a texture from it so it can be used with our renderer
	SDL_Texture* text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
	//fill in the size infos
	*res_w = text_surf->w;
	*res_h = text_surf->h;
	SDL_FreeSurface(text_surf);
	return text_tex;
}

void update_infos()
{
	//base y position of the text
	int base_y = 0;
	//height of a line of text with the current font.
	int line_height = TTF_FontLineSkip(font);
	//buffer to hold the current textto be drawn
	char buf[TEXT_BUF_SIZE];
	
	//retrieve navdata one by one
	int bat = jakopter_com_read_int(DISPLAY_COM_IN, 0);
	//and format it for textual display
	snprintf(buf, TEXT_BUF_SIZE, "Battery : %d%%", bat);
	buf[TEXT_BUF_SIZE-1] = '\0';
	//finally, print it onto a texture using SDL_ttf
	graphs[0].tex = video_make_text(buf, &graphs[0].pos.w, &graphs[0].pos.h);
	//and set its draw position accordingly
	graphs[0].pos.x = 0;
	graphs[0].pos.y = base_y;
	//go to a new line
	base_y += line_height;
	
	int alt = jakopter_com_read_int(DISPLAY_COM_IN, 4);
	snprintf(buf, TEXT_BUF_SIZE, "Altitude : %d", alt);
	buf[TEXT_BUF_SIZE-1] = '\0';
	graphs[1].tex = video_make_text(buf, &graphs[1].pos.w, &graphs[1].pos.h);
	graphs[1].pos.x = 0;
	graphs[1].pos.y = base_y;
}

