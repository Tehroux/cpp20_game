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
  TileStatic(const SDL_FRect &rect);

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, float posX,
              float posY, size_t frameCount) -> void;

protected:
  SDL_FRect sourceRect_;
};

TileStatic::TileStatic(const SDL_FRect &rect) : sourceRect_{rect} {}

auto TileStatic::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                        float posX, float posY, size_t frameCount) -> void {

  SDL_FRect destRect{posX, posY, sourceRect_.w * 2, sourceRect_.h * 2};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect_, &destRect);
}

class AnimatedTile : public TileStatic {
public:
  AnimatedTile(const SDL_FRect &rect);

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, float posX,
              float posY, size_t frameCount) -> void;

private:
  size_t _index;
  size_t _lastFrame;
};

AnimatedTile::AnimatedTile(const SDL_FRect &rect)
    : TileStatic{rect}, _index{0}, _lastFrame{0} {}

auto AnimatedTile::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                          float posX, float posY, size_t frameCount) -> void {

  if (_lastFrame != frameCount && frameCount % 2 == 0) {
    _lastFrame = frameCount;
    _index = ++_index % 3;
  }

  SDL_FRect destRect{posX, posY, sourceRect_.w * 2, sourceRect_.h * 2};

  SDL_FRect sourceRect{sourceRect_.x + _index * sourceRect_.w, sourceRect_.y,
                       sourceRect_.w, sourceRect_.h};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}

export class Tile {
public:
  Tile(std::string name, bool animated, const SDL_FRect& rect);

  auto name() -> const std::string & { return name_; }

  auto setPos(uint32_t x, uint32_t y) {
    posX_ = x;
    posY_ = y;
  }

  auto isSamePos(uint32_t x, uint32_t y) -> bool {
    return posX_ == x && posY_ == y;
  }

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, size_t frameCount)
      -> void;

private:
  std::string name_;
  uint32_t posX_;
  uint32_t posY_;
  std::variant<TileStatic, AnimatedTile> tile_;
};

Tile::Tile(std::string name, bool animated, const SDL_FRect& rect)
    : name_{name} {
  if (animated)
    tile_.emplace<AnimatedTile>(rect);
  else
    tile_.emplace<TileStatic>(rect);
}

auto Tile::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                  size_t frameCount) -> void {
  std::visit(
      [this, renderer, &texture, frameCount](auto &tile) {
        tile.render(renderer, texture, posX_, posY_, frameCount);
      },
      tile_);
}
