#include <cstring>
#include "sqlite3.h"
#include "highscores.hh"

static const char *table =
    "CREATE TABLE IF NOT EXISTS scores (name STRING, score INTEGER)";
static const char *timeout =
    "PRAGMA busy_timeout = 30000";
static const char *top =
    "SELECT name, score FROM scores ORDER BY score DESC LIMIT ?";
static const char *place =
    "SELECT count(*) FROM scores WHERE score >= ?";
static const char *insert =
    "INSERT INTO scores VALUES (?, ?)";

#define REGISTER(name)                                                  \
  sqlite3_prepare_v2(db, name, std::strlen(name), &stmt_##name, nullptr);

HighScores::HighScores(const char *file, int size) {
  size_ = size;
  sqlite3_initialize();
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  sqlite3_open_v2(file, &db, flags, nullptr);

  REGISTER(timeout);
  sqlite3_step(stmt_timeout);
  sqlite3_finalize(stmt_timeout);

  REGISTER(table);
  sqlite3_step(stmt_table);
  sqlite3_finalize(stmt_table);

  REGISTER(top);
  sqlite3_bind_int(stmt_top, 1, size);
  REGISTER(place);
  REGISTER(insert);
}

HighScores::~HighScores() {
  sqlite3_close(db);
  sqlite3_shutdown();
}

bool HighScores::is_best(int score) {
  sqlite3_bind_int(stmt_place, 1, score);
  sqlite3_step(stmt_place);
  int count = sqlite3_column_int(stmt_place, 0);
  sqlite3_reset(stmt_place);
  return count < size_;
}

void HighScores::insert_score(const char *name, int score) {
  sqlite3_bind_text(stmt_insert, 1, name, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt_insert, 2, score);
  sqlite3_step(stmt_insert);
  sqlite3_reset(stmt_insert);
}

std::vector<listing> HighScores::top_scores() {
  std::vector<listing> scores;
  sqlite3_bind_int(stmt_top, 1, size_);
  while (sqlite3_step(stmt_top) == SQLITE_ROW) {
    listing line;
    const char *name = (const char*) sqlite3_column_text(stmt_top, 0);
    size_t length = sqlite3_column_bytes(stmt_top, 0);
    line.name = std::string{name, length};
    line.score = sqlite3_column_int(stmt_top, 1);
    scores.push_back(line);
  }
  sqlite3_reset(stmt_top);
  return scores;
}
