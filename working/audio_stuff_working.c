// Audio
   // ----> Load WAV
   char str_path_wav[1024];;
   snprintf(str_path_wav, sizeof(str_path_wav), "%s\\audio\\%s\\%s", DIR_ABS_RES, "effects", "storm.wav");
   SDL_AudioSpec wav_spec;
   Uint8 * wav_data;
   Uint32 wav_length;
   if (!SDL_LoadWAV(str_path_wav, &wav_spec, &wav_data, &wav_length))
   {
      printf("\nFailed to load WAV [%s] - Error: %s", str_path_wav, SDL_GetError());
      return EXIT_FAILURE;
   }
   // ----> Open music device
   const SDL_AudioDeviceID DEVICE_ID_MUSIC = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
   if (0 == DEVICE_ID_MUSIC)
   {
      printf("\nFailed to open music audio device - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }
   // ----> Get music device spec
   SDL_AudioSpec device_spec_music;
   if (!SDL_GetAudioDeviceFormat(DEVICE_ID_MUSIC, &device_spec_music, NULL))
   {
      printf("\nFailed to get music device spec - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }
   // ----> Create music audio stream
   SDL_AudioStream * audio_stream_music = SDL_CreateAudioStream(NULL, &device_spec_music);
   if (NULL == audio_stream_music)
   {
      printf("\nFailed to create music audio stream - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }
   // ----> Bind music device to audio stream
   if (!SDL_BindAudioStream(DEVICE_ID_MUSIC, audio_stream_music))
   {
      printf("\nFailed to bind music audio stream to device - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }
   // ----> Set WAV audio stream format in music stream
   if (!SDL_SetAudioStreamFormat(audio_stream_music, &wav_spec, NULL))
   {
      printf("\nFailed to set audio stream source format - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }
   // ----> Fill in music
   if (!SDL_PutAudioStreamData(audio_stream_music, wav_data, wav_length))
   {
      printf("\nFailed to play WAV - Error: %s", SDL_GetError());
      return EXIT_FAILURE;
   }