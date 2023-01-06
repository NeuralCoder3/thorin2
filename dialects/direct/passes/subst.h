// TODO: move

#pragma once

#include <vector>

#include <thorin/def.h>
#include <thorin/pass/pass.h>

#include "thorin/analyses/schedule.h"
#include "thorin/phase/phase.h"

namespace thorin::direct {

class SubstPhase : public RWPhase {
public:
    SubstPhase(World& world, Def2Def& subst)
        : RWPhase(world, "cps2ds_rewrite")
        , subst_(subst)
        {}

    const Def* rewrite_structural(const Def*) override;

    static PassTag* ID();

private:
    Def2Def rewritten;
    Def2Def subst_;
};


} // namespace thorin::direct
