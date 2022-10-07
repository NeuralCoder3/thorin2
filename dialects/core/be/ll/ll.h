#pragma once

#include <deque>
#include <fstream>
#include <iomanip>
#include <ostream>

#include "thorin/dialects.h"

namespace thorin {

class World;

namespace ll {

void emit(World&, std::ostream&, Extensions);

int compile(World&, std::string name);
int compile(World&, std::string ll, std::string out);
int compile_and_run(World&, std::string name, std::string args = {});

} // namespace ll
} // namespace thorin
