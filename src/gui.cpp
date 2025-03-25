module;

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <imgui.h>

#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <SDL3_image/SDL_image.h>

#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

export module gui;

import sprite;
import tile;

using SdlSurfacePtr = std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)>;
using SdlWindowPtr = std::unique_ptr<SDL_Window, void (*)(SDL_Window *)>;

class InitError : public std::exception {
public:
  InitError(std::string_view msg) : msg(msg) {}
  const char *what() const noexcept override { return msg.c_str(); }

private:
  const std::string msg;
};

class TextureLoadingError : public std::exception {
public:
  TextureLoadingError(std::string_view msg) : msg(msg) {}
  const char *what() const noexcept override { return msg.c_str(); }

private:
  const std::string msg;
};

export class Gui final {
public:
  Gui();
  ~Gui();

  Gui(const Gui &) = delete;
  Gui(Gui &&) = delete;
  Gui &operator=(const Gui &) = delete;
  Gui &operator=(Gui &&) = delete;

  auto load_texture(std::string_view path) -> SdlTexturePtr;
  auto renderCharacterSelector();
  auto processEvent();

  auto loadEntities() -> void;

  auto showMap() -> void;

  auto frame() -> void;

  auto done() const noexcept -> bool { return _done; }

private:
  SdlWindowPtr window;
  bool _done;

  SDL_Renderer *renderer;

  size_t frameCount;
  SdlTexturePtr texture;
  Uint32 last;

  std::vector<CharacterSprite> characters;
  std::vector<CharacterSprite> enemies;
  std::vector<std::variant<Tile, AnimatedTile>> tiles;
  std::vector<std::vector<std::size_t>> map;
  std::vector<std::vector<std::size_t>> mapWall;

  size_t characterIndex;
  size_t enemyIndex;
  size_t tileIndex;
  bool checkBoxRuning;
  bool checkBoxWall;
};

Gui::Gui()
    : window{nullptr, SDL_DestroyWindow}, _done{false}, frameCount{0},
      texture{nullptr, SDL_DestroyTexture}, last{0}, characterIndex{0},
      enemyIndex{0}, tileIndex{0}, checkBoxRuning{false}, checkBoxWall{0} {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    throw InitError{std::format("SDL_Init(): {}", SDL_GetError())};

  static constexpr Uint32 window_flags = SDL_WINDOW_HIDDEN;

  window = {SDL_CreateWindow("My app", 1280, 720, window_flags),
            SDL_DestroyWindow};
  if (!window)
    throw InitError{std::format("SDL_CreateWindow(): {}", SDL_GetError())};

  renderer = SDL_CreateRenderer(window.get(), nullptr);
  SDL_SetRenderVSync(renderer, 1);
  if (renderer == nullptr)
    throw InitError{std::format("SDL_CreateRenderer(): {}", SDL_GetError())};

  SDL_SetWindowPosition(window.get(), SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window.get());

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  texture = load_texture(
      "rsrc/0x72_DungeonTilesetII_v1.7/0x72_DungeonTilesetII_v1.7.png");

  loadEntities();

  map.resize(15);
  for (auto &e : map) {
    e.resize(10, std::numeric_limits<size_t>::max());
  }

  mapWall.resize(15);
  for (auto &e : mapWall) {
    e.resize(10, std::numeric_limits<size_t>::max());
  }
}

Gui::~Gui() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
}

auto Gui::loadEntities() -> void {
  std::ifstream textureIndex;
  textureIndex.open("rsrc/0x72_DungeonTilesetII_v1.7/tile_list_v1.7.cpy");
  while (!textureIndex.eof()) {
    std::string s;
    textureIndex >> s;
    if (s == "terrain") {
      std::string name;
      unsigned int x, y, h, w;
      textureIndex >> name >> x >> y >> w >> h;
      tiles.emplace_back(Tile{name, x, y, w, h});
    } else if (s == "terrainA") {
      std::string name;
      unsigned int x, y, h, w;
      textureIndex >> name >> x >> y >> w >> h;
      tiles.emplace_back(AnimatedTile{name, x, y, w, h});
    } else if (s == "character") {
      std::string name;
      unsigned int x, y, h, w;
      textureIndex >> name >> x >> y >> w >> h;
      characters.emplace_back(name, x, y, h, w, true, true);
    } else if (s == "enemy") {
      std::string name;
      unsigned int x, y, h, w;
      textureIndex >> name >> x >> y >> w >> h;
      enemies.emplace_back(name, x, y, h, w, true, false);
    } else if (s == "enemyw") {
      std::string name;
      unsigned int x, y, h, w;
      textureIndex >> name >> x >> y >> w >> h;
      enemies.emplace_back(name, x, y, h, w, false, false);
    } else {
      textureIndex.ignore();
    }
  }
}

