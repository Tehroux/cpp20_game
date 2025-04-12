module;

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <imgui.h>

#include <SDL3/SDL_stdinc.h>

#include <format>
#include <fstream>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

export module gui;

import sdlHelpers;
import tile;
import sprite;

/// used to manage ImGui gui
export class Gui {
public:
  Gui() = default;
  /// constructor
  ///
  /// \param[in] Window the window to render the Gui to
  /// \param[in] Renderer the renderer used to render to the window
  Gui(const SdlWindow &window, SdlRenderer renderer);

  Gui(const Gui &) = default;
  Gui(Gui &&) = delete;
  auto operator=(const Gui &) -> Gui & = default;
  auto operator=(Gui &&) -> Gui & = delete;

  ~Gui();

  auto render(const SdlRenderer &renderer,
              std::vector<CharacterSprite> &characters,
              std::vector<CharacterSprite> &enemies,
              std::vector<RendererBuilder> &tiles,
              std::vector<std::unique_ptr<TileConcrete>> &map,
              std::vector<std::unique_ptr<TileConcrete>> &mapWall) -> void;

  [[nodiscard]] auto isEditorMode() const -> bool { return checkEditor_; }
  [[nodiscard]] auto isLevel() const -> bool { return checkLevel_; }
  [[nodiscard]] auto isRunning() const -> bool { return checkBoxRuning_; }
  [[nodiscard]] auto isWall() const -> bool { return checkBoxWall_; }

  auto frameRenderingDuration(Uint64 timeToRenderFrame) {
    this->timeToRenderFrame_ = timeToRenderFrame;
  }

  auto renderEditorOptions(std::vector<CharacterSprite> &characters,
                           std::vector<CharacterSprite> &enemies,
                           std::vector<RendererBuilder> &tiles,
                           std::vector<std::unique_ptr<TileConcrete>> &map,
                           std::vector<std::unique_ptr<TileConcrete>> &mapWall)

      -> void;

  template <class Array>
  auto renderComboBox(const char *name, Array &array, size_t &currentIndex)
      -> void;

  /// processEvent for imgui
  ///
  /// \param Event the event from Sdl
  /// \return true if imgui used the event
  static auto processEvent(SDL_Event &event) -> bool;

  [[nodiscard]] auto getCharacterIndex() const -> size_t {
    return characterIndex_;
  }
  [[nodiscard]] auto getEnemyIndex() const -> size_t { return enemyIndex_; }
  [[nodiscard]] auto getTileIndex() const -> size_t { return tileIndex_; }

private:
  bool checkBoxRuning_{};
  bool checkBoxWall_{};
  bool checkLevel_{};
  bool checkEditor_{};
  Uint64 timeToRenderFrame_{};
  size_t characterIndex_{};
  size_t enemyIndex_{};
  size_t tileIndex_{};
};

Gui::Gui(const SdlWindow &window, SdlRenderer renderer) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &imIo = ImGui::GetIO();
  imIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imIo.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  window.initForRenderer(renderer);
}

Gui::~Gui() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

auto Gui::render(const SdlRenderer &renderer,
                 std::vector<CharacterSprite> &characters,
                 std::vector<CharacterSprite> &enemies,
                 std::vector<RendererBuilder> &tiles,
                 std::vector<std::unique_ptr<TileConcrete>> &map,
                 std::vector<std::unique_ptr<TileConcrete>> &mapWall) -> void {

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

  std::string frameRenderingDuationText =
      std::format("frame ms:{}", timeToRenderFrame_);
  ImGui::TextUnformatted(frameRenderingDuationText.data(),
                         &*frameRenderingDuationText.cend());

  if (checkEditor_) {
    renderEditorOptions(characters, enemies, tiles, map, mapWall);
  }

  ImGui::Render();
  renderer.imguiRenderDrawData();
}

auto Gui::processEvent(SDL_Event &event) -> bool {

  ImGui_ImplSDL3_ProcessEvent(&event);
  return ImGui::GetIO().WantCaptureMouse;
}

template <class Array>
auto Gui::renderComboBox(const char *name, Array &array, size_t &currentIndex)
    -> void {
  if (ImGui::BeginCombo(name, array[currentIndex].name().c_str())) {
    for (auto index = 0; index < array.size(); ++index) {
      if (ImGui::Selectable(array[index].name().c_str(),
                            std::cmp_equal(currentIndex, index))) {
        currentIndex = index;
      }

      if (std::cmp_equal(currentIndex, index)) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}

auto Gui::renderEditorOptions(std::vector<CharacterSprite> &characters,
                              std::vector<CharacterSprite> &enemies,
                              std::vector<RendererBuilder> &tiles,
                              std::vector<std::unique_ptr<TileConcrete>> &map,
                              std::vector<std::unique_ptr<TileConcrete>> &mapWall)
    -> void {
  ImGui::Begin("Editor");
  renderComboBox("Character Selector", characters, characterIndex_);
  renderComboBox("Enemy Selector", enemies, enemyIndex_);

  if (ImGui::Checkbox("running", &checkBoxRuning_)) {
    if (checkBoxRuning_) {
      enemies[enemyIndex_].setRunning(false);
    } else {
      enemies[enemyIndex_].setIdle();
    }
  }

  renderComboBox("Tile Selector", tiles, tileIndex_);

  ImGui::Checkbox("wall", &checkBoxWall_);

  ImGui::Checkbox("Level", &checkLevel_);

  if (ImGui::Button("save")) {
    std::fstream file;
    file.open("test.lvl", std::ios::out | std::ios::trunc);

    for (auto &tile : map) {
      file << *tile << '\n';
    }
    file << "=====\n";
    for (auto &tile : mapWall) {
      file << *tile << '\n';
    }
  }

  if (ImGui::Button("load")) {
    map.clear();
    mapWall.clear();
    std::fstream file;
    file.open("test.lvl", std::ios::in);
    while (!file.eof()) {
      RendererBuilder builder;
      file >> builder;
      file.ignore();
      if (file.good()) {
        map.push_back(builder.build());
      } else {
        break;
      }
    }
    file.clear();
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (!file.eof()) {
      RendererBuilder builder;
      file >> builder;
      file.ignore();
      if (file.good()) {

        mapWall.push_back(builder.build());
      } else {
        file.clear();
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    }
  }

  ImGui::End();
}
