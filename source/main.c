#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// Constants
const char * ARG_KEY_DIR_ABS_RES = "-abs_res_dir";
const bool CONFIG_DO_RENDER_TETRO_COLLISION_MASKS = true;
const bool CONFIG_DO_SET_RANDOM_SEED = true;

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

double help_sdl_time_in_seconds(void)
{
   const static double NANO_SEC_TO_SEC = 1.0 / 1000000000;
   return (double)SDL_GetTicksNS() * NANO_SEC_TO_SEC;
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
   const int FLIPPED_Y = help_texture_rgba_size(instance).y - 1 - y;

   int texel_index;
   if (help_texture_rgba_2d_coords_to_linear(instance, x, FLIPPED_Y, &texel_index))
   {
      instance->texels[texel_index] = color;
      return true;
   }

   return false;
}

void help_texture_rgba_plot_aabb(struct texture_rgba_s * instance, int min_x, int min_y, int width, int height, color_rgba_t color)
{
   for (int box_y = min_y; box_y < (min_y + height); ++box_y)
   {
      for (int box_x = min_x; box_x < (min_x + width); ++box_x)
      {
         help_texture_rgba_plot_texel(instance, box_x, box_y, color);
      }
   }
}

void help_texture_rgba_plot_horizontal_line(struct texture_rgba_s * instance, int min_x, int width, int y, color_rgba_t color)
{
   for (int sx = min_x; sx < (min_x + width); ++sx)
   {
      help_texture_rgba_plot_texel(instance, sx, y, color);
   }
}

void help_texture_rgba_plot_vertical_line(struct texture_rgba_s * instance, int min_y, int height, int x, color_rgba_t color)
{
   for (int sy = min_y; sy < (min_y + height); ++sy)
   {
      help_texture_rgba_plot_texel(instance, x, sy, color);
   }
}

void help_texture_rgba_plot_aabb_outline(struct texture_rgba_s * instance, int min_x, int min_y, int width, int height, color_rgba_t color)
{
   help_texture_rgba_plot_horizontal_line(instance, min_x, width, min_y, color);
   help_texture_rgba_plot_horizontal_line(instance, min_x, width, min_y + height - 1, color);

   help_texture_rgba_plot_vertical_line(instance, min_y, height, min_x, color);
   help_texture_rgba_plot_vertical_line(instance, min_y, height, min_x + height - 1, color);
}

bool help_texture_rgba_access_texel(struct texture_rgba_s * instance, int x, int y, color_rgba_t * out_color)
{
   if (NULL == instance || NULL == out_color) return false;

   const int FLIPPED_Y = help_texture_rgba_size(instance).y - 1 - y;

   int texel_index;
   if (help_texture_rgba_2d_coords_to_linear(instance, x, FLIPPED_Y, &texel_index))
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
            sprite.texture_min.y + sprite.texture_size.y - 1 - spr_y,
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
   TETRO_TYPE_O,
   TETRO_TYPE_COUNT
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
   struct vec_2i_s tile_pos;
};

struct tetro_world_s tetro_world_make(struct tetro_s tetro, int tile_x, int tile_y)
{
   struct tetro_world_s tetro_world;

   tetro_world.data = tetro;
   tetro_world.tile_pos = vec_2i_make_xy(tile_x, tile_y);

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
   for (int py = size - 1; py >= 0; --py)
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
      "##.."
      "##.#"
      "...#"
      "...#"
   );
   help_tetro_plot_right(
      &tetro_I,
      "...#"
      "...#"
      "##.#"
      "##.."
   );

   return tetro_I;
}

struct tetro_s help_tetro_make_type_O(void)
{
   struct tetro_s tetro_O = help_tetro_make_typed_unfinished(TETRO_TYPE_O, 2);
   help_tetro_plot_design(
      &tetro_O,
      "##"
      "##"
   );
   help_tetro_plot_left(
      &tetro_O,
      ".."
      ".."
   );
   help_tetro_plot_right(
      &tetro_O,
      ".."
      ".."
   );

