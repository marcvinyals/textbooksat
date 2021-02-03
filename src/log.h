#pragma once

enum log_level {
  LOG_NOTHING = -1,
  LOG_RESULTS,
  LOG_EFFECTS,
  LOG_DECISIONS,
  LOG_ACTIONS,
  LOG_STATE_SUMMARY,
  LOG_STATE,
  LOG_DETAIL,
};

extern log_level max_log_level;

#define LOG(level) if (level<=max_log_level) cout
