module;

#include <memory>
#include <string>

#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"

export module sprite;

export using SdlTexturePtr =
    std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;

export class CharacterSprite {
public:
  CharacterSprite(const std::string &name, size_t x, size_t y, size_t h,
                  size_t w, bool canRun, bool cantHit);

  auto name() -> const std::string & { return _name; }

  auto getIdleTextureRect() -> SDL_FRect;
  auto getRunTextureRect() -> SDL_FRect;
  auto getHitTextureRect() -> SDL_FRect;
  auto getTextureRect() -> SDL_FRect;
  auto getDestRect(int x, int y) -> SDL_FRect;

  auto incIndex();

  auto setHit() { hit = true; }
  auto setRunning(bool dir) {
    this->running = true;
    direction = dir;
  }
  auto setIdle() { this->running = false; }

  auto render(SDL_Renderer *renderer, SdlTexturePtr &texture, int x, int y,
              size_t frameCount, int winHeight);

private:
  std::string _name;
  size_t _x;
  size_t _y;
  size_t _h;
  size_t _w;
  size_t index;
  bool hit;
  bool running;
  bool direction;
  bool _canRun;
  bool _canHit;
};

CharacterSprite::CharacterSprite(const std::string &name, size_t x, size_t y,
                                 size_t h, size_t w, bool canRun, bool canHit)
    : _name{name}, _x{x}, _y{y}, _h{h}, _w{w}, index{0}, hit{false},
      running{false}, direction{false}, _canRun{canRun}, _canHit{canHit} {}

auto CharacterSprite::getIdleTextureRect() -> SDL_FRect {
  return {static_cast<float>(_x + index * _w), static_cast<float>(_y),
          static_cast<float>(_w), static_cast<float>(_h)};
}

auto CharacterSprite::getRunTextureRect() -> SDL_FRect {
  return {static_cast<float>(_x + (index + 4) * _w), static_cast<float>(_y),
          static_cast<float>(_w), static_cast<float>(_h)};
}

auto CharacterSprite::getHitTextureRect() -> SDL_FRect {
  return {static_cast<float>(_x + 8 * _w), static_cast<float>(_y),
          static_cast<float>(_w), static_cast<float>(_h)};
}

auto CharacterSprite::getTextureRect() -> SDL_FRect {
  if (_canHit && hit) {
    hit = false;
    return getHitTextureRect();
  } else if (_canRun && running) {
    return getRunTextureRect();
  } else {
    return getIdleTextureRect();
  }
}

auto CharacterSprite::getDestRect(int x, int y) -> SDL_FRect {
  return {static_cast<float>(x), static_cast<float>(y),
          static_cast<float>(_w * 2), static_cast<float>(_h * 2)};
}

auto CharacterSprite::incIndex() { index = ++index % 4; }

auto CharacterSprite::render(SDL_Renderer *renderer, SdlTexturePtr &texture,
                             int x, int y, size_t frameCount, int winHeight) {
  if (frameCount % 2 == 0)
    incIndex();

  SDL_FRect destRect = getDestRect(x, winHeight - (y + _h)*2);
  SDL_FRect sourceRect = getTextureRect();
  SDL_FPoint center{0, 0};

  if (direction)
    SDL_RenderTextureRotated(renderer, texture.get(), &sourceRect, &destRect, 0,
                             &center, SDL_FLIP_HORIZONTAL);
  else
    SDL_RenderTexture(renderer, texture.get(), &sourceRect, &destRect);
}
