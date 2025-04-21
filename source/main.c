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

uint8_t color_rgba_channel_red(color_rgba_t color)
{
   return (uint8_t)(color >> 24);
}

uint8_t color_rgba_channel_green(color_rgba_t color)
{
   return (uint8_t)(color >> 16);
}

uint8_t color_rgba_channel_blue(color_rgba_t color)
{
   return (uint8_t)(color >> 8);
}

uint8_t color_rgba_channel_alpha(color_rgba_t color)
{
   return (uint8_t)color;
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

bool help_texture_rgba_plot_texel(struct texture_rgba_s * instance, int x, int y, color_rgba_t color)
{
   int texel_index;
   if (help_texture_rgba_2d_coords_to_linear(instance, x, y, &texel_index))
   {
      instance->texels[texel_index] = color;
      return true;
   }

   return false;
}

bool help_texture_rgba_access_texel(struct texture_rgba_s * instance, int x, int y, color_rgba_t * out_color)
{
   if (NULL == instance || NULL == out_color) return false;

   int texel_index;
   if (help_texture_rgba_2d_coords_to_linear(instance, x, y, &texel_index))
   {
      *out_color = instance->texels[texel_index];
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

struct texture_rgba_s * help_texture_rgba_from_png(const char * dir_abs_file)
{
   // Load image to convert
   SDL_Surface * img_surface = IMG_Load(dir_abs_file);
   if (NULL == img_surface)
   {
      printf("\nFailed to load image [%s] to create texture rgba from", dir_abs_file);
      return NULL;
   }

   // Create texture rgba
   struct texture_rgba_s * img_texture = help_texture_rgba_make(
      img_surface->w,
      img_surface->h,
      color_rgba_make_rgba(0x00, 0x00, 0x00, 0xFF)
   );
   if (NULL == img_texture)
   {
      printf("\nFailed to create target texture for image [%s] conversion", dir_abs_file);
      SDL_DestroySurface(img_surface);
      return NULL;
   }

   // Copy image to texture
   for (int y = 0; y < img_surface->h; ++y)
   {
      for (int x = 0; x < img_surface->w; ++x)
      {
         // Get surface pixel color
         uint8_t surface_red, surface_green, surface_blue, surface_alpha;
         const bool SUCCESS_SURFACE_TEXEL_ACCESS = SDL_ReadSurfacePixel(
            img_surface,
            x,
            y,
            &surface_red,
            &surface_green,
            &surface_blue,
            &surface_alpha
         );
         color_rgba_t surface_color = color_rgba_make_rgba(0xFF, 0x00, 0xFF, 0xFF);
         if (SUCCESS_SURFACE_TEXEL_ACCESS)
         {
            surface_color = color_rgba_make_rgba(surface_red, surface_green, surface_blue, surface_alpha);
         }
         else
         {
            printf("\nFailed to access image [%s] texel [%-4d, %-4d] during conversion");
         }

         // Plot into texture
         help_texture_rgba_plot_texel(img_texture, x, y, surface_color);
      }
   }

   // Cleanup
   SDL_DestroySurface(img_surface);

   // Success
   return img_texture;
}

// Logic - Sprites
struct sprite_s {
   struct vec_2i_s texture_min;
   struct vec_2i_s texture_size;
};

struct sprite_s sprite_make(int tile_x, int tile_y, int tile_size)
{
   struct sprite_s sprite;

   sprite.texture_min = vec_2i_make_xy(tile_x * tile_size, tile_y * tile_size);
   sprite.texture_size = vec_2i_make_xy(tile_size, tile_size);

   return sprite;
}

// Logic - Textured Sprite Rendering
bool help_tex_sprite_render(struct sprite_s sprite, int x, int y, struct texture_rgba_s * texture_sprite, struct texture_rgba_s * texture_target)
{
   if (NULL == texture_sprite || NULL == texture_target) return false;

   // TODO-GS: Plot only what is stricly necessary
   for (int spr_y = 0; spr_y < sprite.texture_size.y; ++spr_y)
   {
      for (int spr_x = 0; spr_x < sprite.texture_size.x; ++spr_x)
      {
         // Access sprite source texture texel
         color_rgba_t source_texel_color;
         const bool SUCCESS_ACCESS_SOURCE_TEXEL = help_texture_rgba_access_texel(
            texture_sprite,
            sprite.texture_min.x + spr_x,
            sprite.texture_min.y + spr_y,
            &source_texel_color
         );
         const color_rgba_t SAFE_SOURCE_TEXEL_COLOR = SUCCESS_ACCESS_SOURCE_TEXEL ? source_texel_color : color_rgba_make_rgba(0xFF, 0x00, 0xFF, 0xFF);

         // Do not draw totally transparent texels for now
         // TODO-GS: Implement alpha blending ?
         if (0 == color_rgba_channel_alpha(SAFE_SOURCE_TEXEL_COLOR))
         {
            continue;
         }

         // Plot sprite source texture color into target texture
         const bool SUCCESS_PLOTTING_TEXEL = help_texture_rgba_plot_texel(
            texture_target,
            x + spr_x,
            y + spr_y,
            SAFE_SOURCE_TEXEL_COLOR
         );
      }
   }

   return true;
}

// Helpers - Tetro
#define TETRO_MAX_SIZE (4)
enum tetro_type_e {
   TETRO_TYPE_I,
   TETRO_TYPE_Lr,
   TETRO_TYPE_L,
   TETRO_TYPE_S,
   TETRO_TYPE_Z,
   TETRO_TYPE_T,
   TETRO_TYPE_O
};

typedef bool tetro_mask[TETRO_MAX_SIZE][TETRO_MAX_SIZE];

struct tetro_s {
   enum tetro_type_e type;
   int size;
   tetro_mask design;
   tetro_mask left;
   tetro_mask right;
};

struct tetro_world_s {
   struct tetro_s data;
   struct vec_2i_s position;
};

struct tetro_world_s tetro_world_make(struct tetro_s tetro, int x, int y)
{
   struct tetro_world_s tetro_world;

   tetro_world.data = tetro;
   tetro_world.position = vec_2i_make_xy(x, y);

   return tetro_world;
}

bool help_tetro_init_mask(tetro_mask out_mask)
{
   if (NULL == out_mask) return false;

   for (int py = 0; py < TETRO_MAX_SIZE; ++py)
   {
      for (int px = 0; px < TETRO_MAX_SIZE; ++px)
      {
         out_mask[px][py] = false;
      }
   }

   return true;
}

struct tetro_s help_tetro_make_typed_unfinished(enum tetro_type_e type, int size)
{
   struct tetro_s tetro;

   tetro.type = type;
   tetro.size = size;
   help_tetro_init_mask(tetro.design);
   help_tetro_init_mask(tetro.left);
   help_tetro_init_mask(tetro.right);

   return tetro;
}

bool help_tetro_plot_mask(tetro_mask mask, int size, const char * pattern)
{
   if (NULL == mask || NULL == pattern) return false;

   const int TETRO_CELL_COUNT = size * size;
   if (strlen(pattern) != TETRO_CELL_COUNT)
   {
      printf("\nFailed to plot tetro mask because pattern length does not match tetro cell count");
      return false;
   }

   int i_pattern = 0;
   for (int py = 0; py < size; ++py)
   {
      for (int px = 0; px < size; ++px)
      {
         // Plot pattern
         // Assumes that cells have been nulled to non-plotted during init
         const bool PLOT_CELL = (pattern[i_pattern++] == '#') ? true : false;
         if (PLOT_CELL)
         {
            mask[px][py] = true;
         }
      }
   }

   // Success
   return true;
}

bool help_tetro_plot_design(struct tetro_s * out_tetro, const char * pattern)
{
   if (NULL == out_tetro || NULL == pattern) return false;

   return help_tetro_plot_mask(out_tetro->design, out_tetro->size, pattern);
}

bool help_tetro_plot_left(struct tetro_s * out_tetro, const char * pattern)
{
   if (NULL == out_tetro || NULL == pattern) return false;

   return help_tetro_plot_mask(out_tetro->left, out_tetro->size, pattern);
}

bool help_tetro_plot_right(struct tetro_s * out_tetro, const char * pattern)
{
   if (NULL == out_tetro || NULL == pattern) return false;

   return help_tetro_plot_mask(out_tetro->right, out_tetro->size, pattern);
}

struct tetro_s help_tetro_make_type_I(void)
{
   struct tetro_s tetro_I = help_tetro_make_typed_unfinished(TETRO_TYPE_I, 4);
   help_tetro_plot_design(
      &tetro_I,
      "..#."
      "..#."
      "..#."
      "..#."
   );
   help_tetro_plot_left(
      &tetro_I,
      "...."
      "...."
      "...."
      "...."
   );
   help_tetro_plot_right(
      &tetro_I,
      "...."
      "...."
      "...."
      "...."
   );

   return tetro_I;
}

void help_tetro_rotate_mask_left(tetro_mask mask, int size)
{
   tetro_mask rotated;
   memcpy(rotated, mask, sizeof(rotated));

   // Rotate
   for (int row = 0; row < size; ++row)
   {
      for (int col = 0; col < size; ++col)
      {
         rotated[col][row] = mask[row][size - 1 - col];
      }
   }

   // Reflect
   memcpy(mask, rotated, sizeof(rotated));
}

void help_tetro_rotate_mask_right(tetro_mask mask, int size)
{
   tetro_mask rotated;
   memcpy(rotated, mask, sizeof(rotated));

   // Rotate
   for (int row = 0; row < size; ++row)
   {
      for (int col = 0; col < size; ++col)
      {
         rotated[col][row] = mask[size - 1 - row][col];
      }
   }

   // Reflect
   memcpy(mask, rotated, sizeof(rotated));
}

bool help_tetro_world_rotate_left(struct tetro_world_s * tetro)
{
   if (NULL == tetro) return false;

   help_tetro_rotate_mask_left(tetro->data.design, tetro->data.size);
   help_tetro_rotate_mask_left(tetro->data.left, tetro->data.size);
   help_tetro_rotate_mask_left(tetro->data.right, tetro->data.size);

   // Success
   return true;
}

bool help_tetro_world_rotate_right(struct tetro_world_s * tetro)
{
   if (NULL == tetro) return false;

   help_tetro_rotate_mask_right(tetro->data.design, tetro->data.size);
   help_tetro_rotate_mask_right(tetro->data.left, tetro->data.size);
   help_tetro_rotate_mask_right(tetro->data.right, tetro->data.size);

   // Success
   return true;
}

struct tetro_world_s help_tetro_world_make_type_at(enum tetro_type_e type, int x, int y)
{
   // TODO-GS: Spawning of tetros at position
   switch (type)
   {
      case TETRO_TYPE_I:
         return tetro_world_make(help_tetro_make_type_I(), x, y);
         break;
      default:
         printf("\nAttempting to spawn un-supported tetro type");
         break;
   }
}

// Helpers - Input
enum custom_key_e {
   CUSTOM_KEY_UP,
   CUSTOM_KEY_DOWN,
   CUSTOM_KEY_LEFT,
   CUSTOM_KEY_RIGHT,
   CUSTOM_KEY_A,
   CUSTOM_KEY_B,
   CUSTOM_KEY_START,
   CUSTOM_KEY_SELECT,
   CUSTOM_KEY_COUNT
};

enum key_state_e {
   KEY_STATE_NONE,
   KEY_STATE_PRESSED,
   KEY_STATE_HELD,
   KEY_STATE_RELEASED
};

struct input_s {
   enum key_state_e key_states[CUSTOM_KEY_COUNT];
};

struct input_s * help_input_make(void)
{
   struct input_s * instance = malloc(sizeof(struct input_s));
   if (NULL == instance)
   {
      printf("\nFailed to allocate input instance");
      return NULL;
   }

   // Initialize custom key states
   for (int custom_key = 0; custom_key < CUSTOM_KEY_COUNT; ++custom_key)
   {
      instance->key_states[custom_key] = KEY_STATE_NONE;
   }

   // Success
   return instance;
}

void help_input_destroy(struct input_s * input)
{
   free(input);
}

enum key_state_e help_input_determine_key_state(enum key_state_e current_state, bool currently_pressed)
{
   switch (current_state)
   {
      case KEY_STATE_NONE:
         return currently_pressed ? KEY_STATE_PRESSED : KEY_STATE_NONE;
         break;
      case KEY_STATE_PRESSED:
         return currently_pressed ? KEY_STATE_HELD : KEY_STATE_RELEASED;
         break;
      case KEY_STATE_HELD:
         return currently_pressed ? KEY_STATE_HELD : KEY_STATE_RELEASED;
         break;
      case KEY_STATE_RELEASED:
         return currently_pressed ? KEY_STATE_PRESSED : KEY_STATE_NONE;
         break;
   }
}

bool help_input_determine_intermediate_state(struct input_s * instance)
{
   if (NULL == instance) return false;

   const bool * KEYBOARD_STATE = SDL_GetKeyboardState(NULL);
   const enum key_state_e NEW_STATE_UP = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_UP], KEYBOARD_STATE[SDL_SCANCODE_W]);
   const enum key_state_e NEW_STATE_DOWN = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_DOWN], KEYBOARD_STATE[SDL_SCANCODE_S]);
   const enum key_state_e NEW_STATE_LEFT = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_LEFT], KEYBOARD_STATE[SDL_SCANCODE_A]);
   const enum key_state_e NEW_STATE_RIGHT = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_RIGHT], KEYBOARD_STATE[SDL_SCANCODE_D]);
   const enum key_state_e NEW_STATE_A = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_A], KEYBOARD_STATE[SDL_SCANCODE_UP]);
   const enum key_state_e NEW_STATE_B = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_B], KEYBOARD_STATE[SDL_SCANCODE_LEFT]);
   const enum key_state_e NEW_STATE_START = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_START], KEYBOARD_STATE[SDL_SCANCODE_RETURN]);
   const enum key_state_e NEW_STATE_SELECT = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_SELECT], KEYBOARD_STATE[SDL_SCANCODE_DELETE]);

   instance->key_states[CUSTOM_KEY_UP] = NEW_STATE_UP;
   instance->key_states[CUSTOM_KEY_DOWN] = NEW_STATE_DOWN;
   instance->key_states[CUSTOM_KEY_LEFT] = NEW_STATE_LEFT;
   instance->key_states[CUSTOM_KEY_RIGHT] = NEW_STATE_RIGHT;
   instance->key_states[CUSTOM_KEY_A] = NEW_STATE_A;
   instance->key_states[CUSTOM_KEY_B] = NEW_STATE_B;
   instance->key_states[CUSTOM_KEY_START] = NEW_STATE_START;
   instance->key_states[CUSTOM_KEY_SELECT] = NEW_STATE_SELECT;

   // Success
   return true;
}

