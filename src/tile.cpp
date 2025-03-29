module;

#include "SDL3/SDL_render.h"
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>

export module tile;
import sdlHelpers;

export template <class T>
concept isRenderer =
    requires(T t, SDL_Renderer *renderer, SdlTexturePtr &texture,
             const SDL_FRect &rect, const SDL_FPoint &pos, size_t frameCount) {
      { t.render(renderer, texture, rect, pos, frameCount) };
    };

export class StaticRenderer {
public:
  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,
              const SDL_FRect &rect, const SDL_FPoint &pos, size_t frameCount)
      -> void;
};

auto StaticRenderer::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                            const SDL_FRect &rect, const SDL_FPoint &pos,
                            size_t frameCount) -> void {

  SDL_FRect destRect{pos.x, pos.y - rect.h *2, rect.w * 2, rect.h * 2};

  SDL_RenderTexture(renderer, texture.get(), &rect, &destRect);
}

std::ostream &operator<<(std::ostream &os, const StaticRenderer &) {
  os << "static";
  return os;
}

export class AnimatedRenderer {
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

  SDL_FRect destRect{pos.x, pos.y - rect.h * 2, rect.w * 2, rect.h * 2};

  SDL_FRect sourceRect{rect.x + _index * rect.w, rect.y, rect.w, rect.h};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}

export class Renderable {
public:
  Renderable(std::string name, const SDL_FRect &rect);
  virtual ~Renderable() = default;

  auto name() -> const std::string & { return name_; }

  auto setPos(const SDL_FPoint &pos) { pos_ = pos; }
  auto getPos() const -> SDL_FPoint  { return {pos_.x, pos_.y + (level_ ? sourceRect_.h * 2 : 0)} ; }
  auto setLevel(bool level) -> void { level_ = level; }
  auto getLevel() -> bool { return level_; }
  auto operator<(const Renderable &other) -> bool {
    return getPos().y < other.getPos().y;
  }

  auto isSamePos(const SDL_FPoint &pos) -> bool {
    return pos_.x == pos.x && pos_.y == pos.y;
  }

  virtual auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                      size_t frameCount) -> void = 0;

  virtual auto serialize(std::ostream &) -> void = 0;

protected:
  std::string name_;
  SDL_FPoint pos_;
  SDL_FRect sourceRect_;
  bool level_;
};

export std::ostream &operator<<(std::ostream &os, Renderable &r) {
  r.serialize(os);
  return os;
}

Renderable::Renderable(std::string name, const SDL_FRect &rect)
    : name_{name}, sourceRect_{rect}, level_{false} {}

export template <class R>
  requires isRenderer<R>
class Tile : public Renderable {
  friend std::ostream &operator<<(std::ostream &os, const Tile<R> &tile);
  friend std::istream &operator>>(std::istream &is, Tile<R> &tile);

public:
  Tile() = default;
  Tile(std::string name, const SDL_FRect &rect);

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, size_t frameCount)
      -> void override;

  virtual auto serialize(std::ostream &os) -> void override {
    os << name_ << ' ' << renderer_ << ' ' << sourceRect_ << ' ' << pos_ << ' '
       << level_;
  }

private:
  R renderer_;
};

template <class R>
  requires isRenderer<R>
Tile<R>::Tile(std::string name, const SDL_FRect &rect)
    : Renderable{name, rect} {}

template <class R>
  requires isRenderer<R>
auto Tile<R>::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                     size_t frameCount) -> void {
  renderer_.render(renderer, texture, sourceRect_, pos_, frameCount);
}

export class RendererBuilder {
  friend std::istream &operator>>(std::istream &is, RendererBuilder &builder);

public:
  RendererBuilder() = default;
  RendererBuilder(std::string_view name, bool animated,
                  const SDL_FRect &sourceRect)
      : name_{name}, sourceRect_{sourceRect}, isAnimated_{animated} {}

  std::unique_ptr<Renderable> build() {

    std::unique_ptr<Renderable> r;
    if (isAnimated_)
      r = std::make_unique<Tile<AnimatedRenderer>>(name_, sourceRect_);
    else
      r = std::make_unique<Tile<StaticRenderer>>(name_, sourceRect_);

    r->setLevel(level_);
    r->setPos(pos_);
    return r;
  }

  auto name() -> const std::string & { return name_; }

private:
  std::string name_;
  SDL_FRect sourceRect_;
  bool isAnimated_;
  SDL_FPoint pos_;
  bool level_;
};

std::istream &operator>>(std::istream &is, RendererBuilder &builder) {

  auto g = is.tellg();
  std::string animated;

  is >> builder.name_ >> animated >> builder.sourceRect_ >> builder.pos_ >>
      builder.level_;

  if (!is) {
    is.clear();
    is.seekg(g);
    is.setstate(std::ios_base::failbit);
    return is;
  }

  builder.isAnimated_ = (animated == "animated");

  return is;
}
