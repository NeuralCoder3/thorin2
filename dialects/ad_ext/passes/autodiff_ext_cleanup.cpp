#include "dialects/ad_ext/passes/autodiff_ext_cleanup.h"

#include <iostream>

#include <thorin/lam.h>

namespace thorin::ad_ext {

void AutoDiffExternalCleanup::enter() {
    Lam* lam = curr_nom();
    if (lam->name().starts_with("internal_diff_")) {
        lam->make_internal();
        world().DLOG("internalized {}", lam);
    }
}

} // namespace thorin::ad_ext
