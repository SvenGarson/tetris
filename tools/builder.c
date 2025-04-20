#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Logic - Arguments
const char * help_args_key_value(int argc, char * argv[], const char * key)
{
   for (int i_key = 0; i_key < argc - 1; ++i_key)
   {
      if (strcmp(argv[i_key], key) == 0)
      {
         // Found matching key-value pair
         return argv[i_key + 1];
      }
   }

   // No match found
   return NULL;
}

// Logic - String
const char * help_string_list_merge(char * strings[], size_t count, char separator)
{
   if (NULL == strings || 0 == count) return NULL;

   size_t separated_string_length = 1; // Including terminator
   for (size_t i = 0; i < count; ++i)
   {
      // substring
      separated_string_length += strlen(strings[i]);
      // following separator
      if (i < count - 1) ++separated_string_length;
   }

   char * merged = malloc(sizeof(char) * separated_string_length);
   if (merged)
   {
      // merge substring with separator
      size_t merged_end = 0;
      for (size_t i_sub = 0; i_sub < count; ++i_sub)
      {
         // substring
         const size_t SUBSTRING_LENGTH = strlen(strings[i_sub]);
         memcpy(merged + merged_end, strings[i_sub], SUBSTRING_LENGTH);
         merged_end += SUBSTRING_LENGTH;

         // separator
         if (i_sub < count - 1)
         {
            merged[merged_end++] = separator;
         }
      }

      // terminate
      merged[separated_string_length - 1] = '\0';
   }

   return merged;
}

const char * help_string_list_merge_prefixed(char * strings[], size_t count, const char * prefix, char separator)
{
   if (NULL == strings || 0 == count || NULL == prefix) return NULL;

   // Construct list of prefixed strings
   char ** prefixed_strings = malloc(sizeof(char *) * count);
   if (NULL == prefixed_strings) return NULL;

   const size_t PREFIX_LENGTH = strlen(prefix);
   for (size_t i_orig = 0; i_orig < count; ++i_orig)
   {
      // Determine prefixed size
      const size_t PREFIXED_SIZE_TERMINATED = strlen(strings[i_orig]) + PREFIX_LENGTH + 1;
      char * prefixed = malloc(sizeof(char) * PREFIXED_SIZE_TERMINATED);
      if (NULL == prefixed) return NULL;

      // Create prefixed string
      memcpy(prefixed, prefix, strlen(prefix));
      memcpy(prefixed + strlen(prefix), strings[i_orig], strlen(strings[i_orig]));
      prefixed[PREFIXED_SIZE_TERMINATED - 1] = '\0';

      // Store in new list
      prefixed_strings[i_orig] = prefixed;
   }

   return help_string_list_merge(prefixed_strings, count, separator);
}

// Constants - Arguments
const char * CONST_ARG_ROOT_FLAG = "-root";

