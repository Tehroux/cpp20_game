module;

#include "SDL3/SDL_rect.h"
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
    requires(Type type, const SdlRenderer &renderer,
             const SdlTexturePtr &texture, const SDL_FRect &rect,
             const SDL_FPoint &pos, size_t frameCount) {
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
  static auto render(const SdlRenderer &renderer, const SdlTexturePtr &texture,
                     const SDL_FRect &rect, const SDL_FPoint &pos,
                     size_t frameCount) -> void;
};

auto StaticRenderer::render(const SdlRenderer &renderer,
                            const SdlTexturePtr &texture, const SDL_FRect &rect,
                            const SDL_FPoint &pos, size_t /*FrameCount*/)
    -> void {

  const SDL_FRect destRect{pos.x * 2, (pos.y - rect.h) * 2, rect.w * 2,
                           rect.h * 2};

  renderer.renderTexture(texture, rect, destRect);
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
  auto render(const SdlRenderer &renderer, const SdlTexturePtr &texture,
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

auto AnimatedRenderer::render(const SdlRenderer &renderer,
                              const SdlTexturePtr &texture,
                              const SDL_FRect &rect, const SDL_FPoint &pos,
                              size_t frameCount) -> void {

  if (lastFrame_ != frameCount && frameCount % 2 == 0) {
    lastFrame_ = frameCount;
    index_ = std::fmod(++index_, frameNumber);
  }

  const SDL_FRect destRect{pos.x * 2, (pos.y - rect.h) * 2, rect.w * 2,
                           rect.h * 2};

  const SDL_FRect sourceRect{(index_ * rect.w) + rect.x, rect.y, rect.w,
                             rect.h};

  renderer.renderTexture(texture, sourceRect, destRect);
}

export class Renderable {
public:
  Renderable() = default;
  Renderable(const Renderable &) = default;
  Renderable(Renderable &&) = delete;
  auto operator=(const Renderable &) -> Renderable & = default;
  auto operator=(Renderable &&) -> Renderable & = delete;
  virtual ~Renderable() = default;

  /// return the name the renderable
  [[nodiscard]] virtual auto name() const noexcept -> std::string = 0;

  /// check if the renderable is in pos
  [[nodiscard]] virtual auto isSamePos(const SDL_FPoint &pos) const -> bool = 0;

  /// render the Renderable to the screen
  ///
  /// \param[in] Renderer the renderer used to render the renderable
  /// \param[in] Texture the texture containing the Renderable sprite
  /// \param[in] FrameCount the number of frame already drawn to the screen
  virtual auto render(const SdlRenderer &renderer, const SdlTexturePtr &texture,
                      size_t frameCount) -> void = 0;

  /// serialize the Renderable to an ostream
  virtual auto serialize(std::ostream &) -> void = 0;

  [[nodiscard]] virtual auto getPos() const noexcept -> SDL_FPoint = 0;
};

export auto operator<<(std::ostream &ostream, Renderable &renderable)
    -> std::ostream & {
  renderable.serialize(ostream);
  return ostream;
}

export class TileConcrete : public Renderable {};

/// concrete renderable class for tiles
///
/// \tparam RendererType the renderer type to use
export template <class RendererType>
  requires isRenderer<RendererType>
class Tile : public TileConcrete {
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
  Tile(std::string name, const SDL_FRect &rect, const SDL_FPoint &pos,
       bool level);

  [[nodiscard]] auto name() const noexcept -> std::string override {
    return name_;
  }

  [[nodiscard]] auto getPos() const noexcept -> SDL_FPoint override {
    return {renderablePos_.x,
            renderablePos_.y + (renderableLevel_ ? sourceRect_.h : 0)};
  }

  /// render the renderable to the screen
  ///
  /// \param[in] Renderer the renderer used to render to the screen
  /// \param[in] Texture the texture to get the renderable sprite from
  /// \param[in] FrameCount the number of frame already drawn
  auto render(const SdlRenderer &renderer, const SdlTexturePtr &texture,
              size_t frameCount) -> void override;

  /// serialize the Renderable to an output stream
  ///
  /// the renderable is write to the output stream in the form:\n
  /// RenderableName RendererType RenderableSourceRect RenderablePos
  /// RenderableLevel
  auto serialize(std::ostream &ostream) -> void override {
    ostream << name_ << ' ' << tileRenderer_ << ' ' << sourceRect_ << ' '
            << renderablePos_ << ' ' << renderableLevel_;
  }

  auto setLevel(bool level) -> void { renderableLevel_ = level; }
  [[nodiscard]] auto getLevel() const -> bool { return renderableLevel_; }

  auto setPos(const SDL_FPoint &pos) -> void { renderablePos_ = pos; }

  [[nodiscard]] auto isSamePos(const SDL_FPoint &pos) const -> bool override {
    return renderablePos_.x == pos.x && renderablePos_.y == pos.y;
  }

private:
  std::string name_;
  RendererType tileRenderer_;
  /// whether the Renderable is on the ground or in the air
  bool renderableLevel_{};

  /// the Renderable Pos on the screen
  SDL_FPoint renderablePos_{};
  /// the Renderable source rectangle on the texture
  SDL_FRect sourceRect_{};
};

template <class R>
  requires isRenderer<R>
Tile<R>::Tile(std::string name, const SDL_FRect &rect, const SDL_FPoint &pos,
              bool level)
    : name_{std::move(name)}, renderableLevel_{level}, renderablePos_{pos},
      sourceRect_{rect} {}

template <class RendererType>
  requires isRenderer<RendererType>
auto Tile<RendererType>::render(const SdlRenderer &renderer,
                                const SdlTexturePtr &texture, size_t frameCount)
    -> void {
  tileRenderer_.render(renderer, texture, sourceRect_, renderablePos_,
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
  auto build(const SDL_FPoint &pos, bool level)
      -> std::unique_ptr<TileConcrete>;
  auto build() -> std::unique_ptr<TileConcrete>;

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

auto RendererBuilder::build(const SDL_FPoint &pos, bool level)
    -> std::unique_ptr<TileConcrete> {

  std::unique_ptr<TileConcrete> renderable;
  if (renderableIsAnimated_) {
    renderable = std::make_unique<Tile<AnimatedRenderer>>(
        renderableName_, renderableSourceRect_, pos, level);
  } else {
    renderable = std::make_unique<Tile<StaticRenderer>>(
        renderableName_, renderableSourceRect_, pos, level);
  }

  return renderable;
}
auto RendererBuilder::build() -> std::unique_ptr<TileConcrete> {

  std::unique_ptr<TileConcrete> renderable;
  if (renderableIsAnimated_) {
    renderable = std::make_unique<Tile<AnimatedRenderer>>(
        renderableName_, renderableSourceRect_, renderablePos_,
        renderableLevel_);
  } else {
    renderable = std::make_unique<Tile<StaticRenderer>>(
        renderableName_, renderableSourceRect_, renderablePos_,
        renderableLevel_);
  }

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
