/* flappy.cc --- ncurses flappy bird clone
 * This is free and unencumbered software released into the public domain.
 * c++ -std=c++11 flappy.cc -lncurses -lm -o flappy
 */

#include <algorithm>
#include <thread>
#include <deque>
#include <ctime>
#include <cmath>
#include <ncurses.h>

struct Display {
  Display(int width = 40, int height = 20) : height{height}, width{width} {
    initscr();
    raw();
    timeout(0);
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    erase();
  }

  ~Display() { endwin(); }

  void erase() {
    ::erase();
    for (int y = 0; y < height; y++) {
      mvaddch(y, 0, '|');
      mvaddch(y, width - 1, '|');
    }
    for (int x = 0; x < width; x++) {
      mvaddch(0, x, '-');
      mvaddch(height - 1, x, '-');
    }
    mvaddch(0, 0, '/');
    mvaddch(height - 1, 0, '\\');
    mvaddch(0, width - 1, '\\');
    mvaddch(height - 1, width - 1, '/');
  }

  void refresh() { ::refresh(); }

  const int height, width;

  int block_getch() {
    refresh();
    timeout(-1);
    int c = getch();
    timeout(0);
    return c;
  }
};

struct World {
  World(Display *display) : display{display} {
    for (int x = 1; x < display->width - 1; x++) {
      walls.push_back(0);
    }
  }

  std::deque<int> walls;
  Display *display;
  int steps = 0;

  constexpr static int kRate = 2, kVGap = 2, kHGap = 10;

  int rand_wall() {
    int h = display->height;
    return (rand() % h / 2) + h / 4;
  }

  void step() {
    steps++;
    if (steps % kRate == 0) {
      walls.pop_front();
      if ((steps % (kRate * kHGap)) == 0) {
        walls.push_back(rand_wall());
      } else {
        walls.push_back(0);
      }
    }
  }

  void draw() {
    for (int i = 0; i < walls.size(); i++) {
      int wall = walls[i];
      if (wall != 0) {
        for (int y = 1; y < display->height - 1; y++) {
          if (y < wall - kVGap || y > wall + kVGap) {
            mvaddch(y, i + 1, '*');
          }
        }
      }
    }
    mvprintw(display->height + 1, 0, "Score: %d", score());
  }

  int score() { return std::max(0, steps / (kRate * kHGap) - 2); }
};

struct Bird {
  Bird(Display *display) : y{display->height / 2.0}, display{display} {}

  double y, dy = 0;
  Display *display;

  void gravity() {
    dy += 0.1;
    y += dy;
  }

  void poke() { dy = -0.8; }

  void draw() { draw('@'); }

  void draw(int c) {
    int h = std::round(y);
    h = std::max(1, std::min(h, display->height - 2));
    mvaddch(h, display->width / 2, c);
  }

  bool is_alive(World &world) {
    if (y <= 0 || y >= display->height) {
      return false;
    }
    int wall = world.walls[display->width / 2 - 1];
    if (wall != 0) {
      return y > wall - World::kVGap && y < wall + World::kVGap;
    }
    return true;
  }
};

int main() {
  srand(std::time(NULL));
  Display display;
  while (true) {
    Bird bird{&display};
    World world{&display};
    while (bird.is_alive(world)) {
      int c = getch();
      if (c == 'q') {
        return 0;
      } else if (c != ERR) {
        while (getch() != ERR)
          ;  // clear repeat buffer
        bird.poke();
      }
      display.erase();
      world.step();
      world.draw();
      bird.gravity();
      bird.draw();
      display.refresh();
      std::this_thread::sleep_for(std::chrono::milliseconds{67});
    }
    bird.draw('X');
    mvprintw(display.height + 2, 0,
             "Game over! Press 'q' to quit, 'r' to retry.");
    while (true) {
      int c = display.block_getch();
      if (c == 'q') {
        return 0;
      } else if (c == 'r') {
        break;
      }
    }
  }
  return 0;
}
