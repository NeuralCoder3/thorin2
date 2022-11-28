#pragma once

#include <queue>

#include "thorin/phase/phase.h"

namespace thorin::mem {

using DefQueue = std::deque<const Def*>;

static int i = 0;

/// The general idea of this pass/phase is to change the shape of signatures of functions.
/// Example: `Cn[ [mem,  A, B], C  , ret]`
/// Flat   : `Cn[ [mem, [A, B , C]], ret]`
/// Arg    : `Cn[  mem,  A, B , C  , ret]`
/// For convenience, we want Arg-style for optimizations.
/// The invariant is that every closed function has at most one "real" argument and a return-continuation.
/// If memory is present, the argument is a pair of memory and the remaining arguments.
/// However, flat style is required for code generation. Especially in the closure conversion.
///
/// The concept is to rewrite all signatures of functions with consistent reassociation of arguments.
/// This change is propagated to (nested) applications.
// TODO: use RWPhase instead
class Reshape : public RWPass<Reshape, Lam> {
public:
    enum Mode { Flat, Arg };

    Reshape(PassMan& man, Mode mode)
        : RWPass(man, "reshape")
        , mode_(mode) {}

    void enter() override;

private:
    /// memoized version of rewrite_convert
    const Def* rewrite(const Def* def);

    const Def* rewrite_(const Def* def);
    Lam* reshapeLam(Lam* def);

    const Def* reshape_type(const Def* T);
    const Def* reshape(const Def* def);
    // ignores mode and follows T for reshaping (we also need the inverse transformation)
    // const Def* reshape(const Def* def, const Def* T);
    // const Def* flatten_ty(const Def* ty);
    // const Def* flatten_ty_convert(const Def* ty);
    // void aggregate_sigma(const Def* ty, DefQueue& ops);
    // // const Def* wrap(const Def* def, const Def* target_ty);
    // const Def* reshape(const Def* mem, const Def* ty, DefQueue& vars);
    // const Def* reshape(const Def* arg, const Pi* target_pi);

    Def2Def old2new_;
    Mode mode_;
    Def2Def old2flatten_;
};

} // namespace thorin::mem
