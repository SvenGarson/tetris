#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// Constants
const char * ARG_KEY_DIR_ABS_RES = "-abs_res_dir";
const bool CONFIG_DO_RENDER_TETRO_COLLISION_MASKS = false;
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

bool vec_2i_equals_xy(struct vec_2i_s v, int x, int y)
{
   return (v.x == x && v.y == y);
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
bool help_tex_sprite_render_tinted(
   struct sprite_s sprite,
   int x,
   int y,
   struct texture_rgba_s * texture_sprite,
   struct texture_rgba_s * texture_target,
   bool do_tint,
   color_rgba_t tint_color
)
{
   if (NULL == texture_sprite || NULL == texture_target) return false;

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

         // TODO-GS: Decide how to render overlapping stuff
         const bool DONT_RENDER_TRANSPARENT_TEXELS = true;
         if (DONT_RENDER_TRANSPARENT_TEXELS && (0x00 == color_rgba_channel_alpha(SAFE_SOURCE_TEXEL_COLOR)))
         {
            continue;
         }

         // Plot sprite source texture color into target texture
         // Tint target texels here for now
         const bool SUCCESS_PLOTTING_TEXEL = help_texture_rgba_plot_texel(
            texture_target,
            x + spr_x,
            y + spr_y,
            do_tint ? tint_color : SAFE_SOURCE_TEXEL_COLOR
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

bool help_tetro_drop_mask(tetro_mask mask, int size, const char * pattern)
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

bool help_tetro_drop_design(struct tetro_s * out_tetro, const char * pattern)
{
   if (NULL == out_tetro || NULL == pattern) return false;

   return help_tetro_drop_mask(out_tetro->design, out_tetro->size, pattern);
}

bool help_tetro_drop_left(struct tetro_s * out_tetro, const char * pattern)
{
   if (NULL == out_tetro || NULL == pattern) return false;

   return help_tetro_drop_mask(out_tetro->left, out_tetro->size, pattern);
}

bool help_tetro_drop_right(struct tetro_s * out_tetro, const char * pattern)
{
   if (NULL == out_tetro || NULL == pattern) return false;

   return help_tetro_drop_mask(out_tetro->right, out_tetro->size, pattern);
}

struct tetro_s help_tetro_make_type_I(void)
{
   struct tetro_s tetro_I = help_tetro_make_typed_unfinished(TETRO_TYPE_I, 4);
   help_tetro_drop_design(
      &tetro_I,
      "..#."
      "..#."
      "..#."
      "..#."
   );
   help_tetro_drop_left(
      &tetro_I,
      "##.."
      "##.#"
      "...#"
      "...#"
   );
   help_tetro_drop_right(
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
   help_tetro_drop_design(
      &tetro_O,
      "##"
      "##"
   );
   help_tetro_drop_left(
      &tetro_O,
      ".."
      ".."
   );
   help_tetro_drop_right(
      &tetro_O,
      ".."
      ".."
   );

   return tetro_O;
}

struct tetro_s help_tetro_make_type_Lr(void)
{
   struct tetro_s tetro_Lr = help_tetro_make_typed_unfinished(TETRO_TYPE_Lr, 3);
   help_tetro_drop_design(
      &tetro_Lr,
      ".#."
      ".#."
      "##."
   );
   help_tetro_drop_left(
      &tetro_Lr,
      "#.."
      "#.#"
      "..#"
   );
   help_tetro_drop_right(
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
   help_tetro_drop_design(
      &tetro_L,
      ".#."
      ".#."
      ".##"
   );
   help_tetro_drop_left(
      &tetro_L,
      "#.#"
      "#.#"
      "..."
   );
   help_tetro_drop_right(
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
   help_tetro_drop_design(
      &tetro_S,
      "..."
      ".##"
      "##."
   );
   help_tetro_drop_left(
      &tetro_S,
      ".##"
      "..."
      "..#"
   );
   help_tetro_drop_right(
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
   help_tetro_drop_design(
      &tetro_Z,
      "..."
      "##."
      ".##"
   );
   help_tetro_drop_left(
      &tetro_Z,
      "..#"
      "..#"
      "#.."
   );
   help_tetro_drop_right(
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
   help_tetro_drop_design(
      &tetro_T,
      "..."
      "###"
      ".#."
   );
   help_tetro_drop_left(
      &tetro_T,
      ".##"
      "..."
      "#.#"
   );
   help_tetro_drop_right(
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
   CUSTOM_KEY_VOLUME_UP,
   CUSTOM_KEY_VOLUME_DOWN,
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
   const enum key_state_e NEW_STATE_VOLUME_UP = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_VOLUME_UP], KEYBOARD_STATE[SDL_SCANCODE_KP_PLUS]);
   const enum key_state_e NEW_STATE_VOLUME_DOWN = help_input_determine_key_state(instance->key_states[CUSTOM_KEY_VOLUME_DOWN], KEYBOARD_STATE[SDL_SCANCODE_KP_MINUS]);

   instance->key_states[CUSTOM_KEY_UP] = NEW_STATE_UP;
   instance->key_states[CUSTOM_KEY_DOWN] = NEW_STATE_DOWN;
   instance->key_states[CUSTOM_KEY_LEFT] = NEW_STATE_LEFT;
   instance->key_states[CUSTOM_KEY_RIGHT] = NEW_STATE_RIGHT;
   instance->key_states[CUSTOM_KEY_A] = NEW_STATE_A;
   instance->key_states[CUSTOM_KEY_B] = NEW_STATE_B;
   instance->key_states[CUSTOM_KEY_START] = NEW_STATE_START;
   instance->key_states[CUSTOM_KEY_SELECT] = NEW_STATE_SELECT;
   instance->key_states[CUSTOM_KEY_VOLUME_UP] = NEW_STATE_VOLUME_UP;
   instance->key_states[CUSTOM_KEY_VOLUME_DOWN] = NEW_STATE_VOLUME_DOWN;

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

bool help_input_any_key_pressed(struct input_s * input)
{
   if (NULL == input) return false;

   for (int key = 0; key < CUSTOM_KEY_COUNT; ++key)
   {
      if (help_input_key_pressed(input, key))
      {
         // At least one of the keys was pressed
         return true;
      }
   }

   // No key was pressed
   return false;
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
   SPRITE_MAP_TILE_BRICK,
   SPRITE_MAP_TILE_HIGHLIGHT,
   SPRITE_MAP_TILE_FONT_GLYPH_A,
   SPRITE_MAP_TILE_FONT_GLYPH_B,
   SPRITE_MAP_TILE_FONT_GLYPH_C,
   SPRITE_MAP_TILE_FONT_GLYPH_D,
   SPRITE_MAP_TILE_FONT_GLYPH_E,
   SPRITE_MAP_TILE_FONT_GLYPH_F,
   SPRITE_MAP_TILE_FONT_GLYPH_G,
   SPRITE_MAP_TILE_FONT_GLYPH_H,
   SPRITE_MAP_TILE_FONT_GLYPH_I,
   SPRITE_MAP_TILE_FONT_GLYPH_J,
   SPRITE_MAP_TILE_FONT_GLYPH_K,
   SPRITE_MAP_TILE_FONT_GLYPH_L,
   SPRITE_MAP_TILE_FONT_GLYPH_M,
   SPRITE_MAP_TILE_FONT_GLYPH_N,
   SPRITE_MAP_TILE_FONT_GLYPH_O,
   SPRITE_MAP_TILE_FONT_GLYPH_P,
   SPRITE_MAP_TILE_FONT_GLYPH_Q,
   SPRITE_MAP_TILE_FONT_GLYPH_R,
   SPRITE_MAP_TILE_FONT_GLYPH_S,
   SPRITE_MAP_TILE_FONT_GLYPH_T,
   SPRITE_MAP_TILE_FONT_GLYPH_U,
   SPRITE_MAP_TILE_FONT_GLYPH_V,
   SPRITE_MAP_TILE_FONT_GLYPH_W,
   SPRITE_MAP_TILE_FONT_GLYPH_X,
   SPRITE_MAP_TILE_FONT_GLYPH_Y,
   SPRITE_MAP_TILE_FONT_GLYPH_Z,
   SPRITE_MAP_TILE_FONT_GLYPH_0,
   SPRITE_MAP_TILE_FONT_GLYPH_1,
   SPRITE_MAP_TILE_FONT_GLYPH_2,
   SPRITE_MAP_TILE_FONT_GLYPH_3,
   SPRITE_MAP_TILE_FONT_GLYPH_4,
   SPRITE_MAP_TILE_FONT_GLYPH_5,
   SPRITE_MAP_TILE_FONT_GLYPH_6,
   SPRITE_MAP_TILE_FONT_GLYPH_7,
   SPRITE_MAP_TILE_FONT_GLYPH_8,
   SPRITE_MAP_TILE_FONT_GLYPH_9,
   SPRITE_MAP_TILE_FONT_GLYPH_PERIOD,
   SPRITE_MAP_TILE_FONT_GLYPH_COMMA,
   SPRITE_MAP_TILE_FONT_GLYPH_COLON,
   SPRITE_MAP_TILE_FONT_GLYPH_HYPHEN,
   SPRITE_MAP_TILE_FONT_GLYPH_PARENTHESES_OPEN,
   SPRITE_MAP_TILE_FONT_GLYPH_PARENTHESES_CLOSED,
   SPRITE_MAP_TILE_FONT_GLYPH_PIPE,
   SPRITE_MAP_TILE_FONT_GLYPH_PLUS,
   SPRITE_MAP_TILE_FONT_COPYRIGHT,
   SPRITE_MAP_TILE_FONT_ARROW_RIGHT,
   SPRITE_MAP_TILE_GAME_OVER_FILL,
   SPRITE_MAP_TILE_HEART,
   SPRITE_MAP_TILE_SPEAKER,
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
   GAME_STATE_SPLASH,
   GAME_STATE_INPUT_MAPPING,
   GAME_STATE_TITLE,
   GAME_STATE_GAME_MUSIC_CONFIG,
   GAME_STATE_NEW_GAME,
   GAME_STATE_CONTROL,
   GAME_STATE_PLACE,
   GAME_STATE_REMOVE_LINES,
   GAME_STATE_CONSOLIDATE_PLAY_FIELD,
   GAME_STATE_RESPAWN,
   GAME_STATE_GAME_OVER,
   GAME_STATE_GAME_OVER_TRANSITION_FILL,
   GAME_STATE_GAME_OVER_TRANSITION_CLEAR,
   GAME_STATE_PAUSE,
   GAME_STATE_QUIT,
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
   struct font_render_s * font_render;
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

   return help_tex_sprite_render_tinted(
      help_sprite_map_sprite_for(help_engine_get_sprite_map(engine), tile_type),
      x,
      y,
      help_engine_get_tex_sprites(engine),
      help_engine_get_tex_virtual(engine),
      false,
      color_rgba_make_rgba(0, 0, 0, 0xFF)
   );
}

// Helpers - Play field
#define PLAY_FIELD_TILE_SIZE (8)
#define PLAY_FIELD_WIDTH (10)
#define PLAY_FIELD_HEIGHT (18)
#define PLAY_FIELD_OFFSET_HORI_TILES (1)
#define PLAY_FIELD_OFFSET_HORI_PIXELS (PLAY_FIELD_OFFSET_HORI_TILES * PLAY_FIELD_TILE_SIZE)

bool help_render_engine_sprite_at_tile(struct engine_s * engine, int tile_x, int tile_y, enum sprite_map_tile_e tile_type)
{
   return help_render_engine_sprite(engine, tile_x * PLAY_FIELD_TILE_SIZE, tile_y * PLAY_FIELD_TILE_SIZE, tile_type);
}

bool help_render_engine_sprite_tinted(struct engine_s * engine, int x, int y, enum sprite_map_tile_e tile_type, bool do_tint, color_rgba_t tint)
{
   if (NULL == engine) return false;

   return help_tex_sprite_render_tinted(
      help_sprite_map_sprite_for(help_engine_get_sprite_map(engine), tile_type),
      x,
      y,
      help_engine_get_tex_sprites(engine),
      help_engine_get_tex_virtual(engine),
      do_tint,
      tint
   );
}

struct play_field_s {
   struct play_field_cell_s cells[PLAY_FIELD_WIDTH][PLAY_FIELD_HEIGHT];
};

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

struct play_field_s help_play_field_make_non_occupied(void)
{
   struct play_field_s fresh_play_field;

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

bool help_play_field_set_cell(struct play_field_s * play_field, int x, int y, struct play_field_cell_s new_cell)
{
   if (help_play_field_coords_out_of_bounds(play_field, x, y)) return false;

   play_field->cells[x][y] = new_cell;

   return true;
}

bool help_play_field_access_cell(struct play_field_s * play_field, int x, int y, struct play_field_cell_s * out_cell)
{
   if (NULL == play_field || NULL == out_cell || help_play_field_coords_out_of_bounds(play_field, x, y)) return false;

   *out_cell = play_field->cells[x][y];

   return true;
}

bool help_play_field_clear_row(struct play_field_s * play_field, int row)
{
   if (NULL == play_field) return false;

   bool row_cleared = true;
   for (int col = 0; col < PLAY_FIELD_WIDTH; ++col)
   {
      const bool CELL_CLEARED = help_play_field_set_cell(play_field, col, row, help_play_field_cell_make_non_occupied());
      if (false == CELL_CLEARED)
      {
         row_cleared = false;
      }
   }

   return row_cleared;
}

bool help_play_field_clear(struct play_field_s * play_field)
{
   if (NULL == play_field) return false;

   for (int row = 0; row < PLAY_FIELD_HEIGHT; ++row)
   {
      help_play_field_clear_row(play_field, row);
   }

   return true;
}

bool help_play_field_row_empty(struct play_field_s * play_field, int row)
{
   if (
      NULL == play_field ||
      row < 0 ||
      row >= PLAY_FIELD_HEIGHT
   ) return true;

   int occupied_cell_count = 0;
   for (int col = 0; col < PLAY_FIELD_WIDTH; ++col)
   {
      if (true == play_field->cells[col][row].occupied)
      {
         ++occupied_cell_count;
      }
   }

   return (0 == occupied_cell_count);
}

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

void help_play_field_render_to_texture(struct play_field_s * play_field, struct engine_s * engine)
{
   if (NULL == play_field || NULL == engine) return;

   // Render with play field offset
   for (int y = 0; y < PLAY_FIELD_HEIGHT; ++y)
   {
      for (int x = 0; x < PLAY_FIELD_WIDTH; ++x)
      {
         /*
         // Render background
         help_texture_rgba_plot_aabb(
            help_engine_get_tex_virtual(engine),
            PLAY_FIELD_OFFSET_HORI_PIXELS + (x * PLAY_FIELD_TILE_SIZE),
            y * PLAY_FIELD_TILE_SIZE,
            PLAY_FIELD_TILE_SIZE,
            PLAY_FIELD_TILE_SIZE,
            color_rgba_make_rgba(0, 50, 0, 0xFF)
         );
         */

         // Render placed cell by type
         const struct play_field_cell_s * CELL = &play_field->cells[x][y];
         if (false == CELL->occupied)
         {
            // Don't render non-occupied cells
            continue;
         }

         // Cell occupied
         help_render_engine_sprite(
            engine,
            PLAY_FIELD_OFFSET_HORI_PIXELS + (x * PLAY_FIELD_TILE_SIZE),
            y * PLAY_FIELD_TILE_SIZE,
            help_tetro_type_to_sprite_tile(CELL->type)
         );
      }
   }
}

bool help_play_field_consolidate(struct play_field_s * play_field)
{
   if (NULL == play_field) return false;

   struct play_field_s play_field_consolidated = help_play_field_make_non_occupied();

   int row_new = 0;
   for (int row_old = 0; row_old < PLAY_FIELD_HEIGHT; ++row_old)
   {
      // Ignore empty rows
      if (help_play_field_row_empty(play_field, row_old))
      {
         continue;
      }

      // Copy non-empty row to new play field
      for (int col = 0; col < PLAY_FIELD_WIDTH; ++col)
      {
         // TODO-GS: Safe access and setting of cells
         struct play_field_cell_s cell_old;
         if (help_play_field_access_cell(play_field, col, row_old, &cell_old))
         {
            help_play_field_set_cell(&play_field_consolidated, col, row_new, cell_old);
         }
      }
      
      // To next row
      ++row_new;
   }

   // Replace old with new play field
   memcpy(play_field->cells, play_field_consolidated.cells, sizeof(play_field_consolidated));

   // Success
   return true;
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

struct tetro_world_s help_tetro_world_clone_at_position(const struct tetro_world_s * tetro, int x, int y)
{
   struct tetro_world_s clone = *tetro;

   clone.tile_pos.x = x;
   clone.tile_pos.y = y;

   return clone;
}

// Helpers - Rendering stuff
void help_tetro_render_to_texture_at_tile_without_position(struct tetro_world_s * tetro, struct engine_s * engine, int tile_x, int tile_y)
{
   if (NULL == tetro || NULL == engine) return;

   // Render tetro at tile position
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
               (tile_x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE),
               (tile_y* PLAY_FIELD_TILE_SIZE) + (ty * PLAY_FIELD_TILE_SIZE),
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
               (tile_x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE),
               (tile_y* PLAY_FIELD_TILE_SIZE) + (ty * PLAY_FIELD_TILE_SIZE),
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
               (tile_x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE) + INSET,
               (tile_y* PLAY_FIELD_TILE_SIZE) + (ty * PLAY_FIELD_TILE_SIZE) + INSET,
               PLAY_FIELD_TILE_SIZE - (2 * INSET),
               PLAY_FIELD_TILE_SIZE - (2 * INSET),
               color_rgba_make_rgba(0, 150, 0, 255)
            );
         }
      }
   }
}

void help_tetro_render_to_texture(struct tetro_world_s * tetro, struct engine_s * engine)
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
               PLAY_FIELD_OFFSET_HORI_PIXELS + (tetro->tile_pos.x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE),
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
               PLAY_FIELD_OFFSET_HORI_PIXELS + (tetro->tile_pos.x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE),
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
               PLAY_FIELD_OFFSET_HORI_PIXELS + (tetro->tile_pos.x * PLAY_FIELD_TILE_SIZE) + (tx * PLAY_FIELD_TILE_SIZE) + INSET,
               (tetro->tile_pos.y * PLAY_FIELD_TILE_SIZE) + (ty * PLAY_FIELD_TILE_SIZE) + INSET,
               PLAY_FIELD_TILE_SIZE - (2 * INSET),
               PLAY_FIELD_TILE_SIZE - (2 * INSET),
               color_rgba_make_rgba(0, 150, 0, 255)
            );
         }
      }
   }
}

// Helpers - Tetro gameplay
bool help_tetro_move_collides(const struct tetro_world_s * TETRO, struct play_field_s * play_field, int dx, int dy)
{
   if (NULL == TETRO || NULL == play_field) return true;

   const struct tetro_world_s TETRO_MOVED = help_tetro_world_clone_at_position(TETRO, TETRO->tile_pos.x + dx, TETRO->tile_pos.y + dy);

   // Does any tetro design cell overlap an occupied play field cell ?
   for (int ty = 0; ty < TETRO_MOVED.data.size; ++ty)
   {
      for (int tx = 0; tx < TETRO_MOVED.data.size; ++tx)
      {
         // Ignore non-plotted design cells
         const bool DESIGN_CELL_PLOTTED = TETRO_MOVED.data.design[tx][ty];
         if (false == DESIGN_CELL_PLOTTED)
         {
            continue;
         }

         // Out-of play field bounds is considered a collision
         const struct vec_2i_s TETRO_TILE_CELL_POS = vec_2i_make_xy(TETRO_MOVED.tile_pos.x + tx, TETRO_MOVED.tile_pos.y + ty);
         if (help_play_field_coords_out_of_bounds(play_field, TETRO_TILE_CELL_POS.x, TETRO_TILE_CELL_POS.y))
         {
            return true;
         }

         // Overlapping occupied play field cell is considered a collision
         if (play_field->cells[TETRO_TILE_CELL_POS.x][TETRO_TILE_CELL_POS.y].occupied)
         {
            return true;
         }
      }
   }

   // No collision detected
   return false;
}

enum rotation_e {
   ROTATION_CW,
   ROTATION_CCW
};

bool help_tetro_rotation_collides(const struct tetro_world_s * TETRO, struct play_field_s * play_field, enum rotation_e rotation)
{
   if (NULL == TETRO || NULL == play_field) return true;

   // Does any tetro design cell overlap an occupied play field cell ?
   for (int ty = 0; ty < TETRO->data.size; ++ty)
   {
      for (int tx = 0; tx < TETRO->data.size; ++tx)
      {
         // Ignore non-plotted rotation cells
         const bool ROTATION_CELL_PLOTTED = (ROTATION_CW == rotation) ? TETRO->data.right[tx][ty] : TETRO->data.left[tx][ty];
         if (false == ROTATION_CELL_PLOTTED)
         {
            continue;
         }

         // Out-of play field bounds is considered a collision
         const struct vec_2i_s TETRO_TILE_CELL_POS = vec_2i_make_xy(TETRO->tile_pos.x + tx, TETRO->tile_pos.y + ty);
         if (help_play_field_coords_out_of_bounds(play_field, TETRO_TILE_CELL_POS.x, TETRO_TILE_CELL_POS.y))
         {
            return true;
         }

         // Overlapping occupied play field cell is considered a collision
         struct play_field_cell_s corresp_field_cell;
         if (help_play_field_access_cell(play_field, TETRO_TILE_CELL_POS.x, TETRO_TILE_CELL_POS.y, &corresp_field_cell))
         {
            if (corresp_field_cell.occupied)
            {
               // Rotation collides with play field
               return true;
            }
         }
      }
   }

   // No collision detected
   return false;
}

bool help_tetro_drop(const struct tetro_world_s * TETRO, struct play_field_s * play_field, int * out_plot_row_min, int * out_plot_row_max)
{
   if (NULL == TETRO || NULL == play_field || NULL == out_plot_row_min || NULL == out_plot_row_max) return false;

   // Keep track of play field region plotted for line deletion
   int plot_row_min = 0;
   int plot_row_max = PLAY_FIELD_HEIGHT - 1;

   // Plotting
   for (int ty = 0; ty < TETRO->data.size; ++ty)
   {
      for (int tx = 0; tx < TETRO->data.size; ++tx)
      {
         // Ignore non-plotted design cells
         const bool DESIGN_CELL_PLOTTED = TETRO->data.design[tx][ty];
         if (false == DESIGN_CELL_PLOTTED)
         {
            continue;
         }

         // Ignore cells out of play field
         const struct vec_2i_s TETRO_TILE_CELL_POS = vec_2i_make_xy(TETRO->tile_pos.x + tx, TETRO->tile_pos.y + ty);
         const bool DESIGN_CELL_OUT_OF_PLAY_FIELD_BOUNDS = help_play_field_coords_out_of_bounds(play_field, TETRO_TILE_CELL_POS.x, TETRO_TILE_CELL_POS.y);
         if (DESIGN_CELL_OUT_OF_PLAY_FIELD_BOUNDS)
         {
            continue;
         }

         // Overlapping occupied play field cell is considered a collision
         const bool PLAY_FIELD_CELL_OCCUPIED = play_field->cells[TETRO_TILE_CELL_POS.x][TETRO_TILE_CELL_POS.y].occupied;
         if (PLAY_FIELD_CELL_OCCUPIED)
         {
            // This should never happen
            printf("\nPlotting on occupied play field cell!");
            continue;
         }

         // Plot design cell into the play field
         play_field->cells[TETRO_TILE_CELL_POS.x][TETRO_TILE_CELL_POS.y] = help_play_field_cell_make(TETRO->data.type, true);

         // Keep track of plotted row range
         if (TETRO_TILE_CELL_POS.y < plot_row_max)
         {
            plot_row_max = TETRO_TILE_CELL_POS.y;
         }
         if (TETRO_TILE_CELL_POS.y > plot_row_min)
         {
            plot_row_min = TETRO_TILE_CELL_POS.y;
         }
      }
   }

   // Communicate plot region
   *out_plot_row_min = plot_row_max;
   *out_plot_row_max = plot_row_min;

   // Success
   return true;
}

// Helpers - Lists
struct list_of_rows_s {
   int list[PLAY_FIELD_HEIGHT];
   int count;
};

struct list_of_rows_s list_of_rows_make_empty(void)
{
   struct list_of_rows_s lor;

   lor.count = 0;

   return lor;
}

bool list_of_rows_append(struct list_of_rows_s * lor, int row)
{
   if (NULL == lor || lor->count >= PLAY_FIELD_HEIGHT) return false;

   lor->list[lor->count++] = row;

   return true;
}

// Helpers - Line deletion
struct list_of_rows_s help_play_field_list_of_full_rows(struct play_field_s * play_field)
{
   struct list_of_rows_s lor = list_of_rows_make_empty();
   if (NULL == play_field) return lor;

   for (int row = 0; row < PLAY_FIELD_HEIGHT; ++row)
   {
      // Row full ?
      int occupied_columns = 0;
      for (int col = 0; col < PLAY_FIELD_WIDTH; ++col)
      {
         if (play_field->cells[col][row].occupied)
         {
            ++occupied_columns;
         }
      }

      if (PLAY_FIELD_WIDTH == occupied_columns)
      {
         const bool ROW_ADDED = list_of_rows_append(&lor, row);
         if (false == ROW_ADDED)
         {
            // This should never occur
            printf("\nFull play field row %d could not be added to full list of rows !", row);
         }
      }
   }

   return lor;
}

// Helpers - Font rendering
struct font_render_glyph_s {
   bool is_mapped;
   enum sprite_map_tile_e sprite_tile;
};

#define FONT_RENDER_MAX_GLYPHS (256)
struct font_render_s {
   struct font_render_glyph_s glyphs[FONT_RENDER_MAX_GLYPHS];
};

struct font_render_glyph_s font_render_glyph_make_mapped(enum sprite_map_tile_e sprite_tile)
{
   struct font_render_glyph_s glyph;

   glyph.is_mapped = true;
   glyph.sprite_tile = sprite_tile;

   return glyph;
}

bool font_render_make(struct font_render_s * instance)
{
   if (NULL == instance) return false;

   // Start with all glyphs as non-mapped
   for (int ascii_code = 0; ascii_code < FONT_RENDER_MAX_GLYPHS; ++ascii_code)
   {
      struct font_render_glyph_s * new_glyph = instance->glyphs + ascii_code;

      // Un-mapped glyphs are un-usable, so the type does not matter
      new_glyph->is_mapped = false;
   }

   return true;
}

bool help_font_render_ascii_code_in_valid(int ascii_code)
{
   return (ascii_code < 0 || ascii_code >= FONT_RENDER_MAX_GLYPHS) ? true : false;
}

bool help_font_render_map_ascii_to_sprite(struct font_render_s * instance, char character, enum sprite_map_tile_e sprite_tile)
{
   const int ASCII_CODE = (int)character;
   if (NULL == instance | help_font_render_ascii_code_in_valid(ASCII_CODE)) return false;

   instance->glyphs[ASCII_CODE] = font_render_glyph_make_mapped(sprite_tile);

   return true;
}

bool help_font_render_char_mapped(struct font_render_s * instance, char character)
{
   return instance && instance->glyphs[(int)character].is_mapped;
}

// Additional engine based rendering
bool help_engine_render_tinted_text_at_tile_internal(struct engine_s * engine, const char * text, int tile_x, int tile_y, bool do_tint, color_rgba_t tint)
{
   if (NULL == engine || NULL == text) return false;

   struct vec_2i_s tile_cursor = vec_2i_make_xy(tile_x, tile_y);

   for (int i_char = 0; i_char < strlen(text); ++i_char)
   {
      const char TEXT_CHAR_UPPERCASED = toupper(text[i_char]);

      if ('\n' == TEXT_CHAR_UPPERCASED)
      {
         // Newline
         tile_cursor.x = tile_x;
         tile_cursor.y -= 1;
      }
      else if ('\t' == TEXT_CHAR_UPPERCASED)
      {
         // Horizontal tab
         const int SPACES_PER_TAB = 1;
         tile_cursor.x += SPACES_PER_TAB;
      }
      else if (' ' == TEXT_CHAR_UPPERCASED)
      {
         // Space
         tile_cursor.x += 1;
      }
      else if (help_font_render_char_mapped(engine->font_render, TEXT_CHAR_UPPERCASED))
      {
         // Renderable font character
         const int CHAR_ASCII = (int)TEXT_CHAR_UPPERCASED;
         help_render_engine_sprite_tinted(
            engine,
            tile_cursor.x * PLAY_FIELD_TILE_SIZE,
            tile_cursor.y * PLAY_FIELD_TILE_SIZE,
            engine->font_render->glyphs[CHAR_ASCII].sprite_tile,
            do_tint,
            tint
         );

         tile_cursor.x += 1;
      }
      else
      {
         // Not supported
         printf("\nCannot render un-supported ascii glyph '%d'", (int)TEXT_CHAR_UPPERCASED);

         // Render debug thing
         help_render_engine_sprite(
            engine,
            tile_cursor.x * PLAY_FIELD_TILE_SIZE,
            tile_cursor.y * PLAY_FIELD_TILE_SIZE,
            SPRITE_MAP_TILE_NA
         );

         tile_cursor.x += 1;
      }
   }

   // Success
   return true;
}

bool help_engine_render_text_at_tile(struct engine_s * engine, const char * text, int tile_x, int tile_y)
{
   return help_engine_render_tinted_text_at_tile_internal(engine, text, tile_x, tile_y, false, color_rgba_make_rgba(0xFF, 0, 0xFF, 0xFF));
}

bool help_engine_render_tinted_text_at_tile(struct engine_s * engine, const char * text, int tile_x, int tile_y, color_rgba_t tint)
{
   return help_engine_render_tinted_text_at_tile_internal(engine, text, tile_x, tile_y, true, tint);
}

// Helpers - Levels
struct score_level_mapping_s {
   int threshold_score;
   int mapped_level;
};

struct score_level_mapping_s score_level_mapping_make(int threshold_score, int mapped_level)
{
   struct score_level_mapping_s mapping;

   mapping.threshold_score = threshold_score;
   mapping.mapped_level = mapped_level;

   return mapping;
}

// Helpers - Limits
float help_limit_clamp_f(float floor, float value, float ceiling)
{
   if (value < floor)
   {
      return floor;
   }
   else if (value > ceiling)
   {
      return ceiling;
   }
   else
   {
      return value;
   }
}

int help_limit_clamp_i(int floor, int value, int ceiling)
{
   if (value < floor)
   {
      return floor;
   }
   else if (value > ceiling)
   {
      return ceiling;
   }
   else
   {
      return value;
   }
}

// Helpers - Audio
typedef int audio_mixer_sample_id_t;
const audio_mixer_sample_id_t AUDIO_MIXER_SAMPLE_ID_INVALID = -1;

struct sdl_audio_data_s {
   SDL_AudioSpec spec;
   Uint8 * data;
   Uint32 length;
};

struct audio_mixer_sample_s {
   struct sdl_audio_data_s * audio;
   bool active;
   Uint32 playback_position;
   bool is_music;
   bool loop_music;
};

#define AUDIO_MIXER_MAX_SAMPLE_STORE_COUNT (64)
#define AUDIO_MIXER_MAX_SAMPLE_QUEUED_COUNT (64)

struct audio_mixer_s {
   // Playback
   SDL_AudioDeviceID playback_device_id;
   SDL_AudioStream * playback_stream;
   // Audio sample source
   struct sdl_audio_data_s samples_store[AUDIO_MIXER_MAX_SAMPLE_STORE_COUNT];
   int samples_store_count;
   // Queued samples
   struct audio_mixer_sample_s samples_queued[AUDIO_MIXER_MAX_SAMPLE_QUEUED_COUNT];
   // Channel volume
   float volume_music;
   float volume_sfx;
   // Channel pause
   bool pause_music;
   bool pause_sfx;
};

void * audio_mixer_destroy(struct audio_mixer_s * instance)
{
   if (instance)
   {
      // Converted samples
      for (int i = 0; i < instance->samples_store_count; ++i)
      {
         SDL_free(instance->samples_store[i].data);
      }

      // Unbind device and stream
      SDL_UnbindAudioStream(instance->playback_stream);

      // Playback stream
      SDL_DestroyAudioStream(instance->playback_stream);

      // Playback audio device
      SDL_CloseAudioDevice(instance->playback_device_id);

      // Instance
      free(instance);
   }

   return NULL;
}

struct SDL_AudioSpec audio_mixer_sdl_audio_spec_make_desired(SDL_AudioFormat desired_format, int desired_channels, int desired_frequency)
{
   struct SDL_AudioSpec desired_spec;

   desired_spec.format = desired_format;
   desired_spec.channels = desired_channels;
   desired_spec.freq = desired_frequency;

   return desired_spec;
}

void audio_mixer_sdl_audio_spec_log(struct SDL_AudioSpec spec)
{
   printf("Format: %#x | Frequency: %d | Channels: %d", spec.format, spec.freq, spec.channels);
}

struct audio_mixer_s * audio_mixer_create(SDL_AudioStreamCallback mixer_callback)
{
   struct audio_mixer_s * instance = malloc(sizeof(struct audio_mixer_s));
   if (NULL == instance) return NULL;

   // Zero instance
   instance->playback_device_id = 0;
   instance->playback_stream = NULL;
   instance->samples_store_count = 0;
   instance->volume_music = 0.25f;
   instance->volume_sfx = 0.25f;
   instance->pause_music = false;
   instance->pause_sfx = false;
   for (int i = 0; i < AUDIO_MIXER_MAX_SAMPLE_QUEUED_COUNT; ++i)
   {
      struct audio_mixer_sample_s * sample = instance->samples_queued + i;
      sample->audio = NULL;
      sample->active = false;
   }

   // Open playback audio device
   instance->playback_device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
   if (0 == instance->playback_device_id)
   {
      printf("\nFailed to open playback audio device - Error: %s", SDL_GetError());
      return audio_mixer_destroy(instance);
   }

   // Create playback stream
   const struct SDL_AudioSpec SPEC_PLAYBACK_STREAM_IN = audio_mixer_sdl_audio_spec_make_desired(SDL_AUDIO_F32, 2, 44100);
   instance->playback_stream = SDL_CreateAudioStream(&SPEC_PLAYBACK_STREAM_IN, NULL);
   if (NULL == instance->playback_stream)
   {
      printf("\nFailed to create playback stream with desired spec - Error: %s", SDL_GetError());
      return audio_mixer_destroy(instance);
   }

   // Bind stream to device
   if (false == SDL_BindAudioStream(instance->playback_device_id, instance->playback_stream))
   {
      printf("\nFailed to bind playback device with playback stream - Error: %s", SDL_GetError());
      return audio_mixer_destroy(instance);
   }

   // Set audio stream mixing callback
   if (false == SDL_SetAudioStreamGetCallback(instance->playback_stream, mixer_callback, instance))
   {
      printf("\nFailed to set audio stream mixing callback - Error: %s", SDL_GetError());
      return audio_mixer_destroy(instance);
   }

   // Log audio mixer attributes
   printf("\nAudio mixer created successfully:");   
   printf("\n\tPlayback device id  : %u", instance->playback_device_id);
   printf("\n\tPlayback stream spec: ");
   audio_mixer_sdl_audio_spec_log(SPEC_PLAYBACK_STREAM_IN);

   // Success
   return instance;
}

bool audio_mixer_samples_full(struct audio_mixer_s * instance)
{
   return (
      NULL == instance ||
      instance->samples_store_count < 0 ||
      instance->samples_store_count >= AUDIO_MIXER_MAX_SAMPLE_STORE_COUNT
   ) ? true : false;
}

audio_mixer_sample_id_t audio_mixer_register_WAV(struct audio_mixer_s * instance, const char * path)
{
   if (NULL == instance || NULL == path || audio_mixer_samples_full(instance))
   {
      return AUDIO_MIXER_SAMPLE_ID_INVALID;
   }

   // Load WAV from file
   struct sdl_audio_data_s wav;
   if (false == SDL_LoadWAV(path, &wav.spec, &wav.data, &wav.length))
   {
      printf("\nFailed to load WAV from path [%s] - Error: %s", path, SDL_GetError());
      return AUDIO_MIXER_SAMPLE_ID_INVALID;
   }

   // Convert to floating point samples for custom mixing
   struct sdl_audio_data_s converted;
   converted.spec = audio_mixer_sdl_audio_spec_make_desired(SDL_AUDIO_F32, 2, 44100);
   if (false == SDL_ConvertAudioSamples(&wav.spec, wav.data, wav.length, &converted.spec, &converted.data, &converted.length))
   {
      SDL_free(wav.data);
      printf("\nFailed to convert loaded WAV to F32 samples - Error: %s", SDL_GetError());
      return AUDIO_MIXER_SAMPLE_ID_INVALID;
   }

   // Register in samples
   const int ID = instance->samples_store_count++;
   instance->samples_store[ID] = converted;

   // Log loaded
   const int DL = 15;
   printf("\n\nLoaded WAV [%s]:", path);
   printf("\n\t%-*s: ", DL, "File spec");
   audio_mixer_sdl_audio_spec_log(wav.spec);
   printf("\n\t%-*s: ", DL, "Conversion spec");
   audio_mixer_sdl_audio_spec_log(converted.spec);

   // Success
   return ID;
}

bool audio_mixer_sample_id_in_valid(struct audio_mixer_s * instance, audio_mixer_sample_id_t id)
{
   return id <= AUDIO_MIXER_SAMPLE_ID_INVALID;
}

struct audio_mixer_sample_s * audio_mixer_access_vacant_sample(struct audio_mixer_s * instance)
{
   if (NULL == instance) return NULL;

   // TODO-GS: Find a faster solution ?
   for (int i = 0; i < AUDIO_MIXER_MAX_SAMPLE_QUEUED_COUNT; ++i)
   {
      struct audio_mixer_sample_s * const SAMPLE = instance->samples_queued + i;
      if (false == SAMPLE->active)
      {
         return SAMPLE;
      }
   }

   // No vacant sample slot found
   return NULL;
}

bool audio_mixer_queue_sample(struct audio_mixer_s * instance, audio_mixer_sample_id_t id, bool is_music, bool loop_music)
{
   if (NULL == instance || audio_mixer_sample_id_in_valid(instance, id)) return false;

   // Slot for another concurrent playback sample ?
   struct audio_mixer_sample_s * sample = audio_mixer_access_vacant_sample(instance);
   if (NULL == sample){
      return false;
   }

   // Update sample
   sample->audio = instance->samples_store + id;

   // Activate sample and forward required state
   sample->active = true;
   sample->playback_position = 0;
   sample->is_music = is_music;
   sample->loop_music = loop_music;

   // Success ?
   return sample->active;
}

bool audio_mixer_queue_sample_music(struct audio_mixer_s * instance, audio_mixer_sample_id_t id, bool loop_music)
{
   return audio_mixer_queue_sample(instance, id, true, loop_music);
}

bool audio_mixer_queue_sample_sfx(struct audio_mixer_s * instance, audio_mixer_sample_id_t id)
{
   return audio_mixer_queue_sample(instance, id, false, false);
}

const char * audio_mixer_build_res_path(const char * dir_abs_res, const char * category, const char * filename)
{
   static char path[4096];

   snprintf(path, sizeof(path), "%s\\audio\\%s\\%s.wav", dir_abs_res, category, filename);

   return path;
}

void audio_mixer_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
   if (NULL == userdata)
   {
      return;
   }

   // Access user data
   struct audio_mixer_s * audio_mixer = (struct audio_mixer_s *)userdata;

   // Allocate mix buffer
   // @Warning: amounts are int's i.e. can be negative -> int to uint conversion wrapping hazard
   const Uint32 SAMPLE_BYTES_REQUIRED = total_amount;
   float * float_mix = malloc(SAMPLE_BYTES_REQUIRED);
   if (NULL == float_mix)
   {
      printf("\nFailed to allocated [%u] bytes for audio float mix buffer", SAMPLE_BYTES_REQUIRED);
      return;
   }

   // Initialize mix to silence
   memset(float_mix, 0, SAMPLE_BYTES_REQUIRED);

   // Mix active audio samples
   for (int i_sample = 0; i_sample < AUDIO_MIXER_MAX_SAMPLE_QUEUED_COUNT; ++i_sample)
   {
      // Skip in-active samples
      struct audio_mixer_sample_s * const sample = audio_mixer->samples_queued + i_sample;
      if (false == sample->active)
      {
         continue;
      }

      // Sample paused ?
      const bool MUSIC_PAUSED = sample->is_music && audio_mixer->pause_music;
      const bool SFX_PAUSED = (false == sample->is_music) && audio_mixer->pause_sfx;
      if (MUSIC_PAUSED || SFX_PAUSED)
      {
         continue;
      }

      // How much sample data to process ?
      const Uint32 SAMPLE_PLAYBACK_BYTES_LEFT = sample->audio->length - sample->playback_position;
      const Uint32 SAMPLE_BYTES_TO_PROCESS = SDL_min(SAMPLE_PLAYBACK_BYTES_LEFT, SAMPLE_BYTES_REQUIRED);

      // Determine channel volume
      const float SAMPLE_VOLUME = sample->is_music ? audio_mixer->volume_music : audio_mixer->volume_sfx;

      // Add up samples
      const Uint32 FLOAT_SAMPLE_MIX_COUNT = SAMPLE_BYTES_TO_PROCESS / sizeof(float);
      const float * FLOAT_SAMPLE_DATA_PLAYBACK = (float *)(sample->audio->data + sample->playback_position);
      for (Uint32 i_sample_amplitude = 0; i_sample_amplitude < FLOAT_SAMPLE_MIX_COUNT; ++i_sample_amplitude)
      {
         const float LAST_MIX_FLOAT_SAMPLE = float_mix[i_sample_amplitude];
         const float MIXED_FLOAT_SAMPLE = LAST_MIX_FLOAT_SAMPLE + (FLOAT_SAMPLE_DATA_PLAYBACK[i_sample_amplitude] * SAMPLE_VOLUME);
         float_mix[i_sample_amplitude] = help_limit_clamp_f(-1.0f, MIXED_FLOAT_SAMPLE, 1.0f);
      }

      // Advance sample playback
      sample->playback_position += SAMPLE_BYTES_TO_PROCESS;

      // Sample played until the end ?
      if (sample->playback_position >= sample->audio->length)
      {
         sample->playback_position = 0;

         if (sample->is_music)
         {
            // Music sample
            sample->active = sample->loop_music;
         }
         else
         {
            // Sfx sample
            sample->active = false;
         }
      }
   }

   // Queue float mix
   if (false == SDL_PutAudioStreamData(stream, float_mix, SAMPLE_BYTES_REQUIRED))
   {
      printf("\nFailed to put audio float mix into stream - Error: %s", SDL_GetError());
   }

   // Cleanup
   free(float_mix);
}

