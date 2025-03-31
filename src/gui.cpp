module;

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <imgui.h>

#include <SDL3/SDL_stdinc.h>

#include <fstream>
#include <limits>
#include <memory>
#include <vector>

export module gui;

import sdlHelpers;
import tile;
import sprite;

/// used to manage ImGui gui
export class Gui {
public:
  /// constructor
  ///
  /// \param[in] Window the window to render the Gui to
  /// \param[in] Renderer the renderer used to render to the window
  Gui(SdlWindowPtr &Window, SDL_Renderer *Renderer);
  ~Gui();

  auto render(SDL_Renderer *Renderer, std::vector<CharacterSprite> &Characters,
              std::vector<CharacterSprite> &Enemies,
              std::vector<RendererBuilder> &Tiles,
              std::vector<std::unique_ptr<Renderable>> &Map,
              std::vector<std::unique_ptr<Renderable>> &MapWall) -> void;

  auto isEditorMode() -> bool { return CheckEditor; }
  auto isLevel() -> bool { return CheckLevel; }
  auto isRunning() -> bool { return CheckBoxRuning; }
  auto isWall() -> bool { return CheckBoxWall; }

  auto setTimeToRenderFrame(int TimeToRenderFrame) {
    this->TimeToRenderFrame = TimeToRenderFrame;
  }

  auto
  renderCharacterSelector(std::vector<CharacterSprite> &Characters,
                          std::vector<CharacterSprite> &Enemies,
                          std::vector<RendererBuilder> &Tiles,
                          std::vector<std::unique_ptr<Renderable>> &Map,
                          std::vector<std::unique_ptr<Renderable>> &MapWall)
      -> void;
  auto processEvent(SDL_Event &Event) -> bool;

  auto getCharacterIndex() -> size_t { return CharacterIndex; }
  auto getEnemyIndex() -> size_t { return EnemyIndex; }
  auto getTileIndex() -> size_t { return TileIndex; }

private:
  bool CheckBoxRuning;
  bool CheckBoxWall;
  bool CheckLevel;
  bool CheckEditor;
  Uint64 TimeToRenderFrame;
  size_t CharacterIndex;
  size_t EnemyIndex;
  size_t TileIndex;
};

Gui::Gui(SdlWindowPtr &Window, SDL_Renderer *Renderer)
    : CheckBoxRuning{false}, CheckBoxWall{false}, CheckEditor{false},
      CheckLevel{false}, TimeToRenderFrame{0}, CharacterIndex{0}, EnemyIndex{0},
      TileIndex{0} {

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &Io = ImGui::GetIO();
  Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  Io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(Window.get(), Renderer);
  ImGui_ImplSDLRenderer3_Init(Renderer);
}

Gui::~Gui() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

auto Gui::render(SDL_Renderer *Renderer,
                 std::vector<CharacterSprite> &Characters,
                 std::vector<CharacterSprite> &Enemies,
                 std::vector<RendererBuilder> &Tiles,
                 std::vector<std::unique_ptr<Renderable>> &Map,
                 std::vector<std::unique_ptr<Renderable>> &MapWall) -> void {

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

  ImGui::Text("frame ms: %lu", TimeToRenderFrame);

  if (CheckEditor)
    renderCharacterSelector(Characters, Enemies, Tiles, Map, MapWall);

  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), Renderer);
}

auto Gui::processEvent(SDL_Event &Event) -> bool {

  ImGui_ImplSDL3_ProcessEvent(&Event);
  return ImGui::GetIO().WantCaptureMouse;
}

auto Gui::renderCharacterSelector(
    std::vector<CharacterSprite> &Characters,
    std::vector<CharacterSprite> &Enemies, std::vector<RendererBuilder> &Tiles,
    std::vector<std::unique_ptr<Renderable>> &Map,
    std::vector<std::unique_ptr<Renderable>> &MapWall) -> void {
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
    for (auto Index = 0; Index < Tiles.size(); ++Index) {
      if (ImGui::Selectable(Tiles[Index].name().c_str(), TileIndex == Index))
        TileIndex = Index;

      if (TileIndex == Index)
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