bool help_input_key_in_state(struct input_s * input, enum custom_key_e key, enum key_state_e state)
{
   if (NULL == input) return false;

   return input->key_states[key] == state;
}

bool help_input_key_none(struct input_s * input, enum custom_key_e key)
{
   return help_input_key_in_state(input, key, KEY_STATE_NONE);
}

bool help_input_key_pressed(struct input_s * input, enum custom_key_e key)
{
   return help_input_key_in_state(input, key, KEY_STATE_PRESSED);
}

bool help_input_key_held(struct input_s * input, enum custom_key_e key)
{
   return help_input_key_in_state(input, key, KEY_STATE_HELD);
}

bool help_input_key_released(struct input_s * input, enum custom_key_e key)
{
   return help_input_key_in_state(input, key, KEY_STATE_RELEASED);
}

// Helper - Field
#define FIELD_WIDTH (5)
#define FIELD_HEIGHT (10)
struct field_cell_s {
   enum tetro_type_e type;
   bool occupied;
};
const int FIELD_TILE_SIZE = 8;

struct field_cell_s field_cell_make(enum tetro_type_e type, bool occupied)
{
   struct field_cell_s field_cell;

   field_cell.type = type;
   field_cell.occupied = occupied;

   return field_cell;
}

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

   // Create input
   struct input_s * input = help_input_make();
   if (NULL == input)
   {
      printf("\nFailed to create input");
      return EXIT_FAILURE;
   }

   // Prepare resource strings
   char dir_abs_res_images[1024];
   snprintf(dir_abs_res_images, sizeof(dir_abs_res_images), "%s\\images\\", DIR_ABS_RES);
   char dir_abs_res_img_tiles[1024];
   const char * TILES_IMAGE_FILE_NAME = "tiles.png";
   snprintf(dir_abs_res_img_tiles, sizeof(dir_abs_res_img_tiles), "%s\\%s", dir_abs_res_images, TILES_IMAGE_FILE_NAME);

   // Load entity texture
   struct texture_rgba_s * tex_sprites = help_texture_rgba_from_png(dir_abs_res_img_tiles);
   if (NULL == tex_sprites)
   {
      printf("\nFailed to convert tile image to tile texture");
      return EXIT_FAILURE;
   }

   // Create rendering sprites
   const struct sprite_s SPR_LIGHTEST = sprite_make(12, 2, FIELD_TILE_SIZE);
   const struct sprite_s SPR_LIGHT = sprite_make(13, 2, FIELD_TILE_SIZE);
   const struct sprite_s SPR_DARK = sprite_make(14, 2, FIELD_TILE_SIZE);
   const struct sprite_s SPR_DARKEST = sprite_make(15, 2, FIELD_TILE_SIZE);
   const struct sprite_s SPR_CELL = sprite_make(3, 6, FIELD_TILE_SIZE);

   // Log engine status
   const int DW = 20;
   printf("\n\nEngine Information");
   printf("\n\t%-*s: %s", DW, "resource directory", DIR_ABS_RES);
   printf("\n\t%-*s: %s", DW, "VSYNC", SUCCESS_USE_VSYNC ? "enabled" : "disabled");

   // FPS counter
   int frames_per_second = 0;
   uint64_t timer_fps_ns = SDL_GetTicksNS();
   const uint64_t NS_PER_S = 1000000000;

   // Prepare tetros
   struct tetro_world_s tetro_active = help_tetro_world_make_type_at(TETRO_TYPE_I, 1, 3);

   // Playing field
   struct field_cell_s field[FIELD_WIDTH][FIELD_HEIGHT];
   for (int fy = 0; fy < FIELD_HEIGHT; ++fy)
   {
      for (int fx = 0; fx < FIELD_WIDTH; ++fx)
      {
         field[fx][fy] = field_cell_make(TETRO_TYPE_I, false);
      }
   }

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

      // Determine input state
      help_input_determine_intermediate_state(input);

      // Tick
      if (help_input_key_pressed(input, CUSTOM_KEY_B)) help_tetro_world_rotate_left(&tetro_active);
      if (help_input_key_pressed(input, CUSTOM_KEY_A)) help_tetro_world_rotate_right(&tetro_active);

      // Render into offline texture
      // -> Clear
      help_texture_rgba_clear(tex_virtual, color_rgba_make_rgba(50, 50, 50, 255));
      // -> Render scene
      // ----> Playing field
      for (int fy = 0; fy < FIELD_HEIGHT; ++fy)
      {
         for (int fx = 0; fx < FIELD_WIDTH; ++fx)
         {
            const struct field_cell_s * FIELD_CELL = &field[fx][fy];
            if (false == FIELD_CELL->occupied) continue;

            help_tex_sprite_render(
               SPR_LIGHT,
               fx * FIELD_TILE_SIZE,
               fy * FIELD_TILE_SIZE,
               tex_sprites,
               tex_virtual
            );
         }
      }
      // ----> Testing tetro creation
      for (int ty = 0; ty < tetro_active.data.size; ++ty)
      {
         for (int tx = 0; tx < tetro_active.data.size; ++tx)
         {
            if (tetro_active.data.design[tx][ty] == false) continue;
            help_tex_sprite_render(
               SPR_CELL,
               tx * FIELD_TILE_SIZE,
               ty * FIELD_TILE_SIZE,
               tex_sprites,
               tex_virtual
            );
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
      SDL_SetRenderDrawColor(sdl_renderer, 0x00, 0x00, 0x00, 0xFF);
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
   help_input_destroy(input);
   help_texture_rgba_destroy(tex_virtual);
   help_texture_rgba_destroy(tex_sprites);

   // Cleanup SDL
   SDL_DestroyTexture(sdl_texture_online);
   SDL_DestroyWindow(sdl_window);
   SDL_DestroyRenderer(sdl_renderer);
   SDL_Quit();

   // Back to OS
   return EXIT_SUCCESS;
}