bool audio_mixer_set_volume_music(struct audio_mixer_s * instance, float new_volume, float * out_new_volume)
{
   if (NULL == instance)
   {
       return false;
   }

   instance->volume_music = help_limit_clamp_f(0.0f, new_volume, 1.0f);

   if (out_new_volume)
   {
      *out_new_volume = instance->volume_music;
   }

   return true;
}

bool audio_mixer_set_volume_sfx(struct audio_mixer_s * instance, float new_volume, float * out_new_volume)
{
   if (NULL == instance)
   {
       return false;
   }

   instance->volume_sfx = help_limit_clamp_f(0.0f, new_volume, 1.0f);

   if (out_new_volume)
   {
      *out_new_volume = instance->volume_music;
   }

   return true;
}

bool audio_mixer_get_volume_music(struct audio_mixer_s * instance, float * out_volume)
{
   if (NULL == instance) return false;

   *out_volume = instance->volume_music;
   return true;
}

bool audio_mixer_get_volume_sfx(struct audio_mixer_s * instance, float * out_volume)
{
   if (NULL == instance) return false;

   *out_volume = instance->volume_sfx;
   return true;
}

bool audio_mixer_adjust_volume_music_by(struct audio_mixer_s * instance, float amount, float * out_adjusted)
{
   float current_music_volume;
   if (audio_mixer_get_volume_music(instance, &current_music_volume))
   {
      return audio_mixer_set_volume_music(instance, current_music_volume + amount, out_adjusted);
   }

   return false;
}