   return tetro_O;
}

struct tetro_s help_tetro_make_type_Lr(void)
{
   struct tetro_s tetro_Lr = help_tetro_make_typed_unfinished(TETRO_TYPE_Lr, 3);
   help_tetro_plot_design(
      &tetro_Lr,
      ".#."
      ".#."
      "##."
   );
   help_tetro_plot_left(
      &tetro_Lr,
      "#.."
      "#.#"
      "..#"
   );
   help_tetro_plot_right(
      &tetro_Lr,
      "#.#"
      "#.#"
      "..."
   );

   return tetro_Lr;
}

struct tetro_s help_tetro_make_type_L(void)
{
   struct tetro_s tetro_L = help_tetro_make_typed_unfinished(TETRO_TYPE_L, 3);
   help_tetro_plot_design(
      &tetro_L,
      ".#."
      ".#."
      ".##"
   );
   help_tetro_plot_left(
      &tetro_L,
      "#.#"
      "#.#"
      "..."
   );
   help_tetro_plot_right(
      &tetro_L,
      "..#"
      "#.#"
      "#.."
   );

   return tetro_L;
}

struct tetro_s help_tetro_make_type_S(void)
{
   struct tetro_s tetro_S = help_tetro_make_typed_unfinished(TETRO_TYPE_S, 3);
   help_tetro_plot_design(
      &tetro_S,
      "..."
      ".##"
      "##."
   );
   help_tetro_plot_left(
      &tetro_S,
      ".##"
      "..."
      "..#"
   );
   help_tetro_plot_right(
      &tetro_S,
      "#.."
      "#.."
      "..#"
   );

   return tetro_S;
}

struct tetro_s help_tetro_make_type_Z(void)
{
   struct tetro_s tetro_Z = help_tetro_make_typed_unfinished(TETRO_TYPE_Z, 3);
   help_tetro_plot_design(
      &tetro_Z,
      "..."
      "##."
      ".##"
   );
   help_tetro_plot_left(
      &tetro_Z,
      "..#"
      "..#"
      "#.."
   );
   help_tetro_plot_right(
      &tetro_Z,
      "##."
      "..."
      "#.."
   );

   return tetro_Z;
}

struct tetro_s help_tetro_make_type_T(void)
{
   struct tetro_s tetro_T = help_tetro_make_typed_unfinished(TETRO_TYPE_T, 3);
   help_tetro_plot_design(
      &tetro_T,
      "..."
      "###"
      ".#."
   );
   help_tetro_plot_left(
      &tetro_T,
      ".##"
      "..."
      "#.#"
   );
   help_tetro_plot_right(
      &tetro_T,
      "##."
      "..."
      "#.#"
   );

   return tetro_T;
}

void help_tetro_rotate_mask_ccw(tetro_mask mask, int size)
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

void help_tetro_rotate_mask_cw(tetro_mask mask, int size)
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

bool help_tetro_world_rotate_ccw(struct tetro_world_s * tetro)
{
   if (NULL == tetro) return false;

   help_tetro_rotate_mask_ccw(tetro->data.design, tetro->data.size);
   help_tetro_rotate_mask_ccw(tetro->data.left, tetro->data.size);
   help_tetro_rotate_mask_ccw(tetro->data.right, tetro->data.size);

   // Success
   return true;
}

bool help_tetro_world_rotate_cw(struct tetro_world_s * tetro)
{
   if (NULL == tetro) return false;

   help_tetro_rotate_mask_cw(tetro->data.design, tetro->data.size);
   help_tetro_rotate_mask_cw(tetro->data.left, tetro->data.size);
   help_tetro_rotate_mask_cw(tetro->data.right, tetro->data.size);

   // Success
   return true;
}

