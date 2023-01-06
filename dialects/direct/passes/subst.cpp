#include "dialects/direct/passes/subst.h"
#include <iostream>
#include <thorin/lam.h>
#include "dialects/direct/direct.h"

namespace thorin::direct {

const Def* SubstPhase::rewrite_structural(const Def* def) {
    auto& world = def->world();

    if(subst_.contains(def)) {
        auto arg = subst_[def];
        world.DLOG("  rewrite {} : {} to {} : {}", def, def->type(), arg, arg->type());
        return arg;
    }


    // ignore unapplied axioms to avoid spurious type replacements
    if (auto ax = def->isa<Axiom>()) { return def; }

    return Rewriter::rewrite_structural(def); // continue recursive rewriting with everything else
}
}
