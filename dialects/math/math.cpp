#include "dialects/math/math.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

#include "dialects/math/passes/lower_math.h"

using namespace thorin;

extern "C" THORIN_EXPORT DialectInfo thorin_get_dialect_info() {
    return {"math", [](Passes& passes) { register_pass<math::lower_math_pass, math::LowerMath>(passes); },
            // [](PipelineBuilder& builder) {
            //         builder.extend_opt_phase(188, [](PassMan& man) {
            //             man.add<math::LowerMath>();
            //         });
            //     },
            [](Backends&) {}, [](Normalizers& normalizers) { math::register_normalizers(normalizers); }};
}
