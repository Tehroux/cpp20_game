module;

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <imgui.h>

#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <SDL3_image/SDL_image.h>

#include <algorithm>
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
  InitError(std::string_view ErrorMessage) : ErrorMessage(ErrorMessage) {}

  /// get the error message
  ///
  /// \return the error message
  const char *what() const noexcept override { return ErrorMessage.c_str(); }

private:
  const std::string ErrorMessage; ///< the error message
};

/// an error occured while loading a texture
class TextureLoadingError : public std::exception {
public:
  /// constructor
  ///
  /// \param[in] ErrorMessage the error message
  TextureLoadingError(std::string_view ErrorMessage)
      : ErrorMessage(ErrorMessage) {}

  /// get the error message
  ///
  /// \return the error message
  const char *what() const noexcept override { return ErrorMessage.c_str(); }

private:
  const std::string ErrorMessage; ///< the error message
};

export class Game final {
public:
  Game();
  ~Game();

  Game(const Game &) = delete;
  Game(Game &&) = delete;
  Game &operator=(const Game &) = delete;
  Game &operator=(Game &&) = delete;

  auto loadTexture(std::string_view Path) -> SdlTexturePtr;
  auto renderCharacterSelector();
  auto processEvent();
  auto processEventEditor(const SDL_Event &Event) -> bool;
  auto processEventCharacter(const SDL_Event &Event) -> bool;

  auto loadEntities() -> void;

  auto showMap() -> void;

  auto frame() -> void;

  auto done() const noexcept -> bool { return Done; }

private:
  SdlWindowPtr Window;
  bool Done;

  SDL_Renderer *Renderer;

  size_t FrameCount;
  SdlTexturePtr Texture;
  Uint32 Last;

  std::optional<Gui> GameGui;

  std::vector<CharacterSprite> Characters;
  std::vector<CharacterSprite> Enemies;
  std::vector<RendererBuilder> Tiles;
  std::vector<std::unique_ptr<Renderable>> Map;
  std::vector<std::unique_ptr<Renderable>> MapWall;

  SDL_FPoint TileCursorPos;
  bool ShowTileSelector;
};