struct tetro_world_s help_tetro_world_make_type_at_tile(enum tetro_type_e type, int tile_x, int tile_y)
{
   switch (type)
   {
      case TETRO_TYPE_I:
         return tetro_world_make(help_tetro_make_type_I(), tile_x, tile_y);
         break;
      case TETRO_TYPE_O:
         return tetro_world_make(help_tetro_make_type_O(), tile_x, tile_y);
         break;
      case TETRO_TYPE_Lr:
         return tetro_world_make(help_tetro_make_type_Lr(), tile_x, tile_y);
         break;
      case TETRO_TYPE_L:
         return tetro_world_make(help_tetro_make_type_L(), tile_x, tile_y);
         break;
      case TETRO_TYPE_S:
         return tetro_world_make(help_tetro_make_type_S(), tile_x, tile_y);
         break;
      case TETRO_TYPE_Z:
         return tetro_world_make(help_tetro_make_type_Z(), tile_x, tile_y);
         break;
      case TETRO_TYPE_T:
         return tetro_world_make(help_tetro_make_type_T(), tile_x, tile_y);
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

bool help_input_key_pressed_or_held(struct input_s * input, enum custom_key_e key)
{
   return (
      help_input_key_in_state(input, key, KEY_STATE_PRESSED) ||
      help_input_key_in_state(input, key, KEY_STATE_HELD)
   );
}

// Helper - Font rendering
struct font_render_glyph_s {
   bool is_mapped;
   struct sprite_s sprite;
};

#define FONT_RENDER_MAX_GLYPHS (256)
struct font_render_s {
   struct font_render_glyph_s glyphs[FONT_RENDER_MAX_GLYPHS];
};

struct font_render_glyph_s font_render_glyph_make(bool is_mapped, struct sprite_s sprite)
{
   struct font_render_glyph_s glyph;

   glyph.is_mapped = is_mapped;
   glyph.sprite = sprite;

   return glyph;
}

void help_font_render_destroy(struct font_render_s * instance)
{
   if (instance)
   {
      free(instance);
   }
}

struct font_render_s * help_font_render_create(void)
{
   struct font_render_s * instance = malloc(sizeof(struct font_render_s));
   
   if (instance)
   {
      for (int ascii_code = 0; ascii_code < FONT_RENDER_MAX_GLYPHS; ++ascii_code)
      {
         instance->glyphs[ascii_code].is_mapped = false;
      }
   }

   return instance;
}
   
bool help_font_render_ascii_in_valid(int ascii_code)
{
   return (
      ascii_code < 0 ||
      ascii_code >= FONT_RENDER_MAX_GLYPHS
   ) ? true : false;
}

bool help_font_render_register_glyph(struct font_render_s * instance, char glyph_char, struct sprite_s sprite)
{
   const int ASCII_CODE = (int)glyph_char;
   if (NULL == instance || help_font_render_ascii_in_valid(ASCII_CODE)) return false;

   // Register and map glyph to sprite
   instance->glyphs[ASCII_CODE] = font_render_glyph_make(true, sprite);

   // Success
   return true;
}

// Helpers - Border rendering
enum sprite_map_tile_e {
   SPRITE_MAP_TILE_TETRO_BLOCK_I,
   SPRITE_MAP_TILE_TETRO_BLOCK_O,
   SPRITE_MAP_TILE_TETRO_BLOCK_Lr,
   SPRITE_MAP_TILE_TETRO_BLOCK_L,
   SPRITE_MAP_TILE_TETRO_BLOCK_S,
   SPRITE_MAP_TILE_TETRO_BLOCK_T,
   SPRITE_MAP_TILE_TETRO_BLOCK_Z,
   SPRITE_MAP_TILE_NA,
   SPRITE_MAP_TILE_COUNT
};

struct sprite_map_s {
   struct sprite_s tile_to_sprite[SPRITE_MAP_TILE_COUNT];
   struct texture_rgba_s * texture;
   int tile_size;
};

bool help_sprite_map_tile(struct sprite_map_s * instance, enum sprite_map_tile_e tile, int sprite_tile_x, int sprite_tile_y)
{
   if (NULL == instance) return false;

   instance->tile_to_sprite[tile] = sprite_make(sprite_tile_x, sprite_tile_y, instance->tile_size);

   return true;
}

struct sprite_map_s * help_sprite_map_create(struct texture_rgba_s * texture, int na_tile_x, int na_tile_y, int tile_size)
{
   if (NULL == texture) return NULL;

   struct sprite_map_s * instance = malloc(sizeof(struct sprite_map_s));

   if (instance)
   {
      instance->texture = texture;
      instance->tile_size = tile_size;
      help_sprite_map_tile(instance, SPRITE_MAP_TILE_NA, na_tile_x, na_tile_y);
   }

   return instance;
}

struct sprite_s help_sprite_map_sprite_for(const struct sprite_map_s * instance, enum sprite_map_tile_e tile)
{
   // TODO-GS: This sucks
   if (NULL == instance) return sprite_make(13, 13, 8);

   // Assume that all tiles are mapped for now
   return instance->tile_to_sprite[tile];
}

// Helpers - Regions
struct region_2d_s {
   struct vec_2i_s min;
   struct vec_2i_s max;
   struct vec_2i_s size;
};

struct region_2d_s region_2d_s_make(int min_x, int min_y, int width, int height)
{
   struct region_2d_s region;

   region.min = vec_2i_make_xy(min_x, min_y);
   region.size = vec_2i_make_xy(width, height);
   region.max = vec_2i_make_xy(
      min_x + width - 1,
      min_y + height - 1
   );

   return region;
}

// Helpers - FSM
enum game_state_e {
   GAME_STATE_NEW_GAME,
   GAME_STATE_CONTROL,
   GAME_STATE_PLACE,
   GAME_STATE_REMOVE_LINES,
   GAME_STATE_RESPAWN,
   GAME_STATE_GAME_OVER,
   GAME_STATE_NONE
};

// Helpers - Bounds
bool help_bounds_out_of_region(int min_x, int min_y, int width, int height, int x, int y)
{
   const struct region_2d_s REGION = region_2d_s_make(min_x, min_y, width, height);
   return (
      x < REGION.min.x ||
      x > REGION.max.x ||
      y < REGION.min.y ||
      y > REGION.max.y
   ) ? true : false;
}

// Helpers - Engine
struct engine_s {
   // Components

   // Resources
   struct texture_rgba_s * tex_virtual;
   struct texture_rgba_s * tex_sprites;
   struct sprite_map_s * sprite_map;
};

struct texture_rgba_s * help_engine_get_tex_virtual(struct engine_s * engine)
{
   return engine ? engine->tex_virtual : NULL;
}

struct texture_rgba_s * help_engine_get_tex_sprites(struct engine_s * engine)
{
   return engine ? engine->tex_sprites : NULL;
}

struct sprite_map_s * help_engine_get_sprite_map(struct engine_s * engine)
{
   return engine ? engine->sprite_map : NULL;
}

// Helpers - Playing field
struct play_field_cell_s {
   enum tetro_type_e type;
   bool occupied;
};

// Helpers - Rendering (Simplified)
bool help_render_engine_sprite(struct engine_s * engine, int x, int y, enum sprite_map_tile_e tile_type)
{
   if (NULL == engine) return false;

   return help_tex_sprite_render(
      help_sprite_map_sprite_for(help_engine_get_sprite_map(engine), tile_type),
      x,
      y,
      help_engine_get_tex_sprites(engine),
      help_engine_get_tex_virtual(engine)
   );
}

// Helpers - Play field
#define PLAY_FIELD_TILE_SIZE (8)
#define PLAY_FIELD_WIDTH (10)
#define PLAY_FIELD_HEIGHT (18)
struct play_field_s {
   struct play_field_cell_s cells[PLAY_FIELD_WIDTH][PLAY_FIELD_HEIGHT];
   int offset_in_tiles_hori;
};

int help_play_field_get_hori_offset_in_tiles(struct play_field_s * play_field)
{
   return play_field ? play_field->offset_in_tiles_hori : 0;
}

int help_play_field_get_hori_offset_in_pixels(struct play_field_s * play_field)
{
   return play_field ? (play_field->offset_in_tiles_hori * PLAY_FIELD_TILE_SIZE) : 0;
}

struct play_field_cell_s help_play_field_cell_make(enum tetro_type_e type, bool occupied)
{
   struct play_field_cell_s cell;

   cell.type = type;
   cell.occupied = occupied;

   return cell;
}

struct play_field_cell_s help_play_field_cell_make_non_occupied(void)
{
   return help_play_field_cell_make(TETRO_TYPE_O, false);
}

struct play_field_s help_play_field_make_non_occupied(int offset_in_tiles_hori)
{
   struct play_field_s fresh_play_field;
   fresh_play_field.offset_in_tiles_hori = offset_in_tiles_hori;

   // Populate cells
   for (int y = 0; y < PLAY_FIELD_HEIGHT; ++y)
   {
      for (int x = 0; x < PLAY_FIELD_WIDTH; ++x)
      {
         fresh_play_field.cells[x][y] = help_play_field_cell_make_non_occupied();
      }
   }

   // Success
   return fresh_play_field;
}

bool help_play_field_coords_out_of_bounds(struct play_field_s * play_field, int x, int y)
{
   if (NULL == play_field) return true;

   return help_bounds_out_of_region(0, 0, PLAY_FIELD_WIDTH, PLAY_FIELD_HEIGHT, x, y);
}

void help_play_field_render_to_texture(struct play_field_s * play_field, struct engine_s * engine)
{
   if (NULL == play_field || NULL == engine) return;

   // Render with play field offset
   for (int y = 0; y < PLAY_FIELD_HEIGHT; ++y)
   {
      for (int x = 0; x < PLAY_FIELD_WIDTH; ++x)
      {
         // Render background
         help_texture_rgba_plot_aabb(
            help_engine_get_tex_virtual(engine),
            (play_field->offset_in_tiles_hori * PLAY_FIELD_TILE_SIZE) + (x * PLAY_FIELD_TILE_SIZE),
            y * PLAY_FIELD_TILE_SIZE,
            PLAY_FIELD_TILE_SIZE,
            PLAY_FIELD_TILE_SIZE,
            color_rgba_make_rgba(0, 50, 0, 0xFF)
         );

         // Render placed cell by type
         const struct play_field_cell_s * CELL = &play_field->cells[x][y];
         if (false == CELL->occupied)
         {
            // Don't render non-occupied cells
            continue;
         }

         // Cell occupied
         help_render_engine_sprite(engine, 0, 0, CELL->type);
      }
   }
}

// Helpers - Tetro spawning
struct tetro_world_s help_tetro_world_make_random_at_spawn(void)
{
   const enum tetro_type_e RANDOM_TETRO_TYPE = rand() % TETRO_TYPE_COUNT;

   // Make tetro at some position
   struct tetro_world_s random_tetro = help_tetro_world_make_type_at_tile(RANDOM_TETRO_TYPE, 0, 0);

   // TODO-GS: Random tetro rotation ?
   for (int rotation = 0; rotation < rand() % 8; ++rotation)
   {
      help_tetro_world_rotate_cw(&random_tetro);
   }

   // Position to spawn based on tetro size
   const int TETRO_SIZE = random_tetro.data.size;
   random_tetro.tile_pos.x = ((PLAY_FIELD_WIDTH - 1) / 2) - (TETRO_SIZE / 2) + 1;
   random_tetro.tile_pos.y = (PLAY_FIELD_HEIGHT - 1) - (TETRO_SIZE - 1);

   // Success
   return random_tetro;
}

// Helpers - Rendering stuff
enum sprite_map_tile_e help_tetro_type_to_sprite_tile(enum tetro_type_e tetro_type)
{
   switch(tetro_type)
   {
      case TETRO_TYPE_I:
         return SPRITE_MAP_TILE_TETRO_BLOCK_I;
         break;
      case TETRO_TYPE_O:
         return SPRITE_MAP_TILE_TETRO_BLOCK_O;
         break;
      case TETRO_TYPE_L:
         return SPRITE_MAP_TILE_TETRO_BLOCK_L;
         break;
      case TETRO_TYPE_Lr:
         return SPRITE_MAP_TILE_TETRO_BLOCK_Lr;
         break;
      case TETRO_TYPE_S:
         return SPRITE_MAP_TILE_TETRO_BLOCK_S;
         break;
      case TETRO_TYPE_Z:
         return SPRITE_MAP_TILE_TETRO_BLOCK_Z;
         break;
      case TETRO_TYPE_T:
         return SPRITE_MAP_TILE_TETRO_BLOCK_T;
         break;
      default:
         return SPRITE_MAP_TILE_NA;
         break;
   }
}

void help_tetro_render_to_texture(struct tetro_world_s * tetro, int play_field_offset_hori, struct engine_s * engine)
{
   if (NULL == tetro || NULL == engine) return;

   // Render tetro at position with play field offset
   for (int ty = 0; ty < tetro->data.size; ++ty)
   {
      for (int tx = 0; tx < tetro->data.size; ++tx)
      {
         // Design cells
         const bool IS_DESIGN_CELL = tetro->data.design[tx][ty];
         if (IS_DESIGN_CELL)
         {
            help_render_engine_sprite(
               engine,
               play_field_offset_hori + (tetro->tile_pos.x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE),
               (tetro->tile_pos.y * PLAY_FIELD_TILE_SIZE) + (ty * PLAY_FIELD_TILE_SIZE),
               help_tetro_type_to_sprite_tile(tetro->data.type)
            );
         }

         // Tetro debug rendering configured ?
         if (false == CONFIG_DO_RENDER_TETRO_COLLISION_MASKS)
         {
            continue;
         }

         // CCW collision mask
         const bool IS_CCW_COLLISION_CELL = tetro->data.left[tx][ty];
         if (IS_CCW_COLLISION_CELL)
         {
            // Render CCW cells a full-sized tile
            help_texture_rgba_plot_aabb_outline(
               help_engine_get_tex_virtual(engine),
               play_field_offset_hori + (tetro->tile_pos.x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE),
               (tetro->tile_pos.y * PLAY_FIELD_TILE_SIZE) + (ty * PLAY_FIELD_TILE_SIZE),
               PLAY_FIELD_TILE_SIZE,
               PLAY_FIELD_TILE_SIZE,
               color_rgba_make_rgba(150, 0, 0, 255)
            );
         }

         // CW collision mask
         const bool IS_CW_COLLISION_CELL = tetro->data.right[tx][ty];
         if (IS_CW_COLLISION_CELL)
         {
            // Render CW cells a less than tile-size tile
            const int INSET = 2;
            help_texture_rgba_plot_aabb_outline(
               help_engine_get_tex_virtual(engine),
               play_field_offset_hori + (tetro->tile_pos.x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE) + INSET,
               (tetro->tile_pos.y * PLAY_FIELD_TILE_SIZE) + (ty * PLAY_FIELD_TILE_SIZE) + INSET,
               PLAY_FIELD_TILE_SIZE - (2 * INSET),
               PLAY_FIELD_TILE_SIZE - (2 * INSET),
               color_rgba_make_rgba(0, 150, 0, 255)
            );
         }
      }
   }
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

   // Other window related configuration
   SDL_SetWindowMouseGrab(sdl_window, true);
   SDL_HideCursor();

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

   // Set random rand() seed ?
   if (CONFIG_DO_SET_RANDOM_SEED)
   {
      // TODO-GS: Safer / Higher resolution argument for srand() ?
      const unsigned int SEED_RANDOM = (unsigned int)SDL_GetTicks();
      srand(SEED_RANDOM);
   }

   // Log engine status
   // TODO-GS: Config symbols
   const int DW = 20;
   printf("\n\nEngine Information");
   printf("\n\t%-*s: %s", DW, "resource directory", DIR_ABS_RES);
   printf("\n\t%-*s: %s", DW, "VSYNC", SUCCESS_USE_VSYNC ? "enabled" : "disabled");

   // FPS counter
   double last_time_fps = help_sdl_time_in_seconds();
   int frames_per_second = 0;

   // Integration
   const int TICKS_PER_SECOND = 50;
   const double FIXED_DELTA_TIME = 1.0 / TICKS_PER_SECOND;
   double time_simulated = 0.0;
   double last_time_tick = help_sdl_time_in_seconds();
   double fixed_delta_time_accumulator = 0.0;

   // Engine
   // ----> Sprite map
   struct sprite_map_s * sprite_map = help_sprite_map_create(tex_sprites, 13, 13, 8);
   if (NULL == sprite_map)
   {
      printf("\nFailed to create sprite map");
      return EXIT_FAILURE;
   }
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_I, 0, 7);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_O, 1, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_Lr, 2, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_L, 3, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_S, 4, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_T, 5, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_Z, 6, 6);