bool audio_mixer_adjust_volume_sfx_by(struct audio_mixer_s * instance, float amount, float * out_adjusted)
{
   float current_sfx_volume;
   if (audio_mixer_get_volume_sfx(instance, &current_sfx_volume))
   {
      return audio_mixer_set_volume_sfx(instance, current_sfx_volume + amount, out_adjusted);
   }

   return false;
}

bool audio_mixer_increase_volume_music_and_sfx_by(struct audio_mixer_s * instance, float amount, float * out_adjusted_music, float * out_adjusted_sfx)
{
   return audio_mixer_adjust_volume_music_by(instance, amount, out_adjusted_music) && audio_mixer_adjust_volume_sfx_by(instance, amount, out_adjusted_sfx);
}

bool audio_mixer_pause_music(struct audio_mixer_s * instance)
{
   if (NULL == instance) return false;

   instance->pause_music = true;

   return true;
}

bool audio_mixer_resume_music(struct audio_mixer_s * instance)
{
   if (NULL == instance) return false;

   instance->pause_music = false;

   return true;
}

bool audio_mixer_pause_sfx(struct audio_mixer_s * instance)
{
   if (NULL == instance) return false;

   instance->pause_sfx = true;

   return true;
}

bool audio_mixer_resume_sfx(struct audio_mixer_s * instance)
{
   if (NULL == instance) return false;

   instance->pause_sfx = false;

   return true;
}

