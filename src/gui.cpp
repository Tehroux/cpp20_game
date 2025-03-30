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
  InitError(std::string_view ErrorMessage) : ErrorMessage(ErrorMessage) {}
  const char *what() const noexcept override { return ErrorMessage.c_str(); }

private:
  const std::string ErrorMessage;
};

class TextureLoadingError : public std::exception {
public:
  TextureLoadingError(std::string_view ErrorMessage)
      : ErrorMessage(ErrorMessage) {}
  const char *what() const noexcept override { return ErrorMessage.c_str(); }

private:
  const std::string ErrorMessage;
};

export class Gui final {
public:
  Gui();
  ~Gui();

  Gui(const Gui &) = delete;
  Gui(Gui &&) = delete;
  Gui &operator=(const Gui &) = delete;
  Gui &operator=(Gui &&) = delete;

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

  std::vector<CharacterSprite> Characters;
  std::vector<CharacterSprite> Enemies;
  std::vector<RendererBuilder> Tiles;
  std::vector<std::unique_ptr<Renderable>> Map;
  std::vector<std::unique_ptr<Renderable>> MapWall;

  size_t CharacterIndex;
  size_t EnemyIndex;
  size_t TileIndex;
  bool CheckBoxRuning;
  bool CheckBoxWall;
  bool CheckLevel;
  bool CheckEditor;
  SDL_FPoint TileCursorPos;
  bool ShowTileSelector;
};

