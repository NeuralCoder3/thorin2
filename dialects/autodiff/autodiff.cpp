#include "dialects/autodiff/autodiff.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

#include "dialects/affine/passes/lower_for.h"
#include "dialects/autodiff/passes/autodiff_eval.h"
#include "dialects/autodiff/passes/autodiff_reduce.h"
#include "dialects/autodiff/passes/def_inliner.h"
#include "dialects/mem/passes/rw/reshape.h"
// #include "dialects/autodiff/passes/autodiff_zero.h"
// #include "dialects/autodiff/passes/autodiff_zero_cleanup.h"
// #include "dialects/direct/passes/ds2cps.h"

using namespace thorin;

class DebugWrapper : public RWPass<DebugWrapper, Lam> {
public:
    DebugWrapper(PassMan& man)
        : RWPass(man, "debug_pass") {}

    void prepare() override { world().debug_dump(); }
};

// builder.extend_opt_phase(107,
//                          [](thorin::PassMan& man) { man.add<thorin::autodiff::AutodiffReduce>(); });
// builder.extend_opt_phase(108, [](thorin::PassMan& man) { man.add<thorin::autodiff::AutoDiffEval>(); });
// builder.add_opt(133);
extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"autodiff",
            [](Passes& passes) {
                register_pass<autodiff::ad_reduce_pass, autodiff::AutodiffReduce>(passes);
                register_pass<autodiff::ad_eval_pass, autodiff::AutoDiffEval>(passes);
                // register_pass<autodiff::ad_eval_pass, autodiff::AutoDiffEval>(passes);
                // register_pass<autodiff::ad_zero_pass, autodiff::AutoDiffZero>(passes);
                // register_pass<autodiff::ad_zero_cleanup_pass, autodiff::AutoDiffZeroCleanup>(passes);
                // register_pass<autodiff::ad_ext_cleanup_pass, autodiff::AutoDiffExternalCleanup>(passes);
            },
            nullptr, [](Normalizers& normalizers) { autodiff::register_normalizers(normalizers); }};
}
