#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// Constants
const char * ARG_KEY_DIR_ABS_RES = "-abs_res_dir";

// Helpers - Arguments
const char * help_args_key_value_first(int argc, char * argv[], const char * key)
{
   if (NULL == argv || NULL == key) return NULL;

   for (int i_key = 0; i_key < argc; ++i_key)
   {
      const char * ARG_KEY = argv[i_key];
      if (strcmp(ARG_KEY, key) == 0)
      {
         // Found matching key-value pair
         const char * ARG_VALUE = argv[i_key + 1];
         return ARG_VALUE;
      }
   }

   // No match found
   return NULL;
}

// Helpers - Colors
typedef uint32_t color_rgba_t;

color_rgba_t color_rgba_make_rgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
   color_rgba_t color = 0x00;

   color |= red;
   color <<= 8;

   color |= green;
   color <<= 8;

   color |= blue;
   color <<= 8;

   color |= alpha;

   return color;
}

// Helpers - Vectors
struct vec_2i_s {
   int x;
   int y;
};

struct vec_2i_s vec_2i_make_xy(int x, int y)
{
   struct vec_2i_s v;

   v.x = x;
   v.y = y;

   return v;
}

struct vec_2i_s vec_2i_make_from_scaled(struct vec_2i_s v, int scale)
{
   return vec_2i_make_xy(
      v.x * scale,
      v.y * scale
   );
}

// Helpers - SDL
struct vec_2i_s help_sdl_window_size(SDL_Window * sdl_window)
{
   int width, height;
   if (SDL_GetWindowSize(sdl_window, &width, &height))
   {
      return vec_2i_make_xy(width, height);
   }
   else
   {
      return vec_2i_make_xy(0, 0);
   }
}

SDL_FRect help_sdl_f_rect_make(float x, float y, float width, float height)
{
   SDL_FRect frect;

   frect.x = x;
   frect.y = y;
   frect.w = width;
   frect.h = height;

   return frect;
}

// Helpers - Min Max
int help_minmax_min_2i(int a, int b)
{
   return (a < b) ? a : b;
}

int help_minmax_max_2i(int a, int b)
{
   return (a > b) ? a : b;
}

// Helpers - Virtual
int help_virtual_max_render_scale(struct vec_2i_s actual_window_size, struct vec_2i_s virtual_window_size)
{
   const int MAX_RENDER_SCALE_HORIZONTAL = actual_window_size.x / virtual_window_size.x;
   const int MAX_RENDER_SCALE_VERTICAL = actual_window_size.y / virtual_window_size.y;
   return help_minmax_min_2i(MAX_RENDER_SCALE_HORIZONTAL, MAX_RENDER_SCALE_VERTICAL);
}

SDL_FRect help_virtual_max_render_scale_region(struct vec_2i_s actual_window_size, struct vec_2i_s virtual_window_size)
{
   const int MAX_RENDER_SCALE = help_virtual_max_render_scale(actual_window_size, virtual_window_size);
   const struct vec_2i_s VIRTUAL_WINDOW_SIZE_SCALED = vec_2i_make_from_scaled(virtual_window_size, MAX_RENDER_SCALE);
   return help_sdl_f_rect_make(
      (actual_window_size.x - VIRTUAL_WINDOW_SIZE_SCALED.x) * 0.5f,
      (actual_window_size.y - VIRTUAL_WINDOW_SIZE_SCALED.y) * 0.5f,
      VIRTUAL_WINDOW_SIZE_SCALED.x,
      VIRTUAL_WINDOW_SIZE_SCALED.y
   );
}

// Helpers - Texture
struct texture_rgba_s {
   color_rgba_t * texels;
   int width;
   int height;
};

int help_texture_rgba_texel_count(struct texture_rgba_s * instance)
{
   return instance ? (instance->width * instance->height) : 0;
}

struct vec_2i_s help_texture_rgba_size(struct texture_rgba_s * instance)
{
   return instance ? vec_2i_make_xy(instance->width, instance->height) : vec_2i_make_xy(0, 0);
}

void * help_texture_rgba_destroy(struct texture_rgba_s * instance)
{
   if (instance)
   {
      free(instance->texels);
   }

   free(instance);

   return NULL;
}

bool help_texture_rgba_clear(struct texture_rgba_s * instance, color_rgba_t clear_color)
{
   if (NULL == instance) return false;

   for (int i = 0; i < help_texture_rgba_texel_count(instance); ++i)
   {
      instance->texels[i] = clear_color;
   }

   return true;
}

