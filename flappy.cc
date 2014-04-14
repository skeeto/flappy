/* flappy.cc --- ncurses flappy bird clone
 * This is free and unencumbered software released into the public domain.
 * c++ -std=c++11 flappy.cc -lncurses -lm -o flappy
 */

#include <algorithm>
#include <thread>
#include <deque>
#include <ctime>
#include <cmath>
#include <cstring>
#include <ncurses.h>
#include "sqlite3.h"
#include "highscores.hh"

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
      switch (steps % (kRate * kHGap)) {
        case 0:
          walls.push_back(rand_wall());
          break;
        case kRate * 1:
        case kRate * 2:
          walls.push_back(walls.back());
          break;
        default:
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
    mvprintw(display->height, 0, "Score: %d", score());
  }

  int score() { return std::max(0, steps / (kRate * kHGap) - 2); }
};

struct Bird {
  Bird(Display *display) : y{display->height / 2.0}, display{display} {}

  static constexpr double kImpulse = -0.8, kGravity = 0.1;

  double y, dy = kImpulse;
  Display *display;

  void gravity() {
    dy += kGravity;
    y += dy;
  }

  void poke() { dy = kImpulse; }

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

struct Game {
  Game(Display *display) : display{display}, bird{display}, world{display} {}

  Display *display;
  Bird bird;
  World world;

  int run() {
    display->erase();
    const char *intro = "[Press any key to hop upwards]";
    mvprintw(display->height / 2 - 2,
             display->width / 2 - std::strlen(intro) / 2,
             intro);
    bird.draw();
    display->block_getch();
    while (bird.is_alive(world)) {
      int c = getch();
      if (c == 'q') {
        return -1;
      } else if (c != ERR) {
        while (getch() != ERR)
          ;  // clear repeat buffer
        bird.poke();
      }
      display->erase();
      world.step();
      world.draw();
      bird.gravity();
      bird.draw();
      display->refresh();
      std::this_thread::sleep_for(std::chrono::milliseconds{67});
    }
    bird.draw('X');
    display->refresh();
    return world.score();
  }
};

int main() {
  srand(std::time(NULL));
  Display display;
  HighScores scores{"/tmp/highscores.db", display.height};
  while (true) {
    Game game{&display};
    int score = game.run();
    if (score < 0) {
      return 0;
    }
    mvprintw(display.height + 1, 0, "Game over!");
    if (scores.is_best(score)) {
      scores.insert_score("Anonymous", score);
    }

    mvprintw(0, display.width + 4, "== High Scores ==");
    int i = 1;
    for (auto &line : scores.top_scores()) {
      mvprintw(i, display.width + 1, "%s", line.name.c_str());
      mvprintw(i, display.width + 24, "%d", line.score);
      i++;
    }

    mvprintw(display.height + 2, 0, "Press 'q' to quit, 'r' to retry.");
    int c;
    while ((c = display.block_getch()) != 'r') {
      if (c == 'q' || c == ERR) {
        return 0;
      }
    }
  }
  return 0;
}
