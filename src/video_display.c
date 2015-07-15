#include <stdint.h>
#include <SDL2/SDL.h>
#include <pthread.h>
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "video_display.h"

/*maximum size in bytes of a text to be displayed*/
#define TEXT_BUF_SIZE 100
#define PI 3.14159265


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

struct graphics_list
{
	int id;
	int type;
	int time_to_live;
	char* string;
	graphics_t* graphic;
	struct graphics_list* next;
};

static struct graphics_list* icon_registry = NULL;
static struct graphics_list* load_icon = NULL;
static struct graphics_list* video_graphic_create();
static void video_graphic_destroy(struct graphics_list* del);
static int load_element();
static int unload_element();

/** Race condition between rendering and list append*/
static pthread_mutex_t mutex_graphics_list = PTHREAD_MUTEX_INITIALIZER;

/*
* channels for data input and output.
* Input expects navdata in the following order :
* battery%, altitude, angles (3), speed (3)
*/
static jakopter_com_channel_t *com_in;
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
static int current_width = 0, current_height = 0;
/*
* Default size to use when the module is initialized without specifying it.
*/
//static int default_w=640, default_h=480;
/*
* Current pitch, roll, yaw and speed (unused for now) of the plane.
* These are kept updated via the input com channel.
*/
static float pitch = 0, roll = 0, yaw = 0;
static float speed = 0;

/*
* Check whether or not the display has been initialized.
*/
static int initialized = 0;
/* Last saved modification timestamp from the input channel */
static double prev_update = 0;

/******** TTF-related functions (for text) ********/
static TTF_Font* font;
static SDL_Color text_color = {0, 200, 0};
static int video_init_text(char* font_path);
static void video_clean_text();
static SDL_Texture* video_make_text(char* text, int* res_w, int* res_h);

/******** Icon-related functions (for img) ********/
static SDL_Texture* video_import_png(char* path, int* res_w, int* res_h);
/*
* Read the input channel to update the displayed informations.
*/
static void update_infos();

/******** Screenshot options ********/
/* Set to 1 if the user wants to capture a screenshot; then set back to 0 automatically */
static int want_screenshot = 0;
/* Total number of screenshots taken, used for screenshot filenames */
static int screenshot_nb = 0;
/* Base screenshot name. Final name = base name + screenshot_nb */
static char* screenshot_baseName = "screen_";
/*
* Take a screenshot, store it in a file named according to the total screenshot count.
*/
static void take_screenshot(uint8_t* frame, int size);

/******** Horizon indicator overlay options ********/
/* Position on the screen */
int horiz_posx, horiz_posy;
/* Length of the horizon */
int horiz_size;
/* Scale of the pitch indicator in pixels/degrees */
float horiz_pitchScale = 1;
/* Draw the drone's attitude indicator */
static void draw_attitude_indic();

/******** Compass overlay options ********/
/* position of the compass */
int compass_x = 0, compass_y = 0;
/* width and height of the compass in pixels */
int compass_w = 200, compass_h = 50;
/* number of vertical bars visible on the compass */
int compass_nbBars = 8;
/* scale of the compass in piels/degrees (how fast the compass spins) */
int compass_scale = 1;
/* Draw the drone's compass */
static void draw_compass();

/*
* Simple point rotation function. Angle in degrees.
*/
static void rotate_point(SDL_Point* point, const SDL_Point* center, float angle);

/**
* Initialize SDL, create the window and the renderer
* to get ready to draw frames.
* @param w
* @param h width and height with which to create the window.
* @return 0 on success, -1 on error.
*/
static int video_display_init_size(int width, int height) {

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "[~][display] Cannot initializing SDL : %s\n", SDL_GetError());
		return -1;
	}
	//create a window of the given size, without options. Make it centered.
	win = SDL_CreateWindow("Drone video",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
	if (win == NULL) {
		fprintf(stderr, "[~][display] Cannot create window : %s\n", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		fprintf(stderr, "[~][display] Cannot create renderer : %s\n", SDL_GetError());
		return -1;
	}

	//set the overlay elements to null so that they don't get drawn
	int i = 0;
	for (i = 0 ; i < VIDEO_NB_NAV_INFOS ; i++)
		graphs[i].tex = NULL;

	//initialize SDL_ttf for font rendering
	if (video_init_text(FONT_PATH) == -1)
		return -1;

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		fprintf(stderr, "[~][display] IMG_Init failed : %s\n", IMG_GetError());
		return -1;
	}

	pitch = 0;
	roll = 0;
	SDL_SetRenderDrawColor(renderer, 0, 250, 0, 255);

	return 0;
}

