/*
 * audio-sdlmixer.cc - Music and sound effects playback using SDL_mixer.
 *
 * Copyright (C) 2012-2019  Wicked_Digger <wicked_digger@mail.ru>
 *
 * This file is part of freeserf.
 *
 * freeserf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * freeserf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with freeserf.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/audio-sdlmixer.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <cmath>
#include <list>
#include <memory>
#include <string>

#include "src/log.h"
#include "src/data.h"
#include "src/audio-dummy.h"

ExceptionSDLmixer::ExceptionSDLmixer(const std::string &_description)
  : ExceptionAudio(_description) {
  sdl_error = SDL_GetError();
  description += " (" + sdl_error + ")";
}

/* Tracks of the sound effect channels and their sounds' volume. */
#define SFX_TRACKS  Audio::kSfxChannels

static float sfx_track_volume[SFX_TRACKS];
static float sfx_master_volume = 1.f;

/* Without a usable audio device the game runs without sound. */
static std::unique_ptr<Audio>
create_audio() {
  try {
    return std::unique_ptr<Audio>(new AudioSDL());
  } catch (ExceptionAudio &e) {
    Log::Error["audio"] << e.what() << "; continuing without sound.";
    return std::unique_ptr<Audio>(new AudioDummy());
  }
}

Audio &
Audio::get_instance() {
  static std::unique_ptr<Audio> audio = create_audio();
  return *audio;
}

AudioSDL::AudioSDL() {
  Log::Info["audio"] << "Initializing \"sdlmixer\".";

  Log::Info["audio"] << "Available drivers:";
  int num_drivers = SDL_GetNumAudioDrivers();
  for (int i = 0; i < num_drivers; ++i) {
    Log::Info["audio"] << "\t" << SDL_GetAudioDriver(i);
  }

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    throw ExceptionSDLmixer("Could not init SDL audio");
  }

  try {
    init_mixer();
  } catch (...) {
    /* The destructor does not run for a failed constructor. */
    Mix_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    throw;
  }
}

void
AudioSDL::init_mixer() {
  SDL_version version;
  SDL_GetVersion(&version);
  Log::Info["audio"] << "Initialized with SDL "
                     << static_cast<int>(version.major) << '.'
                     << static_cast<int>(version.minor) << '.'
                     << static_cast<int>(version.patch)
                     << " (driver: " << SDL_GetCurrentAudioDriver() << ")";

  const SDL_version *mversion = Mix_Linked_Version();
  Log::Info["audio:SDL_mixer"] << "Initializing SDL_mixer "
                               << static_cast<int>(mversion->major) << '.'
                               << static_cast<int>(mversion->minor) << '.'
                               << static_cast<int>(mversion->patch);

  if (Mix_Init(0) != 0) {
    throw ExceptionSDLmixer("Could not init SDL_mixer");
  }

  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512) != 0) {
    throw ExceptionSDLmixer("Could not open audio device");
  }

  /* Allocate the sound effect channels. */
  Mix_AllocateChannels(SFX_TRACKS);
  for (int i = 0; i < SFX_TRACKS; i++) {
    sfx_track_volume[i] = 1.f;
  }

  volume = 1.f;

  sfx_player = std::make_shared<AudioSDL::PlayerSFX>();
  midi_player = std::make_shared<AudioSDL::PlayerMIDI>();

  Log::Info["audio:SDL_mixer"] << "Initialized";
}

