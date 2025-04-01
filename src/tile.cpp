module;

#include "SDL3/SDL_render.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>

export module tile;
import sdlHelpers;

/// renderer concept
export template <class Type>
concept isRenderer =
    requires(Type type, SDL_Renderer *renderer, SdlTexturePtr &texture,
             const SDL_FRect &rect, const SDL_FPoint &pos, size_t frameCount) {
      { type.render(renderer, texture, rect, pos, frameCount) };
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
  static auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                     const SDL_FRect &rect, const SDL_FPoint &pos,
                     size_t frameCount) -> void;
};

auto StaticRenderer::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                            const SDL_FRect &rect, const SDL_FPoint &pos,
                            size_t /*FrameCount*/) -> void {

  SDL_FRect destRect{pos.x, pos.y - (rect.h * 2), rect.w * 2, rect.h * 2};

  SDL_RenderTexture(renderer, texture.get(), &rect, &destRect);
}

namespace {
auto operator<<(std::ostream &ostream, const StaticRenderer & /*unused*/)
    -> std::ostream & {
  ostream << "static";
  return ostream;
}
} // namespace

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
  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,
              const SDL_FRect &sourceRect, const SDL_FPoint &pos,
              size_t frameCount) -> void;

private:
  static constexpr float frameNumber = 3;

  float index_;      ///< the index in the animation fram list
  size_t lastFrame_; ///< the last frame count rendered
};

namespace {
/// output 'animated' to an output stream
///
/// \param[in] Os the output stream
///
/// \return the output stream
auto operator<<(std::ostream &ostream, const AnimatedRenderer & /*unused*/)
    -> std::ostream & {
  ostream << "animated";
  return ostream;
}
} // namespace

auto AnimatedRenderer::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                              const SDL_FRect &rect, const SDL_FPoint &pos,
                              size_t frameCount) -> void {

  if (lastFrame_ != frameCount && frameCount % 2 == 0) {
    lastFrame_ = frameCount;
    index_ = std::fmod(++index_, frameNumber);
  }

  SDL_FRect destRect{pos.x, pos.y - (rect.h * 2), rect.w * 2, rect.h * 2};

  SDL_FRect sourceRect{(index_ * rect.w) + rect.x, rect.y, rect.w, rect.h};

  SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}

export class Renderable {
public:
  Renderable(std::string name, const SDL_FRect &rect);

  Renderable(const Renderable &) = default;
  Renderable(Renderable &&) = delete;
  auto operator=(const Renderable &) -> Renderable & = default;
  auto operator=(Renderable &&) -> Renderable & = delete;
  virtual ~Renderable() = default;

  /// return the name the renderable
  auto name() -> const std::string & { return renderableName_; }

  auto setPos(const SDL_FPoint &pos) { renderablePos_ = pos; }
  [[nodiscard]] auto getPos() const -> SDL_FPoint {
    return {renderablePos_.x,
            renderablePos_.y + (renderableLevel_ ? sourceRect_.h * 2 : 0)};
  }

  auto setLevel(bool level) -> void { renderableLevel_ = level; }
  [[nodiscard]] auto getLevel() const -> bool { return renderableLevel_; }

  /// operator less than for sorting
  auto operator<(const Renderable &other) const -> bool {
    return getPos().y < other.getPos().y;
  }

  /// check if the renderable is in pos
  [[nodiscard]] auto isSamePos(const SDL_FPoint &pos) const -> bool {
    return renderablePos_.x == pos.x && renderablePos_.y == pos.y;
  }

  /// render the Renderable to the screen
  ///
  /// \param[in] Renderer the renderer used to render the renderable
  /// \param[in] Texture the texture containing the Renderable sprite
  /// \param[in] FrameCount the number of frame already drawn to the screen
  virtual auto render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                      size_t frameCount) -> void = 0;

  /// serialize the Renderable to an ostream
  virtual auto serialize(std::ostream &) -> void = 0;

protected:
  [[nodiscard]] auto getRenderableName() const -> std::string {
    return renderableName_;
  }
  [[nodiscard]] auto getRenderablePos() const -> SDL_FPoint {
    return renderablePos_;
  }
  [[nodiscard]] auto getSourceRect() const -> const SDL_FRect& { return sourceRect_; }
  [[nodiscard]] auto getRenderableLevel() const -> bool {
    return renderableLevel_;
  }

private:
  /// the Renderable name
  std::string renderableName_;
  /// the Renderable Pos on the screen
  SDL_FPoint renderablePos_{};
  /// the Renderable source rectangle on the texture
  SDL_FRect sourceRect_{};
  /// whether the Renderable is on the ground or in the air
  bool renderableLevel_{};
};