static int video_init_text(char* font_path)
{
	if (TTF_Init() == -1) {
		fprintf(stderr, "[~][display] TTF_Init failed : %s\n", TTF_GetError());
		return -1;
	}
	font = TTF_OpenFont(font_path, 16);
	if (!font) {
		fprintf(stderr, "[~][display] Couldn't load font : %s\n", TTF_GetError());
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
	if (frameTex == NULL) {
		fprintf(stderr, "[~][display] Failed to create frame texture : %s\n", SDL_GetError());
		return -1;
	}
	current_width = w;
	current_height = h;
	//recalculate the attitude indicator's and the compass' position
	horiz_size = 200;
	horiz_posx = w/2 - horiz_size/2;
	horiz_posy = h - horiz_pitchScale*180;
	compass_x = horiz_posx;
	compass_y = h - compass_h*1.5;
	return 0;
}


int video_display_init()
{
	com_in = jakopter_com_add_channel(CHANNEL_DISPLAY, DISPLAY_COM_IN_SIZE);
	if (com_in == NULL) {
		fprintf(stderr, "[~][Display] Couldn't create com channel.\n");
		return -1;
	}
	prev_update = 0;

	return 0;
}

struct graphics_list* video_graphic_create(int type)
{
	struct graphics_list* l = (struct graphics_list *)malloc(sizeof(struct graphics_list));
	l->id = 0;
	l->type = type;
	l->time_to_live = -1;
	l->string = NULL;
	l->graphic = NULL;
	l->next = NULL;

	return l;
}

void video_graphic_destroy(struct graphics_list* del)
{
	if (del != NULL && del->graphic != NULL) {
		if (del->string != NULL) {
			free(del->string);
			del->string = NULL;
		}
		if (del->graphic->tex != NULL) {
			SDL_DestroyTexture(del->graphic->tex);
			del->graphic->tex = NULL;
		}
		free(del->graphic);
		del->graphic = NULL;
	}
	if (del != NULL) {
		free(del);
		del = NULL;
	}
}

/**
* Clean the display context : close the window and clean SDL structures.
*/
void video_display_destroy() {
	//no need to clean stuff if it hasn't been initialized.
	if (!initialized)
		return;

	int i = 0;
	initialized = 0;

	SDL_DestroyTexture(frameTex);
	for (i = 0 ; i < VIDEO_NB_NAV_INFOS ; i++)
		if (graphs[i].tex != NULL)
			SDL_DestroyTexture(graphs[i].tex);

	//Free graphical elements list
	pthread_mutex_lock(&mutex_graphics_list);
	struct graphics_list *current = icon_registry;
	while (current != NULL) {
		struct graphics_list *del = current;
		current = current->next;
		video_graphic_destroy(del);
	}
	icon_registry = NULL;
	pthread_mutex_unlock(&mutex_graphics_list);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);
	video_clean_text();
	IMG_Quit();
	SDL_Quit();
	jakopter_com_remove_channel(CHANNEL_DISPLAY);
}

static void video_clean_text()
{
	TTF_CloseFont(font);
	TTF_Quit();
}

