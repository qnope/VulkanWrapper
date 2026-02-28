export module vw.window:sdl_initializer;
import sdl3rd;

export namespace vw {

class SDL_Initializer {
  public:
    SDL_Initializer();
    SDL_Initializer(const SDL_Initializer &) = delete;
    SDL_Initializer(SDL_Initializer &&) = delete;
    SDL_Initializer &operator=(SDL_Initializer &&) = delete;
    SDL_Initializer &operator=(const SDL_Initializer &) = delete;
    ~SDL_Initializer();
};
} // namespace vw
