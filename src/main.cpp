import game;

auto main(int, char **) -> int {
  Game GameInstance;

  while (!GameInstance.done()) {
    GameInstance.frame();
  }
  return 0;
}
