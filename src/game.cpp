module;

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <imgui.h>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

export module game;

import sprite;
import tile;
import sdlHelpers;
import gui;

struct Rad {
  float value;

  static constexpr float radConvertionRatio{std::numbers::pi / 180};
  static constexpr auto fromDeg(float deg) -> Rad {
    return {deg * radConvertionRatio};
  }
};

struct PolarVec {
  float radius;
  Rad angle;
};

struct Vec {
  constexpr Vec(const PolarVec &other)
      : x{other.radius * std::cos(other.angle.value)},
        y{other.radius * std::sin(other.angle.value)} {}

  float x;
  float y;
};

struct Point {
  auto operator+(const Vec &other) const -> Point {
    return {.x = x + other.x, .y = y + other.y};
  }

  auto operator+=(const Vec &other) noexcept -> Point & {
    x += other.x;
    y += other.y;
    return *this;
  }

  auto asSdlPoint() -> SDL_FPoint { return {x, y}; }

  float x;
  float y;
};

struct PolarPoint {
  float radius;
  Rad angle;
};

class Character {
public:
  Character(const Point &pos, CharacterSprite *renderable)
      : pos_{pos}, renderable_{renderable} {}

  /// set the position of the character
  auto setPos(const Point &newPos) noexcept -> void { pos_ = newPos; }

  /// set a new direction
  auto updateAngle(Rad newAngle) noexcept -> void { vec_.angle = newAngle; }

  /// set a new speed
  auto updateSpeed(float newSpeed) noexcept -> void { vec_.radius = newSpeed; }

  /// update the position of the character
  /// \param[in] deltaTime the time since the last update
  auto update(Uint64 deltaTime) noexcept -> void {
    const auto vec =
        PolarVec{.radius = static_cast<float>(deltaTime) * vec_.radius,
                 .angle = vec_.angle};
    pos_ += vec;
  }

  /// get the renderable
  [[nodiscard]] auto getRenderable() const noexcept -> CharacterSprite * {
    return renderable_;
  };

  /// set the renderable
  auto setRenderable(CharacterSprite *newRenderable) noexcept -> void {
    renderable_ = newRenderable;
  };

  /// update the renderable position
  auto updateRenderable() noexcept -> void {
    if (renderable_)
      renderable_->setPos(pos_.asSdlPoint());
  }

  /// get the position of the character
  [[nodiscard]] auto getPos() const noexcept -> Point { return pos_; }

  static constexpr float speed{0.06};

private:
  /// character position
  Point pos_;
  /// direction vector
  PolarVec vec_{};
  /// graphic renderable
  CharacterSprite *renderable_;
};

export class Game final {
public:
  Game();
  ~Game();

  Game(const Game &) = delete;
  Game(Game &&) = delete;
  auto operator=(const Game &) -> Game & = delete;
  auto operator=(Game &&) -> Game & = delete;

  /// process Sdl events
  auto processEvent() noexcept -> void;
  /// process event in editor mode
  auto processEventEditor(const SDL_Event &event) noexcept -> bool;
  /// process event for the character
  auto processEventCharacter(const SDL_Event &event) noexcept -> bool;

  auto loadEntities() noexcept -> void;

  auto render() noexcept -> void;

  auto frame() -> void;
  auto checkKeys() noexcept -> void;

  [[nodiscard]] auto done() const noexcept -> bool { return done_; }

private:
  static constexpr float gridSize{16};
  static constexpr Uint64 minFrameDuration{1000 / 30};
  static constexpr Uint32 minimizedDelay{10};
  static constexpr SDL_Point windowSize{1280, 720};
  static constexpr Point playerStartingPoint{.x = 100, .y = 100};

  SdlWindow window_{"My Game", windowSize, SDL_WINDOW_HIDDEN};
  SdlRenderer renderer_{window_.createRenderer()};
  Gui gameGui_{window_, renderer_};

  bool done_{};

  size_t frameCount_{};
  SdlTexturePtr texture_{nullptr, SDL_DestroyTexture};
  Uint32 last_{};

  Character player_{playerStartingPoint, nullptr};

