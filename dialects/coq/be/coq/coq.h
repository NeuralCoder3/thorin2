#pragma once

#include <ostream>

namespace thorin {

class World;

namespace coq {

void emit(World&, std::ostream&);

int compile(World&, std::string name);
int compile(World&, std::string ll, std::string out);
int compile_and_run(World&, std::string name, std::string args = {});

} // namespace coq
} // namespace thorin
