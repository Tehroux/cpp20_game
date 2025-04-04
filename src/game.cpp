module;

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <imgui.h>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_surface.h"
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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module game;

import sprite;
import tile;
import sdlHelpers;
import gui;

/// an error occured while initializing
class InitError : public std::exception {
public:
  /// constructor
  ///
  /// \param[in] ErrorMessage the error message
  InitError(std::string_view errorMessage) : errorMessage_(errorMessage) {}

  /// get the error message
  ///
  /// \return the error message
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return errorMessage_.c_str();
  }

private:
  std::string errorMessage_; ///< the error message
};

class Character {
public:
  Character(float posX, float posY) : posX_{posX}, posY_{posY} {}
  auto setPos(float newPosX, float newPosY) -> void {
    posX_ = newPosX;
    posY_ = newPosY;
  }
  auto updateAngle(float newAngle) -> void { angle_ = newAngle; }
  auto updateSpeed(float newSpeed) -> void { speed_ = newSpeed; }

  auto update(Uint64 deltaTime) -> void {
    auto distance = static_cast<float>(deltaTime) * speed_;
    auto angleRad = angle_ * std::numbers::pi / 180;
    auto deltaX = distance * std::cos(angleRad);
    auto deltaY = distance * std::sin(angleRad);
    posX_ += static_cast<float>(deltaX);
    posY_ += static_cast<float>(deltaY);
  }

  [[nodiscard]] auto getPos() const -> SDL_FPoint {
    return {posX_ * 2, posY_ * 2};
  }
  static constexpr float speed{0.06};

private:
  float posX_;
  float posY_;
  float angle_{};
  float speed_{};
};

/// an error occured while loading a texture
class TextureLoadingError : public std::exception {
public:
  /// constructor
  ///
  /// \param[in] ErrorMessage the error message
  TextureLoadingError(std::string_view errorMessage)
      : errorMessage_(errorMessage) {}

  /// get the error message
  ///
  /// \return the error message
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return errorMessage_.c_str();
  }

private:
  std::string errorMessage_; ///< the error message
};

export class Game final {
public:
  Game();
  ~Game();

  Game(const Game &) = delete;
  Game(Game &&) = delete;
  auto operator=(const Game &) -> Game & = delete;
  auto operator=(Game &&) -> Game & = delete;

  auto loadTexture(const char *path) -> SdlTexturePtr;
  auto renderCharacterSelector();
  auto processEvent();
  auto processEventEditor(const SDL_Event &event) -> bool;
  auto processEventCharacter(const SDL_Event &event) -> bool;

  auto loadEntities() -> void;

  auto showMap() -> void;

  auto frame() -> void;
  auto checkKeys() -> void;

  [[nodiscard]] auto done() const noexcept -> bool { return done_; }

private:
  static constexpr float gridSize{32};
  static constexpr Uint64 minFrameDuration{1000 / 30};
  static constexpr Uint32 minimizedDelay{10};
  static constexpr SDL_Point windowSize{1280, 720};

  SdlWindowPtr window_{nullptr, SDL_DestroyWindow};
  bool done_{};

  SDL_Renderer *renderer_;

  size_t frameCount_{};
  SdlTexturePtr texture_{nullptr, SDL_DestroyTexture};
  Uint32 last_{};

  std::optional<Gui> gameGui_;

  Character player{100, 100};

  std::vector<CharacterSprite> characters_;
  std::vector<CharacterSprite> enemies_;
  std::vector<RendererBuilder> tiles_;
  std::vector<std::unique_ptr<Renderable>> map_;
  std::vector<std::unique_ptr<Renderable>> mapWall_;

  SDL_FPoint tileCursorPos_{};
  bool showTileSelector_{};
};

Game::Game() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    throw InitError{std::format("SDL_Init(): {}", SDL_GetError())};
  }

  static constexpr Uint32 windowFlags = SDL_WINDOW_HIDDEN;

  window_ = {
      SDL_CreateWindow("My app", windowSize.x, windowSize.y, windowFlags),
      SDL_DestroyWindow};
  if (!window_) {
    throw InitError{std::format("SDL_CreateWindow(): {}", SDL_GetError())};
  }

  renderer_ = SDL_CreateRenderer(window_.get(), nullptr);
  SDL_SetRenderVSync(renderer_, 1);
  if (renderer_ == nullptr) {
    throw InitError{std::format("SDL_CreateRenderer(): {}", SDL_GetError())};
  }

  SDL_SetWindowPosition(window_.get(), SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window_.get());

  texture_ = loadTexture(
      "rsrc/0x72_DungeonTilesetII_v1.7/0x72_DungeonTilesetII_v1.7.png");

  loadEntities();
  gameGui_.emplace(window_, renderer_);
}

Game::~Game() { SDL_Quit(); }

auto Game::loadEntities() -> void {
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

auto Game::processEvent() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {

    if (Gui::processEvent(event)) {
      showTileSelector_ = false;
      continue;
    }
    showTileSelector_ = true;

    SDL_FPoint mousePos;

    SDL_GetMouseState(&mousePos.x, &mousePos.y);

    tileCursorPos_ = {.x = mousePos.x - std::fmod(mousePos.x, gridSize),
                      .y = mousePos.y - std::fmod(mousePos.y, gridSize)};

    if (event.type == SDL_EVENT_QUIT) {
      done_ = true;
    }
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == SDL_GetWindowID(window_.get())) {
      done_ = true;
    }

    if (gameGui_->isEditorMode() && processEventEditor(event)) {
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
  gameGui_->frameRenderingDuration(fps);

  if (SDL_WINDOW_MINIMIZED & SDL_GetWindowFlags(window_.get())) {
    SDL_Delay(minimizedDelay);
    return;
  }

  processEvent();

  checkKeys();
  player.update(fps);

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);

  std::ranges::sort(map_);
  std::ranges::sort(
      mapWall_, [](auto &a, auto &b) { return a->getPos().y < b->getPos().y; });

  showMap();

  if (gameGui_->isEditorMode() && showTileSelector_) {
    SDL_FRect cursorRect{tileCursorPos_.x, tileCursorPos_.y, gridSize,
                         gridSize};
    SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
    SDL_RenderRect(renderer_, &cursorRect);
  }

  enemies_[gameGui_->getEnemyIndex()].setPos({300, 100});
  enemies_[gameGui_->getEnemyIndex()].render(renderer_, texture_, frameCount_);

  gameGui_->render(renderer_, characters_, enemies_, tiles_, map_, mapWall_);
  SDL_RenderPresent(renderer_);
}

