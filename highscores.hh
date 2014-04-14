#ifndef FLAPPY_HIGHSCORES_HH
#define FLAPPY_HIGHSCORES_HH

#include <vector>
#include <string>
#include "sqlite3.h"

struct listing {
  std::string name;
  int score;
};

class HighScores {
 public:
  HighScores(const char *file, int size = 10);
  ~HighScores();

  bool is_best(int score);
  void insert_score(const char *name, int score);
  std::vector<listing> top_scores();

 private:
  int size_;
  sqlite3 *db;
  sqlite3_stmt *stmt_table, *stmt_timeout, *stmt_top, *stmt_place, *stmt_insert;
};

#endif
