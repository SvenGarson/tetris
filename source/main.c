#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

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

// Constants
const char * ARG_KEY_DIR_ABS_RES = "-abs_res_dir";

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

   // Log engine status
   printf("\n\nEngine Information");
   printf("\n\t%-15s: %s", "resource directory", DIR_ABS_RES);

   // Back to OS
   return EXIT_SUCCESS;
}