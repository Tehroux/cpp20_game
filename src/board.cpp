module;

#include <format>
#include <istream>

export module board;

export class Board {
public:
private:
  friend std::istream &operator>>(std::istream &is, Board &b);

public:
  int x;
  int y;
};

template <> struct std::formatter<Board> {
   constexpr auto parse(auto &ctx) {
    return ctx.begin();
  }

   auto format(Board &b, auto &ctx) const {
    return std::format_to(ctx.out(), "{} {}", b.x, b.y);
  }
};

export std::istream &operator>>(std::istream &is, Board &b) {
  is >> b.x >> b.y;
  return is;
}

