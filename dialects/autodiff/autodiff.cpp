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
                // builder.extend_opt_phase(105, [](thorin::PassMan& man) {
                builder.extend_opt_phase(99, [](thorin::PassMan& man) {
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

                // builder.add_opt(120);
                builder.extend_opt_phase(299, [](PassMan& man) {
                    man.add<thorin::autodiff::AutoDiffZeroCleanup>();
                    man.add<thorin::autodiff::AutoDiffExternalCleanup>();
                });
            },
            nullptr, [](Normalizers& normalizers) { autodiff::register_normalizers(normalizers); }};
}
