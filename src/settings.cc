/*
 * settings.cc - Settings kept between the runs of the game
 *
 * Copyright (C) 2026  Wicked_Digger <wicked_digger@mail.ru>
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

#include "src/settings.h"

#include <SDL.h>

#include <fstream>

#include "src/log.h"

/* SDL2 has no SDL_GetPathInfo (SDL3 only): check the file with the
   standard library instead. */
static bool
file_exists(const std::string &path) {
  std::ifstream f(path.c_str());
  return f.good();
}

Settings::Settings() {
  char *folder = SDL_GetPrefPath("", "freeserf");
  if (folder != nullptr) {
    path = std::string(folder) + "freeserf.ini";
    SDL_free(folder);
  }
}

Settings &
Settings::get_instance() {
  static Settings instance;
  return instance;
}

bool
Settings::load() {
  if (path.empty() || !file_exists(path)) {
    return false;
  }
  if (!config.load(path)) {
    Log::Warn["settings"] << "Unable to read the settings from " << path;
    config = ConfigFile();
    return false;
  }
  Log::Info["settings"] << "Settings loaded from " << path;
  return true;
}

bool
Settings::save() {
  if (path.empty() || !config.save(path)) {
    Log::Warn["settings"] << "Unable to save the settings to " << path;
    return false;
  }
  return true;
}
