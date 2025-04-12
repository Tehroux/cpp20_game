module;

#include "SDL3/SDL_rect.h"

#include <SDL3_image/SDL_image.h>

#include <cmath>
#include <string>
#include <utility>

export module sprite;
import tile;
import sdlHelpers;

export class CharacterSprite final : public Renderable {
public:
  CharacterSprite(std::string name, const SDL_FRect &rect, bool canRun,
                  bool canHit);

  CharacterSprite(const CharacterSprite &) = default;
  CharacterSprite(CharacterSprite &&) = delete;
  auto operator=(const CharacterSprite &) -> CharacterSprite & = default;
  auto operator=(CharacterSprite &&) -> CharacterSprite & = delete;

  ~CharacterSprite() override = default;

  [[nodiscard]] auto getIdleTextureRect() const noexcept -> SDL_FRect;
  [[nodiscard]] auto getRunTextureRect() const noexcept -> SDL_FRect;
  [[nodiscard]] auto getHitTextureRect() const noexcept -> SDL_FRect;
  [[nodiscard]] auto getTextureRect() -> SDL_FRect;
  [[nodiscard]] auto getDestRect() const noexcept -> SDL_FRect;

  auto serialize(std::ostream &ostream) -> void override {}

  [[nodiscard]] auto name() const noexcept -> std::string override {
    return renderableName_;
  }

  auto incIndex();

  auto setHit() { hit_ = true; }
  auto setRunning(bool dir) {
    this->running_ = true;
    direction_ = dir;
  }
  auto setRunning() { this->running_ = true; }
  auto setIdle() { this->running_ = false; }

  auto render(const SdlRenderer &renderer, const SdlTexturePtr &texture,
              size_t frameCount) -> void override;

  [[nodiscard]] auto isSamePos(const SDL_FPoint &pos) const -> bool override {
    return renderablePos_.x == pos.x && renderablePos_.y == pos.y;
  }

  auto setPos(const SDL_FPoint &pos) noexcept -> void { renderablePos_ = pos; };
  [[nodiscard]] auto getPos() const noexcept -> SDL_FPoint override {
    return {renderablePos_.x,
            renderablePos_.y + (renderableLevel_ ? sourceRect_.h : 0)};
  }

private:
  static constexpr float runFrameIndex = 4;
  static constexpr float hitFrameIndex = 8;
  static constexpr float animationFrameNumber = 4;

  float index_{};
  bool hit_{};
  bool running_{};
  bool direction_{};
  bool canRun_;
  bool canHit_;
  int hitFrame_ = 0;

  std::string renderableName_;
  /// the Renderable rectangle area in the texture
  SDL_FRect sourceRect_{};
  /// whether the Renderable is animated
  bool renderableIsAnimated_{};
  /// The renderable position on the screen
  SDL_FPoint renderablePos_{};
  /// whether the Renderable is on the ground or in the air
  bool renderableLevel_{};
};

CharacterSprite::CharacterSprite(std::string name, const SDL_FRect &rect,
                                 bool canRun, bool canHit)
    : renderableName_{std::move(name)}, sourceRect_{rect},
      canRun_{canRun}, canHit_{canHit} {}

auto CharacterSprite::getIdleTextureRect() const noexcept -> SDL_FRect {

  return {sourceRect_.x + (index_ * sourceRect_.w),
          sourceRect_.y, sourceRect_.w,
          sourceRect_.h};
}

auto CharacterSprite::getRunTextureRect() const noexcept -> SDL_FRect {
  return {sourceRect_.x +
              ((runFrameIndex + index_) * sourceRect_.w),
          sourceRect_.y, sourceRect_.w,
          sourceRect_.h};
}

auto CharacterSprite::getHitTextureRect() const noexcept -> SDL_FRect {
  return {sourceRect_.x + (hitFrameIndex * sourceRect_.w),
          sourceRect_.y, sourceRect_.w,
          sourceRect_.h};
}

auto CharacterSprite::getTextureRect() -> SDL_FRect {
  if (canHit_ && hit_) {
    if (++hitFrame_ == 2) {
      hitFrame_ = 0;
      hit_ = false;
    }
    return getHitTextureRect();
  }

  if (canRun_ && running_) {
    return getRunTextureRect();
  }

  return getIdleTextureRect();
}

auto CharacterSprite::getDestRect() const noexcept -> SDL_FRect {
  const SDL_FRect pos{.x = renderablePos_.x * 2,
                      .y = (renderablePos_.y - sourceRect_.h) * 2};

  return {pos.x, pos.y, sourceRect_.w * 2,
          sourceRect_.h * 2};
}

auto CharacterSprite::incIndex() {
  index_ = std::fmod(++index_, animationFrameNumber);
}

auto CharacterSprite::render(const SdlRenderer &renderer,
                             const SdlTexturePtr &texture, size_t frameCount)
    -> void {
  if (frameCount % 2 == 0) {
    incIndex();
  }

  const auto destRect = getDestRect();
  const auto sourceTextureRect = getTextureRect();

  if (direction_) {
    const SDL_FPoint center{0, 0};
    renderer.renderTextureRotated(texture, sourceTextureRect, destRect, 0,
                                  center, SDL_FLIP_HORIZONTAL);
  } else {
    renderer.renderTexture(texture, sourceTextureRect, destRect);
  }
}
