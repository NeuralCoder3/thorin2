#pragma once

#include <vector>

#include <thorin/def.h>
#include <thorin/pass/pass.h>

#include "thorin/analyses/schedule.h"
#include "thorin/analyses/scope.h"
#include "thorin/phase/phase.h"

#include "dialects/direct/passes/subst.h"

namespace thorin::direct {

// TODO: resolve Problem
// Problem: We need to schedule the function calls dependent on the other functions
// Conceptually, the functions depend on the result (res = cps2ds fun args).
// However, these defs are not bound to a fixed position.
// In reality, the function invocations depend mutually on each other.
// The function depends on the replaced result (argument of call continuation).
// The problem is that the continuation only exist after the function call is scheduled.
// We either need to create dummy continuations first, memorize the calls, replace the cps2ds calls,
// and then schedule/invoke the calls.
// Or we need a more intelligent scheduling that finds a correct order of the functions.

// Attempt:
// * Collect calls, create continuations
// * Substitute cps2ds calls with continuation arguments
// * schedule cps calls in place of continuations

/// This is the second part of ds2cps.
/// We replace all ds call sites of cps (or ds converted) functions with the cps calls.
/// `b = f args` becomes `f (args,cont)` with a newly introduced continuation `cont : cn b`.
///
/// Scheme:
/// * Enter a function λ.
/// * Rewrite the body.
/// * Encounter a ds call C.
/// * Find the place where to put the call.
/// * Generate a cps call CC with a continuation λC.
/// * Enter λC (as the current lambda).
/// * Place the body of λ into λC with C replaced by the argument of λC.
/// * Repeat for every ds call and function.

struct DSCallSite {
    const Def* call;
    Lam* continuation;
    const Def* cps_call;
    // const Def* nom;
    // const Def* root;
    const Scope& scope;
    Def* root;
};

class CPS2DSCollector : public ScopePhase {
public:
    CPS2DSCollector(World& world)
        : ScopePhase(world, "cps2ds_collect", true) {
        dirty_ = true;
    }

    void visit(const Scope&) override;
    // void enter() override;

    /// Associates old ds calls with the arguments of the cps continuation.
    // Def2Def call_to_arg;
    std::vector<DSCallSite> call_sites;
    DefSet visited_sites;

private:
    // const Scope* scope_;
    Scheduler sched_;

    /// Memoization for arbitrary defs (rewritten calls).
    // Def2Def rewritten_;

    /// Keeps track of the current lambda (place to put new body in).
    /// At the start of rewriting a function, this is the lambda of the function.
    /// But a rewritten ds call changes it to be the continuation of the call.
    /// Afterward, we place the old body into it where the original call is replaced with the continuation argument.
    // std::vector<Lam*> lam_stack;
    // Lam* curr_lam_ = nullptr;

    // Def2Def rewritten_lams;
    DefSet visited;

    void visit_def(const Def*);
    void visit_def_(const Def*);

    // void rewrite_lam(Lam* lam);
    // const Def* rewrite_body(const Def*);
    // const Def* rewrite_body_(const Def*);
};

// TODO: handle state better
const Def* subst_def_using_map(Def2Def& subst, const Def* def);
void place_calls(std::vector<DSCallSite>);

class CPS2DSWrapper : public RWPass<CPS2DSWrapper, Lam> {
public:
    CPS2DSWrapper(PassMan& man)
        : RWPass(man, "cps2ds") {}

    void prepare() override {
        auto collector = CPS2DSCollector(world());
        collector.run();
        world().DLOG("Print After Collection Functions, Placing them at Scope");
        world().debug_dump();

        Def2Def call_to_arg;
        for (auto& site : collector.call_sites) {
            auto cont              = site.continuation;
            auto arg               = cont->var();
            call_to_arg[site.call] = arg;
        }
        world().DLOG("substituting call results in program");
        // Alternative for problem below: inline change/ correspondence map
        // TODO: wrong references in cps_calls due to subst
        // SubstPhase(world(), call_to_arg).run();

        // inline_subst
        // subst_def_using_map(call_to_arg, const Def *def)
        for (auto ext : world().externals()) {
            world().DLOG("rewrite ext {} : {}", ext.second, ext.second->type());
            subst_def_using_map(call_to_arg, ext.second);
        }

        // only possible using recursive dumping
        // world().debug_dump();
        world().DLOG("substituting call results in calls");
        for (auto& site : collector.call_sites) {
            // substitute in call other (cps) calls away => correct arguments

            Def2Def subst_map;
            for (auto& site2 : collector.call_sites) {
                if (site2.call == site.call) continue;
                subst_map[site2.call] = site2.continuation->var();
            }

            // substitute without residing in a scope
            auto cps_call = subst_def_using_map(subst_map, site.cps_call);
            site.cps_call = cps_call;

            // Scope r_scope{ty->as_nom()}; // scope that surrounds ret_ty
            // inst_ret_ty = thorin::rewrite(ret_ty, ty_dom, args, r_scope);
        }
        world().DLOG("now placing calls");
        place_calls(collector.call_sites);
        // TODO: why is this necessary (rematerilization of cps2ds in continuation)
        // => entry refers to old place => find current entry
        // SubstPhase(world(), call_to_arg).run();
        world().debug_dump();
        assert(false);
    }
};

} // namespace thorin::direct
