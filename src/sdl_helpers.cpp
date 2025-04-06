module;

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "SDL3_image/SDL_image.h"

#include <exception>
#include <format>
#include <istream>
#include <memory>
#include <ostream>

export module sdlHelpers;

/// Used to  auto delete SDL_Texture
export using SdlTexturePtr =
    std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)>;

/// Used to  auto delete SDL_Surface
export using SdlSurfacePtr =
    std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)>;

/// add SDL_FPoint to an output stream in the form 'x y'
export auto operator<<(std::ostream &ostream, const SDL_FPoint &point)
    -> std::ostream & {
  ostream << point.x << ' ' << point.y;
  return ostream;
}

/// add SDL_Frect to an output stream in the form 'x y w h'
export auto operator<<(std::ostream &ostream, const SDL_FRect &rect)
    -> std::ostream & {
  ostream << rect.x << ' ' << rect.y << ' ' << rect.w << ' ' << rect.h;
  return ostream;
}

/// read an SDL_FPoint from an input stream in the form 'x y'
export auto operator>>(std::istream &istream, SDL_FPoint &point)
    -> std::istream & {
  istream >> point.x >> point.y;
  return istream;
}

/// read an SDL_Frect from an input stream in the form 'x y w h'
export auto operator>>(std::istream &istream, SDL_FRect &rect)
    -> std::istream & {
  istream >> rect.x >> rect.y >> rect.w >> rect.h;
  return istream;
}

/// an error occured while initializing
export class InitError : public std::exception {
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

/// an error occured while loading a texture
export class TextureLoadingError : public std::exception {
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

export class SdlRenderer {
  friend class SdlWindow;

public:
  explicit SdlRenderer(SDL_Renderer *renderer) noexcept : renderer_{renderer} {}

  auto createTextureFromPath(const char *path) const -> SdlTexturePtr {
    auto *iostr = SDL_IOFromFile(path, "r");
    if (iostr == nullptr) {
      throw TextureLoadingError{
          std::format("SDL_IOFromFile(): {}", SDL_GetError())};
    }

    const SdlSurfacePtr surface{IMG_LoadPNG_IO(iostr), SDL_DestroySurface};
    if (!surface) {
      throw TextureLoadingError{
          std::format("IMG_LoadPNG_IO(): {}", SDL_GetError())};
    }

    SdlTexturePtr texture = {
        SDL_CreateTextureFromSurface(renderer_, surface.get()),
        SDL_DestroyTexture};
    if (!texture) {
      throw TextureLoadingError{
          std::format("SDL_CreateTextureFromSurface(): {}", SDL_GetError())};
    }

    SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
    return texture;
  }

  auto setRenderDrawColor(const SDL_Color &color) const noexcept -> void {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  }

  auto renderClear() const noexcept -> void { SDL_RenderClear(renderer_); }
  auto renderRect(const SDL_FRect &rect) const noexcept -> void {
    SDL_RenderRect(renderer_, &rect);
  }

  auto renderTextureRotated(const SdlTexturePtr &texture,
                            const SDL_FRect &sourceRect,
                            const SDL_FRect &destRect, double angle,
                            const SDL_FPoint &center,
                            SDL_FlipMode flipMode) const noexcept -> void {
    SDL_RenderTextureRotated(renderer_, texture.get(), &sourceRect, &destRect,
                             angle, &center, flipMode);
  }

  auto renderTexture(const SdlTexturePtr &texture, const SDL_FRect &sourceRect,
                     const SDL_FRect &destRect) const noexcept -> void {
    SDL_RenderTexture(renderer_, texture.get(), &sourceRect, &destRect);
  }

  auto renderPresent() const noexcept -> void { SDL_RenderPresent(renderer_); }

  auto imguiRenderDrawData() const noexcept -> void {
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
  }

private:
  [[nodiscard]] auto get() const noexcept -> SDL_Renderer * {
    return renderer_;
  }

  SDL_Renderer *renderer_;
};

export class SdlWindow {
public:
  SdlWindow(const char *name, const SDL_Point &size, Uint32 flags)
      : window_(SDL_CreateWindow(name, size.x, size.y, flags)) {
    if (window_ == nullptr) {
      throw InitError(std::format("SDL_CreateWindow(): {}", SDL_GetError()));
    }
  }

  SdlWindow(const SdlWindow &) = default;
  SdlWindow(SdlWindow &&) = delete;
  auto operator=(const SdlWindow &) -> SdlWindow & = default;
  auto operator=(SdlWindow &&) -> SdlWindow & = delete;

  auto setPosition(int horizontalPosition, int verticalPosition) noexcept
      -> void {
    SDL_SetWindowPosition(window_, horizontalPosition, verticalPosition);
  }

  auto showWindow() -> void { SDL_ShowWindow(window_); }

  auto initForRenderer(SdlRenderer renderer) const noexcept -> void {
    ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer.get());
    ImGui_ImplSDLRenderer3_Init(renderer.get());
  }

  [[nodiscard]] auto createRenderer() const -> SdlRenderer {
    auto *rendererPtr = SDL_CreateRenderer(window_, nullptr);
    if (rendererPtr == nullptr) {
      throw InitError{std::format("SDL_CreateRenderer(): {}", SDL_GetError())};
    }
    SDL_SetRenderVSync(rendererPtr, 1);

    SdlRenderer renderer{rendererPtr};
    return renderer;
  }

  [[nodiscard]] auto getWindowID() const noexcept -> SDL_WindowID {
    return SDL_GetWindowID(window_);
  };
  [[nodiscard]] auto getWindowFlags() const noexcept -> SDL_WindowFlags {
    return SDL_GetWindowFlags(window_);
  }

  ~SdlWindow() noexcept { SDL_DestroyWindow(window_); }

private:
  SDL_Window *window_;
};
