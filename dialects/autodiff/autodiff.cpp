#include "dialects/autodiff/autodiff.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

#include "thorin/pass/fp/beta_red.h"
#include "thorin/pass/fp/eta_exp.h"
#include "thorin/pass/fp/eta_red.h"
#include "thorin/pass/fp/tail_rec_elim.h"
#include "thorin/pass/pipelinebuilder.h"
#include "thorin/pass/rw/lam_spec.h"
#include "thorin/pass/rw/partial_eval.h"
#include "thorin/pass/rw/ret_wrap.h"
#include "thorin/pass/rw/scalarize.h"

#include "dialects/affine/passes/lower_for.h"
#include "dialects/autodiff/passes/autodiff_eval.h"
#include "dialects/autodiff/passes/autodiff_ext_cleanup.h"
#include "dialects/autodiff/passes/autodiff_zero.h"
#include "dialects/autodiff/passes/autodiff_zero_cleanup.h"
#include "dialects/direct/passes/ds2cps.h"
#include "dialects/mem/passes/rw/reshape.h"

using namespace thorin;

/// optimization idea:
/// * optimize [100]
/// * perform ad [105]
/// * resolve unsolved zeros (not added) [111]
/// * optimize further, cleanup direct style [115-120] (in direct)
/// * cleanup (zeros, externals) [126]
extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"autodiff",
            [](thorin::PipelineBuilder& builder) {
                builder.add_opt(110);

                // builder.extend_opt_phase(104, [](PassMan& man) { man.add<mem::Reshape>(mem::Reshape::Arg); });
                builder.extend_opt_phase(105, [](thorin::PassMan& man) { man.add<thorin::autodiff::AutoDiffEval>(); });
                builder.extend_opt_phase(106, [](thorin::PassMan& man) { man.add<thorin::affine::LowerFor>(); });
                builder.extend_opt_phase(107, [](thorin::PassMan& man) {
                    // in theory only after partial eval (beta, ...)
                    // but before other simplification
                    // zero and add need to be close together
                    man.add<thorin::autodiff::AutoDiffZero>();
                });
                builder.add_opt(125);
                builder.extend_opt_phase(126, [](PassMan& man) {
                    man.add<thorin::autodiff::AutoDiffZeroCleanup>();
                    man.add<thorin::autodiff::AutoDiffExternalCleanup>();
                });

                builder.extend_opt_phase(127, [](thorin::PassMan& man) { man.add<Scalerize>(); });
                builder.extend_opt_phase(128, [](thorin::PassMan& man) { man.add<EtaRed>(); });
                builder.extend_opt_phase(129, [](thorin::PassMan& man) { man.add<TailRecElim>(); });
                builder.add_opt(130);
            },
            [](Passes& passes) {
                register_pass<autodiff::ad_eval_pass, autodiff::AutoDiffEval>(passes);
                register_pass<autodiff::ad_zero_pass, autodiff::AutoDiffZero>(passes);
                register_pass<autodiff::ad_zero_cleanup_pass, autodiff::AutoDiffZeroCleanup>(passes);
                register_pass<autodiff::ad_ext_cleanup_pass, autodiff::AutoDiffExternalCleanup>(passes);
            },
            nullptr, [](Normalizers& normalizers) { autodiff::register_normalizers(normalizers); }};
}
