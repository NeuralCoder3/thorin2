#include "dialects/autodiff/autodiff.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

#include "thorin/pass/fp/beta_red.h"
#include "thorin/pass/fp/eta_exp.h"
#include "thorin/pass/fp/eta_red.h"
#include "thorin/pass/rw/partial_eval.h"

#include "dialects/autodiff/passes/autodiff_eval.h"
#include "dialects/autodiff/passes/autodiff_ext_cleanup.h"
#include "dialects/autodiff/passes/autodiff_zero.h"
#include "dialects/autodiff/passes/autodiff_zero_cleanup.h"
#include "dialects/direct/passes/ds2cps.h"
#include "dialects/mem/passes/fp/copy_prop.h"
#include "dialects/mem/passes/fp/ssa_constr.h"

using namespace thorin;

/// Eta Problem:
/// Functions are eta expanded and the eta expansion remains after optimization
/// => functions that theoretically stay alone but are not closed
/// Solutions:
/// * persistent maps => wrong derivative type => not possible
/// * eta reduction => does not seem to work (and maybe eta expansion + inlining is helpful)
/// * before optimizations: might be asymptotically worse
///   ^ we use this currently
/// better solution: handle non-closed full functions (D_A on functions)

/// Continuation Problem:
/// if a function is differentiated twice,
/// the return continuation (after the first differentiation)
/// looks like a closed function
/// current solution: Look at name
/// better solution: check closedness

// closedness test:
// Scope s{lam}; s.free_vars().empty()

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"autodiff",
            [](thorin::PipelineBuilder& builder) {
                // builder.extend_opt_phase(103, [](thorin::PassMan& man) {
                //     // man.add<PartialEval>();
                //     // man.add<BetaRed>();
                //     // man.add<EtaRed>();
                //     // man.add<CopyProp>();
                //     auto br = man.add<BetaRed>();
                //     auto er = man.add<EtaRed>();
                //     auto ee = man.add<EtaExp>(er);
                //     man.add<mem::SSAConstr>(ee);
                //     man.add<mem::CopyProp>(br, ee);
                // });
                // builder.add_opt(100);
                builder.extend_opt_phase(105, [](thorin::PassMan& man) {
                    // builder.extend_opt_phase(99, [](thorin::PassMan& man) {
                    // man.add<PartialEval>();
                    // man.add<BetaRed>();
                    // man.add<EtaRed>();

                    man.add<thorin::autodiff::AutoDiffEval>();
                });
                // builder.extend_opt_phase(99, [](thorin::PassMan& man) { man.add<thorin::autodiff::AutoDiffEval>();
                // });

                // builder.add_opt(110);
                builder.extend_opt_phase(111, [](thorin::PassMan& man) {
                    // in theory only after partial eval (beta, ...)
                    // but before other simplification
                    // zero and add need to be close together
                    man.add<thorin::autodiff::AutoDiffZero>();
                });

                builder.add_opt(120);
                builder.extend_opt_phase(299, [](PassMan& man) {
                    man.add<thorin::autodiff::AutoDiffZeroCleanup>();
                    man.add<thorin::autodiff::AutoDiffExternalCleanup>();
                });
            },
            nullptr, [](Normalizers& normalizers) { autodiff::register_normalizers(normalizers); }};
}