export auto operator<<(std::ostream &ostream, Renderable &renderable)
    -> std::ostream & {
  renderable.serialize(ostream);
  return ostream;
}

Renderable::Renderable(std::string name, const SDL_FRect &rect)
    : renderableName_{std::move(name)}, sourceRect_{rect} {}

/// concrete renderable class for tiles
///
/// \tparam RendererType the renderer type to use
export template <class RendererType>
  requires isRenderer<RendererType>
class Tile : public Renderable {
  friend auto operator<<(std::ostream &ostream, const Tile<RendererType> &tile)
      -> std::ostream &;
  friend auto operator>>(std::istream &istream, Tile<RendererType> &tile)
      -> std::istream &;

public:
  /// default constructor
  Tile() = default;

  /// constructor
  ///
  /// \param[in] Name the name of the Renderable
  /// \param[in] Rect the Renderable source area in the texture
  Tile(const std::string &name, const SDL_FRect &rect);

  /// render the renderable to the screen
  ///
  /// \param[in] Renderer the renderer used to render to the screen
  /// \param[in] Texture the texture to get the renderable sprite from
  /// \param[in] FrameCount the number of frame already drawn
  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, size_t frameCount)
      -> void override;

  /// serialize the Renderable to an output stream
  ///
  /// the renderable is write to the output stream in the form:\n
  /// RenderableName RendererType RenderableSourceRect RenderablePos
  /// RenderableLevel
  auto serialize(std::ostream &ostream) -> void override {
    ostream << getRenderableName() << ' ' << tileRenderer_ << ' '
            << getSourceRect() << ' ' << getRenderablePos() << ' '
            << getRenderableLevel();
  }

private:
  RendererType tileRenderer_;
};

template <class R>
  requires isRenderer<R>
Tile<R>::Tile(const std::string &name, const SDL_FRect &rect)
    : Renderable{name, rect} {}

template <class RendererType>
  requires isRenderer<RendererType>
auto Tile<RendererType>::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                                size_t frameCount) -> void {
  tileRenderer_.render(renderer, texture, getSourceRect(), getRenderablePos(),
                       frameCount);
}

/// Factory used to create Renderable
export class RendererBuilder {
  friend auto operator>>(std::istream &istream, RendererBuilder &builder)
      -> std::istream &;

public:
  /// default constructor
  RendererBuilder() = default;

  /// constructor
  ///
  /// \param[in] Name the name of the Renderable
  /// \param[in] Animated whether the Renterable is animated
  /// \param[in] SourceRect the source area for the renderable in the texture
  RendererBuilder(std::string_view name, bool animated,
                  const SDL_FRect &sourceRect)
      : renderableName_{name}, renderableSourceRect_{sourceRect},
        renderableIsAnimated_{animated} {}

  /// create the Renterable
  auto build() -> std::unique_ptr<Renderable>;

  /// get the name of the Renderable
  auto name() -> const std::string & { return renderableName_; }

private:
  /// the name of the Renderable
  std::string renderableName_;
  /// the Renderable rectangle area in the texture
  SDL_FRect renderableSourceRect_{};
  /// whether the Renderable is animated
  bool renderableIsAnimated_{};
  /// The renderable position on the screen
  SDL_FPoint renderablePos_{};
  /// whether the Renderable is on the ground or in the air
  bool renderableLevel_{};
};

auto RendererBuilder::build() -> std::unique_ptr<Renderable> {

  std::unique_ptr<Renderable> renderable;
  if (renderableIsAnimated_) {
    renderable = std::make_unique<Tile<AnimatedRenderer>>(
        renderableName_, renderableSourceRect_);
  } else {
    renderable = std::make_unique<Tile<StaticRenderer>>(renderableName_,
                                                        renderableSourceRect_);
  }

  renderable->setLevel(renderableLevel_);
  renderable->setPos(renderablePos_);
  return renderable;
}

/// read Builder from an input stream
/// the Builder information to read has to be in the form :\n
/// 'RenderableName RenderableType RenderableRourceRect RenderablePos
/// RenderableLevel'\n with:\n RenderableType = (Static|Animated) \n
/// RenderableSourceRect = x y w h\n
/// RenderablePos = x y
///
/// \param &Builer the Builder to put the information to
export auto operator>>(std::istream &istream, RendererBuilder &builder)
    -> std::istream & {

  auto streamPos = istream.tellg();
  std::string animated;

  istream >> builder.renderableName_ >> animated >>
      builder.renderableSourceRect_ >> builder.renderablePos_ >>
      builder.renderableLevel_;

  if (!istream) {
    istream.clear();
    istream.seekg(streamPos);
    istream.setstate(std::ios_base::failbit);
    return istream;
  }

  builder.renderableIsAnimated_ = (animated == "animated");

  return istream;
}