auto Gui::processEvent() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_QUIT)
      _done = true;
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == SDL_GetWindowID(window.get()))
      _done = true;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
      size_t x = event.button.x / 32;
      size_t y = event.button.y / 32;
      if (x < map.size() && y < map[x].size()) {
        if (checkBoxWall)
          mapWall[x][y] = tileIndex;
        else
          map[x][y] = tileIndex;
      }
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_RIGHT) {
      size_t x = event.button.x / 32;
      size_t y = event.button.y / 32;
      if (x < map.size() && y < map[x].size()) {
        if (checkBoxWall)
          mapWall[x][y] = std::numeric_limits<size_t>::max();
        else
          map[x][y] = std::numeric_limits<size_t>::max();
      }
    }

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_A) {
      characters[characterIndex].setHit();
    } else if (event.type == SDL_EVENT_KEY_DOWN &&
               event.key.key == SDLK_RIGHT) {
      characters[characterIndex].setRunning(false);
    } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_LEFT) {
      characters[characterIndex].setRunning(true);
    } else if (event.type == SDL_EVENT_KEY_UP &&
               (event.key.key == SDLK_RIGHT || event.key.key == SDLK_LEFT)) {
      characters[characterIndex].setIdle();
    }
  }
}

auto Gui::renderCharacterSelector() {
  ImGui::Begin("Character Selector");
  if (ImGui::BeginCombo("Character Selector",
                        characters[characterIndex].name().c_str())) {
    for (auto i = 0; i < characters.size(); ++i) {
      if (ImGui::Selectable(characters[i].name().c_str(), characterIndex == i))
        characterIndex = i;

      if (characterIndex == i)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  if (ImGui::BeginCombo("Enemy Selector", enemies[enemyIndex].name().c_str())) {
    for (auto i = 0; i < enemies.size(); ++i) {
      if (ImGui::Selectable(enemies[i].name().c_str(), characterIndex == i))
        enemyIndex = i;

      if (enemyIndex == i)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (ImGui::Checkbox("running", &checkBoxRuning)) {
    if (checkBoxRuning)
      enemies[enemyIndex].setRunning(false);
    else
      enemies[enemyIndex].setIdle();
  }

  std::visit(
      [this](auto &tile) {
        if (ImGui::BeginCombo("Tile Selector", tile.name().c_str())) {
          for (auto i = 0; i < tiles.size(); ++i) {
            std::visit(
                [this, i](auto &tile) {
                  if (ImGui::Selectable(tile.name().c_str(), tileIndex == i))
                    tileIndex = i;
                },
                tiles[i]);

            if (tileIndex == i)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
      },
      tiles[tileIndex]);
  ImGui::Checkbox("wall", &checkBoxWall);

  ImGui::End();
}

auto Gui::frame() -> void {
  auto now = SDL_GetTicks();
  auto fps = now - last;
  if (fps >= 1000 / 30) {
    last = now;
    ++frameCount;
  } else {
    return;
  }

  processEvent();

  if (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::Text("frame ms: %d", (int)fps);

  renderCharacterSelector();

  ImGui::Render();
  SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  int winHeight{0};
  SDL_GetWindowSize(window.get(), NULL, &winHeight);

  characters[characterIndex].render(renderer, texture, 100, 100, frameCount,
                                    winHeight);
  enemies[enemyIndex].render(renderer, texture, 300, 100, frameCount,
                             winHeight);

  showMap();

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
  SDL_RenderPresent(renderer);
}

auto Gui::load_texture(std::string_view path) -> SdlTexturePtr {

  auto *iostr = SDL_IOFromFile(path.data(), "r");
  if (iostr == NULL)
    throw TextureLoadingError{
        std::format("SDL_IOFromFile(): {}", SDL_GetError())};

  SdlSurfacePtr surfaceWizard{IMG_LoadPNG_IO(iostr), SDL_DestroySurface};
  if (!surfaceWizard)
    throw TextureLoadingError{
        std::format("IMG_LoadPNG_IO(): {}", SDL_GetError())};

  SdlTexturePtr texture = {
      SDL_CreateTextureFromSurface(renderer, surfaceWizard.get()),
      SDL_DestroyTexture};
  if (!texture)
    throw TextureLoadingError{
        std::format("SDL_CreateTextureFromSurface(): {}", SDL_GetError())};

  SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
  return texture;
}

auto Gui::showMap() -> void {
  for (auto i = 0; i < map.size(); ++i) {
    for (auto j = 0; j < map[i].size(); ++j) {
      if (map[i][j] >= tiles.size())
        continue;
      std::visit(
          [this, i, j](auto &tile) {
            tile.render(renderer, texture, i * 32, j * 32, frameCount);
          },
          tiles[map[i][j]]);
    }
  }

  for (auto i = 0; i < mapWall.size(); ++i) {
    for (auto j = 0; j < mapWall[i].size(); ++j) {
      if (mapWall[i][j] >= tiles.size())
        continue;
      std::visit(
          [this, i, j](auto &tile) {
            tile.render(renderer, texture, i * 32, j * 32, frameCount);
          },
          tiles[mapWall[i][j]]);
    }
  }
}