bool audio_mixer_resume_music_and_sfx(struct audio_mixer_s * instance)
{
   return audio_mixer_resume_music(instance) && audio_mixer_resume_sfx(instance);
}

bool audio_mixer_stop_music(struct audio_mixer_s * instance)
{
   if (NULL == instance) return false;

   // De-activate all music samples
   for (int i_sample = 0; i_sample < AUDIO_MIXER_MAX_SAMPLE_QUEUED_COUNT; ++i_sample)
   {
      struct audio_mixer_sample_s * const sample = instance->samples_queued + i_sample;
      const bool SAMPLE_IS_MUSIC = sample->is_music;
      if (SAMPLE_IS_MUSIC)
      {
         sample->active = false;
      }
   }

   return true;
}

bool audio_mixer_stop_sfx(struct audio_mixer_s * instance)
{
   if (NULL == instance) return false;

   // De-activate all music samples
   for (int i_sample = 0; i_sample < AUDIO_MIXER_MAX_SAMPLE_QUEUED_COUNT; ++i_sample)
   {
      struct audio_mixer_sample_s * const sample = instance->samples_queued + i_sample;
      const bool SAMPLE_IS_SFX = (false == sample->is_music);
      if (SAMPLE_IS_SFX)
      {
         sample->active = false;
      }
   }

   return true;
}

bool audio_mixer_stop_music_and_sfx(struct audio_mixer_s * instance)
{
   return audio_mixer_stop_music(instance) && audio_mixer_stop_sfx(instance);
}

