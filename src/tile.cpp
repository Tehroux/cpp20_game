module;

#include "SDL3/SDL_render.h"
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>

export module tile;
import sdlHelpers;

export template <class Type>
concept isRenderer =
    requires(Type T, SDL_Renderer *Renderer, SdlTexturePtr &Texture,
             const SDL_FRect &Rect, const SDL_FPoint &Pos, size_t FrameCount) {
      { T.render(Renderer, Texture, Rect, Pos, FrameCount) };
    };

export class StaticRenderer {
public:
  auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
              const SDL_FRect &Rect, const SDL_FPoint &Pos, size_t FrameCount)
      -> void;
};

auto StaticRenderer::render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
                            const SDL_FRect &Rect, const SDL_FPoint &Pos,
                            size_t FrameCount) -> void {

  SDL_FRect DestRect{Pos.x, Pos.y - Rect.h * 2, Rect.w * 2, Rect.h * 2};

  SDL_RenderTexture(Renderer, Texture.get(), &Rect, &DestRect);
}

static std::ostream &operator<<(std::ostream &Os, const StaticRenderer &) {
  Os << "static";
  return Os;
}

export class AnimatedRenderer {
public:
  auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
              const SDL_FRect &Rect, const SDL_FPoint &Pos, size_t FrameCount)
      -> void;

private:
  size_t Index;
  size_t LastFrame;
};

static std::ostream &operator<<(std::ostream &Os, const AnimatedRenderer &) {
  Os << "animated";
  return Os;
}

auto AnimatedRenderer::render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
                              const SDL_FRect &Rect, const SDL_FPoint &Pos,
                              size_t FrameCount) -> void {

  if (LastFrame != FrameCount && FrameCount % 2 == 0) {
    LastFrame = FrameCount;
    Index = ++Index % 3;
  }

  SDL_FRect DestRect{Pos.x, Pos.y - Rect.h * 2, Rect.w * 2, Rect.h * 2};

  SDL_FRect SourceRect{Rect.x + Index * Rect.w, Rect.y, Rect.w, Rect.h};

  SDL_RenderTexture(Renderer, Texture.get(), &SourceRect, &DestRect);
}

export class Renderable {
public:
  Renderable(std::string Name, const SDL_FRect &Rect);
  virtual ~Renderable() = default;

  auto name() -> const std::string & { return RenderableName; }

  auto setPos(const SDL_FPoint &Pos) { RenderablePos = Pos; }
  auto getPos() const -> SDL_FPoint {
    return {RenderablePos.x,
            RenderablePos.y + (RenderableLevel ? SourceRect.h * 2 : 0)};
  }
  auto setLevel(bool Level) -> void { RenderableLevel = Level; }
  auto getLevel() -> bool { return RenderableLevel; }
  auto operator<(const Renderable &Other) -> bool {
    return getPos().y < Other.getPos().y;
  }

  auto isSamePos(const SDL_FPoint &Pos) -> bool {
    return RenderablePos.x == Pos.x && RenderablePos.y == Pos.y;
  }

  virtual auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
                      size_t FrameCount) -> void = 0;

  virtual auto serialize(std::ostream &) -> void = 0;

protected:
  std::string RenderableName;
  SDL_FPoint RenderablePos;
  SDL_FRect SourceRect;
  bool RenderableLevel;
};

export std::ostream &operator<<(std::ostream &Os, Renderable &R) {
  R.serialize(Os);
  return Os;
}

Renderable::Renderable(std::string Name, const SDL_FRect &Rect)
    : RenderableName{Name}, SourceRect{Rect}, RenderableLevel{false} {}

export template <class RendererType>
  requires isRenderer<RendererType>
class Tile : public Renderable {
  friend std::ostream &operator<<(std::ostream &Os,
                                  const Tile<RendererType> &Tile);
  friend std::istream &operator>>(std::istream &Is, Tile<RendererType> &Tile);

public:
  Tile() = default;
  Tile(std::string Name, const SDL_FRect &Rect);

  auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture, size_t FrameCount)
      -> void override;

  virtual auto serialize(std::ostream &Os) -> void override {
    Os << RenderableName << ' ' << TileRenderer << ' ' << SourceRect << ' '
       << RenderablePos << ' ' << RenderableLevel;
  }

private:
  RendererType TileRenderer;
};

template <class R>
  requires isRenderer<R>
Tile<R>::Tile(std::string Name, const SDL_FRect &Rect)
    : Renderable{Name, Rect} {}

template <class R>
  requires isRenderer<R>
auto Tile<R>::render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
                     size_t FrameCount) -> void {
  TileRenderer.render(Renderer, Texture, SourceRect, RenderablePos, FrameCount);
}

export class RendererBuilder {
  friend std::istream &operator>>(std::istream &Is, RendererBuilder &Builder);

public:
  RendererBuilder() = default;
  RendererBuilder(std::string_view Name, bool Animated,
                  const SDL_FRect &SourceRect)
      : RenderableName{Name}, RenderableSourceRect{SourceRect},
        RenderableIsAnimated{Animated} {}

  std::unique_ptr<Renderable> build() {

    std::unique_ptr<Renderable> Renderable;
    if (RenderableIsAnimated)
      Renderable = std::make_unique<Tile<AnimatedRenderer>>(
          RenderableName, RenderableSourceRect);
    else
      Renderable = std::make_unique<Tile<StaticRenderer>>(RenderableName,
                                                          RenderableSourceRect);

    Renderable->setLevel(RenderableLevel);
    Renderable->setPos(RenderablePos);
    return Renderable;
  }

  auto name() -> const std::string & { return RenderableName; }

private:
  std::string RenderableName;
  SDL_FRect RenderableSourceRect;
  bool RenderableIsAnimated;
  SDL_FPoint RenderablePos;
  bool RenderableLevel;
};

std::istream &operator>>(std::istream &Is, RendererBuilder &Builder) {

  auto StreamPos = Is.tellg();
  std::string Animated;

  Is >> Builder.RenderableName >> Animated >> Builder.RenderableSourceRect >>
      Builder.RenderablePos >> Builder.RenderableLevel;

  if (!Is) {
    Is.clear();
    Is.seekg(StreamPos);
    Is.setstate(std::ios_base::failbit);
    return Is;
  }

  Builder.RenderableIsAnimated = (Animated == "animated");

  return Is;
}
