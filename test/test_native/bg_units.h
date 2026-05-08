// Pure-logic glucose unit conversion. Lives in test/ for now so the native
// test runner can compile it without pulling in any Arduino headers. If
// production code needs the same conversion, promote this header to src/
// and #include it from both places.

#pragma once

#include <cmath>

inline float mgdl_to_mmol(int mgdl) { return mgdl / 18.0f; }

inline int mmol_to_mgdl(float mmol) { return static_cast<int>(std::lround(mmol * 18.0f)); }