Game::Game()
    : Window{nullptr, SDL_DestroyWindow}, Done{false}, FrameCount{0},
      Texture{nullptr, SDL_DestroyTexture}, Last{0}, TileCursorPos{0, 0},
      ShowTileSelector{false} {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    throw InitError{std::format("SDL_Init(): {}", SDL_GetError())};

  static constexpr Uint32 WindowFlags = SDL_WINDOW_HIDDEN;

  Window = {SDL_CreateWindow("My app", 1280, 720, WindowFlags),
            SDL_DestroyWindow};
  if (!Window)
    throw InitError{std::format("SDL_CreateWindow(): {}", SDL_GetError())};

  Renderer = SDL_CreateRenderer(Window.get(), nullptr);
  SDL_SetRenderVSync(Renderer, 1);
  if (Renderer == nullptr)
    throw InitError{std::format("SDL_CreateRenderer(): {}", SDL_GetError())};

  SDL_SetWindowPosition(Window.get(), SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(Window.get());

  Texture = loadTexture(
      "rsrc/0x72_DungeonTilesetII_v1.7/0x72_DungeonTilesetII_v1.7.png");

  loadEntities();
  GameGui.emplace(Window, Renderer);
}

Game::~Game() { SDL_Quit(); }

auto Game::loadEntities() -> void {
  std::ifstream TextureIndex;
  TextureIndex.open("rsrc/0x72_DungeonTilesetII_v1.7/tile_list_v1.7.cpy");
  while (!TextureIndex.eof()) {
    std::string TileType;
    std::string TileName;
    SDL_FRect SourceRect;
    TextureIndex >> TileType >> TileName >> SourceRect.x >> SourceRect.y >>
        SourceRect.w >> SourceRect.h;

    if (TileType == "terrain") {
      Tiles.emplace_back(TileName, false, SourceRect);
    } else if (TileType == "terrainA") {
      Tiles.emplace_back(TileName, true, SourceRect);
    } else if (TileType == "character") {
      Characters.emplace_back(TileName, SourceRect, true, true);
    } else if (TileType == "enemy") {
      Enemies.emplace_back(TileName, SourceRect, true, false);
    } else if (TileType == "enemyw") {
      Enemies.emplace_back(TileName, SourceRect, false, false);
    } else {
      TextureIndex.ignore();
    }
  }
}

auto Game::processEvent() {
  SDL_Event Event;
  while (SDL_PollEvent(&Event)) {

    if (GameGui->processEvent(Event)) {
      ShowTileSelector = false;
      continue;
    }
    ShowTileSelector = true;

    SDL_FPoint MousePos;

    SDL_GetMouseState(&MousePos.x, &MousePos.y);

    TileCursorPos = {static_cast<float>(static_cast<int>(MousePos.x) -
                                        static_cast<int>(MousePos.x) % 32),
                     static_cast<float>(static_cast<int>(MousePos.y) -
                                        static_cast<int>(MousePos.y) % 32)};

    if (Event.type == SDL_EVENT_QUIT)
      Done = true;
    if (Event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        Event.window.windowID == SDL_GetWindowID(Window.get()))
      Done = true;

    if (GameGui->isEditorMode() && processEventEditor(Event))
      return;

    processEventCharacter(Event);
  }
}

auto Game::frame() -> void {
  auto Now = SDL_GetTicks();
  auto Fps = Now - Last;
  if (Fps >= 1000 / 30) {
    Last = Now;
    ++FrameCount;
  } else {
    return;
  }
  GameGui->setTimeToRenderFrame(Fps);

  if (SDL_GetWindowFlags(Window.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  processEvent();

  SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
  SDL_RenderClear(Renderer);

  std::sort(Map.begin(), Map.end());
  std::sort(MapWall.begin(), MapWall.end());

  showMap();

  if (GameGui->isEditorMode() && ShowTileSelector) {
    SDL_FRect CursorRect{TileCursorPos.x, TileCursorPos.y, 32, 32};
    SDL_SetRenderDrawColor(Renderer, 150, 150, 150, 255);
    SDL_RenderRect(Renderer, &CursorRect);
  }

  Enemies[GameGui->getEnemyIndex()].render(Renderer, Texture, 300, 100,
                                           FrameCount);

  GameGui->render(Renderer, Characters, Enemies, Tiles, Map, MapWall);
  SDL_RenderPresent(Renderer);
}

auto Game::loadTexture(std::string_view Path) -> SdlTexturePtr {

  auto *Iostr = SDL_IOFromFile(Path.data(), "r");
  if (Iostr == NULL)
    throw TextureLoadingError{
        std::format("SDL_IOFromFile(): {}", SDL_GetError())};

  SdlSurfacePtr SurfaceWizard{IMG_LoadPNG_IO(Iostr), SDL_DestroySurface};
  if (!SurfaceWizard)
    throw TextureLoadingError{
        std::format("IMG_LoadPNG_IO(): {}", SDL_GetError())};

  SdlTexturePtr Texture = {
      SDL_CreateTextureFromSurface(Renderer, SurfaceWizard.get()),
      SDL_DestroyTexture};
  if (!Texture)
    throw TextureLoadingError{
        std::format("SDL_CreateTextureFromSurface(): {}", SDL_GetError())};

  SDL_SetTextureScaleMode(Texture.get(), SDL_SCALEMODE_NEAREST);
  return Texture;
}

auto Game::processEventEditor(const SDL_Event &Event) -> bool {
  if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      Event.button.button == SDL_BUTTON_LEFT) {
    SDL_FPoint Point;
    Point.x = (Event.button.x) - static_cast<int>(Event.button.x) % 32;
    Point.y = (Event.button.y) - static_cast<int>(Event.button.y) % 32 + 32;

    auto Tile = Tiles[GameGui->getTileIndex()].build();
    Tile->setPos(Point);
    if (GameGui->isWall()) {
      Tile->setLevel(GameGui->isLevel());
      std::erase_if(MapWall,
                    [Point](auto &Tile) { return Tile->isSamePos(Point); });
      MapWall.push_back(std::move(Tile));
    } else {
      std::erase_if(Map,
                    [Point](auto &Tile) { return Tile->isSamePos(Point); });
      Map.push_back(std::move(Tile));
    }
    return true;
  }
  if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      Event.button.button == SDL_BUTTON_RIGHT) {
    SDL_FPoint Point;
    Point.x = (Event.button.x) - static_cast<int>(Event.button.x) % 32;
    Point.y = (Event.button.y) - static_cast<int>(Event.button.y) % 32 + 32;

    if (GameGui->isWall())
      std::erase_if(MapWall,
                    [Point](auto &Tile) { return Tile->isSamePos(Point); });
    else
      std::erase_if(Map,
                    [Point](auto &Tile) { return Tile->isSamePos(Point); });
    return true;
  }
  return false;
}

auto Game::processEventCharacter(const SDL_Event &Event) -> bool {

  if (Event.type == SDL_EVENT_KEY_DOWN && Event.key.key == SDLK_A) {
    Characters[GameGui->getCharacterIndex()].setHit();
    return true;
  }
  if (Event.type == SDL_EVENT_KEY_DOWN && Event.key.key == SDLK_RIGHT) {
    Characters[GameGui->getCharacterIndex()].setRunning(false);
    return true;
  }
  if (Event.type == SDL_EVENT_KEY_DOWN && Event.key.key == SDLK_LEFT) {
    Characters[GameGui->getCharacterIndex()].setRunning(true);
    return true;
  }
  if (Event.type == SDL_EVENT_KEY_UP &&
      (Event.key.key == SDLK_RIGHT || Event.key.key == SDLK_LEFT)) {
    Characters[GameGui->getCharacterIndex()].setIdle();
    return true;
  }
  return false;
}

auto Game::showMap() -> void {

  for (auto &Tile : Map) {
    Tile->render(Renderer, Texture, FrameCount);
  }

  auto Crendered = false;
  for (auto &Tile : MapWall) {
    if (!Crendered && Tile->getPos().y > 108) {
      Crendered = true;
      Characters[GameGui->getCharacterIndex()].render(Renderer, Texture, 100,
                                                      108, FrameCount);
    }
    Tile->render(Renderer, Texture, FrameCount);
  }

  if (!Crendered) {
    Crendered = true;
    Characters[GameGui->getCharacterIndex()].render(Renderer, Texture, 100, 108,
                                                    FrameCount);
  }
}
