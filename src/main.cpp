import game;

auto main(int  /*argc*/, char * /*argv*/[]) -> int {
  Game gameInstance;

  while (!gameInstance.done()) {
    gameInstance.frame();
  }
  return 0;
}
