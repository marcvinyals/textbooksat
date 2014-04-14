#pragma once

#include "cdcl.h"

struct ui {
  static literal_or_restart get_decision(cdcl& solver);
};
