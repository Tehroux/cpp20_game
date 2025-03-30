import gui;

auto main(int, char **) -> int {
  Gui GameGui;

  while (!GameGui.done()) {
    GameGui.frame();
  }
  return 0;
}