int video_display_process(uint8_t* frame, int width, int height, int size) {

	//if we get a NULL frame, stop displaying.
	if(frame == NULL) {
		video_display_destroy();
		return 0;
	}

	//first time called ? Initialize things.
	if(!initialized) {
		if(video_display_init_size(width, height) < 0) {
			fprintf(stderr, "[~][Display] Failed initialization.\n");
			return -1;
		}
		if(video_display_set_size(width, height) < 0)
			return -1;
		initialized = 1;
	}

	//check whether the size of the video has changed
	if (width != current_width || height != current_height)
		if (video_display_set_size(width, height) < 0)
			return -1;

	//check whether there's new stuff in the input com buffer
	double new_update = jakopter_com_get_timestamp(com_in);
	if (new_update > prev_update) {
		update_infos();
		if (want_screenshot) {
			take_screenshot(frame, size);
			want_screenshot = 0;
			jakopter_com_write_int(com_in, 24, 0);
		}
		prev_update = new_update;
	}

	unload_element();

	if (load_element() < 0)
		fprintf(stderr, "Couldn't load graphic element\n");

	// update the texture with our new frame
	if (SDL_UpdateTexture(frameTex, NULL, frame, width) < 0) {
		fprintf(stderr, "[~][Display] Failed to update frame texture : %s\n", SDL_GetError());
		return -1;
	}

	//clear the renderer, then update it so that we get the new frame displayed.
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, frameTex, NULL, NULL);
	//SDL_RenderFillRect(renderer, &rectangle);
	//draw all overlay elements, when they exist
	int i = 0;
	for (i = 0 ; i < VIDEO_NB_NAV_INFOS ; i++)
		if (graphs[i].tex != NULL)
			SDL_RenderCopy(renderer, graphs[i].tex, NULL, &graphs[i].pos);

	pthread_mutex_lock(&mutex_graphics_list);
	struct graphics_list *current = icon_registry;
	while (current != NULL) {
		if (current->graphic != NULL && current->graphic->tex != NULL){
			int ret = SDL_RenderCopy(renderer, current->graphic->tex, NULL, &current->graphic->pos);
			if (ret < 0)
				fprintf(stderr, "[~][display] RenderCopy() failed: %s\n", SDL_GetError());
		}
		// else
		// 	fprintf(stderr, "[*][display] Texture of %d not loaded\n", current->id);

		current = current->next;
	}
	pthread_mutex_unlock(&mutex_graphics_list);

	draw_attitude_indic();
	draw_compass();
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

SDL_Texture* video_import_png(char* path, int* res_w, int* res_h)
{
	SDL_Texture *img_tex = NULL;
	if (path == NULL) {
		fprintf(stderr, "[~][display] Path not defined\n");
		return NULL;
	}
	//Open in read mode, 'b' indicates it's a binary file
	SDL_RWops *rwop = SDL_RWFromFile(path, "rb");
	if (rwop == NULL) {
		fprintf(stderr, "[~][display] Open file failed: %s\n", SDL_GetError());
		return NULL;
	}
	SDL_Surface *img_surf = IMG_LoadPNG_RW(rwop);
	if (img_surf == NULL) {
		fprintf(stderr, "[~][display] Load PNG: %s\n", IMG_GetError());
		SDL_RWclose(rwop);
		return NULL;
	}

	img_tex = SDL_CreateTextureFromSurface(renderer, img_surf);
	if (img_tex  == NULL) {
		fprintf(stderr, "[~][display] Create Texture failed: %s\n", SDL_GetError());
	}

	*res_w = img_surf->w;
	*res_h = img_surf->h;

	SDL_FreeSurface(img_surf);
	SDL_RWclose(rwop);
	return img_tex;
}

