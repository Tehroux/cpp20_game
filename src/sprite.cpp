module;

#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"

#include <SDL3_image/SDL_image.h>

#include <cmath>
#include <memory>
#include <string>

export module sprite;
import tile;
import sdlHelpers;

export using SdlTexturePtr =
    std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;

export class CharacterSprite {
public:
  CharacterSprite(const std::string &Name, const SDL_FRect &Rect, bool CanRun,
                  bool CantHit);

  auto name() -> const std::string & { return Name; }

  auto getIdleTextureRect() -> SDL_FRect;
  auto getRunTextureRect() -> SDL_FRect;
  auto getHitTextureRect() -> SDL_FRect;
  auto getTextureRect() -> SDL_FRect;
  auto getDestRect(const SDL_FPoint &Pos) -> SDL_FRect;

  auto incIndex();

  auto setHit() { Hit = true; }
  auto setRunning(bool Dir) {
    this->Running = true;
    Direction = Dir;
  }
  auto setIdle() { this->Running = false; }

  auto render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
              const SDL_FPoint &Pos, size_t FrameCount);

private:
  static constexpr float RunFrameIndex = 4;
  static constexpr float HitFrameIndex = 8;
  static constexpr float AnimationFrameNumber = 4;

  std::string Name;
  SDL_FRect SourceRect;
  float Index{};
  bool Hit{};
  bool Running{};
  bool Direction{};
  bool CanRun;
  bool CanHit;
  int HitFrame = 0;
};

CharacterSprite::CharacterSprite(const std::string &Name, const SDL_FRect &Rect,
                                 bool CanRun, bool CanHit)
    : Name{Name}, SourceRect{Rect}, CanRun{CanRun}, CanHit{CanHit} {}

auto CharacterSprite::getIdleTextureRect() -> SDL_FRect {
  return {SourceRect.x + Index * SourceRect.w, SourceRect.y, SourceRect.w,
          SourceRect.h};
}

auto CharacterSprite::getRunTextureRect() -> SDL_FRect {
  return {SourceRect.x + (RunFrameIndex + Index) * SourceRect.w, SourceRect.y,
          SourceRect.w, SourceRect.h};
}

auto CharacterSprite::getHitTextureRect() -> SDL_FRect {
  return {SourceRect.x + HitFrameIndex * SourceRect.w, SourceRect.y,
          SourceRect.w, SourceRect.h};
}

auto CharacterSprite::getTextureRect() -> SDL_FRect {
  if (CanHit && Hit) {
    if (++HitFrame == 2) {
      HitFrame = 0;
      Hit = false;
    }
    return getHitTextureRect();
  }

  if (CanRun && Running) {
    return getRunTextureRect();
  }

  return getIdleTextureRect();
}

auto CharacterSprite::getDestRect(const SDL_FPoint &Pos) -> SDL_FRect {
  return {Pos.x, Pos.y, SourceRect.w * 2, SourceRect.h * 2};
}

auto CharacterSprite::incIndex() {
  Index = std::fmod(++Index, AnimationFrameNumber);
}

auto CharacterSprite::render(SDL_Renderer *Renderer, SdlTexturePtr &Texture,
                             const SDL_FPoint &Pos, size_t FrameCount) {
  if (FrameCount % 2 == 0)
    incIndex();

  SDL_FRect DestRect = getDestRect({Pos.x, Pos.y - SourceRect.h * 2});
  SDL_FRect SourceRect = getTextureRect();
  SDL_FPoint Center{0, 0};

  if (Direction)
    SDL_RenderTextureRotated(Renderer, Texture.get(), &SourceRect, &DestRect, 0,
                             &Center, SDL_FLIP_HORIZONTAL);
  else
    SDL_RenderTexture(Renderer, Texture.get(), &SourceRect, &DestRect);
}