  std::vector<CharacterSprite> characters_;
  std::vector<CharacterSprite> enemies_;
  std::vector<RendererBuilder> tiles_;
  std::vector<std::unique_ptr<TileConcrete>> map_;
  std::vector<std::unique_ptr<TileConcrete>> mapWall_;

  std::vector<Renderable *> toRender_;

  SDL_FPoint tileCursorPos_{};
  bool showTileSelector_{};
};

Game::Game() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    throw InitError{std::format("SDL_Init(): {}", SDL_GetError())};
  }

  window_.setPosition(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  window_.showWindow();

  texture_ = renderer_.createTextureFromPath(
      "rsrc/0x72_DungeonTilesetII_v1.7/0x72_DungeonTilesetII_v1.7.png");

  loadEntities();
}

Game::~Game() { SDL_Quit(); }

auto Game::loadEntities() noexcept -> void {
  std::ifstream textureIndex;
  textureIndex.open("rsrc/0x72_DungeonTilesetII_v1.7/tile_list_v1.7.cpy");
  while (!textureIndex.eof()) {
    std::string tileType;
    std::string tileName;
    SDL_FRect sourceRect;
    textureIndex >> tileType >> tileName >> sourceRect.x >> sourceRect.y >>
        sourceRect.w >> sourceRect.h;

    if (tileType == "terrain") {
      tiles_.emplace_back(tileName, false, sourceRect);
    } else if (tileType == "terrainA") {
      tiles_.emplace_back(tileName, true, sourceRect);
    } else if (tileType == "character") {
      characters_.emplace_back(tileName, sourceRect, true, true);
    } else if (tileType == "enemy") {
      enemies_.emplace_back(tileName, sourceRect, true, false);
    } else if (tileType == "enemyw") {
      enemies_.emplace_back(tileName, sourceRect, false, false);
    } else {
      textureIndex.ignore();
    }
  }
}

auto Game::processEvent() noexcept -> void {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {

    if (Gui::processEvent(event)) {
      showTileSelector_ = false;
      continue;
    }
    showTileSelector_ = true;

    SDL_FPoint mousePos;

    SDL_GetMouseState(&mousePos.x, &mousePos.y);

    tileCursorPos_ = {.x = mousePos.x - std::fmod(mousePos.x, gridSize * 2),
                      .y = mousePos.y - std::fmod(mousePos.y, gridSize * 2)};

    if (event.type == SDL_EVENT_QUIT) {
      done_ = true;
    }
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == window_.getWindowID()) {
      done_ = true;
    }

    if (gameGui_.isEditorMode() && processEventEditor(event)) {
      return;
    }

    processEventCharacter(event);
  }
}

auto Game::frame() -> void {
  auto now = SDL_GetTicks();
  auto fps = now - last_;
  if (fps >= minFrameDuration) {
    last_ = now;
    ++frameCount_;
  } else {
    return;
  }

  gameGui_.frameRenderingDuration(fps);

  if (SDL_WINDOW_MINIMIZED & window_.getWindowFlags()) {
    SDL_Delay(minimizedDelay);
    return;
  }

  processEvent();

  checkKeys();

  player_.update(fps);

  constexpr SDL_Color clearColor{0, 0, 0, 255};
  renderer_.setRenderDrawColor(clearColor);
  renderer_.renderClear();

  render();

  if (gameGui_.isEditorMode() && showTileSelector_) {
    const SDL_FRect cursorRect{tileCursorPos_.x, tileCursorPos_.y, gridSize * 2,
                               gridSize * 2};

    constexpr SDL_Color cursorColor{150, 150, 150, 255};
    renderer_.setRenderDrawColor(cursorColor);
    renderer_.renderRect(cursorRect);
  }

  gameGui_.render(renderer_, characters_, enemies_, tiles_, map_, mapWall_);
  renderer_.renderPresent();
}