bool help_texture_rgba_coords_out_of_bounds(struct texture_rgba_s * instance, int x, int y)
{
   return (
      NULL == instance ||
      x < 0 ||
      x >= instance->width ||
      y < 0 ||
      y >= instance->height
   ) ? true : false;
}

bool help_texture_rgba_2d_coords_to_linear(struct texture_rgba_s * instance, int x, int y, int * out_linear)
{
   if (
      NULL == instance ||
      NULL == out_linear ||
      help_texture_rgba_coords_out_of_bounds(instance, x, y)
   ) return false;

   *out_linear = (y * instance->width) + x;
   return true;
}

bool help_texture_rgba_plot_pixel(struct texture_rgba_s * instance, int x, int y, color_rgba_t color)
{
   int texel_index;
   if (help_texture_rgba_2d_coords_to_linear(instance, x, y, &texel_index))
   {
      instance->texels[texel_index] = color;
      return true;
   }

   return false;
}

struct texture_rgba_s * help_texture_rgba_make(int width, int height, color_rgba_t clear_color)
{
   // Allocate instance
   struct texture_rgba_s * instance = malloc(sizeof(struct texture_rgba_s));
   if (NULL == instance)
   {
      printf("\nFailed to create texture rgba instance");
      return NULL;
   }

   // Null instance
   instance->texels = NULL;
   instance->width = width;
   instance->height = height;

   // Allocate texels
   const int TEXEL_COUNT = width * height;
   instance->texels = malloc(sizeof(color_rgba_t) * TEXEL_COUNT);
   if (NULL == instance->texels)
   {
      printf("\nFailed to allocate texture rgba for [%s] texels", TEXEL_COUNT);
      return help_texture_rgba_destroy(instance);
   }

   // Initial clear
   help_texture_rgba_clear(instance, clear_color);

   // Success
   return instance;
}

/*
   > render_sprite(x, y, type, buffer, virtual dimensions)
      - tiled and non-tiled ?
      - various sizes at 8x8 and higher dimensions ?

   > virtual render target
      - virtual size
      - rgba buffer

   > rendering
      render_sprite_into(vrt, x, y, type)

   > What is a sprite ?
      - specific texture -> texels
      - sprite dimensions
      - type ?

   > What we need to render
      - sprites (size, position, texture)
      - font (scale, position,)

   > Attributes required
      - no transparency 
*/

