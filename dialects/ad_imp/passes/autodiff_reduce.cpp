#include "dialects/ad_imp/passes/autodiff_reduce.h"
#include "thorin/rewrite.h"

#include "thorin/analyses/schedule.h"

#include "dialects/ad_imp/autodiff.h"
#include "dialects/affine/affine.h"
#include "dialects/math/math.h"

namespace thorin::ad_imp {

static Lam* duplicate(Lam* lam) {
    auto& w     = lam->world();
    Lam* diffee = w.nom_lam(lam->type()->as<Pi>(), lam->dbg());
    diffee->set(lam->reduce(diffee->var()));
    return diffee;
}

Lam* AutodiffReduce::reduce(Lam* lam) {
    lam         = duplicate(lam);
    auto result = reduce(lam, lam->ret_var());
    return result->as_nom<Lam>();
}

static const Def* build_wrapper(const Def* ret) {
    auto& w      = ret->world();
    auto wrapper = w.nom_lam(ret->type()->as<Pi>(), ret->dbg());
    wrapper->app(false, ret, wrapper->var(), ret->dbg());
    return wrapper;
}

const Def* AutodiffReduce::wrap(const Def* def) {
    auto return_wrapper = return_wrappers[def];
    if (!return_wrapper) {
        return_wrapper       = build_wrapper(def);
        return_wrappers[def] = return_wrapper;
    }

    return return_wrapper;
}

const Def* AutodiffReduce::reduce(const Def* def, const Def* ret) {
    if (def == ret) { return wrap(def); }

    auto lam = def->as_nom<Lam>();

    auto& w = world();
    while (true) {
        auto body = lam->body();
        if (auto app = body->isa<App>()) {
            auto arg = app->arg();
            if (match<affine::For>(app)) {
                arg = arg->refine(4, reduce(app->arg(4)->as_nom<Lam>()));
                arg = arg->refine(5, build_wrapper(reduce(app->arg(5), ret)));
                lam->set_body(w.app(app->callee(), arg));
                break;
            } else {
                auto callee = app->callee();
                if (callee == ret) {
                    lam->set_body(w.app(wrap(callee), arg));
                    break;
                }

                if (callee->is_set()) {
                    auto arg = app->arg();
                    if (auto extract = callee->isa<Extract>()) {
                        DefArray new_branches(extract->tuple()->ops(),
                                              [&](const Def* def) { return build_wrapper(reduce(def, ret)); });
                        auto new_callee = w.extract(w.tuple(new_branches), extract->index());
                        lam->set_body(w.app(new_callee, arg));
                        break;
                    } else {
                        auto callee_lam = callee->as_nom<Lam>();
                        world().DLOG("autodiff_reduce: callee is a set but not an extract: {}", callee);
                        // print lam with type
                        // arg with type
                        world().DLOG("autodiff_reduce: lam = {} : {}", lam, lam->type());
                        world().DLOG("autodiff_reduce: arg = {} : {}", arg, arg->type());
                        // callee with type
                        world().DLOG("autodiff_reduce: callee = {} : {}", callee, callee->type());
                        /*
autodiff_reduce: callee is a set but not an extract: for_6438959

autodiff_reduce: lam = break_9419970 : .Cn %mem.M

autodiff_reduce: arg = (4572:(.Idx 4294967296), %mem.remem _9420049#0:(.Idx 2), _9420089) : [.Idx 4294967296, %mem.M,
%math.F (52, 11)]

autodiff_reduce: callee = for_6438959 : .Cn [.Idx 4294967296, %mem.M, %math.F (52, 11)]
                        */
                        auto callee_body = callee_lam->body();
                        world().DLOG("autodiff_reduce: callee_lam to reduce {} : {}", callee_lam, callee_lam->type());
                        // world().DLOG("autodiff_reduce: arg to reduce {} : {}", arg, arg->type());
                        world().DLOG("autodiff_reduce: callee_body = {} : {}", callee_body, callee_body->type());


                        // manual reduction
                        Scope scope(callee_lam);
                        ScopeRewriter rewriter(callee_lam->world(), scope);
                        rewriter.map(callee_lam->var(), arg);
                        auto new_body = rewriter.rewrite(callee_body);


                        // auto new_body = callee_lam->reduce(arg);
                        // lam->set(callee->reduce(arg));
                        // lam->set_body(w.app(callee, arg));
                        // lam->set(new_body);
                        lam->set_filter(callee_lam->filter());
                        lam->set_body(new_body);
                    }
                } else {
                    auto last_index = app->num_args() - 1;
                    auto ret_cont   = app->arg(last_index);

                    ret_cont     = reduce(ret_cont, ret);
                    auto new_arg = app->arg()->refine(last_index, ret_cont);

                    lam->set_body(w.app(callee, new_arg));
                    break;
                }
            }
        }
    }

    return lam;
}

const Def* AutodiffReduce::rewrite(const Def* def) {
    if (auto ad_app = match<ad>(def); ad_app && !visited.contains(def)) {
        auto diffee  = ad_app->arg()->as_nom<Lam>();
        auto reduced = reduce(diffee);
        def          = op_autodiff(reduced);
        visited.insert(def);
    }

    return def;
}

} // namespace thorin::ad_imp
