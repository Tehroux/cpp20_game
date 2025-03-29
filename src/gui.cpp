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
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module gui;

import sprite;
import tile;
import sdlHelpers;

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
  auto processEventEditor(const SDL_Event &event) -> bool;
  auto processEventCharacter(const SDL_Event &event) -> bool;

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
  std::vector<RendererBuilder> tiles;
  std::vector<std::unique_ptr<Renderable>> map_;
  std::vector<std::unique_ptr<Renderable>> mapWall_;

  size_t characterIndex;
  size_t enemyIndex;
  size_t tileIndex;
  bool checkBoxRuning;
  bool checkBoxWall;
  bool checkLevel;
  bool checkEditor_;
  int tileX;
  int tileY;
  bool showTileSelector;
};

Gui::Gui()
    : window{nullptr, SDL_DestroyWindow}, _done{false}, frameCount{0},
      texture{nullptr, SDL_DestroyTexture}, last{0}, characterIndex{0},
      enemyIndex{0}, tileIndex{0}, checkBoxRuning{false}, checkBoxWall{false},
      checkLevel{false}, checkEditor_{false}, tileX{0}, tileY{0},
      showTileSelector{false} {
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
    std::string name;
    SDL_FRect rect;
    textureIndex >> s >> name >> rect.x >> rect.y >> rect.w >> rect.h;

    if (s == "terrain") {
      tiles.emplace_back(name, false, rect);
    } else if (s == "terrainA") {
      tiles.emplace_back(name, true, rect);
    } else if (s == "character") {
      characters.emplace_back(name, rect, true, true);
    } else if (s == "enemy") {
      enemies.emplace_back(name, rect, true, false);
    } else if (s == "enemyw") {
      enemies.emplace_back(name, rect, false, false);
    } else {
      textureIndex.ignore();
    }
  }
}

auto Gui::processEvent() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);

    if (ImGui::GetIO().WantCaptureMouse) {
      showTileSelector = false;
      continue;
    } else {
      showTileSelector = true;
    }

    float mx, my;

    SDL_GetMouseState(&mx, &my);

    tileX = static_cast<int>(mx) - static_cast<int>(mx) % 32;
    tileY = static_cast<int>(my) - static_cast<int>(my) % 32;

    if (event.type == SDL_EVENT_QUIT)
      _done = true;
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == SDL_GetWindowID(window.get()))
      _done = true;

    if (checkEditor_ && processEventEditor(event))
      return;

    processEventCharacter(event);
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

  auto tile = tiles[tileIndex];
  if (ImGui::BeginCombo("Tile Selector", tile.name().c_str())) {
    for (auto i = 0; i < tiles.size(); ++i) {
      if (ImGui::Selectable(tiles[i].name().c_str(), tileIndex == i))
        tileIndex = i;

      if (tileIndex == i)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  ImGui::Checkbox("wall", &checkBoxWall);

  ImGui::Checkbox("Level", &checkLevel);

  if (ImGui::Button("save")) {
    std::fstream file;
    file.open("test.lvl", std::ios::out | std::ios::trunc);

    for (auto &tile : map_) {
      file << *tile << '\n';
    }
    file << "=====\n";
    for (auto &tile : mapWall_) {
      file << *tile << '\n';
    }
  }

  if (ImGui::Button("load")) {
    map_.clear();
    mapWall_.clear();
    std::fstream file;
    file.open("test.lvl", std::ios::in);
    while (!file.eof()) {
      RendererBuilder b;
      file >> b;
      file.ignore();
      if (file.good()) {
        map_.push_back(b.build());
      } else {
        break;
      }
    }
    file.clear();
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (!file.eof()) {
      RendererBuilder t;
      file >> t;
      file.ignore();
      if (file.good()) {

        mapWall_.push_back(t.build());
      } else {
        file.clear();
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    }
  }

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

  if (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }
  processEvent();

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("Editor mode", nullptr, &checkEditor_);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  ImGui::Text("frame ms: %d", (int)fps);

  if (checkEditor_)
    renderCharacterSelector();

  ImGui::Render();
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  std::sort(map_.begin(), map_.end());
  std::sort(mapWall_.begin(), mapWall_.end());

  showMap();

  if (checkEditor_ && showTileSelector) {
    SDL_FRect r{static_cast<float>(tileX), static_cast<float>(tileY), 32, 32};
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderRect(renderer, &r);
  }

  enemies[enemyIndex].render(renderer, texture, 300, 100, frameCount);

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

auto Gui::processEventEditor(const SDL_Event &event) -> bool {
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT && checkEditor_) {
    SDL_FPoint point;
    point.x = (event.button.x) - static_cast<int>(event.button.x) % 32;
    point.y = (event.button.y) - static_cast<int>(event.button.y) % 32 + 32;

    auto tile = tiles[tileIndex].build();
    tile->setPos(point);
    if (checkBoxWall) {
      tile->setLevel(checkLevel);
      std::erase_if(mapWall_,
                    [point](auto &tile) { return tile->isSamePos(point); });
      mapWall_.push_back(std::move(tile));
    } else {
      std::erase_if(map_,
                    [point](auto &tile) { return tile->isSamePos(point); });
      map_.push_back(std::move(tile));
    }
    return true;
  } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
             event.button.button == SDL_BUTTON_RIGHT && checkEditor_) {
    SDL_FPoint point;
    point.x = (event.button.x) - static_cast<int>(event.button.x) % 32;
    point.y = (event.button.y) - static_cast<int>(event.button.y) % 32 + 32;

    if (checkBoxWall)
      std::erase_if(mapWall_,
                    [point](auto &tile) { return tile->isSamePos(point); });
    else
      std::erase_if(map_,
                    [point](auto &tile) { return tile->isSamePos(point); });
    return true;
  }
  return false;
}

auto Gui::processEventCharacter(const SDL_Event &event) -> bool {

  if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_A) {
    characters[characterIndex].setHit();
    return true;
  } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RIGHT) {
    characters[characterIndex].setRunning(false);
    return true;
  } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_LEFT) {
    characters[characterIndex].setRunning(true);
    return true;
  } else if (event.type == SDL_EVENT_KEY_UP &&
             (event.key.key == SDLK_RIGHT || event.key.key == SDLK_LEFT)) {
    characters[characterIndex].setIdle();
    return true;
  }
  return false;
}

auto Gui::showMap() -> void {

  for (auto &tile : map_) {
    tile->render(renderer, texture, frameCount);
  }

  auto crendered = false;
  for (auto &tile : mapWall_) {
    if (!crendered && tile->getPos().y > 108) {
      crendered = true;
      characters[characterIndex].render(renderer, texture, 100, 108,
                                        frameCount);
    }
    tile->render(renderer, texture, frameCount);
  }

  if (!crendered) {
    crendered = true;
    characters[characterIndex].render(renderer, texture, 100, 108, frameCount);
  }
}
