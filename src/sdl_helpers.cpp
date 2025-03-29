module;

#include "SDL3/SDL_render.h"

#include <memory>
#include <iostream>

export module sdlHelpers;

export using SdlTexturePtr = std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;
export using SdlSurfacePtr = std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)>;
export using SdlWindowPtr = std::unique_ptr<SDL_Window, void (*)(SDL_Window *)>;


export std::ostream &operator<<(std::ostream &os, const SDL_FPoint &point) {
  os << point.x << ' ' << point.y;
  return os;
}

export std::ostream &operator<<(std::ostream &os, const SDL_FRect &rect) {
  os << rect.x << ' ' << rect.y << ' ' << rect.w << ' ' << rect.h;
  return os;
}

export std::istream &operator>>(std::istream &is, SDL_FPoint &point) {
  is >> point.x >> point.y;
  return is;
}

export std::istream &operator>>(std::istream &is, SDL_FRect &rect) {
  is >> rect.x >> rect.y >> rect.w >> rect.h;
  return is;
}

