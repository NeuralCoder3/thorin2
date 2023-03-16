#include "dialects/ad_imp/autodiff.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

#include "dialects/ad_imp/passes/autodiff_eval.h"
#include "dialects/ad_imp/passes/autodiff_reduce.h"
#include "dialects/ad_imp/passes/def_inliner.h"
#include "dialects/affine/passes/lower_for.h"
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
//                          [](thorin::PassMan& man) { man.add<thorin::ad_imp::AutodiffReduce>(); });
// builder.extend_opt_phase(108, [](thorin::PassMan& man) { man.add<thorin::ad_imp::AutoDiffEval>(); });
// builder.add_opt(133);
extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {
        "ad_imp",
        [](Passes& passes) {
            register_pass<ad_imp::ad_reduce_pass, ad_imp::AutodiffReduce>(passes);
            register_pass<ad_imp::ad_eval_pass, ad_imp::AutoDiffEval>(passes);
            // register_pass<ad_imp::ad_eval_pass, ad_imp::AutoDiffEval>(passes);
            // register_pass<ad_imp::ad_zero_pass, ad_imp::AutoDiffZero>(passes);
            // register_pass<ad_imp::ad_zero_cleanup_pass, ad_imp::AutoDiffZeroCleanup>(passes);
            // register_pass<ad_imp::ad_ext_cleanup_pass, ad_imp::AutoDiffExternalCleanup>(passes);
        },
        nullptr,
        //
        // nullptr
        [](Normalizers& normalizers) { ad_imp::register_normalizers(normalizers); }
        //
    };
}
