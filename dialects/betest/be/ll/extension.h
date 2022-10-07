#pragma once

#include "thorin/world.h"

namespace thorin {
namespace ll {
class Emitter;
class BB;
std::optional<std::string> extend(Emitter* emitter, BB* bb, const Def*& def);
} // namespace ll
} // namespace thorin