/* With SDL, we can't create or destroy texture from a different thread than the rendering one */
int load_element()
{
	pthread_mutex_lock(&mutex_graphics_list);

	if (icon_registry == NULL) {
		pthread_mutex_unlock(&mutex_graphics_list);
		return 0;
	}

	if (load_icon == NULL)
		load_icon = icon_registry;

	//find the first that isn't loaded yet
	while (load_icon->next != NULL && load_icon->string == NULL)
		load_icon = load_icon->next;
	//nothing found
	if (load_icon->string == NULL) {
		pthread_mutex_unlock(&mutex_graphics_list);
		return 0;
	}

	if (load_icon->graphic == NULL) {
		fprintf(stderr, "[~][display] Graphic element isn't initialized\n");
		pthread_mutex_unlock(&mutex_graphics_list);
		return -1;
	}

	load_icon->graphic->tex = NULL;
	char* string = load_icon->string;

	if (load_icon->type == VIDEO_TEXT) {
		graphics_t* graph = load_icon->graphic;
		graph->tex = video_make_text(string, &graph->pos.w, &graph->pos.h);

		if (graph->pos.x >= current_width-(graph->pos.w)
			|| graph->pos.y >= current_height-(graph->pos.h)) {
			fprintf(stderr, "[~][display] Position of \"%s\" doesn't fit the bounds of the window\n", string);
			pthread_mutex_unlock(&mutex_graphics_list);
			display_graphic_remove(load_icon->id);
			return -1;
		}
	}
	else if (load_icon->type == VIDEO_ICON) {
		graphics_t* graph = load_icon->graphic;
		int width = 0;
		int height = 0;

		SDL_Texture *icon = video_import_png(string, &width, &height);
		if (icon == NULL) {
			fprintf(stderr, "[~][display] Couldn't import the image %s\n", string);
			pthread_mutex_unlock(&mutex_graphics_list);
			display_graphic_remove(load_icon->id);
			return -1;
		}
		graph->tex = icon;

		if (graph->pos.w == 0 || graph->pos.h == 0) {
			graph->pos.w = width;
			graph->pos.h = height;
		}

		if (graph->pos.x >= current_width-(graph->pos.w)
			|| graph->pos.y >= current_height-(graph->pos.h)) {
			fprintf(stderr, "[~][display] Position of %s doesn't fit the bounds of the window\n", string);
			pthread_mutex_unlock(&mutex_graphics_list);
			display_graphic_remove(load_icon->id);
			return -1;
		}
	}

	//String used for load, indicates now the item is loaded
	load_icon->string = NULL;
	free(string);

	pthread_mutex_unlock(&mutex_graphics_list);

	return 0;
}

static int unload_element()
{
	pthread_mutex_lock(&mutex_graphics_list);
	struct graphics_list* current = icon_registry;
	struct graphics_list* last    = NULL;

	if (current == NULL) {
		pthread_mutex_unlock(&mutex_graphics_list);
		return 0;
	}

	while (current != NULL) {
		if (current->time_to_live == 0) {
			if (last == NULL)
				icon_registry = current->next;
			else
				last->next = current->next;

			video_graphic_destroy(current);
			current = (last == NULL) ? icon_registry : last;
		}
		else if (current->time_to_live > 0) {
			current->time_to_live--;
		}
		last = current;
		current = current->next;
	}

	pthread_mutex_unlock(&mutex_graphics_list);
	return 0;
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
	int bat = jakopter_com_read_int(com_in, 0);
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

	int alt = jakopter_com_read_int(com_in, 4);
	snprintf(buf, TEXT_BUF_SIZE, "Altitude : %d", alt);
	buf[TEXT_BUF_SIZE-1] = '\0';
	graphs[1].tex = video_make_text(buf, &graphs[1].pos.w, &graphs[1].pos.h);
	graphs[1].pos.x = 0;
	graphs[1].pos.y = base_y;

	//update pitch, roll, yaw and speed
	pitch = jakopter_com_read_float(com_in, 8);
	roll  = jakopter_com_read_float(com_in, 12);
	yaw   = jakopter_com_read_float(com_in, 16);
	speed = jakopter_com_read_float(com_in, 20);
	//check if the user wants a screenshot
	want_screenshot = jakopter_com_read_int(com_in, 24);
}

static int graphic_append(struct graphics_list *item)
{
	int id = 1;
	pthread_mutex_lock(&mutex_graphics_list);
	if (icon_registry == NULL) {
		icon_registry = item;
		icon_registry->id = id;
	}
	else {
		struct graphics_list *current = icon_registry;
		while (current->next != NULL)
			current = current->next;
		current->next = item;
		id = current->next->id = current->id + 1;
	}

	pthread_mutex_unlock(&mutex_graphics_list);

	return id;
}

