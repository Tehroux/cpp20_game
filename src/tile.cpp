module;

#include "SDL3/SDL_render.h"
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>

export module tile;
import sdlHelpers;

/// renderer concept
export template <class Type>
concept isRenderer =
    requires(Type T, SDL_Renderer *Renderer, SdlTexturePtr &Texture,
             const SDL_FRect &Rect, const SDL_FPoint &Pos, size_t FrameCount) {
      { T.render(Renderer, Texture, Rect, Pos, FrameCount) };
    };

/// renderer for static sprite
export class StaticRenderer {
public:
  /// render the static sprite to the screen
  ///
  /// \param[in] Renderer the renderer to use to render the static sprite to the
  /// screen
  /// \param[in] Texture the texture to get the sprite from
  /// \param[in] SourceRect the source area for the static sprites
  /// \param[in] Pos the position on the screen where to render the static
  /// sprite
  /// \param[in] FrameCount the number of frame already rendered
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

/// renderer for animated sprite
export class AnimatedRenderer {
public:
  /// render the animation sprite to the screen
  ///
  /// \param[in] Renderer the renderer to use to render the animation sprite to
  /// the screen
  /// \param[in] Texture the texture to get the sprite from
  /// \param[in] SourceRect the source area for the animation sprites
  /// \param[in] Pos the position on the screen where to render the animation
  /// sprite
  /// \param[in] FrameCount the number of frame already rendered
  auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
              const SDL_FRect &SourceRect, const SDL_FPoint &Pos,
              size_t FrameCount) -> void;

private:
  size_t Index;     ///< the index in the animation fram list
  size_t LastFrame; ///< the last frame count rendered
};

/// output 'animated' to an output stream
///
/// \param[in] Os the output stream
///
/// \return the output stream
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

  /// render the Renderable to the screen
  ///
  /// \param[in] Renderer the renderer used to render the renderable
  /// \param[in] Texture the texture containing the Renderable sprite
  /// \param[in] FrameCount the number of frame already drawn to the screen
  virtual auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
                      size_t FrameCount) -> void = 0;

  /// serialize the Renderable to an ostream
  virtual auto serialize(std::ostream &) -> void = 0;

protected:
  /// the Renderable name
  std::string RenderableName;
  /// the Renderable Pos on the screen
  SDL_FPoint RenderablePos;
  /// the Renderable source rectangle on the texture
  SDL_FRect SourceRect;
  /// whether the Renderable is on the ground or in the air
  bool RenderableLevel;
};

export std::ostream &operator<<(std::ostream &Os, Renderable &R) {
  R.serialize(Os);
  return Os;
}

Renderable::Renderable(std::string Name, const SDL_FRect &Rect)
    : RenderableName{Name}, SourceRect{Rect}, RenderableLevel{false} {}

/// concrete renderable class for tiles
///
/// \tparam RendererType the renderer type to use
export template <class RendererType>
  requires isRenderer<RendererType>
class Tile : public Renderable {
  friend std::ostream &operator<<(std::ostream &Os,
                                  const Tile<RendererType> &Tile);
  friend std::istream &operator>>(std::istream &Is, Tile<RendererType> &Tile);

public:
  /// default constructor
  Tile() = default;

  /// constructor
  ///
  /// \param[in] Name the name of the Renderable
  /// \param[in] Rect the Renderable source area in the texture
  Tile(std::string Name, const SDL_FRect &Rect);

  /// render the renderable to the screen
  ///
  /// \param[in] Renderer the renderer used to render to the screen
  /// \param[in] Texture the texture to get the renderable sprite from
  /// \param[in] FrameCount the number of frame already drawn
  auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture, size_t FrameCount)
      -> void override;

  /// serialize the Renderable to an output stream
  ///
  /// the renderable is write to the output stream in the form:\n
  /// RenderableName RendererType RenderableSourceRect RenderablePos
  /// RenderableLevel
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

template <class RendererType>
  requires isRenderer<RendererType>
auto Tile<RendererType>::render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
                                size_t FrameCount) -> void {
  TileRenderer.render(Renderer, Texture, SourceRect, RenderablePos, FrameCount);
}

/// Factory used to create Renderable
export class RendererBuilder {
  friend std::istream &operator>>(std::istream &Is, RendererBuilder &Builder);

public:
  /// default constructor
  RendererBuilder() = default;

  /// constructor
  ///
  /// \param[in] Name the name of the Renderable
  /// \param[in] Animated whether the Renterable is animated
  /// \param[in] SourceRect the source area for the renderable in the texture
  RendererBuilder(std::string_view Name, bool Animated,
                  const SDL_FRect &SourceRect)
      : RenderableName{Name}, RenderableSourceRect{SourceRect},
        RenderableIsAnimated{Animated} {}

  /// create the Renterable
  auto build() -> std::unique_ptr<Renderable>;

  /// get the name of the Renderable
  auto name() -> const std::string & { return RenderableName; }

private:
  /// the name of the Renderable
  std::string RenderableName;
  /// the Renderable rectangle area in the texture
  SDL_FRect RenderableSourceRect;
  /// whether the Renderable is animated
  bool RenderableIsAnimated;
  /// The renderable position on the screen
  SDL_FPoint RenderablePos;
  /// whether the Renderable is on the ground or in the air
  bool RenderableLevel;
};

std::unique_ptr<Renderable> RendererBuilder::build() {

  std::unique_ptr<Renderable> Renderable;
  if (RenderableIsAnimated)
    Renderable = std::make_unique<Tile<AnimatedRenderer>>(RenderableName,
                                                          RenderableSourceRect);
  else
    Renderable = std::make_unique<Tile<StaticRenderer>>(RenderableName,
                                                        RenderableSourceRect);

  Renderable->setLevel(RenderableLevel);
  Renderable->setPos(RenderablePos);
  return Renderable;
}

/// read Builder from an input stream
/// the Builder information to read has to be in the form :\n
/// 'RenderableName RenderableType RenderableRourceRect RenderablePos
/// RenderableLevel'\n with:\n RenderableType = (Static|Animated) \n
/// RenderableSourceRect = x y w h\n
/// RenderablePos = x y
///
/// \param &Builer the Builder to put the information to
export std::istream &operator>>(std::istream &Is, RendererBuilder &Builder) {

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
