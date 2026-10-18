/*
 * font-sdl.cc - TrueType font of the texts using SDL_ttf.
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

#include "src/font.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include "src/log.h"

/* Pixels per em of the font's glyphs, twice the original 8. */
static const int kGridSize = 16;

Font::Font()
  : font(nullptr) {
}

Font::~Font() {
  if (font != nullptr) {
    TTF_CloseFont(reinterpret_cast<TTF_Font*>(font));
    TTF_Quit();
  }
}

bool
Font::load_resource(const std::string &name) {
  const char *base = SDL_GetBasePath();
  if (base != nullptr && load(std::string(base) + "resources/" + name)) {
    return true;
  }
#ifdef FREESERF_RESOURCES_DIR
  /* Running from the build tree. */
  if (load(std::string(FREESERF_RESOURCES_DIR) + "/" + name)) {
    return true;
  }
#endif
  return false;
}

bool
Font::load(const std::string &path) {
  if (TTF_Init() != 0) {
    return false;
  }
  TTF_Font *ttf = TTF_OpenFont(path.c_str(), kGridSize);
  if (ttf == nullptr) {
    TTF_Quit();
    return false;
  }
  TTF_SetFontHinting(ttf, TTF_HINTING_MONO);
  font = ttf;
  return true;
}

const Font::Glyph *
Font::get_glyph(uint32_t ch) {
  auto it = glyphs.find(ch);
  if (it == glyphs.end()) {
    it = glyphs.emplace(ch, render(ch)).first;
  }
  return it->second.get();
}

std::unique_ptr<Font::Glyph>
Font::render(uint32_t ch) {
  TTF_Font *ttf = reinterpret_cast<TTF_Font*>(font);
  int minx = 0, maxx = 0, miny = 0, maxy = 0, advance = 0;
  if (ttf == nullptr || !TTF_GlyphIsProvided(ttf, ch) ||
      TTF_GlyphMetrics(ttf, ch, &minx, &maxx, &miny, &maxy, &advance) != 0) {
    return nullptr;
  }

  std::unique_ptr<Glyph> glyph(new Glyph());
  glyph->advance = advance / 2;
  glyph->left = 0;
  glyph->top = 0;
  glyph->width = 0;
  glyph->height = 0;

  SDL_Color white = { 0xff, 0xff, 0xff, 0xff };
  SDL_Surface *image = TTF_RenderGlyph_Solid(ttf, ch, white);
  if (image == nullptr) {
    return glyph;  /* No pixels, the space. */
  }
  SDL_Surface *surf = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_RGBA32,
                                               0);
  SDL_FreeSurface(image);
  if (surf == nullptr) {
    return glyph;
  }

  /* The surface is the full line box; the glyph is at (minx,
     kGridSize-maxy) within it. The image in the grid of 16 pixels per em,
     y from the top of the letters; the 2x2 blocks of the original pixels
     start at even coordinates. */
  int x0 = minx;
  int y0 = kGridSize - maxy;
  int x1 = maxx;
  int y1 = kGridSize - miny;
  int bx = x0 & ~1;
  int by = y0 & ~1;
  glyph->left = bx / 2;
  glyph->top = by / 2;
  glyph->width = (x1 - bx + 1) / 2;
  glyph->height = (y1 - by + 1) / 2;
  glyph->pixels.assign(glyph->width * glyph->height, false);

  SDL_LockSurface(surf);
  std::vector<int> count(glyph->pixels.size(), 0);
  for (int y = y0; y < y1; y++) {
    const uint8_t *row = reinterpret_cast<const uint8_t*>(surf->pixels) +
                         y * surf->pitch;
    for (int x = x0; x < x1; x++) {
      /* The edges of the pixels overlap a little in the outlines: only
         the pixels mostly covered count. */
      if (row[x * 4 + 3] >= 0x80) {
        int px = (x - bx) / 2;
        int py = (y - by) / 2;
        count[py * glyph->width + px]++;
      }
    }
  }
  SDL_UnlockSurface(surf);
  SDL_FreeSurface(surf);

  /* Scale2x changes at most one of the four pixels of an original one:
     two or more of them are a pixel. */
  for (size_t i = 0; i < count.size(); i++) {
    glyph->pixels[i] = (count[i] >= 2);
  }
  return glyph;
}