   // Package engine components
   struct engine_s engine;
   engine.tex_virtual = tex_virtual;
   engine.tex_sprites = tex_sprites;
   engine.sprite_map = sprite_map;

   // Game state
   // ----> Tick state
   enum game_state_e game_state = GAME_STATE_NEW_GAME;
   enum game_state_e next_game_state = GAME_STATE_NONE;
   // ----> Playing field
   struct play_field_s play_field = help_play_field_make_non_occupied(1);
   // ----> Active tetro
   struct tetro_world_s tetro_active = help_tetro_world_make_random_at_spawn();

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

      // Tick
      const double NEW_TIME = help_sdl_time_in_seconds();
      const double LAST_FRAME_DURATION = help_sdl_time_in_seconds() - last_time_tick;
      last_time_tick = NEW_TIME;

      fixed_delta_time_accumulator += LAST_FRAME_DURATION;
      while (fixed_delta_time_accumulator >= FIXED_DELTA_TIME)
      {
         // TODO-GS: Fix missing input and double-firing etc.
         help_input_determine_intermediate_state(input);

         // Tick housekeeping
         fixed_delta_time_accumulator -= FIXED_DELTA_TIME;
         time_simulated += FIXED_DELTA_TIME;

         // Tick based on game state
         if (GAME_STATE_NEW_GAME == game_state)
         {
            // Action - New game
         }
         else if (GAME_STATE_CONTROL == game_state)
         {
            // Action - Control
         }
         else if (GAME_STATE_PLACE == game_state)
         {
            // Action - Place
         }
         else if (GAME_STATE_REMOVE_LINES == game_state)
         {
            // Action - Remove lines
         }
         else if (GAME_STATE_RESPAWN == game_state)
         {
            // Action - Remove respawn
         }
         else if (GAME_STATE_GAME_OVER == game_state)
         {
            // Action - Game over
         }
         else if (GAME_STATE_NONE == game_state)
         {
            // Action - None - Should never happen
            printf("\nGame state NONE - Nothing to simulate");
         }

         // Game state housekeeping
         if (GAME_STATE_NONE != next_game_state)
         {
            game_state = next_game_state;
            next_game_state = GAME_STATE_NONE;
         }
      }

      // Offline scene rendering
      const color_rgba_t OFFLINE_CLEAR_COLOR = color_rgba_make_rgba(0, 0, 50, 0xFF);
      help_texture_rgba_clear(tex_virtual, OFFLINE_CLEAR_COLOR);
      // ----> Play field
      help_play_field_render_to_texture(&play_field, &engine);
      // ----> Active tetro
      help_tetro_render_to_texture(&tetro_active, help_play_field_get_hori_offset_in_tiles(&play_field), &engine);

      // TODO-GS: Timed rendering when VSync off ?
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
      if (help_sdl_time_in_seconds() >= (last_time_fps + 1.0))
      {
         const int FPS = frames_per_second;
         //printf("\nFPS: %d", FPS);
         frames_per_second = 0;
         last_time_fps = help_sdl_time_in_seconds();
      }

      // Count frame towards FPS
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