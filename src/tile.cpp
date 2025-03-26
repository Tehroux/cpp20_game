module;

#include <memory>
#include <stdint.h>
#include <string>
#include <variant>

#include "SDL3/SDL_render.h"

export module tile;

using SdlTexturePtr = std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;

class TileStatic {
public:
  TileStatic() = default;
  TileStatic(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, int posX, int posY,size_t frameCount)
      -> void;

protected:
  uint32_t _x;
  uint32_t _y;
  uint32_t _w;
  uint32_t _h;
};

TileStatic::TileStatic(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
    : _x{x}, _y{y}, _w{w}, _h{h}{}

auto TileStatic::render(SDL_Renderer *renderer, SdlTexturePtr &texture, int posX, int posY,
                  size_t frameCount) -> void {

  SDL_FRect destRect{static_cast<float>(posX), static_cast<float>(posY),
                     static_cast<float>(_w * 2), static_cast<float>(_h * 2)};

  SDL_FRect sourceRect{static_cast<float>(_x), static_cast<float>(_y),
                       static_cast<float>(_w), static_cast<float>(_h)};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}

class AnimatedTile : public TileStatic {
public:
  AnimatedTile( uint32_t x, uint32_t y, uint32_t w, uint32_t h);

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, int posX, int posY, size_t frameCount)
      -> void;

private:
  size_t _index;
  size_t _lastFrame;
};

AnimatedTile::AnimatedTile(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
    : TileStatic{x, y, w, h}, _index{0}, _lastFrame{0} {}

auto AnimatedTile::render(SDL_Renderer *renderer, SdlTexturePtr &texture, int posX, int posY, size_t frameCount) -> void {

  if (_lastFrame != frameCount && frameCount % 2 == 0) {
    _lastFrame = frameCount;
    _index = ++_index % 3;
  }

  SDL_FRect destRect{static_cast<float>(posX), static_cast<float>(posY),
                     static_cast<float>(_w * 2), static_cast<float>(_h * 2)};

  SDL_FRect sourceRect{static_cast<float>(_x + _index * _w),
                       static_cast<float>(_y), static_cast<float>(_w),
                       static_cast<float>(_h)};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}

export class Tile{
public:
  Tile(std::string name, bool animated, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

  auto name() -> const std::string & { return name_; }

  auto setPos(uint32_t x, uint32_t y) {
    posX_ = x;
    posY_ = y;
  }

  auto isSamePos(uint32_t x, uint32_t y) -> bool{
    return posX_ == x && posY_ == y;
  }

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,size_t frameCount) -> void;

private:
  std::string name_;
  uint32_t posX_;
  uint32_t posY_;
  std::variant<TileStatic, AnimatedTile> tile_;
};

Tile::Tile(std::string name, bool animated, uint32_t x, uint32_t y, uint32_t w, uint32_t h): name_{name} {
  if ( animated )
    tile_.emplace<AnimatedTile>(x, y, w, h);
  else 
    tile_.emplace<TileStatic>(x, y, w, h);
}

auto Tile::render(SDL_Renderer *renderer, SdlTexturePtr &texture,size_t frameCount) -> void{
  std::visit([this, renderer, &texture, frameCount](auto &tile) {
    tile.render(renderer, texture, posX_, posY_, frameCount);
  }, tile_);
}