// Logic - Main
int main(int argc, char * argv[])
{
   // Expect root directory as argument
   const char * DIR_ABS_ROOT = help_args_key_value(argc, argv, CONST_ARG_ROOT_FLAG);
   if (NULL == DIR_ABS_ROOT)
   {
      printf("\nAbsolute root directory required for compilation (use '%s' flag)", CONST_ARG_ROOT_FLAG);
      return EXIT_FAILURE;
   }

   printf("\n\n=============== Builder ===============");
   printf("\n\n# Root Directory\n\t[%s]", DIR_ABS_ROOT);

   // Prepare SDL3 library directories
   const char * STR_DIR_ABS_SDL_ROOT = "C:\\dev\\libraries\\SDL3-3.2.6\\i686-w64-mingw32";

   char str_dir_abs_sdl_bin[1024];
   char str_dir_abs_sdl_include[1024];
   char str_dir_abs_sdl_lib[1024];

   snprintf(str_dir_abs_sdl_bin, sizeof(str_dir_abs_sdl_bin), "%s\\%s", STR_DIR_ABS_SDL_ROOT, "bin");
   snprintf(str_dir_abs_sdl_include, sizeof(str_dir_abs_sdl_include), "%s\\%s", STR_DIR_ABS_SDL_ROOT, "include");
   snprintf(str_dir_abs_sdl_lib, sizeof(str_dir_abs_sdl_lib), "%s\\%s", STR_DIR_ABS_SDL_ROOT, "lib");

   printf("\n\n# SDL Directories");
   printf("\n\t%-15s: %s", "root", STR_DIR_ABS_SDL_ROOT);
   printf("\n\t%-15s: %s", "bin", str_dir_abs_sdl_bin);
   printf("\n\t%-15s: %s", "include", str_dir_abs_sdl_include);
   printf("\n\t%-15s: %s", "lib", str_dir_abs_sdl_lib);

   // Prepare list of source files to compile
   char * SOURCE_FILES[] = { "main.c" };
   char str_root_source[1024];
   snprintf(str_root_source, sizeof(str_root_source), "%s%s\\", DIR_ABS_ROOT, "source");
   const char * STR_SOURCES = help_string_list_merge_prefixed(SOURCE_FILES, sizeof(SOURCE_FILES) / sizeof(SOURCE_FILES[0]), str_root_source, ' ');

   // Prepare project include directory
   char str_dir_abs_include[1024];
   snprintf(str_dir_abs_include, sizeof(str_dir_abs_include), "%s%s", DIR_ABS_ROOT, "include");

   // Clear build
   char str_build_dir_abs[1024];
   snprintf(str_build_dir_abs, sizeof(str_build_dir_abs), "%s%s", DIR_ABS_ROOT, "build");
   char str_clear_build_dir[1024];
   snprintf(str_clear_build_dir, sizeof(str_clear_build_dir), "rmdir %s /s /q", str_build_dir_abs);

   printf("\n\n# Clear Build\n");
   const bool SUCCESS_CLEAR_BUILD = system(str_clear_build_dir) == 0;
   if (SUCCESS_CLEAR_BUILD)
   {
      printf("\n\tCleared build [%s] successfully", str_build_dir_abs);
   }
   else
   {
      printf("\n\tFailed to clear build [%s]", str_build_dir_abs);
   }

   // Fresh build directory
   char str_build_new_directory[1024];
   snprintf(str_build_new_directory, sizeof(str_build_new_directory), "mkdir %s", str_build_dir_abs);

   printf("\n\n# Fresh Build Directory\n");
   const bool SUCCESS_FRESH_BUILD_DIR = system(str_build_new_directory) == 0;
   if (SUCCESS_FRESH_BUILD_DIR)
   {
      printf("\n\tCreated build directory [%s] successfully", str_build_dir_abs);
   }
   else
   {
      printf("\n\tFailed to create build directory [%s] - Exiting ...", str_build_dir_abs);
      return EXIT_FAILURE;
   }

   // Prepare compilation string
   char str_compilation[4096];
   snprintf(
      str_compilation,
      sizeof(str_compilation),
      "gcc %s -L%s -I%s -I%s -lSDL3 -o %s%s\\%s",
      STR_SOURCES,
      str_dir_abs_sdl_lib,
      str_dir_abs_sdl_include,
      str_dir_abs_include,
      DIR_ABS_ROOT,
      "build",
      "tetris"
   );

   printf("\n\n# Compile Build\n");
   const bool SUCCESS_SYS_COMPILATION = system(str_compilation) == 0;
   if (SUCCESS_SYS_COMPILATION)
   {
      printf("\n\tCompiled build successfully");
   }
   else
   {
      printf("\n\tFailed to compile build - Exiting ...");
      return EXIT_FAILURE;
   }

   // Pull in executable dependencies
   char str_copy_exec_dependencies[2048];
   snprintf(
      str_copy_exec_dependencies,
      sizeof(str_copy_exec_dependencies),
      "copy %s\\SDL3.dll %s%s /y",
      str_dir_abs_sdl_bin,
      DIR_ABS_ROOT,
      "build"
   );

   printf("\n\n# Pull Executable Dependencies\n");
   const bool SUCCESS_PULL_EXEC_DEPENDENCIES = system(str_copy_exec_dependencies) == 0;
   if (SUCCESS_PULL_EXEC_DEPENDENCIES)
   {
      printf("\n\tPulled in executable dependencies successfully");
   }
   else
   {
      printf("\n\tFailed to pull in executable dependencies - Exiting ...");
      return EXIT_FAILURE;
   }

   // Push executable project resources
   char str_push_exec_resources[2048];
   snprintf(str_push_exec_resources, sizeof(str_push_exec_resources), "xcopy %s%s %s\\resources\\ /e /y /f", DIR_ABS_ROOT, "resources", str_build_dir_abs);
   system(str_push_exec_resources);

   // Push executable run script
   char str_push_exec_run_script[2048];
   snprintf(str_push_exec_run_script, sizeof(str_push_exec_run_script), "copy %s%s\\%s %s /y", DIR_ABS_ROOT, "tools", "run.bat", str_build_dir_abs);

   printf("\n\n# Push Executable Run Script\n");
   const bool SUCCESS_PUSH_EXEC_RUN_SCRIPT = system(str_push_exec_run_script) == 0;
   if (SUCCESS_PUSH_EXEC_RUN_SCRIPT)
   {
      printf("\n\tPushed executable run script to build successfully");
   }
   else
   {
      printf("\n\tFailed to push executable run script to build - Exiting ...");
      return EXIT_FAILURE;
   }

   // Back to OS
   return EXIT_SUCCESS;
}