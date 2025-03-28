module;

#include "SDL3/SDL_render.h"
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>
#include <variant>

export module tile;

using SdlTexturePtr = std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;

class StaticRenderer {
public:
  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,
              const SDL_FRect &rect, const SDL_FPoint &pos, size_t frameCount)
      -> void;
};

auto StaticRenderer::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                            const SDL_FRect &rect, const SDL_FPoint &pos,
                            size_t frameCount) -> void {

  SDL_FRect destRect{pos.x, pos.y, rect.w * 2, rect.h * 2};

  SDL_RenderTexture(renderer, texture.get(), &rect, &destRect);
}

std::ostream &operator<<(std::ostream &os, const StaticRenderer &) {
  os << "static";
  return os;
}

class AnimatedRenderer {
public:
  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,
              const SDL_FRect &rect, const SDL_FPoint &pos, size_t frameCount)
      -> void;

private:
  size_t _index;
  size_t _lastFrame;
};

std::ostream &operator<<(std::ostream &os, const AnimatedRenderer &) {
  os << "animated";
  return os;
}

auto AnimatedRenderer::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                              const SDL_FRect &rect, const SDL_FPoint &pos,
                              size_t frameCount) -> void {

  if (_lastFrame != frameCount && frameCount % 2 == 0) {
    _lastFrame = frameCount;
    _index = ++_index % 3;
  }

  SDL_FRect destRect{pos.x, pos.y, rect.w * 2, rect.h * 2};

  SDL_FRect sourceRect{rect.x + _index * rect.w, rect.y, rect.w, rect.h};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}

export class Tile {
  friend std::ostream &operator<<(std::ostream &os, const Tile &tile);
  friend std::istream &operator>>(std::istream &is, Tile &tile);

public:
  Tile() = default;
  Tile(std::string name, bool animated, const SDL_FRect &rect);

  auto name() -> const std::string & { return name_; }

  auto setPos(const SDL_FPoint &pos) { pos_ = pos; }
  auto getPos() -> const SDL_FPoint & { return pos_; }
  auto setLevel(bool level) -> void { level_ = level; }
  auto getLevel() -> bool { return level_; }
  auto getFlatenedPos() -> SDL_FPoint {
    SDL_FPoint p = pos_;
    p.y = p.y + sourceRect_.h * 2 + (level_ ? sourceRect_.h * 2: 0);
    return p;
  }

  auto isSamePos(const SDL_FPoint &pos) -> bool {
    return pos_.x == pos.x && pos_.y == pos.y;
  }

  auto operator<(const Tile &other) -> bool { return pos_.y < other.pos_.y; }

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, size_t frameCount)
      -> void;

private:
  std::string name_;

  SDL_FPoint pos_;

  SDL_FRect sourceRect_;
  std::variant<StaticRenderer, AnimatedRenderer> renderer_;
  bool level_;
};

Tile::Tile(std::string name, bool animated, const SDL_FRect &rect)
    : name_{name}, sourceRect_{rect}, level_{false} {
  if (animated)
    renderer_.emplace<AnimatedRenderer>();
  else
    renderer_.emplace<StaticRenderer>();
}

auto Tile::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                  size_t frameCount) -> void {
  std::visit(
      [this, renderer, &texture, frameCount](auto &r) {
        r.render(renderer, texture, sourceRect_, pos_, frameCount);
      },
      renderer_);
}

std::ostream &operator<<(std::ostream &os, const SDL_FPoint &point) {
  os << point.x << ' ' << point.y;
  return os;
}

std::ostream &operator<<(std::ostream &os, const SDL_FRect &rect) {
  os << rect.x << ' ' << rect.y << ' ' << rect.w << ' ' << rect.h;
  return os;
}

std::istream &operator>>(std::istream &is, SDL_FPoint &point) {
  is >> point.x >> point.y;
  return is;
}

std::istream &operator>>(std::istream &is, SDL_FRect &rect) {
  is >> rect.x >> rect.y >> rect.w >> rect.h;
  return is;
}

export std::ostream &operator<<(std::ostream &os, const Tile &tile) {
  std::visit(
      [&os, &tile](const auto &r) {
        os << tile.name_ << ' ' << tile.level_ << ' ' << r << ' ' << tile.pos_
           << ' ' << tile.sourceRect_;
      },
      tile.renderer_);
  return os;
}

export std::istream &operator>>(std::istream &is, Tile &tile) {
  auto g = is.tellg();
  std::string animated;
  is >> tile.name_ >> tile.level_ >> animated >> tile.pos_ >> tile.sourceRect_;

  if (!is) {
    is.clear();
    is.seekg(g);
    is.setstate(std::ios_base::failbit);
    return is;
  }

  if (animated == "animated")
    tile.renderer_.emplace<AnimatedRenderer>();
  else
    tile.renderer_.emplace<StaticRenderer>();

  return is;
}