AudioSDL::~AudioSDL() {
  sfx_player = nullptr;
  midi_player = nullptr;

  Mix_HookMusicFinished(nullptr);
  Mix_CloseAudio();
  Mix_Quit();
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

float
AudioSDL::get_volume() {
  return volume;
}

void
AudioSDL::set_volume(float _volume) {
  _volume = std::max(0.f, std::min(_volume, 1.f));
  if (fabs(volume - _volume) < 0.01f) {
    return;
  }
  volume = _volume;

  if (midi_player != nullptr) {
    Audio::PVolumeController volume_controller =
                                           midi_player->get_volume_controller();
    if (volume_controller) {
      volume_controller->set_volume(volume);
    }
  }

  if (sfx_player != nullptr) {
    Audio::PVolumeController volume_controller =
                                            sfx_player->get_volume_controller();
    if (volume_controller) {
      volume_controller->set_volume(volume);
    }
  }
}

void
AudioSDL::volume_up() {
  float vol = get_volume();
  set_volume(vol + 0.1f);
}

void
AudioSDL::volume_down() {
  float vol = get_volume();
  set_volume(vol - 0.1f);
}

AudioSDL::PlayerSFX::PlayerSFX()
  : volume(1.f) {
}

Audio::PTrack
AudioSDL::PlayerSFX::create_track(int track_id) {
  Data &data = Data::get_instance();
  Data::PSource data_source = data.get_data_source();

  PBuffer wav = data_source->get_sound(track_id);
  if (!wav) {
    return nullptr;
  }

  SDL_RWops *rw = SDL_RWFromConstMem(wav->get_data(), wav->get_size());
  Mix_Chunk *chunk = Mix_LoadWAV_RW(rw, 1);
  if (chunk == nullptr) {
    Log::Error["audio:SDL_mixer"] << "Mix_LoadWAV_RW: " << SDL_GetError();
    return nullptr;
  }

  return std::make_shared<AudioSDL::TrackSFX>(chunk);
}

void
AudioSDL::PlayerSFX::enable(bool enable) {
  enabled = enable;
  if (!enabled) {
    stop();
  }
}

void
AudioSDL::PlayerSFX::stop() {
  for (int i = 0; i < SFX_TRACKS; i++) {
    Mix_HaltChannel(i);
  }
}

float
AudioSDL::PlayerSFX::get_volume() {
  return volume;
}

void
AudioSDL::PlayerSFX::set_volume(float _volume) {
  volume = std::max(0.f, std::min(_volume, 1.f));
  sfx_master_volume = volume;
  for (int i = 0; i < SFX_TRACKS; i++) {
    Mix_Volume(i, static_cast<int>(volume * sfx_track_volume[i] *
                                   MIX_MAX_VOLUME));
  }
}

void
AudioSDL::PlayerSFX::volume_up() {
  set_volume(get_volume() + 0.1f);
}

void
AudioSDL::PlayerSFX::volume_down() {
  set_volume(get_volume() - 0.1f);
}

AudioSDL::TrackSFX::TrackSFX(Mix_Chunk *_chunk) {
  chunk = _chunk;
}

AudioSDL::TrackSFX::~TrackSFX() {
  Mix_FreeChunk(chunk);
}

void
AudioSDL::TrackSFX::play() {
  /* Play on the first free track. */
  for (int i = 0; i < SFX_TRACKS; i++) {
    if (Mix_Playing(i)) {
      continue;
    }
    Mix_Volume(i, static_cast<int>(sfx_master_volume * sfx_track_volume[i] *
                                   MIX_MAX_VOLUME));
    if (Mix_PlayChannel(i, chunk, 0) == -1) {
      Log::Error["audio:SDL_mixer"] << "Could not play SFX clip: "
                                    << SDL_GetError();
    }
    return;
  }

  Log::Warn["audio:SDL_mixer"] << "Could not play SFX clip: no free track";
}

void
AudioSDL::TrackSFX::play_on_channel(int channel, float volume, float ratio) {
  if (channel < 0 || channel >= SFX_TRACKS) {
    return;
  }
  Mix_HaltChannel(channel);
  sfx_track_volume[channel] = volume;
  Mix_Volume(channel, static_cast<int>(sfx_master_volume * volume *
                                       MIX_MAX_VOLUME));
  if (Mix_PlayChannel(channel, chunk, 0) == -1) {
    Log::Error["audio:SDL_mixer"] << "Could not play SFX clip: "
                                  << SDL_GetError();
  }
}

AudioSDL::PlayerMIDI::PlayerMIDI() {
  if (current_midi_player != nullptr) {
    throw ExceptionSDLmixer("Only one midi player is allowed");
  }
  current_track = TypeMidiNone;
  current_midi_player = this;
  Mix_HookMusicFinished(music_finished_hook);
}

AudioSDL::PlayerMIDI::~PlayerMIDI() {
  current_midi_player = nullptr;
  Mix_HookMusicFinished(nullptr);
}

Audio::PTrack
AudioSDL::PlayerMIDI::create_track(int track_id) {
  Data &data = Data::get_instance();
  Data::PSource data_source = data.get_data_source();

  PBuffer midi = data_source->get_music(track_id);
  if (!midi) {
    return nullptr;
  }

  /* MIDI (DOS data) is played by the timidity decoder of SDL2_mixer;
     MOD music (Amiga data) needs no setup. */
  SDL_RWops *rw = SDL_RWFromConstMem(midi->get_data(), midi->get_size());
  Mix_Music *music = Mix_LoadMUS_RW(rw, 1);
  if (music == nullptr) {
    Log::Warn["audio:SDL_mixer"] << "Could not load music track: "
                                 << SDL_GetError();
    return nullptr;
  }

  return std::make_shared<AudioSDL::TrackMIDI>(midi, music);
}

Audio::PTrack
AudioSDL::PlayerMIDI::play_track(int track_id) {
  /* Skip missing tracks; after the last one start again with the first,
     but only once, so that it ends when no track can be played. */
  Audio::PTrack track;
  bool wrapped = false;
  while (!track) {
    if ((track_id <= TypeMidiNone) || (track_id > TypeMidiTrackLast)) {
      if (wrapped) {
        break;
      }
      wrapped = true;
      track_id = TypeMidiTrack0;
    }
    current_track = static_cast<TypeMidi>(track_id);
    Log::Info["audio:SDL_mixer"] << "Playing MIDI track: " << current_track;
    track = Audio::Player::play_track(track_id);
    track_id++;
  }
  return track;
}

void
AudioSDL::PlayerMIDI::enable(bool enable) {
  enabled = enable;
  if (!enabled) {
    stop();
  } else if (!Mix_PlayingMusic()) {
    /* Start the music again where it was stopped. */
    play_track((current_track == TypeMidiNone) ? TypeMidiTrack0 :
                                                 current_track);
  }
}

void
AudioSDL::PlayerMIDI::clear_cache() {
  /* The old tracks are kept until the new one plays: the track playing
     reads from its data. */
  TrackCache old_tracks;
  old_tracks.swap(track_cache);
  if (enabled) {
    play_track((current_track == TypeMidiNone) ? TypeMidiTrack0 :
                                                 current_track);
  } else {
    Mix_HookMusicFinished(nullptr);
    Mix_HaltMusic();
    Mix_HookMusicFinished(PlayerMIDI::music_finished_hook);
  }
}

void
AudioSDL::data_changed(bool sounds, bool music) {
  /* Sound effects are decoded when loaded, the playing ones keep their
     audio (Mix_Chunk is reference counted). */
  if (sounds && sfx_player) sfx_player->clear_cache();
  if (music && midi_player) midi_player->clear_cache();
}

void
AudioSDL::PlayerMIDI::stop() {
  Mix_HaltMusic();
}

float
AudioSDL::PlayerMIDI::get_volume() {
  return static_cast<float>(Mix_VolumeMusic(-1)) / MIX_MAX_VOLUME;
}

void
AudioSDL::PlayerMIDI::set_volume(float volume) {
  volume = std::max(0.f, std::min(volume, 1.f));
  Mix_VolumeMusic(static_cast<int>(volume * MIX_MAX_VOLUME));
}

void
AudioSDL::PlayerMIDI::volume_up() {
  set_volume(get_volume() + 0.1f);
}

void
AudioSDL::PlayerMIDI::volume_down() {
  set_volume(get_volume() - 0.1f);
}

AudioSDL::PlayerMIDI *
AudioSDL::PlayerMIDI::current_midi_player = nullptr;

void
AudioSDL::PlayerMIDI::music_finished_hook() {
  if (current_midi_player != nullptr) {
    EventLoop &event_loop = EventLoop::get_instance();
    event_loop.deferred_call([](void*){
      current_midi_player->music_finished();
    }, nullptr);
  }
}

void
AudioSDL::PlayerMIDI::music_finished() {
  if (is_enabled()) {
    Audio::PTrack t = play_track(current_track + 1);
  }
}

AudioSDL::TrackMIDI::TrackMIDI(PBuffer _data, Mix_Music *_music)
  : data(_data)
  , music(_music) {
}

AudioSDL::TrackMIDI::~TrackMIDI() {
  Mix_FreeMusic(music);
}

void
AudioSDL::TrackMIDI::play() {
  /* Replacing the playing track must not run the finished hook. */
  Mix_HookMusicFinished(nullptr);
  Mix_HaltMusic();
  bool r = (Mix_PlayMusic(music, 0) == 0);
  Mix_HookMusicFinished(PlayerMIDI::music_finished_hook);
  if (!r) {
    Log::Warn["audio:SDL_mixer"] << "Could not play MIDI track: "
                                 << SDL_GetError();
    PlayerMIDI::music_finished_hook();
  }
}