Gui::Gui()
    : Window{nullptr, SDL_DestroyWindow}, Done{false}, FrameCount{0},
      Texture{nullptr, SDL_DestroyTexture}, Last{0}, CharacterIndex{0},
      EnemyIndex{0}, TileIndex{0}, CheckBoxRuning{false}, CheckBoxWall{false},
      CheckLevel{false}, CheckEditor{false}, TileCursorPos{0, 0},
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

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &Io = ImGui::GetIO();
  Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  Io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(Window.get(), Renderer);
  ImGui_ImplSDLRenderer3_Init(Renderer);

  Texture = loadTexture(
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

auto Gui::processEvent() {
  SDL_Event Event;
  while (SDL_PollEvent(&Event)) {
    ImGui_ImplSDL3_ProcessEvent(&Event);

    if (ImGui::GetIO().WantCaptureMouse) {
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

    if (CheckEditor && processEventEditor(Event))
      return;

    processEventCharacter(Event);
  }
}

auto Gui::renderCharacterSelector() {
  ImGui::Begin("Character Selector");
  if (ImGui::BeginCombo("Character Selector",
                        Characters[CharacterIndex].name().c_str())) {
    for (auto Index = 0; Index < Characters.size(); ++Index) {
      if (ImGui::Selectable(Characters[Index].name().c_str(),
                            CharacterIndex == Index))
        CharacterIndex = Index;

      if (CharacterIndex == Index)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  if (ImGui::BeginCombo("Enemy Selector", Enemies[EnemyIndex].name().c_str())) {
    for (auto Index = 0; Index < Enemies.size(); ++Index) {
      if (ImGui::Selectable(Enemies[Index].name().c_str(),
                            CharacterIndex == Index))
        EnemyIndex = Index;

      if (EnemyIndex == Index)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (ImGui::Checkbox("running", &CheckBoxRuning)) {
    if (CheckBoxRuning)
      Enemies[EnemyIndex].setRunning(false);
    else
      Enemies[EnemyIndex].setIdle();
  }

  auto Tile = Tiles[TileIndex];
  if (ImGui::BeginCombo("Tile Selector", Tile.name().c_str())) {
    for (auto i = 0; i < Tiles.size(); ++i) {
      if (ImGui::Selectable(Tiles[i].name().c_str(), TileIndex == i))
        TileIndex = i;

      if (TileIndex == i)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  ImGui::Checkbox("wall", &CheckBoxWall);

  ImGui::Checkbox("Level", &CheckLevel);

  if (ImGui::Button("save")) {
    std::fstream File;
    File.open("test.lvl", std::ios::out | std::ios::trunc);

    for (auto &Tile : Map) {
      File << *Tile << '\n';
    }
    File << "=====\n";
    for (auto &Tile : MapWall) {
      File << *Tile << '\n';
    }
  }

  if (ImGui::Button("load")) {
    Map.clear();
    MapWall.clear();
    std::fstream File;
    File.open("test.lvl", std::ios::in);
    while (!File.eof()) {
      RendererBuilder Builder;
      File >> Builder;
      File.ignore();
      if (File.good()) {
        Map.push_back(Builder.build());
      } else {
        break;
      }
    }
    File.clear();
    File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (!File.eof()) {
      RendererBuilder Builder;
      File >> Builder;
      File.ignore();
      if (File.good()) {

        MapWall.push_back(Builder.build());
      } else {
        File.clear();
        File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    }
  }

  ImGui::End();
}

auto Gui::frame() -> void {
  auto Now = SDL_GetTicks();
  auto Fps = Now - Last;
  if (Fps >= 1000 / 30) {
    Last = Now;
    ++FrameCount;
  } else {
    return;
  }

  if (SDL_GetWindowFlags(Window.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }
  processEvent();

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("Editor mode", nullptr, &CheckEditor);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  ImGui::Text("frame ms: %d", (int)Fps);

  if (CheckEditor)
    renderCharacterSelector();

  ImGui::Render();
  SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
  SDL_RenderClear(Renderer);

  std::sort(Map.begin(), Map.end());
  std::sort(MapWall.begin(), MapWall.end());

  showMap();

  if (CheckEditor && ShowTileSelector) {
    SDL_FRect CursorRect{TileCursorPos.x, TileCursorPos.y, 32, 32};
    SDL_SetRenderDrawColor(Renderer, 150, 150, 150, 255);
    SDL_RenderRect(Renderer, &CursorRect);
  }

  Enemies[EnemyIndex].render(Renderer, Texture, 300, 100, FrameCount);

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), Renderer);
  SDL_RenderPresent(Renderer);
}

auto Gui::loadTexture(std::string_view Path) -> SdlTexturePtr {

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

auto Gui::processEventEditor(const SDL_Event &Event) -> bool {
  if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      Event.button.button == SDL_BUTTON_LEFT && CheckEditor) {
    SDL_FPoint Point;
    Point.x = (Event.button.x) - static_cast<int>(Event.button.x) % 32;
    Point.y = (Event.button.y) - static_cast<int>(Event.button.y) % 32 + 32;

    auto Tile = Tiles[TileIndex].build();
    Tile->setPos(Point);
    if (CheckBoxWall) {
      Tile->setLevel(CheckLevel);
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
      Event.button.button == SDL_BUTTON_RIGHT && CheckEditor) {
    SDL_FPoint Point;
    Point.x = (Event.button.x) - static_cast<int>(Event.button.x) % 32;
    Point.y = (Event.button.y) - static_cast<int>(Event.button.y) % 32 + 32;

    if (CheckBoxWall)
      std::erase_if(MapWall,
                    [Point](auto &Tile) { return Tile->isSamePos(Point); });
    else
      std::erase_if(Map,
                    [Point](auto &Tile) { return Tile->isSamePos(Point); });
    return true;
  }
  return false;
}

auto Gui::processEventCharacter(const SDL_Event &Event) -> bool {

  if (Event.type == SDL_EVENT_KEY_DOWN && Event.key.key == SDLK_A) {
    Characters[CharacterIndex].setHit();
    return true;
  }
  if (Event.type == SDL_EVENT_KEY_DOWN && Event.key.key == SDLK_RIGHT) {
    Characters[CharacterIndex].setRunning(false);
    return true;
  }
  if (Event.type == SDL_EVENT_KEY_DOWN && Event.key.key == SDLK_LEFT) {
    Characters[CharacterIndex].setRunning(true);
    return true;
  }
  if (Event.type == SDL_EVENT_KEY_UP &&
      (Event.key.key == SDLK_RIGHT || Event.key.key == SDLK_LEFT)) {
    Characters[CharacterIndex].setIdle();
    return true;
  }
  return false;
}

auto Gui::showMap() -> void {

  for (auto &Tile : Map) {
    Tile->render(Renderer, Texture, FrameCount);
  }

  auto Crendered = false;
  for (auto &Tile : MapWall) {
    if (!Crendered && Tile->getPos().y > 108) {
      Crendered = true;
      Characters[CharacterIndex].render(Renderer, Texture, 100, 108,
                                        FrameCount);
    }
    Tile->render(Renderer, Texture, FrameCount);
  }

  if (!Crendered) {
    Crendered = true;
    Characters[CharacterIndex].render(Renderer, Texture, 100, 108, FrameCount);
  }
}
