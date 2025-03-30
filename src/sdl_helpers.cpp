module;

#include "SDL3/SDL_render.h"

#include <istream>
#include <memory>
#include <ostream>

export module sdlHelpers;

export using SdlTexturePtr =
    std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;
export using SdlSurfacePtr =
    std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)>;
export using SdlWindowPtr = std::unique_ptr<SDL_Window, void (*)(SDL_Window *)>;

export std::ostream &operator<<(std::ostream &Os, const SDL_FPoint &Point) {
  Os << Point.x << ' ' << Point.y;
  return Os;
}

export std::ostream &operator<<(std::ostream &Os, const SDL_FRect &Rect) {
  Os << Rect.x << ' ' << Rect.y << ' ' << Rect.w << ' ' << Rect.h;
  return Os;
}

export std::istream &operator>>(std::istream &Is, SDL_FPoint &Point) {
  Is >> Point.x >> Point.y;
  return Is;
}

export std::istream &operator>>(std::istream &Is, SDL_FRect &Rect) {
  Is >> Rect.x >> Rect.y >> Rect.w >> Rect.h;
  return Is;
}
