module;

#include "SDL3/SDL_render.h"

#include <istream>
#include <memory>
#include <ostream>

export module sdlHelpers;

/// Used to  auto delete SDL_Texture
export using SdlTexturePtr =
    std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;

/// Used to  auto delete SDL_Surface
export using SdlSurfacePtr =
    std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)>;

/// Used to auto delete SDL_Window
export using SdlWindowPtr = std::unique_ptr<SDL_Window, void (*)(SDL_Window *)>;

/// add SDL_FPoint to an output stream in the form 'x y'
export std::ostream &operator<<(std::ostream &Os, const SDL_FPoint &Point) {
  Os << Point.x << ' ' << Point.y;
  return Os;
}

/// add SDL_Frect to an output stream in the form 'x y w h'
export std::ostream &operator<<(std::ostream &Os, const SDL_FRect &Rect) {
  Os << Rect.x << ' ' << Rect.y << ' ' << Rect.w << ' ' << Rect.h;
  return Os;
}

/// read an SDL_FPoint from an input stream in the form 'x y'
export std::istream &operator>>(std::istream &Is, SDL_FPoint &Point) {
  Is >> Point.x >> Point.y;
  return Is;
}

/// read an SDL_Frect from an input stream in the form 'x y w h'
export std::istream &operator>>(std::istream &Is, SDL_FRect &Rect) {
  Is >> Rect.x >> Rect.y >> Rect.w >> Rect.h;
  return Is;
}