/** \return the icon id*/
int display_draw_icon(const char* path, int x, int y, int width, int height)
{
	struct graphics_list* item = video_graphic_create(VIDEO_ICON);
	int id = graphic_append(item);

	pthread_mutex_lock(&mutex_graphics_list);
	graphics_t* graph = malloc(sizeof(graphics_t));
	int size_str = strlen(path)+1;
	item->string = (char*)malloc(size_str*sizeof(char));
	strncpy(item->string, path, size_str);
	item->graphic = graph;
	item->graphic->tex = NULL;

	//user defined resolution
	if (width > 0 && height > 0) {
		if (width <= current_width && height <= current_height) {
			graph->pos.w = width;
			graph->pos.h = height;
		}
		else {
			fprintf(stderr, "[*][display] Width or height of %s greater than the window. Ignoring user values\n", path);
		}
	}

	if (x >= 0 && y >= 0 && x < current_width-(graph->pos.w) && y < current_height-(graph->pos.h)) {
		graph->pos.x = x;
		graph->pos.y = y;
	}
	else {
		fprintf(stderr, "[~][display] Position of %s doesn't fit the bounds of the window\n", path);
		pthread_mutex_unlock(&mutex_graphics_list);
		return -1;
	}
	pthread_mutex_unlock(&mutex_graphics_list);

	return id;
}

int display_draw_text(const char* string, int x, int y)
{
	struct graphics_list* item = video_graphic_create(VIDEO_TEXT);
	int id = graphic_append(item);

	pthread_mutex_lock(&mutex_graphics_list);
	graphics_t* graph = malloc(sizeof(graphics_t));
	int size_str = strlen(string)+1;
	item->string = (char*)malloc(size_str*sizeof(char));
	strncpy(item->string, string, size_str);
	item->graphic = graph;
	item->graphic->tex = NULL;

	if (x >= 0 && y >= 0) {
		graph->pos.x = x;
		graph->pos.y = y;
	}
	else {
		fprintf(stderr, "[~][display] Position of \"%s\" doesn't fit the bounds of the window\n", string);
		pthread_mutex_unlock(&mutex_graphics_list);
		return -1;
	}
	pthread_mutex_unlock(&mutex_graphics_list);

	return id;
}

void display_graphic_remove(int id)
{
	pthread_mutex_lock(&mutex_graphics_list);
	if (icon_registry == NULL) {
		fprintf(stderr, "[*][display] Not any icon referenced\n");
		pthread_mutex_unlock(&mutex_graphics_list);
		return;
	}

	struct graphics_list *current = icon_registry;

	while (current != NULL) {
		if (current->id == id) {
			current->time_to_live = DISPLAY_CYCLES;
			current = NULL;
		}
		else {
			if (current->next == NULL)
				fprintf(stderr, "[*][display] Icon %d not referenced\n", id);
			current = current->next;
		}
	}

	pthread_mutex_unlock(&mutex_graphics_list);
}

void display_graphic_resize(int id, int width, int height)
{
	if (width <= 0 || height <= 0 ||
		width > current_width || height > current_height) {
		fprintf(stderr, "[~][display] Width values don't fit the bounds of the window\n");
		return;
	}

	pthread_mutex_lock(&mutex_graphics_list);
	struct graphics_list *current = icon_registry;
	while (current != NULL) {
		if (current->id == id && current->graphic != NULL) {
			current->graphic->pos.w = width;
			current->graphic->pos.h = height;
			//Drop the pointer
			current = NULL;
		}
		else
			current = current->next;
	}
	pthread_mutex_unlock(&mutex_graphics_list);
}

