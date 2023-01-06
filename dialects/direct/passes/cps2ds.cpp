#include "dialects/direct/passes/cps2ds.h"

#include <iostream>

#include <thorin/lam.h>

#include "dialects/direct/direct.h"

namespace thorin::direct {

void CPS2DSCollector::visit(const Scope& scope) {
    if (auto entry = scope.entry()->isa_nom<Lam>()) {
        scope.free_noms(); // cache this.
        sched_ = Scheduler{scope};
        visit_def(entry->body());
        // rewrite_lam(entry);
    }
}

// void CPS2DSCollector::rewrite_lam(Lam* lam) {
//     // TODO: only use if sure that no lambda needs to be rewritten twice (e.g. two calls are placed into the same
//     lambda).
//     // if (rewritten_lams.contains(lam)) return;
//     // rewritten_lams.insert(lam);
//     if (!lam->isa_nom()) {
//         lam->world().DLOG("skipped non-nom {}", lam);
//         return;
//     }
//     if (!lam->is_set()) {
//         lam->world().DLOG("skipped non-set {}", lam);
//         return;
//     }
//     if (lam->codom()->isa<Type>()) {
//         world().DLOG("skipped type {}", lam);
//         return;
//     }

//     lam->world().DLOG("Rewrite lam: {}", lam->name());

//     lam_stack.push_back(curr_lam_);
//     curr_lam_ = lam;

//     auto result = rewrite_body(curr_lam_->body());
//     // curr_lam_ might be different at this point (newly introduced continuation).
//     auto& w = curr_lam_->world();
//     w.DLOG("Result of rewrite {} in {}", lam, curr_lam_);
//     curr_lam_->set_body(result);

//     curr_lam_ = lam_stack.back();
//     lam_stack.pop_back();
// }

// const Def* CPS2DSCollector::rewrite_body(const Def* def) {
//     if (auto i = rewritten_.find(def); i != rewritten_.end()) return i->second;
//     // TODO: here or below check for rewritten ds calls
//     auto new_def    = rewrite_body_(def);
//     rewritten_[def] = new_def;
//     return rewritten_[def];
// }

// void CPS2DS::enter() {
//     Lam* lam = curr_nom();
//     rewrite_lam(lam);
// }

void CPS2DSCollector::visit_def(const Def* def) {
    if (visited.contains(def)) return;
    visited.insert(def);
    visit_def_(def);
}

void CPS2DSCollector::visit_def_(const Def* def) {
    auto& world = def->world();
    if (auto app = def->isa<App>()) {
        auto callee = app->callee();
        auto args   = app->arg();
        // world.DLOG("app callee {} : {}", callee, callee->type());
        // world.DLOG("app args {} : {}", args, args->type());
        visit_def(callee);
        visit_def(app->arg());
        // auto new_callee = rewrite_body(callee);
        // auto new_arg    = rewrite_body(app->arg());

        if (auto fun_app = callee->isa<App>()) {
            if (auto ty_app = fun_app->callee()->isa<App>(); ty_app) {
                if (auto axiom = ty_app->callee()->isa<Axiom>()) {
                    if (axiom->flags() == ((flags_t)Axiom::Base<cps2ds_dep>)) {
                        // We encountered a `cps2ds fun` call (a ds call).
                        // We want to generate a corresponding cps call to the function.
                        // This cps call redirects to a continuation that forwards the result.

                        // world.DLOG("rewrite callee {} : {}", callee, callee->type());
                        // world.DLOG("rewrite args {} : {}", args, args->type());
                        // world.DLOG("rewrite cps axiom {} : {}", ty_app, ty_app->type());

                        if (call_to_arg.contains(app)) {
                            auto result = call_to_arg[app];
                            world.DLOG("found already rewritten call {} : {}", app, app->type());
                            world.DLOG("result {} : {}", result, result->type());
                            return;
                        }

                        // TODO: rewrite function here?
                        auto cps_fun = fun_app->arg();
                        // cps_fun      = rewrite_body(cps_fun);
                        // if (!cps_fun->isa_nom<Lam>()) { world.DLOG("cps_fun {} is not a lambda", cps_fun); }
                        // rewrite_lam(cps_fun->as_nom<Lam>());
                        world.DLOG("function: {} : {}", cps_fun, cps_fun->type());

                        // ```
                        // h:
                        // b = f a
                        // C[b]
                        // ```
                        // =>
                        // ```
                        // h:
                        //     f'(a,h_cont)
                        //
                        // h_cont(b):
                        //     C[b]
                        //
                        // f : A -> B
                        // f': .Cn [A, ret: .Cn[B]]
                        // ```

                        // TODO: rewrite map vs thorin::rewrite
                        // TODO: unify replacements

                        // We instantiate the function type with the applied argument.
                        auto ty     = callee->type();
                        auto ret_ty = ty->as<Pi>()->codom();
                        // world.DLOG("callee {} : {}", callee, ty);
                        // world.DLOG("new arguments {} : {}", args, args->type());
                        // world.DLOG("ret_ty {}", ret_ty);

                        // TODO: use reduce (beta reduction)
                        const Def* inst_ret_ty;
                        if (auto ty_pi = ty->isa_nom<Pi>()) {
                            auto ty_dom = ty_pi->var();
                            // world.DLOG("replace ty_dom: {} : {} <{};{}>", ty_dom, ty_dom->type(),
                            // ty_dom->unique_name(),
                            //            ty_dom->node_name());

                            Scope r_scope{ty->as_nom()}; // scope that surrounds ret_ty
                            inst_ret_ty = thorin::rewrite(ret_ty, ty_dom, args, r_scope);
                            // world.DLOG("inst_ret_ty {}", inst_ret_ty);
                        } else {
                            inst_ret_ty = ret_ty;
                        }

                        auto new_name = cps_fun->name();
                        // append _cps_cont
                        // if name contains _cps_cont append _1
                        // if it contains _[n] append _[n+1]
                        std::string append = "_cps_cont";
                        auto pos           = new_name.find(append);
                        if (pos != std::string::npos) {
                            auto num = new_name.substr(pos + append.size());
                            if (num.empty()) {
                                new_name += "_1";
                            } else {
                                num      = num.substr(1);
                                num      = std::to_string(std::stoi(num) + 1);
                                new_name = new_name.substr(0, pos + append.size()) + "_" + num;
                            }
                        } else {
                            new_name += append;
                        }

                        // The continuation that receives the result of the cps function call.
                        auto fun_cont = world.nom_lam(world.cn(inst_ret_ty), world.dbg(new_name));
                        // rewritten_lams.insert(fun_cont);
                        // Generate the cps function call `f a` -> `f_cps(a,cont)`
                        // TODO: remove debug filter
                        if (cps_fun->isa_nom<Lam>()) cps_fun->as_nom<Lam>()->set_filter(false);
                        auto cps_call = world.app(cps_fun, {args, fun_cont}, world.dbg("cps_call"));

                        world.DLOG("continuation: {} : {}", fun_cont, fun_cont->type());

                        // `result` is the result of the cps function.
                        auto result = fun_cont->var();

                        // We have:
                        // `cps_call` as cps call for the ds call of `cps_fun`
                        // `fun_cont` as continuation that receives the result of the cps call

                        // We get the correct place for the cps call (high enough for all uses, low enough for all ops).
                        // Afterward, we place the call into the place and continue to rewrite the body of the place.
                        // In doing so, we throw away the old progress and start in the higher place.
                        // TODO: check
                        // This is no problem as we will arrive again at this point later on.

                        // Associate the ds call with the result of the cps call.
                        call_to_arg[app] = result;

                        // Get the place where the cps call should be placed.
                        // The place depends on the uses of the result of the ds call (`fun_app`).
                        auto place = sched_.smart(fun_app);
                        // auto place = sched_.early(app);

                        world.DLOG("  call place {} : {} [{}]", place, place->type(), place->node_name());

                        // TODO: is this correct
                        // drop curr_lam_ and switch to place
                        assert(place->isa_nom<Lam>() && "place is not a lambda");
                        // curr_lam_ = place->as_nom<Lam>();
                        auto place_lam = place->as_nom<Lam>();

                        // world.DLOG("  curr_lam {}", curr_lam_->name());
                        auto place_body = place_lam->body();
                        place_lam->set_body(cps_call);

                        // Fixme: would be great to PE the newly added overhead away..
                        // The current PE just does not terminate on loops.. :/
                        // TODO: Set filter (inline call wrapper)
                        // curr_lam_->set_filter(true);

                        // The filter can only be set here (not earlier) as otherwise a debug print causes the "some
                        // operands are set" issue.
                        fun_cont->set_filter(place_lam->filter());
                        fun_cont->set_body(place_body);

                        // TODO: how to abort rewriting inner lam and instead rewrite body of place?

                        // We write the body context in the newly created continuation that has access to the result (as
                        // its argument).
                        // curr_lam_ = fun_cont;

                        world.DLOG("  result {} : {} instead of {} : {}", result, result->type(), def, def->type());

                        return;
                    }
                }
            }
        }
        // No ds call => just recurse through the ápp (replace parts).

        return;
    }

    // No call => iterate through body rewrite on the way.

    // TODO: are ops rewrites + app calle/arg rewrites all possible combinations?
    // TODO: check if lam is necessary or if var is enough

    if (auto lam = def->isa_nom<Lam>()) {
        visit_def(lam->body());
        // We only change the body/introduce new lambda. But the old ones are still valid (with the same meaning).
        return;
    }

    for (auto op : def->ops()) { visit_def(op); }
    return;

    // // TODO: remove
    // // We need this case to not descend into infinite chains through function
    // // if (auto var = def->isa<Var>()) { return var; }

    // // We special-case tuples to recompute their type (and avoid invalid type overwrites).
    // // if (auto tuple = def->isa<Tuple>()) {
    // //     // DefArray elements(tuple->ops(), [&](const Def* op) { return rewrite_body(op); });
    // //     // return world.tuple(elements, tuple->dbg());
    // //     return;
    // // }

    // // TODO: remove
    // // if (auto old_nom = def->isa_nom()) { return old_nom; }

    // // Just replace all ops by rewritten ops.
    // // DefArray new_ops{def->ops(), [&](const Def* op) { return rewrite_body(op); }};

    // // There are issues with recursing/replacing debug and type.
    // // However, we only change types of application callees. (And we ignore type level calls for the most parts.)
    // // Therefore, we are safe to ignore these rewrites.

    // // auto new_dbg = rewrite_body(def->dbg());
    // // auto new_type = rewrite_body(def->type());
    // auto new_dbg = def->dbg();
    // auto new_type = def->type();

    // world.DLOG("def {} : {} [{}]", def, def->type(), def->node_name());

    // // TODO: where does this come from?
    // // example: ./build/bin/thorin -d matrix -d affine -d direct lit/matrix/read_transpose.thorin -o - -VVVV
    // if (def->isa<Infer>()) {
    //     world.WLOG("infer node {} : {} [{}]", def, new_type, def->node_name());
    //     return def;
    // }

    // return def->rebuild(world, new_type, new_ops, new_dbg);
}

} // namespace thorin::direct
