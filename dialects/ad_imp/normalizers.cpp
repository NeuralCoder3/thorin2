#include <iostream>

#include "thorin/axiom.h"
#include "thorin/world.h"

#include "dialects/ad_imp/autodiff.h"
#include "dialects/ad_imp/utils/helper.h"
#include "dialects/core/core.h"
#include "dialects/mem/autogen.h"
#include "dialects/mem/mem.h"

namespace thorin::ad_imp {

const Def* normalize_autodiff_type_imp(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world = type->world();
    // return arg;

    auto result = autodiff_type_fun(arg);
    if (result != nullptr) { return result; }
    return world.raw_app(type, callee, arg, dbg);
}

const Def* normalize_autodiff_imp(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    // callee = autodiff T
    // arg    = f:T

    // return op_autodiff(arg);
    auto& world = type->world();
    return world.raw_app(type, callee, arg, dbg);
}

const Def* normalize_tangent_type_imp(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    return tangent_type_fun(arg);
    // auto& world = type->world();
    // return world.raw_app(type, callee, arg, dbg);
}

THORIN_ad_imp_NORMALIZER_IMPL

} // namespace thorin::ad_imp