auto Game::loadTexture(const char *path) -> SdlTexturePtr {

  auto *iostr = SDL_IOFromFile(path, "r");
  if (iostr == nullptr) {
    throw TextureLoadingError{
        std::format("SDL_IOFromFile(): {}", SDL_GetError())};
  }

  SdlSurfacePtr surfaceWizard{IMG_LoadPNG_IO(iostr), SDL_DestroySurface};
  if (!surfaceWizard) {
    throw TextureLoadingError{
        std::format("IMG_LoadPNG_IO(): {}", SDL_GetError())};
  }

  SdlTexturePtr texture = {
      SDL_CreateTextureFromSurface(renderer_, surfaceWizard.get()),
      SDL_DestroyTexture};
  if (!texture) {
    throw TextureLoadingError{
        std::format("SDL_CreateTextureFromSurface(): {}", SDL_GetError())};
  }

  SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
  return texture;
}

auto Game::processEventEditor(const SDL_Event &event) -> bool {
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    SDL_FPoint point;
    point.x = event.button.x - std::fmod(event.button.x, gridSize);
    point.y = event.button.y - std::fmod(event.button.y, gridSize) + gridSize;

    auto tile = tiles_[gameGui_->getTileIndex()].build();
    tile->setPos(point);
    if (gameGui_->isWall()) {
      tile->setLevel(gameGui_->isLevel());
      std::erase_if(mapWall_,
                    [point](auto &tile) { return tile->isSamePos(point); });
      mapWall_.push_back(std::move(tile));
    } else {
      std::erase_if(map_,
                    [point](auto &tile) { return tile->isSamePos(point); });
      map_.push_back(std::move(tile));
    }
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_RIGHT) {
    SDL_FPoint point;
    point.x = event.button.x - std::fmod(event.button.x, gridSize);
    point.y = event.button.y - std::fmod(event.button.y, gridSize) + gridSize;

    if (gameGui_->isWall()) {
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

auto Game::processEventCharacter(const SDL_Event &event) -> bool {

  if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_A) {
    characters_[gameGui_->getCharacterIndex()].setHit();
    return true;
  }
  return false;
}

auto Game::checkKeys() -> void {
  SDL_PumpEvents();
  auto *keys = SDL_GetKeyboardState(nullptr);

  if (keys[SDL_SCANCODE_UP]) {
    player.updateSpeed(Character::speed);
    if (keys[SDL_SCANCODE_LEFT]) {
      player.updateAngle(135);
      characters_[gameGui_->getCharacterIndex()].setRunning(true);
    } else if (keys[SDL_SCANCODE_RIGHT]) {
      player.updateAngle(45);
      characters_[gameGui_->getCharacterIndex()].setRunning(false);
    } else {
      characters_[gameGui_->getCharacterIndex()].setRunning();
      player.updateAngle(90);
    }
  } else if (keys[SDL_SCANCODE_DOWN]) {
    player.updateSpeed(Character::speed);
    if (keys[SDL_SCANCODE_LEFT]) {
      characters_[gameGui_->getCharacterIndex()].setRunning(true);
      player.updateAngle(225);
    } else if (keys[SDL_SCANCODE_RIGHT]) {
      characters_[gameGui_->getCharacterIndex()].setRunning(false);
      player.updateAngle(315);
    } else {
      characters_[gameGui_->getCharacterIndex()].setRunning();
      player.updateAngle(270);
    }
  } else if (keys[SDL_SCANCODE_LEFT]) {
    player.updateSpeed(Character::speed);
    characters_[gameGui_->getCharacterIndex()].setRunning(true);
    player.updateAngle(180);
  } else if (keys[SDL_SCANCODE_RIGHT]) {
    player.updateSpeed(Character::speed);
    characters_[gameGui_->getCharacterIndex()].setRunning(false);
    player.updateAngle(0);
  } else {
    player.updateSpeed(0);
    characters_[gameGui_->getCharacterIndex()].setIdle();
  }
}

auto Game::showMap() -> void {

  for (auto &tile : map_) {
    tile->render(renderer_, texture_, frameCount_);
  }

  auto &character = characters_[gameGui_->getCharacterIndex()];
  character.setPos(player.getPos());

  auto crendered = false;
  for (auto &tile : mapWall_) {
    if (!crendered && tile->getPos().y > character.getPos().y) {
      crendered = true;
      character.render(renderer_, texture_, frameCount_);
    }
    tile->render(renderer_, texture_, frameCount_);
  }
  if (!crendered) {
    crendered = true;
    character.render(renderer_, texture_, frameCount_);
  }

  SDL_SetRenderDrawColor(renderer_, 50, 250, 50, 255);
  SDL_FRect r = {character.getPos().x, character.getPos().y, 16, 1};
  SDL_RenderRect(renderer_, &r);
}
