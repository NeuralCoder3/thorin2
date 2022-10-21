#include "dialects/ad_ext/ad_ext.h"

#include <thorin/config.h>
#include <thorin/dialects.h>
#include <thorin/pass/pass.h>

#include "dialects/ad_ext/passes/autodiff_ext_cleanup.h"

using namespace thorin;

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"ad_ext",
            [](thorin::PipelineBuilder& builder) {
                builder.extend_opt_phase(299, [](PassMan& man) { man.add<thorin::ad_ext::AutoDiffExternalCleanup>(); });
            },
            nullptr, nullptr};
}
