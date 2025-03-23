import gui;


auto main(int, char **) -> int {
  Gui g;

  while (!g.done()) {
    g.frame();
  }
  return 0;
}