// Logic - Main
int main(int argc, char * argv[])
{
   // Expect absolute executable resource directory
   const char * DIR_ABS_RES = help_args_key_value_first(argc, argv, ARG_KEY_DIR_ABS_RES);
   if (NULL == DIR_ABS_RES)
   {
      printf("\nAbsolute executable resource directory required (Use argument flag '%s' followed by resource directory)", ARG_KEY_DIR_ABS_RES);
      return EXIT_FAILURE;
   }

   // Initialize SDL
   if (!SDL_Init(SDL_INIT_VIDEO))
   {
      printf("\nFailed to initialize SDL - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }

   // Create SDL window
   SDL_Window * sdl_window = SDL_CreateWindow("Tetris", 0, 0, SDL_WINDOW_FULLSCREEN);
   if (NULL == sdl_window)
   {
      printf("\nFailed to create SDL window - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }

   // Create SDL renderer
   SDL_Renderer * sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
   if (NULL == sdl_renderer)
   {
      printf("\nFailed to create SDL renderer - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }

   // Enable renderer VSYNC
   const bool SUCCESS_USE_VSYNC = SDL_SetRenderVSync(sdl_renderer, 1);

   // Create offline rendering resources
   struct texture_rgba_s * tex_virtual = help_texture_rgba_make(160, 144, color_rgba_make_rgba(0x00, 0x00, 0x00, 0xFF));
   if (NULL == tex_virtual)
   {
      printf("\nFailed to create virtual texture");
      return EXIT_FAILURE;
   }
   const struct vec_2i_s VIRTUAL_SIZE = help_texture_rgba_size(tex_virtual);

   // Create online rendering texture
   SDL_Texture * sdl_texture_online = SDL_CreateTexture(
      sdl_renderer,
      SDL_PIXELFORMAT_RGBA8888,
      SDL_TEXTUREACCESS_STREAMING,
      VIRTUAL_SIZE.x,
      VIRTUAL_SIZE.y
   );
   if (NULL == sdl_texture_online)
   {
      printf("\nFailed to create sdl online texture - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }

   // Set online texture filtering
   const bool SUCCESS_TEXTURE_NEAREST = SDL_SetTextureScaleMode(sdl_texture_online, SDL_SCALEMODE_NEAREST);
   if (!SUCCESS_TEXTURE_NEAREST)
   {
      printf("\nFailed to set online texture filtering to nearest - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }

   // Load required resources
   char dir_abs_res_images[1024];
   snprintf(dir_abs_res_images, sizeof(dir_abs_res_images), "%s\\images\\", DIR_ABS_RES);

   char dir_abs_res_img_tiles[1024];
   const char * TILES_IMAGE_FILE_NAME = "tiles.png";
   snprintf(dir_abs_res_img_tiles, sizeof(dir_abs_res_img_tiles), "%s\\%s", dir_abs_res_images, TILES_IMAGE_FILE_NAME);
   SDL_Surface * img_tiles = IMG_Load(dir_abs_res_img_tiles);
   if (NULL == img_tiles)
   {
      printf("\nFailed to load tiles image [%s] from resources [%s]", TILES_IMAGE_FILE_NAME, dir_abs_res_images);
      return EXIT_FAILURE;
   }

   // Log engine status
   const int DW = 20;
   printf("\n\nEngine Information");
   printf("\n\t%-*s: %s", DW, "resource directory", DIR_ABS_RES);
   printf("\n\t%-*s: %s", DW, "VSYNC", SUCCESS_USE_VSYNC ? "enabled" : "disabled");

   // FPS counter
   int frames_per_second = 0;
   uint64_t timer_fps_ns = SDL_GetTicksNS();
   const uint64_t NS_PER_S = 1000000000;

   // Game loop
   bool tetris_close_requested = false;
   while (false == tetris_close_requested)
   {
      // Consume window events
      SDL_Event window_event;
      while (SDL_PollEvent(&window_event))
      {
         if (SDL_EVENT_KEY_DOWN == window_event.type && SDL_SCANCODE_ESCAPE == window_event.key.scancode)
         {
            tetris_close_requested = true;
         }
      }

      // Render into offline texture
      // -> Clear
      help_texture_rgba_clear(tex_virtual, color_rgba_make_rgba(15, 15, 15, 255));
      // -> Render scene
      for (int y = 0; y < VIRTUAL_SIZE.y; ++y)
      {
         for (int x = 0; x < VIRTUAL_SIZE.x; ++x)
         {
            help_texture_rgba_plot_pixel(tex_virtual, x, y, color_rgba_make_rgba(rand() % 256, 0x00, 0x00, 0xFF));
         }
      }

      // Copy offline to online texture
      const bool SUCCESS_UPDATE_TEXTURE = SDL_UpdateTexture(
         sdl_texture_online,
         NULL,
         tex_virtual->texels,
         sizeof(color_rgba_t) * VIRTUAL_SIZE.x
      );

      // Clear backbuffer
      SDL_SetRenderDrawColor(sdl_renderer, 10, 10, 10, 0xFF);
      const bool SUCCESS_BACKBUFFER_CLEAR = SDL_RenderClear(sdl_renderer);
      if (!SUCCESS_BACKBUFFER_CLEAR)
      {
         printf("\nFailed to clear backbuffer - Error: %s", SDL_GetError());
         break;
      }

      // Render scaled virtual texture
      const SDL_FRect VIRTUAL_REGION = help_virtual_max_render_scale_region(help_sdl_window_size(sdl_window), VIRTUAL_SIZE);
      const bool SUCCESS_RENDER_TEXTURE = SDL_RenderTexture(
         sdl_renderer,
         sdl_texture_online,
         NULL,
         &VIRTUAL_REGION
      );
      if (!SUCCESS_RENDER_TEXTURE)
      {
         printf("\nFailed to render online texture - Error: %s", SDL_GetError());
         break;
      }

      // Swap buffers
      const bool SUCCESS_RENDER = SDL_RenderPresent(sdl_renderer);
      if (!SUCCESS_RENDER)
      {
         printf("\nFailed to render - Error: %s", SDL_GetError());
         break;
      }

      // Determine FPS
      if (SDL_GetTicksNS() >= (timer_fps_ns + NS_PER_S))
      {
         printf("\nFPS: %d", frames_per_second);

         // Reset FPS timer
         frames_per_second = 0;
         timer_fps_ns = SDL_GetTicksNS();
      }

      // Count frames executed
      ++frames_per_second;
   }

   // Cleanup custom
   help_texture_rgba_destroy(tex_virtual);

   // Cleanup SDL
   SDL_DestroyTexture(sdl_texture_online);
   SDL_DestroyWindow(sdl_window);
   SDL_DestroyRenderer(sdl_renderer);
   SDL_Quit();

   // Back to OS
   return EXIT_SUCCESS;
}