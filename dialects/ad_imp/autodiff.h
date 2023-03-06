#pragma once

#include "thorin/world.h"

#include "dialects/ad_imp/autogen.h"

namespace thorin::ad_imp {

inline const Def* op_autodiff(const Def* fun) {
    World& world = fun->world();
    // We rely on the normalized thorin convention that all arguments in functions are grouped.
    // `cn[[args], cont:=cn[returns]]`
    return world.app(world.app(world.ax<ad>(), fun->type()), fun);
}

} // namespace thorin::ad_imp
