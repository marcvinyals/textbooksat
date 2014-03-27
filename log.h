#pragma once

enum log_level {
  LOG_NOTHING = 0,
  LOG_ACTIONS,
  LOG_STATE,
  LOG_DETAIL,
};

extern log_level max_log_level;

#define LOG(level) if (level<=max_log_level) cout
