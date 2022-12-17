#include "dialects/math/passes/lower_math.h"

#include <iostream>
#include <thorin/dump.cpp>

#include <thorin/analyses/deptree.h>
#include <thorin/lam.h>

#include "dialects/autodiff/autodiff.h"
#include "dialects/autodiff/utils/helper.h"

namespace thorin::math {


const Def* LowerMath::rewrite(const Def* def) {
    if (auto exp = match<math::exp>(def)) {
        if( exp.id() == math::exp::sigmoid ){
            auto x = exp->arg();

            auto &w = world();
            auto s = math::isa_f(x->type());
            assert(s);
            auto zero = math::lit_f(w, *s, 0.0);
            auto one = math::lit_f(w, *s, 1.0);


            auto neg_x = math::op(math::arith::sub, math::Mode::fast, zero, x);
            auto exp_ned_x = math::op(math::exp::exp, math::Mode::fast, neg_x);
            auto divisor = math::op(math::arith::add, math::Mode::fast, one, exp_ned_x);
            auto result = math::op(math::arith::div, math::Mode::fast, one, divisor);

            return result;
        }
    }

    return def;
}

} // namespace thorin::autodiff
