#pragma once

#include <thorin/analyses/scope.h>
#include <thorin/axiom.h>
#include <thorin/def.h>
#include <thorin/lam.h>

#include "dialects/affine/affine.h"
#include "dialects/autodiff/auxiliary/autodiff_flow_analysis.h"
#include "dialects/autodiff/auxiliary/autodiff_war_analysis.h"
#include "dialects/math/math.h"
#include "dialects/mem/mem.h"

namespace thorin::autodiff {

class CacheAnalysis {
public:
    DefSet requirements;
    DefSet requirements_filtered;
    DefSet targets_;

    WARAnalysis war_analysis;
    FlowAnalysis flow_analysis;
    explicit CacheAnalysis(Lam* lam)
        : flow_analysis(lam)
        , war_analysis(lam) {
        run();
    }

    FlowAnalysis& flow() { return flow_analysis; }

    WARAnalysis& war() { return war_analysis; }

    DefSet& targets() { return targets_; }

    bool requires_caching(const Def* def) { return targets_.contains(def); }

    void require(const Def* def) { requirements.insert(def); }

    void visit(const Def* def) {
        if (auto rop = match<math::arith>(def)) {
            if (rop.id() == math::arith::mul) {
                require(rop->arg(0));
                require(rop->arg(1));
            } else if (rop.id() == math::arith::div) {
                require(rop->arg(0));
                require(rop->arg(1));
            }
        } else if (auto extrema = match<math::extrema>(def)) {
            if (extrema.id() == math::extrema::maximum || extrema.id() == math::extrema::minimum) {
                require(extrema->arg(0));
                require(extrema->arg(1));
            }
        }

        if (auto exp = match<math::exp>(def)) {
            if (exp.id() == math::exp::exp) {
                require(exp);
            } else if (exp.id() == math::exp::log) {
                require(exp->arg());
            }
        }

        if (auto lea = match<mem::lea>(def)) {
            auto index = lea->arg(1);
            if (contains_load(index)) { require(index); }
        }

        if (auto gamma = match<math::gamma>(def)) { require(gamma->arg(0)); }
    }

    void filter(const Def* def) {
        if (auto arith = match<math::arith>(def)) {
            auto [left, right] = arith->args<2>();

            filter(left);
            filter(right);
        } else {
            requirements_filtered.insert(def);
        }
    }

    void filter() {
        for (auto requirement : requirements) { filter(requirement); }
    }

    void run() {
        for (auto flow_def : flow_analysis.flow_defs()) { visit(flow_def); }

        filter();

        for (auto requirement : requirements_filtered) {
            if (auto load = is_load_val(requirement)) {
                load->dump(1);
                auto ptr = load->arg(1);
                if (war_analysis.is_overwritten(load)) { 
                    targets_.insert(requirement); 
                }
            } else if (!requirement->isa<Lit>() && !isa_nested_var(requirement)) {
                targets_.insert(requirement);
            }
        }
    }

    bool isa_nested_var(const Def* def) {
        if (auto extract = def->isa<Extract>()) {
            return isa_nested_var(extract->tuple());
        } else if (def->isa<Var>()) {
            return true;
        }

        return false;
    }

    const App* is_load_val(const Def* def) {
        if (auto extr = def->isa<Extract>()) {
            auto tuple = extr->tuple();
            if (auto load = match<mem::load>(tuple)) { return load; }
        }

        return nullptr;
    }

    bool contains_load(const Def* def) {
        if (def->isa<Var>()) return false;
        if (match<mem::load>(def)) return true;

        for (auto op : def->ops()) {
            if (contains_load(op)) { return true; }
        }

        return false;
    }
};

} // namespace thorin::autodiff
