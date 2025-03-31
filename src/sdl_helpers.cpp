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
export auto operator<<(std::ostream &ostream, const SDL_FPoint &point) -> std::ostream & {
  ostream << point.x << ' ' << point.y;
  return ostream;
}

/// add SDL_Frect to an output stream in the form 'x y w h'
export auto operator<<(std::ostream &ostream, const SDL_FRect &rect) -> std::ostream & {
  ostream << rect.x << ' ' << rect.y << ' ' << rect.w << ' ' << rect.h;
  return ostream;
}

/// read an SDL_FPoint from an input stream in the form 'x y'
export auto operator>>(std::istream &istream, SDL_FPoint &point) -> std::istream & {
  istream >> point.x >> point.y;
  return istream;
}

/// read an SDL_Frect from an input stream in the form 'x y w h'
export auto operator>>(std::istream &istream, SDL_FRect &rect) -> std::istream & {
  istream >> rect.x >> rect.y >> rect.w >> rect.h;
  return istream;
}