auto Game::processEventEditor(const SDL_Event &event) noexcept -> bool {
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    SDL_FPoint point;
    point.x = (event.button.x - std::fmod(event.button.x, gridSize * 2)) / 2;
    point.y = (event.button.y - std::fmod(event.button.y, gridSize * 2) +
               gridSize * 2) /
              2;

    auto tile = tiles_[gameGui_.getTileIndex()];
    if (gameGui_.isWall()) {
      std::erase_if(mapWall_,
                    [point](auto &tile) { return tile->isSamePos(point); });
      mapWall_.push_back(tile.build(point, gameGui_.isLevel()));
    } else {
      std::erase_if(map_,
                    [point](auto &tile) { return tile->isSamePos(point); });
      map_.push_back(tile.build(point, gameGui_.isLevel()));
    }
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_RIGHT) {
    SDL_FPoint point;
    point.x = (event.button.x - std::fmod(event.button.x, gridSize * 2)) / 2;
    point.y = (event.button.y - std::fmod(event.button.y, gridSize * 2) +
               gridSize * 2) /
              2;

    if (gameGui_.isWall()) {
      std::erase_if(mapWall_,
                    [point](auto &tile) { return tile->isSamePos(point); });
    } else {
      std::erase_if(map_,
                    [point](auto &tile) { return tile->isSamePos(point); });
    }
    return true;
  }
  return false;
}

auto Game::processEventCharacter(const SDL_Event &event) noexcept -> bool {

  if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_A) {
    player_.getRenderable()->setHit();
    return true;
  }
  return false;
}

auto Game::checkKeys() noexcept -> void {
  SDL_PumpEvents();

  int ksize{0};
  const bool *kptr = SDL_GetKeyboardState(&ksize);
  const std::span<const bool> keys{kptr, static_cast<size_t>(ksize)};

  constexpr Rad dirUpLeft{Rad::fromDeg(135)};
  constexpr Rad dirUpRight{Rad::fromDeg(45)};
  constexpr Rad dirUp{Rad::fromDeg(90)};
  constexpr Rad dirDownLeft{Rad::fromDeg(255)};
  constexpr Rad dirDownRight{Rad::fromDeg(315)};
  constexpr Rad dirDown{Rad::fromDeg(270)};
  constexpr Rad dirLeft{Rad::fromDeg(180)};
  constexpr Rad dirRight{Rad::fromDeg(0)};

  if (keys[SDL_SCANCODE_UP]) {
    player_.updateSpeed(Character::speed);
    if (keys[SDL_SCANCODE_LEFT]) {
      player_.updateAngle(dirUpLeft);
      player_.getRenderable()->setRunning(true);
    } else if (keys[SDL_SCANCODE_RIGHT]) {
      player_.updateAngle(dirUpRight);
      player_.getRenderable()->setRunning(false);
    } else {
      player_.getRenderable()->setRunning();
      player_.updateAngle(dirUp);
    }
  } else if (keys[SDL_SCANCODE_DOWN]) {
    player_.updateSpeed(Character::speed);
    if (keys[SDL_SCANCODE_LEFT]) {
      player_.getRenderable()->setRunning(true);
      player_.updateAngle(dirDownLeft);
    } else if (keys[SDL_SCANCODE_RIGHT]) {
      player_.getRenderable()->setRunning(false);
      player_.updateAngle(dirDownRight);
    } else {
      player_.getRenderable()->setRunning();
      player_.updateAngle(dirDown);
    }
  } else if (keys[SDL_SCANCODE_LEFT]) {
    player_.updateSpeed(Character::speed);
    player_.getRenderable()->setRunning(true);
    player_.updateAngle(dirLeft);
  } else if (keys[SDL_SCANCODE_RIGHT]) {
    player_.updateSpeed(Character::speed);
    player_.getRenderable()->setRunning(false);
    player_.updateAngle(dirRight);
  } else {
    player_.updateSpeed(0);
    if (player_.getRenderable())
      player_.getRenderable()->setIdle();
  }
}

auto Game::render() noexcept -> void {
  for (const auto &tile : map_) {
    tile->render(renderer_, texture_, frameCount_);
  }

  toRender_.clear();
  for (const auto &item : mapWall_) {
    toRender_.push_back(item.get());
  }

  player_.setRenderable(&characters_[gameGui_.getCharacterIndex()]);
  player_.updateRenderable();
  toRender_.push_back(player_.getRenderable());

  std::ranges::sort(toRender_, [](auto &lhs, auto &rhs) {
    return lhs->getPos().y < rhs->getPos().y;
  });

  for (const auto &tile : toRender_) {
    tile->render(renderer_, texture_, frameCount_);
  }
}
