module;

#include <memory>
#include <stdint.h>
#include <string>

#include "SDL3/SDL_render.h"

export module tile;

using SdlTexturePtr = std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;

export class Tile {
public:
  Tile(std::string name, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

  auto name() -> const std::string & { return _name; }

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, int x, int y,
              size_t frameCount) -> void;

protected:
  std::string _name;
  uint32_t _x;
  uint32_t _y;
  uint32_t _w;
  uint32_t _h;
};

Tile::Tile(std::string name, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
    : _name{name}, _x{x}, _y{y}, _w{w}, _h{h} {}

auto Tile::render(SDL_Renderer *renderer, SdlTexturePtr &texture, int x, int y,
                  size_t frameCount) -> void {

  SDL_FRect destRect{static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(_w * 2), static_cast<float>(_h * 2)};
  SDL_FRect sourceRect{static_cast<float>(_x), static_cast<float>(_y),
                       static_cast<float>(_w), static_cast<float>(_h)};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}

export class AnimatedTile : public Tile {
public:
  AnimatedTile(std::string name, uint32_t x, uint32_t y, uint32_t w,
               uint32_t h);

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, int x, int y,
              size_t frameCount) -> void;

private:
  size_t _index;
  size_t _lastFrame;
};

AnimatedTile::AnimatedTile(std::string name, uint32_t x, uint32_t y, uint32_t w,
                           uint32_t h)
    : Tile{name, x, y, w, h}, _index{0}, _lastFrame{0} {}

auto AnimatedTile::render(SDL_Renderer *renderer, SdlTexturePtr &texture, int x,
                          int y, size_t frameCount) -> void {

  if (_lastFrame != frameCount && frameCount % 2 == 0) {
    _lastFrame = frameCount;
    _index = ++_index % 3;
  }

  SDL_FRect destRect{static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(_w * 2), static_cast<float>(_h * 2)};
  SDL_FRect sourceRect{static_cast<float>(_x + _index * _w),
                       static_cast<float>(_y), static_cast<float>(_w),
                       static_cast<float>(_h)};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}
