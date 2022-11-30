#include "dialects/compile/autogen.h"
#include "dialects/compile/compile.h"

namespace thorin::compile {

// `pass_phase (pass_list pass1 ... passn)` -> `passes_to_phase n (pass1, ..., passn)`
const Def* normalize_pass_phase(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world = type->world();
    world.DLOG("normalize pass_phase");
    world.DLOG("  callee {} : {}", callee, callee->type());
    world.DLOG("  arg    {} : {}", arg, arg->type());

    auto [ax, _] = collect_args(arg);
    if (ax->flags() != flags_t(Axiom::Base<pass_list>)) {
        world.ELOG("pass_phase expects a pass_list as argument, but got {}", ax->name());
    }
    assert(ax->flags() == flags_t(Axiom::Base<pass_list>) && "pass_phase expects a pass_list (after normalization)");

    auto [f_ax, pass_list_defs] = collect_args(arg);
    assert(f_ax->flags() == flags_t(Axiom::Base<pass_list>));
    auto n = pass_list_defs.size();

    auto pass2phase = world.app(world.ax<passes_to_phase>(), world.lit_nat(n));
    return world.app(pass2phase, world.tuple(pass_list_defs), dbg);
}

/// `combined_phase (phase_list phase1 ... phasen)` -> `phases_to_phase n (phase1, ..., phasen)`
const Def* normalize_combined_phase(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world = type->world();
    world.DLOG("normalize combined_phase");
    world.DLOG("  callee {} : {}", callee, callee->type());
    world.DLOG("  arg    {} : {}", arg, arg->type());

    auto [ax, phase_list_defs] = collect_args(arg);
    assert(ax->flags() == flags_t(Axiom::Base<phase_list>));
    auto n = phase_list_defs.size();

    auto pass2phase = world.app(world.ax<phases_to_phase>(), world.lit_nat(n));
    return world.app(pass2phase, world.tuple(phase_list_defs), dbg);
}

/// `single_pass_phase pass` -> `passes_to_phase 1 pass`
const Def* normalize_single_pass_phase(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world = type->world();
    world.DLOG("normalize single pass phase");
    world.DLOG("  callee {} : {}", callee, callee->type());
    world.DLOG("  arg    {} : {}", arg, arg->type());
    auto pass2phase = world.app(world.ax<passes_to_phase>(), world.lit_nat_1());
    return world.app(pass2phase, arg, dbg);
}

/// `combine_pass_list K (pass_list pass11 ... pass1N) ... (pass_list passK1 ... passKM) = pass_list pass11 ... p1N ...
/// passK1 ... passKM`
const Def* normalize_combine_pass_list(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world     = type->world();
    auto pass_lists = arg->projs();
    DefVec passes;
    world.DLOG("normalize combine_pass_list");
    world.DLOG("  callee {} : {}", callee, callee->type());
    world.DLOG("  arg    {} : {}", arg, arg->type());

    assert(0);

    for (auto pass_list_def : pass_lists) {
        auto [ax, pass_list_defs] = collect_args(pass_list_def);
        assert(ax->flags() == flags_t(Axiom::Base<pass_list>));
        passes.insert(passes.end(), pass_list_defs.begin(), pass_list_defs.end());
    }
    return world.raw_app(type, world.ax<pass_list>(), world.tuple(passes), dbg);
}

THORIN_compile_NORMALIZER_IMPL

} // namespace thorin::compile