void display_graphic_move(int id, int x, int y)
{
	if (!initialized)
		return;
	if (x < 0 || y < 0) {
		fprintf(stderr, "[~][display] Position values don't fit the bounds of the window\n");
		return;
	}

	pthread_mutex_lock(&mutex_graphics_list);
	struct graphics_list *current = icon_registry;
	if (current != NULL && current->graphic != NULL
		&& (x > current_width-(current->graphic->pos.w)
			|| y > current_height-(current->graphic->pos.h))) {

		fprintf(stderr, "[~][display] Position values don't fit the bounds of the window\n");
		return;
	}
	while (current != NULL) {
		if (current->id == id && current->graphic != NULL) {
			current->graphic->pos.x = x;
			current->graphic->pos.y = y;
			//Drop the pointer
			current = NULL;
		}
		else
			current = current->next;
	}
	pthread_mutex_unlock(&mutex_graphics_list);
}




/**
* \brief simple attitude indicator with the horizon represented by a straight line
* and the drone by a line with a center point.
*/
void draw_attitude_indic()
{
	int i = 0;
	//nose inclination = y offset from the horizon
	int nose_incl = (int)(horiz_pitchScale * pitch);
	//"center" of the drone, unaffected by roll
	SDL_Point center = {horiz_posx + horiz_size/2, horiz_posy-nose_incl};
	//series of points representing the drone on the indicator, affected by pitch
	SDL_Point drone_points[] = {
		{horiz_posx, center.y},
		{center.x-5, center.y},
		{center.x, center.y-5},
		{center.x+5, center.y},
		{horiz_posx+horiz_size, center.y}
	};
	int nb_points = sizeof(drone_points)/sizeof(SDL_Point);
	//apply roll to the points
	for (i = 0; i < nb_points; i++)
		rotate_point(&drone_points[i], &center, roll);
	//1. draw the horizon
	SDL_RenderDrawLine(renderer, horiz_posx, horiz_posy, horiz_posx+horiz_size, horiz_posy);
	//2. draw the drone's "flight line"
	SDL_RenderDrawLines(renderer, drone_points, nb_points);
}

/*draw every vertical bar of the compass.
Their position reflects the drone's yaw value.*/
void draw_compass()
{
	int i = 0;
	int bar_interval = compass_w/compass_nbBars;
	//yaw displacement in pixels.
	int offset = ((int)ceilf(yaw*compass_scale)) % bar_interval;
	//consider negative offsets (=substraction from the interval)
	offset = (bar_interval+offset)%bar_interval;
	//compute the position of each bar, and render them
	for (i = 0; i < compass_nbBars; i++) {
		int x = compass_x + offset + i*bar_interval;
		SDL_RenderDrawLine(renderer, x, compass_y, x, compass_y+compass_h);
	}
}

void rotate_point(SDL_Point* point, const SDL_Point* center, float angle)
{
	//convert the angle to radians for use with C math functions.
	double a_rad = angle * PI/180.;
	double a_cos = cos(a_rad), a_sin = sin(a_rad);
	//translate to origin before rotating
	point->x -= center->x;
	point->y -= center->y;
	//compute the rotation
	int newx = point->x*a_cos - point->y*a_sin;
	int newy = point->x*a_sin + point->y*a_cos;
	//translate back to position
	point->x = newx + center->x;
	point->y = newy + center->y;
}

void take_screenshot(uint8_t* frame, int size)
{
	//get the final filename length (+1 for the \0)
	int name_length = snprintf(NULL, 0, "%s%d.yuv", screenshot_baseName, screenshot_nb) + 1;
	char* filename = (char*)malloc(sizeof(char)*name_length);
	if (filename == NULL) {
		fprintf(stderr, "Display : couldn't allocate memory for screenshot filename\n");
		return;
	}
	snprintf(filename, name_length, "%s%d.yuv", screenshot_baseName, screenshot_nb);
	//dump the frame to a new file, don't do any conversion for now.
	FILE* f = fopen(filename, "w");
	if (f == NULL) {
		fprintf(stderr, "Display : couldn't open file %s for writing\n", filename);
		free(filename);
		return;
	}
	fwrite(frame, sizeof(uint8_t), size/sizeof(uint8_t), f);

	fclose(f);
	printf("Display : screenshot taken, saved to %s\n", filename);
	free(filename);
	screenshot_nb++;
}

