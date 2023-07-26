
#pragma once

#include "thorin/analyses/schedule.h"
#include "thorin/phase/phase.h"

namespace thorin::cache {

class Memoize : public Phase {
public:
    Memoize(World& world, Phase* phase)
        : Phase(world, "memoize", false)
        , phase_(phase) {}

    // void visit(const Scope&) override;

private:
    Phase* phase_;
};

} // namespace thorin::cache