bool audio_mixer_pause_music_and_sfx(struct audio_mixer_s * instance)
{
   return audio_mixer_pause_music(instance) && audio_mixer_pause_sfx(instance);
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
   if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
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
   //SDL_SetWindowMouseGrab(sdl_window, true);
   //SDL_HideCursor();

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

   // Engine
   // ----> Sprite map
   struct sprite_map_s * sprite_map = help_sprite_map_create(tex_sprites, 13, 13, 8);
   if (NULL == sprite_map)
   {
      printf("\nFailed to create sprite map");
      return EXIT_FAILURE;
   }
   // ----> Register tetro sprites
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_I, 0, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_O, 1, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_Lr, 2, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_L, 3, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_S, 4, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_T, 5, 6);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_TETRO_BLOCK_Z, 6, 6);
   // ----> Register play field sprites
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_BRICK, 0, 4);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_HIGHLIGHT, 12, 5);   
   // ----> Register font rendering sprites
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_A, 0, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_B, 1, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_C, 2, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_D, 3, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_E, 4, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_F, 5, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_G, 6, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_H, 7, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_I, 8, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_J, 9, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_K, 10, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_L, 11, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_M, 12, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_N, 13, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_O, 14, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_P, 15, 0);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_Q, 0, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_R, 1, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_S, 2, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_T, 3, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_U, 4, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_V, 5, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_W, 6, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_X, 7, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_Y, 8, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_Z, 9, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_0, 0, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_1, 1, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_2, 2, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_3, 3, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_4, 4, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_5, 5, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_6, 6, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_7, 7, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_8, 8, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_9, 9, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_PERIOD, 10, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_HYPHEN, 11, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_COMMA, 12, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_COLON, 15, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_PARENTHESES_OPEN, 13, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_PARENTHESES_CLOSED, 14, 1);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_PIPE, 13, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_GLYPH_PLUS, 15, 1);
   
   // >> Register additional sprites
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_COPYRIGHT, 10, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_FONT_ARROW_RIGHT, 11, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_GAME_OVER_FILL, 5, 4);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_HEART, 12, 2);
   help_sprite_map_tile(sprite_map, SPRITE_MAP_TILE_SPEAKER, 14, 2);

   // Create font render component
   struct font_render_s font_render;
   if (false == font_render_make(&font_render))
   {
      printf("\nFailed to create font render component");
      return EXIT_FAILURE;
   }
   // ----> Map font render ascii codes to renderable sprites
   help_font_render_map_ascii_to_sprite(&font_render, 'A', SPRITE_MAP_TILE_FONT_GLYPH_A);
   help_font_render_map_ascii_to_sprite(&font_render, 'B', SPRITE_MAP_TILE_FONT_GLYPH_B);
   help_font_render_map_ascii_to_sprite(&font_render, 'C', SPRITE_MAP_TILE_FONT_GLYPH_C);
   help_font_render_map_ascii_to_sprite(&font_render, 'D', SPRITE_MAP_TILE_FONT_GLYPH_D);
   help_font_render_map_ascii_to_sprite(&font_render, 'E', SPRITE_MAP_TILE_FONT_GLYPH_E);
   help_font_render_map_ascii_to_sprite(&font_render, 'F', SPRITE_MAP_TILE_FONT_GLYPH_F);
   help_font_render_map_ascii_to_sprite(&font_render, 'G', SPRITE_MAP_TILE_FONT_GLYPH_G);
   help_font_render_map_ascii_to_sprite(&font_render, 'H', SPRITE_MAP_TILE_FONT_GLYPH_H);
   help_font_render_map_ascii_to_sprite(&font_render, 'I', SPRITE_MAP_TILE_FONT_GLYPH_I);
   help_font_render_map_ascii_to_sprite(&font_render, 'J', SPRITE_MAP_TILE_FONT_GLYPH_J);
   help_font_render_map_ascii_to_sprite(&font_render, 'K', SPRITE_MAP_TILE_FONT_GLYPH_K);
   help_font_render_map_ascii_to_sprite(&font_render, 'L', SPRITE_MAP_TILE_FONT_GLYPH_L);
   help_font_render_map_ascii_to_sprite(&font_render, 'M', SPRITE_MAP_TILE_FONT_GLYPH_M);
   help_font_render_map_ascii_to_sprite(&font_render, 'N', SPRITE_MAP_TILE_FONT_GLYPH_N);
   help_font_render_map_ascii_to_sprite(&font_render, 'O', SPRITE_MAP_TILE_FONT_GLYPH_O);
   help_font_render_map_ascii_to_sprite(&font_render, 'P', SPRITE_MAP_TILE_FONT_GLYPH_P);
   help_font_render_map_ascii_to_sprite(&font_render, 'Q', SPRITE_MAP_TILE_FONT_GLYPH_Q);
   help_font_render_map_ascii_to_sprite(&font_render, 'R', SPRITE_MAP_TILE_FONT_GLYPH_R);
   help_font_render_map_ascii_to_sprite(&font_render, 'S', SPRITE_MAP_TILE_FONT_GLYPH_S);
   help_font_render_map_ascii_to_sprite(&font_render, 'T', SPRITE_MAP_TILE_FONT_GLYPH_T);
   help_font_render_map_ascii_to_sprite(&font_render, 'U', SPRITE_MAP_TILE_FONT_GLYPH_U);
   help_font_render_map_ascii_to_sprite(&font_render, 'V', SPRITE_MAP_TILE_FONT_GLYPH_V);
   help_font_render_map_ascii_to_sprite(&font_render, 'W', SPRITE_MAP_TILE_FONT_GLYPH_W);
   help_font_render_map_ascii_to_sprite(&font_render, 'X', SPRITE_MAP_TILE_FONT_GLYPH_X);
   help_font_render_map_ascii_to_sprite(&font_render, 'Y', SPRITE_MAP_TILE_FONT_GLYPH_Y);
   help_font_render_map_ascii_to_sprite(&font_render, 'Z', SPRITE_MAP_TILE_FONT_GLYPH_Z);
   help_font_render_map_ascii_to_sprite(&font_render, '0', SPRITE_MAP_TILE_FONT_GLYPH_0);
   help_font_render_map_ascii_to_sprite(&font_render, '1', SPRITE_MAP_TILE_FONT_GLYPH_1);
   help_font_render_map_ascii_to_sprite(&font_render, '2', SPRITE_MAP_TILE_FONT_GLYPH_2);
   help_font_render_map_ascii_to_sprite(&font_render, '3', SPRITE_MAP_TILE_FONT_GLYPH_3);
   help_font_render_map_ascii_to_sprite(&font_render, '4', SPRITE_MAP_TILE_FONT_GLYPH_4);
   help_font_render_map_ascii_to_sprite(&font_render, '5', SPRITE_MAP_TILE_FONT_GLYPH_5);
   help_font_render_map_ascii_to_sprite(&font_render, '6', SPRITE_MAP_TILE_FONT_GLYPH_6);
   help_font_render_map_ascii_to_sprite(&font_render, '7', SPRITE_MAP_TILE_FONT_GLYPH_7);
   help_font_render_map_ascii_to_sprite(&font_render, '8', SPRITE_MAP_TILE_FONT_GLYPH_8);
   help_font_render_map_ascii_to_sprite(&font_render, '9', SPRITE_MAP_TILE_FONT_GLYPH_9);

   help_font_render_map_ascii_to_sprite(&font_render, '.', SPRITE_MAP_TILE_FONT_GLYPH_PERIOD);
   help_font_render_map_ascii_to_sprite(&font_render, '-', SPRITE_MAP_TILE_FONT_GLYPH_HYPHEN);
   help_font_render_map_ascii_to_sprite(&font_render, ',', SPRITE_MAP_TILE_FONT_GLYPH_COMMA);
   help_font_render_map_ascii_to_sprite(&font_render, '(', SPRITE_MAP_TILE_FONT_GLYPH_PARENTHESES_OPEN);
   help_font_render_map_ascii_to_sprite(&font_render, ')', SPRITE_MAP_TILE_FONT_GLYPH_PARENTHESES_CLOSED);
   help_font_render_map_ascii_to_sprite(&font_render, '|', SPRITE_MAP_TILE_FONT_GLYPH_PIPE);
   help_font_render_map_ascii_to_sprite(&font_render, '+', SPRITE_MAP_TILE_FONT_GLYPH_PLUS);
   help_font_render_map_ascii_to_sprite(&font_render, ':', SPRITE_MAP_TILE_FONT_GLYPH_COLON);

   // Score level mappings
   struct score_level_mapping_s score_level_mapping[] = {
      score_level_mapping_make(0, 0),
      score_level_mapping_make(20, 1),
      score_level_mapping_make(40, 2),
      score_level_mapping_make(60, 3),
      score_level_mapping_make(80, 4),
      score_level_mapping_make(100, 5),
      score_level_mapping_make(120, 6),
      score_level_mapping_make(140, 7),
      score_level_mapping_make(160, 8),
      score_level_mapping_make(180, 9)
   };

   // Setup audio mixer
   struct audio_mixer_s * audio_mixer = audio_mixer_create(audio_mixer_callback);
   if (NULL == audio_mixer)
   {
      printf("\nFailed to create audio mixer");
      return EXIT_FAILURE;
   }
   // >> Register sounds for playback
   const audio_mixer_sample_id_t AMSID_MUSIC_TITLE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "music", "title"));
   const audio_mixer_sample_id_t AMSID_MUSIC_GAME_A_TYPE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "music", "a-type"));
   const audio_mixer_sample_id_t AMSID_MUSIC_GAME_B_TYPE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "music", "b-type"));
   const audio_mixer_sample_id_t AMSID_MUSIC_GAME_C_TYPE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "music", "c-type"));
   const audio_mixer_sample_id_t AMSID_MUSIC_GAME_OVER = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "music", "game-over"));
   const audio_mixer_sample_id_t AMSID_EFFECT_SPLASH = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "splash"));
   const audio_mixer_sample_id_t AMSID_EFFECT_INVALID = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "invalid"));
   const audio_mixer_sample_id_t AMSID_EFFECT_SELECT = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "select"));
   const audio_mixer_sample_id_t AMSID_EFFECT_MOVE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "move"));
   const audio_mixer_sample_id_t AMSID_EFFECT_ROTATE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "rotate"));
   const audio_mixer_sample_id_t AMSID_EFFECT_PLACE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "place"));
   const audio_mixer_sample_id_t AMSID_EFFECT_HIGHLIGHT = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "highlight"));
   const audio_mixer_sample_id_t AMSID_EFFECT_DESTROY = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "destroy"));
   const audio_mixer_sample_id_t AMSID_EFFECT_GAME_OVER = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "game-over"));
   const audio_mixer_sample_id_t AMSID_EFFECT_DROP = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "drop"));
   const audio_mixer_sample_id_t AMSID_EFFECT_BLIP = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "blip"));
   const audio_mixer_sample_id_t AMSID_EFFECT_INCREASE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "increase"));
   const audio_mixer_sample_id_t AMSID_EFFECT_DECREASE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "decrease"));
   const audio_mixer_sample_id_t AMSID_EFFECT_PAUSE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "pause"));
   const audio_mixer_sample_id_t AMSID_EFFECT_UN_PAUSE = audio_mixer_register_WAV(audio_mixer, audio_mixer_build_res_path(DIR_ABS_RES, "effects", "un-pause"));

   // Package engine components
   struct engine_s engine;
   engine.tex_virtual = tex_virtual;
   engine.tex_sprites = tex_sprites;
   engine.sprite_map = sprite_map;
   engine.font_render = &font_render;

   // Game state
   // ----> Tick state
   enum game_state_e game_state = GAME_STATE_SPLASH;
   enum game_state_e next_game_state = GAME_STATE_NONE;
   // ----> Playing field
   struct play_field_s play_field = help_play_field_make_non_occupied();
   // ----> Active tetro
   struct tetro_world_s tetro_active = help_tetro_world_make_random_at_spawn();
   struct tetro_world_s tetro_next = help_tetro_world_make_random_at_spawn();
   // ---- Timers
   double time_splash_start = help_sdl_time_in_seconds();
   double time_last_tetro_drop = help_sdl_time_in_seconds();
   double time_last_tetro_player_move = help_sdl_time_in_seconds();
   double time_last_tetro_player_drop = help_sdl_time_in_seconds();
   double time_last_removal_flash_timer = help_sdl_time_in_seconds();
   double time_last_row_deletion_timer = help_sdl_time_in_seconds();
   double time_last_sfx = help_sdl_time_in_seconds();
   // >> Game over transition
   const double TIME_SEC_GAME_OVER_TRANSITION_FILL = 1.5;
   const double TIME_SEC_GAME_OVER_TRANSITION_FILL_ROW = TIME_SEC_GAME_OVER_TRANSITION_FILL / PLAY_FIELD_HEIGHT;
   double time_last_game_over_transition_row = help_sdl_time_in_seconds();
   // ----> Shared game state data points
   // Shared game state transition data points
   int plot_row_min, plot_row_max;
   struct list_of_rows_s list_of_full_rows;
   // ----> Stats
   int stat_score = 0;
   int stat_lines = 0;
   int stat_level = 0;
   // >> Quit
   const double TIME_SEC_QUIT = 2.5;
   double time_last_quit = help_sdl_time_in_seconds();
   audio_mixer_sample_id_t configured_game_music = AMSID_MUSIC_GAME_A_TYPE;
   // >> Game type music config
   const int GAME_MUSIC_TYPE_WIDTH = 2;
   const int GAME_MUSIC_TYPE_HEIGHT = 2;
   struct vec_2i_s game_music_cursor = vec_2i_make_xy(0, 0);
   // >> Game over transition
   int game_over_transition_field_lines_filled = 0;
   int game_over_transition_field_lines_cleared = 0;
   // >> Input mapping
   bool keybr_key_confirmed[CUSTOM_KEY_COUNT];
   for (size_t keybr = 0; keybr < sizeof(keybr_key_confirmed) / sizeof(keybr_key_confirmed[0]); ++keybr)
   {
      keybr_key_confirmed[keybr] = false;
   }
   // >> Volume
   const float VOLUME_ADJUST_STEP_PER_PRESS = 0.1f;
   const double TIME_SEC_VOLUME_OVERLAY_SHOW = 1.0f;
   double time_until_show_volume_overlay = help_sdl_time_in_seconds();

   // FPS counter
   double last_time_fps = help_sdl_time_in_seconds();
   int frames_per_second = 0;

   // Integration
   const int TICKS_PER_SECOND = 50;
   const double FIXED_DELTA_TIME = 1.0 / TICKS_PER_SECOND;
   double time_simulated = 0.0;
   double last_time_tick = help_sdl_time_in_seconds();
   double fixed_delta_time_accumulator = 0.0;

   // Game loop
   bool tetris_close_requested = false;
   while (false == tetris_close_requested)
   {
      // Consume window events
      SDL_Event window_event;
      while (SDL_PollEvent(&window_event))
         ;

      // Tick
      const double NEW_TIME = help_sdl_time_in_seconds();
      const double LAST_FRAME_DURATION = help_sdl_time_in_seconds() - last_time_tick;
      last_time_tick = NEW_TIME;
      fixed_delta_time_accumulator += LAST_FRAME_DURATION;

      // Iterative fixed time step integration
      while (fixed_delta_time_accumulator >= FIXED_DELTA_TIME)
      {
         // Update input state
         help_input_determine_intermediate_state(input);

         // Tick housekeeping
         time_simulated += FIXED_DELTA_TIME;
         fixed_delta_time_accumulator -= FIXED_DELTA_TIME;


         // Tick based on game state
         if (GAME_STATE_INPUT_MAPPING != game_state)
         {
            // Volume adjustable at any time out of the input mapping screen
            if (help_input_key_pressed(input, CUSTOM_KEY_VOLUME_UP))
            {
               float adjusted_volume_music, adjusted_volume_sfx;
               if (audio_mixer_increase_volume_music_and_sfx_by(audio_mixer, VOLUME_ADJUST_STEP_PER_PRESS, &adjusted_volume_music, &adjusted_volume_sfx))
               {
                  audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_INCREASE);
                  time_until_show_volume_overlay = help_sdl_time_in_seconds() + TIME_SEC_VOLUME_OVERLAY_SHOW;
               }
            }
            if (help_input_key_pressed(input, CUSTOM_KEY_VOLUME_DOWN))
            {
               float adjusted_volume_music, adjusted_volume_sfx;
               if (audio_mixer_increase_volume_music_and_sfx_by(audio_mixer, -VOLUME_ADJUST_STEP_PER_PRESS, &adjusted_volume_music, &adjusted_volume_sfx))
               {
                  audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_DECREASE);
                  time_until_show_volume_overlay = help_sdl_time_in_seconds() + TIME_SEC_VOLUME_OVERLAY_SHOW;
               }
            }
         }
         if (GAME_STATE_SPLASH == game_state)
         {
            // SFX
            static bool init_splash = true;
            if (init_splash)
            {
               audio_mixer_stop_music_and_sfx(audio_mixer);
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_SPLASH);
               init_splash = false;
            }

            // Wait until game start or press button
            const float CONTINUE_TIME = 6.0f;
            const bool CONTINUE_TIME_PASSED = (help_sdl_time_in_seconds() >= (time_splash_start + CONTINUE_TIME));

            if (CONTINUE_TIME_PASSED || help_input_key_pressed(input, CUSTOM_KEY_START))
            {
               // Show and check game controls
               next_game_state = GAME_STATE_INPUT_MAPPING;
               audio_mixer_stop_music_and_sfx(audio_mixer);
            }
         }
         if (GAME_STATE_INPUT_MAPPING == game_state)
         {
            // Confirm input mapping
            bool valid_keybr_confirmed = false;
            if (false == keybr_key_confirmed[CUSTOM_KEY_UP] && help_input_key_pressed(input, CUSTOM_KEY_UP))
            {
               keybr_key_confirmed[CUSTOM_KEY_UP] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_DOWN] && help_input_key_pressed(input, CUSTOM_KEY_DOWN))
            {
               keybr_key_confirmed[CUSTOM_KEY_DOWN] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_LEFT] && help_input_key_pressed(input, CUSTOM_KEY_LEFT))
            {
               keybr_key_confirmed[CUSTOM_KEY_LEFT] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_RIGHT] && help_input_key_pressed(input, CUSTOM_KEY_RIGHT))
            {
               keybr_key_confirmed[CUSTOM_KEY_RIGHT] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_A] && help_input_key_pressed(input, CUSTOM_KEY_A))
            {
               keybr_key_confirmed[CUSTOM_KEY_A] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_B] && help_input_key_pressed(input, CUSTOM_KEY_B))
            {
               keybr_key_confirmed[CUSTOM_KEY_B] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_START] && help_input_key_pressed(input, CUSTOM_KEY_START))
            {
               keybr_key_confirmed[CUSTOM_KEY_START] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_SELECT] && help_input_key_pressed(input, CUSTOM_KEY_SELECT))
            {
               keybr_key_confirmed[CUSTOM_KEY_SELECT] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_VOLUME_UP] && help_input_key_pressed(input, CUSTOM_KEY_VOLUME_UP))
            {
               keybr_key_confirmed[CUSTOM_KEY_VOLUME_UP] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }
            if (false == keybr_key_confirmed[CUSTOM_KEY_VOLUME_DOWN] && help_input_key_pressed(input, CUSTOM_KEY_VOLUME_DOWN))
            {
               keybr_key_confirmed[CUSTOM_KEY_VOLUME_DOWN] = true;
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_BLIP);
               valid_keybr_confirmed = true;
            }

            bool all_mapped_controls_confirmed = true;
            for (size_t keybr = 0; keybr < sizeof(keybr_key_confirmed) / sizeof(keybr_key_confirmed[0]); ++keybr)
            {
               if (false == keybr_key_confirmed[keybr])
               {
                  all_mapped_controls_confirmed = false;
                  break;
               }
            }
            if (all_mapped_controls_confirmed)
            {
               // Go to title screen
               next_game_state = GAME_STATE_TITLE;
               audio_mixer_stop_music_and_sfx(audio_mixer);
               audio_mixer_queue_sample_music(audio_mixer, AMSID_MUSIC_TITLE, true);
            }
         }
         if (GAME_STATE_TITLE == game_state)
         {
            // Player count select
            if (help_input_key_pressed(input, CUSTOM_KEY_LEFT) || help_input_key_pressed(input, CUSTOM_KEY_RIGHT))
            {
               // TODO-GS: Two-player mode not yet supported -> Gray out for now
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_INVALID);
            }

            // Start game (for now in single player more)
            if (help_input_key_pressed(input, CUSTOM_KEY_START))
            {
               // Stop title music
               audio_mixer_stop_music_and_sfx(audio_mixer);
               // Play selection to config screen
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_SELECT);
               // Init config screen state
               next_game_state = GAME_STATE_GAME_MUSIC_CONFIG;
               // Config music off
               configured_game_music = AUDIO_MIXER_SAMPLE_ID_INVALID;
               game_music_cursor = vec_2i_make_xy(1, 0);
            }

            // Quit game
            if (help_input_key_pressed(input, CUSTOM_KEY_SELECT))
            {
               next_game_state = GAME_STATE_QUIT;
               audio_mixer_stop_music_and_sfx(audio_mixer);
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_SELECT);
               time_last_quit = help_sdl_time_in_seconds();
            }
         }
         if (GAME_STATE_QUIT == game_state)
         {
            // Quit game after timer runs out
            if (help_sdl_time_in_seconds() > time_last_quit + TIME_SEC_QUIT)
            {
               tetris_close_requested = true;
            }
         }
         if (GAME_STATE_GAME_MUSIC_CONFIG == game_state)
         {
            // Move music type cursor
            bool music_type_changed = false;
            if (help_input_key_pressed(input, CUSTOM_KEY_UP))
            {
               const int NEW_GAME_CURSOR_Y = help_limit_clamp_i(0, game_music_cursor.y + 1, GAME_MUSIC_TYPE_HEIGHT - 1);
               if (NEW_GAME_CURSOR_Y != game_music_cursor.y) music_type_changed = true;
               game_music_cursor.y = NEW_GAME_CURSOR_Y;
            }
            if (help_input_key_pressed(input, CUSTOM_KEY_DOWN))
            {
               const int NEW_GAME_CURSOR_Y = help_limit_clamp_i(0, game_music_cursor.y - 1, GAME_MUSIC_TYPE_HEIGHT - 1);
               if (NEW_GAME_CURSOR_Y != game_music_cursor.y) music_type_changed = true;
               game_music_cursor.y = NEW_GAME_CURSOR_Y;
            }
            if (help_input_key_pressed(input, CUSTOM_KEY_LEFT))
            {
               const int NEW_GAME_CURSOR_X = help_limit_clamp_i(0, game_music_cursor.x - 1, GAME_MUSIC_TYPE_WIDTH - 1);
               if (NEW_GAME_CURSOR_X != game_music_cursor.x) music_type_changed = true;
               game_music_cursor.x = NEW_GAME_CURSOR_X;
            }
            if (help_input_key_pressed(input, CUSTOM_KEY_RIGHT))
            {
               const int NEW_GAME_CURSOR_X = help_limit_clamp_i(0, game_music_cursor.x + 1, GAME_MUSIC_TYPE_WIDTH - 1);
               if (NEW_GAME_CURSOR_X != game_music_cursor.x) music_type_changed = true;
               game_music_cursor.x = NEW_GAME_CURSOR_X;
            }

            // Select music type if selection changed or checked the first time
            if (music_type_changed)
            {
               if (vec_2i_equals_xy(game_music_cursor, 0, 1))
               {
                  // Selected music type A
                  audio_mixer_stop_music(audio_mixer);
                  audio_mixer_queue_sample_music(audio_mixer, AMSID_MUSIC_GAME_A_TYPE, true);
                  configured_game_music = AMSID_MUSIC_GAME_A_TYPE;
               }

               if (vec_2i_equals_xy(game_music_cursor, 1, 1))
               {
                  // Selected music type B
                  audio_mixer_stop_music(audio_mixer);
                  audio_mixer_queue_sample_music(audio_mixer, AMSID_MUSIC_GAME_B_TYPE, true);
                  configured_game_music = AMSID_MUSIC_GAME_B_TYPE;
               }

               if (vec_2i_equals_xy(game_music_cursor, 0, 0))
               {
                  // Selected music type C
                  audio_mixer_stop_music(audio_mixer);
                  audio_mixer_queue_sample_music(audio_mixer, AMSID_MUSIC_GAME_C_TYPE, true);
                  configured_game_music = AMSID_MUSIC_GAME_C_TYPE;
               }

               if (vec_2i_equals_xy(game_music_cursor, 1, 0))
               {
                  // Selected music type Off
                  audio_mixer_stop_music(audio_mixer);
                  configured_game_music = AUDIO_MIXER_SAMPLE_ID_INVALID;
               }
            }

            // Start game with selected music
            if (help_input_key_pressed(input, CUSTOM_KEY_START))
            {
               next_game_state = GAME_STATE_NEW_GAME;
            }
         }
         if (GAME_STATE_NEW_GAME == game_state)
         {
            // Reset stats
            stat_score = 0;
            stat_lines = 0;
            stat_level = 0;
            // Kickstart next tetro
            tetro_next = help_tetro_world_make_random_at_spawn();
            // Action - New game
            play_field = help_play_field_make_non_occupied();
            tetro_active = help_tetro_world_make_random_at_spawn();

            // Start gameplay
            next_game_state = GAME_STATE_CONTROL;
            time_last_tetro_drop = help_sdl_time_in_seconds();
         }
         else if (GAME_STATE_CONTROL == game_state)
         {
            // Action - Rotate tetro
            if (help_input_key_pressed(input, CUSTOM_KEY_A))
            {
               if (!help_tetro_rotation_collides(&tetro_active, &play_field, ROTATION_CW))
               {
                  help_tetro_world_rotate_cw(&tetro_active);
                  audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_ROTATE);
               }
            }
            if (help_input_key_pressed(input, CUSTOM_KEY_B))
            {
               if (!help_tetro_rotation_collides(&tetro_active, &play_field, ROTATION_CCW))
               {
                  help_tetro_world_rotate_ccw(&tetro_active);
                  audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_ROTATE);
               }
            }

            // Action - Drop tetro
            const double TIME_DELTA_TETRO_DROP = 0.85;
            if (help_sdl_time_in_seconds() >= time_last_tetro_drop + TIME_DELTA_TETRO_DROP)
            {
               // Drop tetro if possible
               if (help_tetro_move_collides(&tetro_active, &play_field, 0, -1))
               {
                  // Drop collision - Place the tetro
                  next_game_state = GAME_STATE_PLACE;
               }
               else
               {
                  // No drop collision
                  --tetro_active.tile_pos.y;
                  audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_DROP);
               }

               // Update drop timer
               time_last_tetro_drop = help_sdl_time_in_seconds();
            }

            // Action - Control tetro horizontally
            const double TIME_DELTA_TETRO_MOVE = 0.15;
            const bool MOVE_LEFT = help_input_key_pressed_or_held(input, CUSTOM_KEY_LEFT);
            const bool MOVE_RIGHT = help_input_key_pressed_or_held(input, CUSTOM_KEY_RIGHT);
            const bool DO_MOVE = MOVE_LEFT || MOVE_RIGHT;

            if (DO_MOVE && help_sdl_time_in_seconds() >= (time_last_tetro_player_move + TIME_DELTA_TETRO_MOVE))
            {
               // Movement direction ?
               int move_direction = 0;
               move_direction -= MOVE_LEFT ? 1 : 0;
               move_direction += MOVE_RIGHT ? 1 : 0;
               
               // Move only if no collision occurs
               if (false == help_tetro_move_collides(&tetro_active, &play_field, move_direction, 0))
               {
                  tetro_active.tile_pos.x += move_direction;
                  audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_MOVE);
               }

               // Update movement timer
               time_last_tetro_player_move = help_sdl_time_in_seconds();
            }

            // Action - Control tetro drop
            const double TIME_DELTA_TETRO_PLAYER_DROP = 0.1;
            if (help_sdl_time_in_seconds() >= time_last_tetro_player_drop + TIME_DELTA_TETRO_PLAYER_DROP)
            {
               if (help_input_key_pressed_or_held(input, CUSTOM_KEY_DOWN))
               {
                  // Place tetro if collision occurse
                  if (help_tetro_move_collides(&tetro_active, &play_field, 0, -1))
                  {
                     // Drop collision - Place the tetro
                     next_game_state = GAME_STATE_PLACE;
                  }
                  else
                  {
                     // No drop collision
                     --tetro_active.tile_pos.y;
                     audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_DROP);
                  }
               }

               // Update drop timer
               time_last_tetro_player_drop = help_sdl_time_in_seconds();
            }

            // Pause
            if (help_input_key_pressed(input, CUSTOM_KEY_START))
            {
               next_game_state = GAME_STATE_PAUSE;

               // Pause music but let sfx ring out
               audio_mixer_pause_music(audio_mixer);
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_PAUSE);
            }
         }
         else if (GAME_STATE_PAUSE == game_state)
         {
            // Un-pause
            if (help_input_key_pressed(input, CUSTOM_KEY_START))
            {
               next_game_state = GAME_STATE_CONTROL;

               // Resume music
               audio_mixer_resume_music(audio_mixer);
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_UN_PAUSE);
            }

            // Quit
            if (help_input_key_pressed(input, CUSTOM_KEY_SELECT))
            {
               audio_mixer_stop_music_and_sfx(audio_mixer);
               audio_mixer_queue_sample_music(audio_mixer, AMSID_MUSIC_TITLE, true);
               audio_mixer_resume_music_and_sfx(audio_mixer);
               next_game_state = GAME_STATE_TITLE;
            }
         }
         else if (GAME_STATE_PLACE == game_state)
         {
            // Action - Place
            help_tetro_drop(&tetro_active, &play_field, &plot_row_min, &plot_row_max);
            audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_PLACE);

            // Line deletion required ?
            list_of_full_rows = help_play_field_list_of_full_rows(&play_field);
            if (0 == list_of_full_rows.count)
            {
               // No rows to delete - Spawn new tetro and take it from there
               next_game_state = GAME_STATE_RESPAWN;
            }
            else
            {
               // There are rows to delete
               // Start deletion timers
               next_game_state = GAME_STATE_REMOVE_LINES;
               time_last_removal_flash_timer = help_sdl_time_in_seconds();
               time_last_row_deletion_timer = help_sdl_time_in_seconds();
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_HIGHLIGHT);
            }
         }
         else if (GAME_STATE_REMOVE_LINES == game_state)
         {
            // Action - Remove lines
            const double TIME_DELTA_ROW_DELETION = 2.0;
            if (help_sdl_time_in_seconds() >= time_last_row_deletion_timer + TIME_DELTA_ROW_DELETION)
            {
               // Time to delete rows
               // Clear all rows that were detected full on tetro placement
               for (int i_del_row = 0; i_del_row < list_of_full_rows.count; ++i_del_row)
               {
                  const int DELETION_ROW = list_of_full_rows.list[i_del_row];
                  if (help_play_field_clear_row(&play_field, DELETION_ROW))
                  {
                     // Reflect in stats
                     ++stat_lines;

                     // TODO-GS: Score line removal
                     stat_score += 10;
                  }
               }

               // Consolidate play field
               next_game_state = GAME_STATE_CONSOLIDATE_PLAY_FIELD;
            }
         }
         else if (GAME_STATE_CONSOLIDATE_PLAY_FIELD == game_state)
         {
            // Action - Consolidate play field after full row deletion - Until no more rows are moved
            help_play_field_consolidate(&play_field);
            audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_DESTROY);

            // Spawn new tetro
            next_game_state = GAME_STATE_RESPAWN;
         }
         else if (GAME_STATE_RESPAWN == game_state)
         {
            // Select active and next tetro
            tetro_active = tetro_next;
            tetro_next = help_tetro_world_make_random_at_spawn();

            // Game over if new tetro overlaps any occupied play field cell
            if (help_tetro_move_collides(&tetro_active, &play_field, 0, 0))
            {
               // Transition to game over
               next_game_state = GAME_STATE_GAME_OVER_TRANSITION_FILL;
               time_last_game_over_transition_row = help_sdl_time_in_seconds();
               game_over_transition_field_lines_filled = 0;
               audio_mixer_stop_music_and_sfx(audio_mixer);
               audio_mixer_queue_sample_sfx(audio_mixer, AMSID_EFFECT_GAME_OVER);
            }
            else
            {
               // No collision on spawn - Back to gameplay
               next_game_state = GAME_STATE_CONTROL;
            }
         }
         else if (GAME_STATE_GAME_OVER_TRANSITION_FILL == game_state)
         {
            if (help_sdl_time_in_seconds() >= time_last_game_over_transition_row + TIME_SEC_GAME_OVER_TRANSITION_FILL_ROW)
            {
               ++game_over_transition_field_lines_filled;
               if (game_over_transition_field_lines_filled > PLAY_FIELD_HEIGHT)
               {
                     audio_mixer_stop_music_and_sfx(audio_mixer);
                     audio_mixer_queue_sample_music(audio_mixer, AMSID_MUSIC_GAME_OVER, false);
                     time_last_game_over_transition_row = help_sdl_time_in_seconds();
                     game_over_transition_field_lines_cleared = PLAY_FIELD_HEIGHT;
                     next_game_state = GAME_STATE_GAME_OVER_TRANSITION_CLEAR;
               }

               // Track timer
               time_last_game_over_transition_row = help_sdl_time_in_seconds();
            }
         }
         else if (GAME_STATE_GAME_OVER_TRANSITION_CLEAR == game_state)
         {
            if (help_sdl_time_in_seconds() >= time_last_game_over_transition_row + TIME_SEC_GAME_OVER_TRANSITION_FILL_ROW)
            {
               --game_over_transition_field_lines_cleared;
               if (game_over_transition_field_lines_cleared < 0)
               {
                  help_play_field_clear(&play_field);
                  next_game_state = GAME_STATE_GAME_OVER;
               }

               // Track timer
               time_last_game_over_transition_row = help_sdl_time_in_seconds();
            }
         }
         else if (GAME_STATE_GAME_OVER == game_state)
         {
            // Action - Game over
            if (help_input_key_pressed(input, CUSTOM_KEY_START))
            {
               next_game_state = GAME_STATE_NEW_GAME;
               audio_mixer_queue_sample_music(audio_mixer, configured_game_music, true);
            }
            
         }
         else if (GAME_STATE_NONE == game_state)
         {
            // Action - None - Should never happen
            printf("\nGame state NONE - Nothing to simulate");
            exit(-1);
         }

         // Level detection
         // TODO-GS: Parameterize level-up's and use a clean table instead of this
         const struct score_level_mapping_s * ACTIVE_SCORE_MAPPING = NULL;
         for (size_t i = 0; i < (sizeof(score_level_mapping) / sizeof(score_level_mapping[0])); ++i)
         {
            const struct score_level_mapping_s * SCORE_MAPPING_CANDIDATE = score_level_mapping + i;
            if (stat_score >= SCORE_MAPPING_CANDIDATE->threshold_score)
            {
               ACTIVE_SCORE_MAPPING = SCORE_MAPPING_CANDIDATE;
            }
         }

         // Set level based on found mapping
         if (ACTIVE_SCORE_MAPPING)
         {
            stat_level = ACTIVE_SCORE_MAPPING->mapped_level;
         }
         else
         {
            stat_level = -1;
         }

         // Game state housekeeping
         if (GAME_STATE_NONE != next_game_state)
         {
            game_state = next_game_state;
            next_game_state = GAME_STATE_NONE;
         }
      }

      // Render to scene - All game states
      // ----> Clear offline texture
      const color_rgba_t COL_GBC_LIGHTEST = color_rgba_make_rgba(238, 255, 102, 0xFF);
      help_texture_rgba_clear(engine.tex_virtual, COL_GBC_LIGHTEST);
      // >> Render based on active game mode
      if (GAME_STATE_SPLASH == game_state)
      {
         const char * const SPLASH_TEXT = "Just another\nunfinished TETRIS\nclone made for the\nlove of coding."
                                          "\n\nThis, is a work\nof fiction and\nnon-commercial."
                                          "\n\n\nWait or hit start."
                                          "\n\n\n\nSven Garson - 2025";
         help_engine_render_text_at_tile(&engine, SPLASH_TEXT, 1, PLAY_FIELD_HEIGHT - 2);
      }
      if (GAME_STATE_TITLE == game_state)
      { 
         const char * STR_PLAYER_1 = "1PLAYER";
         help_engine_render_text_at_tile(&engine, STR_PLAYER_1, 2, 3);
         help_engine_render_tinted_text_at_tile(&engine, "2PLAYER", 2 + 1 + strlen(STR_PLAYER_1) + 2, 3, color_rgba_make_rgba(180, 200, 180, 0xFF));
         help_render_engine_sprite_at_tile(&engine, 1, 3, SPRITE_MAP_TILE_FONT_ARROW_RIGHT);
         help_engine_render_text_at_tile(&engine, "2025 Sven Garson", 2, 1);


         help_engine_render_text_at_tile(&engine, "SELECT TO QUIT", 3, PLAY_FIELD_HEIGHT - 4);
      }
      if (GAME_STATE_GAME_MUSIC_CONFIG == game_state)
      {
         // Game type
         help_engine_render_text_at_tile(&engine, "GAME TYPE", 5, PLAY_FIELD_HEIGHT - 4);
         help_engine_render_text_at_tile(&engine, "A-TYPE", 3, PLAY_FIELD_HEIGHT - 6);
         help_engine_render_tinted_text_at_tile(&engine, "B-TYPE", 11, PLAY_FIELD_HEIGHT - 6, color_rgba_make_rgba(180, 200, 180, 0xFF));

         // Music type based on selection
         help_engine_render_text_at_tile(&engine, "MUSIC TYPE", 5, 7);

         // A-Type
         if (vec_2i_equals_xy(game_music_cursor, 0, 1))
         {
            help_engine_render_text_at_tile(&engine, "A-TYPE", 3, 5);
         }
         else
         {
            help_engine_render_tinted_text_at_tile(&engine, "A-TYPE", 3, 5, color_rgba_make_rgba(180, 200, 180, 0xFF));
         }

         // B-Type
         if (vec_2i_equals_xy(game_music_cursor, 1, 1))
         {
            help_engine_render_text_at_tile(&engine, "B-TYPE", 11, 5);
         }
         else
         {
            help_engine_render_tinted_text_at_tile(&engine, "B-TYPE", 11, 5, color_rgba_make_rgba(180, 200, 180, 0xFF));
         }

         // C-Type
         if (vec_2i_equals_xy(game_music_cursor, 0, 0))
         {
            help_engine_render_text_at_tile(&engine, "C-TYPE", 3, 3);
         }
         else
         {
            help_engine_render_tinted_text_at_tile(&engine, "C-TYPE", 3, 3, color_rgba_make_rgba(180, 200, 180, 0xFF));
         }

         // Off
         if (vec_2i_equals_xy(game_music_cursor, 1, 0))
         {
            help_engine_render_text_at_tile(&engine, "OFF", 12, 3);
         }
         else
         {
            help_engine_render_tinted_text_at_tile(&engine, "OFF", 12, 3, color_rgba_make_rgba(180, 200, 180, 0xFF));
         }
      }
      if (
         GAME_STATE_CONTROL == game_state ||
         GAME_STATE_PLACE == game_state ||
         GAME_STATE_REMOVE_LINES == game_state ||
         GAME_STATE_CONSOLIDATE_PLAY_FIELD == game_state ||
         GAME_STATE_RESPAWN == game_state ||
         GAME_STATE_GAME_OVER == game_state ||
         GAME_STATE_GAME_OVER_TRANSITION_FILL == game_state ||
         GAME_STATE_GAME_OVER_TRANSITION_CLEAR == game_state ||
         GAME_STATE_PAUSE == game_state
      )
      {
         if (GAME_STATE_GAME_OVER != game_state && GAME_STATE_GAME_OVER_TRANSITION_CLEAR != game_state && GAME_STATE_PAUSE != game_state)
         {
            // >> Render play field
            help_play_field_render_to_texture(&play_field, &engine);
            // >> Active tetro
            help_tetro_render_to_texture(&tetro_active, &engine);
            // >> Next tetro
            help_tetro_render_to_texture_at_tile_without_position(&tetro_next, &engine, 14, 1);
         }
         // >> Play field walls
         for (int y_wall = 0; y_wall < PLAY_FIELD_HEIGHT; ++y_wall)
         {
            // Left wall
            help_render_engine_sprite(&engine, 0, y_wall * PLAY_FIELD_TILE_SIZE, SPRITE_MAP_TILE_BRICK);
            // Right wall
            help_render_engine_sprite(
               &engine,
               (PLAY_FIELD_WIDTH * PLAY_FIELD_TILE_SIZE) + PLAY_FIELD_OFFSET_HORI_PIXELS,
               y_wall * PLAY_FIELD_TILE_SIZE,
               SPRITE_MAP_TILE_BRICK
            );
         }
         // >> Gameplay stats
         const char * STR_LABEL_SCORE = "SCORE";
         help_engine_render_text_at_tile(&engine, STR_LABEL_SCORE, 13, PLAY_FIELD_HEIGHT - 2);
         char str_stat_score[32];
         snprintf(str_stat_score, sizeof(str_stat_score), "%*d", strlen(STR_LABEL_SCORE), stat_score);
         help_engine_render_text_at_tile(&engine, str_stat_score, 13, PLAY_FIELD_HEIGHT - 3);

         const char * STR_LABEL_LEVEL = "LEVEL";
         help_engine_render_text_at_tile(&engine, STR_LABEL_LEVEL, 13, PLAY_FIELD_HEIGHT - 8);
         char str_stat_level[32];
         snprintf(str_stat_level, sizeof(str_stat_level), "%*d", strlen(STR_LABEL_LEVEL), stat_level);
         help_engine_render_text_at_tile(&engine, str_stat_level, 13, PLAY_FIELD_HEIGHT - 9);

         const char * STR_LABEL_LINES = "LINES";
         help_engine_render_text_at_tile(&engine, STR_LABEL_LINES, 13, PLAY_FIELD_HEIGHT - 11);
         char str_stat_lines[32];
         snprintf(str_stat_lines, sizeof(str_stat_lines), "%*d", strlen(STR_LABEL_LINES), stat_lines);
         help_engine_render_text_at_tile(&engine, str_stat_lines, 13, PLAY_FIELD_HEIGHT - 12);

      }
      if (GAME_STATE_REMOVE_LINES == game_state)
      {
         // Latch highlight rendering
         const double TIME_DELTA_ROW_FLASH = 0.22;
         static bool highlight_latch = true;
         if (help_sdl_time_in_seconds() >= time_last_removal_flash_timer + TIME_DELTA_ROW_FLASH)
         {
            // Latch
            highlight_latch = !highlight_latch;

            // Update timer
            time_last_removal_flash_timer = help_sdl_time_in_seconds();
         }

         // Highlight rows on timer
         if (highlight_latch)
         {
            for (int i_full_rows = 0; i_full_rows < list_of_full_rows.count; ++i_full_rows)
            {
               const int DELETION_ROW = list_of_full_rows.list[i_full_rows];
               for (int deletion_col = 0; deletion_col < PLAY_FIELD_WIDTH; ++deletion_col)
               {
                  help_render_engine_sprite(
                     &engine,
                     PLAY_FIELD_OFFSET_HORI_PIXELS + (deletion_col * PLAY_FIELD_TILE_SIZE),
                     DELETION_ROW * PLAY_FIELD_TILE_SIZE,
                     SPRITE_MAP_TILE_HIGHLIGHT
                  );
               }
            }
         }
      }
      if (GAME_STATE_GAME_OVER_TRANSITION_FILL == game_state)
      {
         for (int game_over_row = 0; game_over_row < game_over_transition_field_lines_filled; ++game_over_row)
         {
            for (int col = 0; col < PLAY_FIELD_WIDTH; ++col)
            {
               help_render_engine_sprite(
                  &engine,
                  PLAY_FIELD_OFFSET_HORI_PIXELS + (col * PLAY_FIELD_TILE_SIZE),
                  game_over_row * PLAY_FIELD_TILE_SIZE,
                  SPRITE_MAP_TILE_GAME_OVER_FILL
               );
            }
         }
      }
      if (GAME_STATE_GAME_OVER == game_state || GAME_STATE_GAME_OVER_TRANSITION_CLEAR == game_state)
      {
         help_engine_render_text_at_tile(&engine, "GAME\n\nOVER", 4, PLAY_FIELD_HEIGHT - 4);
         help_engine_render_text_at_tile(&engine, " HIT\n\nSTART\n\n TO TRY\n\nAGAIN", 3, 8);
      }
      if (GAME_STATE_GAME_OVER_TRANSITION_CLEAR == game_state)
      {
         for (int game_over_row = 0; game_over_row < game_over_transition_field_lines_cleared; ++game_over_row)
         {
            for (int col = 0; col < PLAY_FIELD_WIDTH; ++col)
            {
               help_render_engine_sprite(
                  &engine,
                  PLAY_FIELD_OFFSET_HORI_PIXELS + (col * PLAY_FIELD_TILE_SIZE),
                  (PLAY_FIELD_HEIGHT * PLAY_FIELD_TILE_SIZE) - (game_over_row * PLAY_FIELD_TILE_SIZE),
                  SPRITE_MAP_TILE_GAME_OVER_FILL
               );
            }
         }
      }
      if (GAME_STATE_PAUSE == game_state)
      {
         help_engine_render_text_at_tile(&engine, " HIT\n\n START\n\n   TO\n\nCONTINUE\n\n   OR\n\n SELECT\n\n TO QUIT", 2, PLAY_FIELD_HEIGHT - 4);
      }
      if (GAME_STATE_QUIT == game_state)
      {
         help_engine_render_text_at_tile(&engine, "THANK YOU FOR\n\n  PLAYING", 3, PLAY_FIELD_HEIGHT - 5);
         help_render_engine_sprite_at_tile(&engine, 3 + 10, PLAY_FIELD_HEIGHT - 5 - 2, SPRITE_MAP_TILE_HEART);
      }
      if (GAME_STATE_INPUT_MAPPING == game_state)
      {
         help_engine_render_text_at_tile(&engine, "-----------+--------", 0, PLAY_FIELD_HEIGHT - 1);
         help_engine_render_text_at_tile(&engine, " GAME BOY  | KEYBRD ", 0, PLAY_FIELD_HEIGHT - 2);
         help_engine_render_text_at_tile(&engine, "-----------+--------", 0, PLAY_FIELD_HEIGHT - 3);


         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_UP]          ? "UP         |   OK   " : "UP         |W       ", 0, PLAY_FIELD_HEIGHT - 4);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_DOWN]        ? "DOWN       |   OK   " : "DOWN       |S       ", 0, PLAY_FIELD_HEIGHT - 5);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_LEFT]        ? "LEFT       |   OK   " : "LEFT       |A       ", 0, PLAY_FIELD_HEIGHT - 6);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_RIGHT]       ? "RIGHT      |   OK   " : "RIGHT      |D       ", 0, PLAY_FIELD_HEIGHT - 7);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_A]           ? "A          |   OK   " : "A          |UP ARR  ", 0, PLAY_FIELD_HEIGHT - 8);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_B]           ? "B          |   OK   " : "B          |LEFT ARR", 0, PLAY_FIELD_HEIGHT - 9);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_START]       ? "START      |   OK   " : "START      |ENTER   ", 0, PLAY_FIELD_HEIGHT - 10);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_SELECT]      ? "SELECT     |   OK   " : "SELECT     |DELETE  ", 0, PLAY_FIELD_HEIGHT - 11);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_VOLUME_UP]   ? "VOLUME UP  |   OK   " : "VOLUME UP  |PLUS KP ", 0, PLAY_FIELD_HEIGHT - 12);
         help_engine_render_text_at_tile(&engine, keybr_key_confirmed[CUSTOM_KEY_VOLUME_DOWN] ? "VOLUME DOWN|   OK   " : "VOLUME DOWN|MINUS KP", 0, PLAY_FIELD_HEIGHT - 13);

         help_engine_render_text_at_tile(&engine, "   HIT ALL KEYBRD   ", 0, PLAY_FIELD_HEIGHT - 13 - 2);
         help_engine_render_text_at_tile(&engine, "  KEYS TO CONTINUE  ", 0, PLAY_FIELD_HEIGHT - 13 - 4);
      }
      if (help_sdl_time_in_seconds() <= time_until_show_volume_overlay)
      {
         float volume_music, volume_sfx;
         if (audio_mixer_get_volume_music(audio_mixer, &volume_music) && audio_mixer_get_volume_sfx(audio_mixer, &volume_sfx))
         {
            static char str_volumes[64];
            snprintf(str_volumes, sizeof(str_volumes), "Volume\n  Music: %.2f\n  Sfx  : %.2f", volume_music, volume_sfx);
            help_engine_render_tinted_text_at_tile(&engine, str_volumes, 0, PLAY_FIELD_HEIGHT - 1, color_rgba_make_rgba(255, 0, 0, 255));
         }
      }

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
   audio_mixer_destroy(audio_mixer);
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