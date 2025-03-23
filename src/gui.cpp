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
#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module gui;

import sprite;

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
  auto loadCharacters() -> void;

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
  size_t characterIndex;
};

Gui::Gui()
    : window{nullptr, SDL_DestroyWindow}, _done{false}, frameCount{0},
      texture{nullptr, SDL_DestroyTexture}, last{0}, characterIndex{0} {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    throw InitError{std::format("SDL_Init(): {}", SDL_GetError())};

  static constexpr Uint32 window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;

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
  loadCharacters();
}

Gui::~Gui() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
}

auto Gui::loadCharacters() -> void {
  characters.emplace_back("knight f", 128, 68, 28, 16);
  characters.emplace_back("elve f", 128, 4, 28, 16);
  characters.emplace_back("wizard f", 128, 132, 28, 16);
  characters.emplace_back("dwarf f", 128,260, 28, 16);
  characters.emplace_back("lizard f", 128, 196, 28, 16);
  characters.emplace_back("knight m", 128, 100, 28, 16);
  characters.emplace_back("elve m", 128, 4, 36, 16);
  characters.emplace_back("wizard m", 128, 164, 28, 16);
  characters.emplace_back("dwarf m", 128,292, 28, 16);
  characters.emplace_back("lizard m", 128, 228, 28, 16);
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
  if (ImGui::BeginCombo("Character Selector", characters[characterIndex].name().c_str())) {
    for (auto i = 0; i < characters.size(); ++i) {
      if (ImGui::Selectable(characters[i].name().c_str(), characterIndex == i))
        characterIndex = i;

      if (characterIndex ==i)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
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

  characters[characterIndex].render(renderer, texture, 100, 100, frameCount);

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
