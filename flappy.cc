/* flappy.cc --- ncurses flappy bird clone
 * This is free and unencumbered software released into the public domain.
 */

#include <algorithm>
#include <thread>
#include <deque>
#include <ctime>
#include <cmath>
#include <cstring>
#include <ncurses.h>
#include <unistd.h>
#include "sqlite3.h"
#include "highscores.hh"

#define STR_(x) #x
#define STR(x) STR_(x)

struct Display {
  Display(int width = 40, int height = 20) : height{height}, width{width} {
    initscr();
    start_color();
    raw();
    timeout(0);
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    erase();
  }

  ~Display() {
    endwin();
    fflush(stdout);
  }

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

  bool read_name(int y, int x, char *target, size_t n) {
    int p = 0;
    timeout(-1);
    curs_set(1);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    int style = A_BOLD | A_UNDERLINE | COLOR_PAIR(4);
    attron(style);
    bool reading = true, cancelled = false;
    while (reading) {
      move(y, x + p);
      refresh();
      int c = getch();
      switch (c) {
        case '':
          cancelled = true;
        case KEY_ENTER:
        case '\n':
        case '\r':
        case ERR:
          reading = false;
          break;
        case KEY_LEFT:
        case KEY_BACKSPACE:
          attroff(style);
          if (p > 0) mvaddch(y, x + --p, ' ');
          attron(style);
          break;
        case ' ':
          if (p == 0) {
            break;
          }
        default:
          if (p < n - 1) {
            target[p] = c;
            mvaddch(y, x + p++, c);
          }
      }
    }
    target[p + 1] = '\0';
    attroff(style);
    timeout(0);
    curs_set(0);
    return !cancelled;
  }

  void center(int yoff, const char *str) {
    mvprintw(height / 2 + yoff, width / 2 - std::strlen(str) / 2, "%s", str);
  }
};

bool is_exit(int c) { return c == 'q' || c == ''; }

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
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_GREEN);
    attron(COLOR_PAIR(2));
    for (int i = 0; i < walls.size(); i++) {
      int wall = walls[i];
      if (wall != 0) {
        for (int y = 1; y < display->height - 1; y++) {
          if (y == wall - kVGap - 1 || y == wall + kVGap + 1) {
            attroff(COLOR_PAIR(2));
            attron(COLOR_PAIR(3));
            mvaddch(y, i + 1, '=');
            attroff(COLOR_PAIR(3));
            attron(COLOR_PAIR(2));
          } else if (y < wall - kVGap || y > wall + kVGap) {
            mvaddch(y, i + 1, '|');
          }
        }
      }
    }
    attroff(COLOR_PAIR(2));
    attron(A_BOLD);
    mvprintw(display->height, 0, "Score: %d", score());
    attroff(A_BOLD);
  }

  int score() { return std::max(0, (steps - 2) / (kRate * kHGap) - 2); }
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

  void draw() {
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    attron(COLOR_PAIR(1) | A_BOLD);
    draw('@');
    attroff(COLOR_PAIR(1) | A_BOLD);
  }

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
    const char *title = "Flappy Curses", *version = "v" STR(VERSION),
               *intro = "[Press SPACE to hop upwards]",
                 *url = "https://github.com/skeeto/flappy";
    display->center(-3, title);
    display->center(-2, version);
    display->center(2, intro);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    attron(COLOR_PAIR(6) | A_UNDERLINE);
    display->center(10, url);
    attroff(COLOR_PAIR(6) | A_UNDERLINE);
    bird.draw();
    if (is_exit(display->block_getch())) return -1;
    while (bird.is_alive(world)) {
      int c = getch();
      if (is_exit(c)) {
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
    init_pair(5, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(5) | A_BOLD);
    bird.draw('X');
    attroff(COLOR_PAIR(5) | A_BOLD);
    display->refresh();
    return world.score();
  }
};

void print_scores(Display &display, HighScores &scores) {
  attron(A_BOLD);
  mvprintw(0, display.width + 4, "== High Scores ==");
  attroff(A_BOLD);
  int i = 1;
  for (auto &line : scores.top_scores()) {
    mvprintw(i, display.width + 1, "%s", line.name.c_str());
    clrtoeol();
    mvprintw(i, display.width + 24, "%d", line.score);
    i++;
  }
}

int main(int argc, char **argv) {
  srand(std::time(NULL));

  /* Parse command line arguments. */
  int opt;
  const char *filename = "/tmp/flappy-scores.db", *host = "localhost";
  while ((opt = getopt(argc, argv, "d:h:p")) != -1) {
    switch (opt) {
      case 'd':
        filename = optarg;
        break;
      case 'h':
        host = optarg;
        break;
      case 'p':  // ignore
        break;
    }
  }

  Display display;
  HighScores scores{filename, display.height - 1};

  while (true) {
    Game game{&display};

    int score = game.run();
    if (score < 0) {
      return 0;  // game quit early
    }

    /* Game over */
    mvprintw(display.height + 1, 0, "Game over!");
    print_scores(display, scores);

    /* Enter new high score */
    if (scores.is_best(score)) {
      attron(A_BOLD);
      mvprintw(display.height + 2, 0, "You have a high score!");
      mvprintw(display.height + 3, 0, "Enter name: ");
      attroff(A_BOLD);
      char name[23] = {0};
      if (!display.read_name(display.height + 3, 12, name, sizeof(name))) {
        return 0;
      }
      if (std::strlen(name) == 0) {
        std::strcpy(name, "(anonymous)");
      }
      scores.insert_score(name, score);
      move(display.height + 3, 0);
      clrtoeol();
      print_scores(display, scores);
    }

    /* Handle quit/restart */
    mvprintw(display.height + 2, 0, "Press 'q' to quit, 'r' to retry.");
    int c;
    while ((c = display.block_getch()) != 'r') {
      if (is_exit(c) || c == ERR) {
        return 0;
      }
    }
  }
  return 0;
}
