module;

#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"

#include <SDL3_image/SDL_image.h>

#include <cmath>
#include <string>
#include <utility>

export module sprite;
import tile;
import sdlHelpers;

export class CharacterSprite final : public Renderable {
public:
  CharacterSprite(const std::string &name, const SDL_FRect &rect, bool canRun,
                  bool canHit);

  CharacterSprite(const CharacterSprite &) = default;
  CharacterSprite(CharacterSprite &&) = delete;
  auto operator=(const CharacterSprite &) -> CharacterSprite & = default;
  auto operator=(CharacterSprite &&) -> CharacterSprite & = delete;

  ~CharacterSprite() override = default;

  [[nodiscard]] auto getIdleTextureRect() const -> SDL_FRect;
  [[nodiscard]] auto getRunTextureRect() const -> SDL_FRect;
  [[nodiscard]] auto getHitTextureRect() const -> SDL_FRect;
  [[nodiscard]] auto getTextureRect() -> SDL_FRect;
  [[nodiscard]] auto getDestRect(const SDL_FPoint &pos) const -> SDL_FRect;

  auto serialize(std::ostream &ostream) -> void override {}

  auto incIndex();

  auto setHit() { hit_ = true; }
  auto setRunning(bool dir) {
    this->running_ = true;
    direction_ = dir;
  }
  auto setRunning() {
    this->running_ = true;
  }
  auto setIdle() { this->running_ = false; }

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, size_t frameCount)
      -> void override;

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
};

CharacterSprite::CharacterSprite(const std::string &name, const SDL_FRect &rect,
                                 bool canRun, bool canHit)
    : Renderable{name, rect}, canRun_{canRun}, canHit_{canHit} {}

auto CharacterSprite::getIdleTextureRect() const -> SDL_FRect {

  auto sourceRect = getSourceRect();
  return {sourceRect.x + (index_ * sourceRect.w), sourceRect.y, sourceRect.w,
          sourceRect.h};
}

auto CharacterSprite::getRunTextureRect() const -> SDL_FRect {
  auto sourceRect = getSourceRect();
  return {sourceRect.x + ((runFrameIndex + index_) * sourceRect.w),
          sourceRect.y, sourceRect.w, sourceRect.h};
}

auto CharacterSprite::getHitTextureRect() const -> SDL_FRect {
  auto sourceRect = getSourceRect();
  return {sourceRect.x + (hitFrameIndex * sourceRect.w), sourceRect.y,
          sourceRect.w, sourceRect.h};
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

auto CharacterSprite::getDestRect(const SDL_FPoint &pos) const -> SDL_FRect {
  auto sourceRect = getSourceRect();
  return {pos.x, pos.y, sourceRect.w * 2, sourceRect.h * 2};
}

auto CharacterSprite::incIndex() {
  index_ = std::fmod(++index_, animationFrameNumber);
}

auto CharacterSprite::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                             size_t frameCount) -> void {
  if (frameCount % 2 == 0) {
    incIndex();
  }

  auto pos = getPos();

  auto sourceRect_ = getSourceRect();
  SDL_FRect destRect = getDestRect({pos.x *2, (pos.y -sourceRect_.h)*2});
  SDL_FRect sourceRect = getTextureRect();
  SDL_FPoint center{0, 0};

  if (direction_) {
    SDL_RenderTextureRotated(renderer, texture.get(), &sourceRect, &destRect, 0,
                             &center, SDL_FLIP_HORIZONTAL);
  } else {
    SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
  }
}
