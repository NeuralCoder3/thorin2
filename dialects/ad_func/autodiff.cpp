#include "dialects/ad_func/autodiff.h"

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

#include "dialects/ad_func/passes/autodiff_eval.h"
#include "dialects/ad_func/passes/autodiff_zero.h"
#include "dialects/ad_func/passes/autodiff_zero_cleanup.h"
#include "dialects/affine/passes/lower_for.h"
#include "dialects/compile/passes/internal_cleanup.h"
#include "dialects/direct/passes/ds2cps.h"
#include "dialects/mem/passes/rw/reshape.h"

using namespace thorin;
using namespace thorin::ad_func;

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"ad_func",
            [](Passes& passes) {
                register_pass<ad_eval_pass, AutoDiffEval>(passes);
                register_pass<ad_zero_pass, AutoDiffZero>(passes);
                register_pass<ad_zero_cleanup_pass, AutoDiffZeroCleanup>(passes);
                // register_pass<ad_func::ad_ext_cleanup_pass, ad_func::AutoDiffExternalCleanup>(passes);
                register_pass<ad_ext_cleanup_pass, compile::InternalCleanup>(passes, "internal_diff_");
            },
            nullptr, [](Normalizers& normalizers) { register_normalizers(normalizers); }};
}
