#include "dialects/autodiff/auxiliary/autodiff_cache_optimizer.h"

#include "thorin/def.h"
#include "thorin/tuple.h"

#include "dialects/autodiff/autodiff.h"
#include "dialects/autodiff/auxiliary/analysis.h"
#include "dialects/autodiff/auxiliary/analysis_factory.h"
#include "dialects/autodiff/builder.h"
#include "dialects/math/math.h"
#include "dialects/mem/autogen.h"
#include "dialects/mem/mem.h"

namespace thorin::autodiff {

CacheOptimizer::CacheOptimizer(AnalysisFactory& factory)
    : Analysis(factory)
    , war(factory.war())
    , dfa(factory.dfa())
    , utils(factory.utils())
    , alias(factory.alias())
    , best(nullptr) {}

void CacheOptimizer::explore(std::shared_ptr<CacheState>& last, const Def* removed, const Def* parts) {
    auto next_state = std::make_shared<CacheState>(*last);

    next_state->cached_defs.erase(removed);
    if (next_state->computed_defs.insert(removed).second) {
        for (auto def : parts->projs()) {
            if (!next_state->computed_defs.contains(def)) {
                if (needs_cache(def) && !depends_on_cache(def, next_state->cached_defs)) {
                    auto result = next_state->cached_defs.insert(def);

                    if (result.second && !requires_cache(def)) { next_state->queue.push(def); }
                }
            }
        }
    }

    if (explored.insert(next_state).second) {
        if (!best->is_better_than(next_state)) {
            std::cout << next_state->cached_defs.size() << std::endl;
            best = next_state;
        }

        search(next_state);
    }
}

void CacheOptimizer::find(const Def* def) {
    DefSet visited;
    auto node = dfa.node(def);
    find(node, visited);
}

bool CacheOptimizer::is_flow_op(const Def* def) {
    return match<math::arith>(def) || match<core::wrap>(def) || match<math::conv>(def) || match<core::conv>(def) ||
           match<math::exp>(def) || match<math::gamma>(def) || def->isa<Tuple>() || def->isa<Pack>() ||
           def->isa<Extract>() || match<mem::load>(def);
}

void CacheOptimizer::canonicalize(DefSet defs) {
    while (defs.size() > 0) {
        const Def* def = *defs.begin();
        DefSet visited;
        auto node = dfa.node(def);
        find(node, visited);

        DefSet& canonical_group = groups.emplace_back();
        for (auto visited_def : visited) {
            if (defs.contains(visited_def)) {
                defs.erase(visited_def);
                canonical_group.insert(visited_def);
            }
        }
    }
}

void CacheOptimizer::find(AffineDFNode* node, DefSet& visited) {
    auto def = alias.get(node->def());

    if (!is_flow_op(def)) return;
    if (!visited.insert(def).second) return;
    if (!is_load_val(def)) {
        for (auto succ : node->preds()) { find(succ, visited); }
    } else if (!war.is_overwritten(def)) {
        return;
    }

    for (auto prev : node->succs()) { find(prev, visited); }
}

void CacheOptimizer::search(std::shared_ptr<CacheState> current) {
    auto& queue = current->queue;
    for (; queue.size() > 0;) {
        auto def = queue.front();
        queue.pop();
        if (auto app = match<math::arith>(def)) {
            explore(current, def, app->arg());
        } else if (auto app = match<core::wrap>(def)) {
            explore(current, def, app->arg());
        } else if (auto app = match<math::conv>(def)) {
            explore(current, def, app->arg());
        } else if (auto app = match<core::conv>(def)) {
            explore(current, def, app->arg());
        } else if (auto app = match<math::exp>(def)) {
            explore(current, def, app->arg());
        } else if (auto app = match<math::gamma>(def)) {
            explore(current, def, app->arg());
        }
    }
}

bool CacheOptimizer::requires_cache(const Def* def) {
    if (is_load_val(def) && war.is_overwritten(def)) { return true; }

    return false;
}

bool CacheOptimizer::needs_cache(const Def* def) {
    if (factory().utils().is_loop_index(def)) return false;
    if (def->isa<Lit>()) return false;
    if (utils.is_root_var(def)) return false;
    if (is_load_val(def) && !war.is_overwritten(def)) return false;
    return true;
}

bool CacheOptimizer::depends_on_cache(const Def* def, DefSet& set, bool init) {
    def = alias.get(def);

    if (!needs_cache(def)) return true;
    if (!init && set.contains(def)) return true;
    if (auto app = def->isa<App>()) {
        return depends_on_cache(app->arg(), set, false);
    } else if (auto tuple = def->isa<Tuple>()) {
        for (auto op : tuple->ops()) {
            if (!depends_on_cache(op, set, false)) { return false; }
        }

        return true;
    }

    return false;
}
/*
void CacheOptimizer::depends_on_loads(const Def* def, DefSet& set, DefSet& loads) {
    def = alias.get(def);

    if (set.contains(def)){
        return;
    }else if(is_load_val(def)){
        loads.insert(def);
    }else if (auto app = def->isa<App>()) {
        depends_on_loads(app->arg(), set, loads);
    } else if (auto tuple = def->isa<Tuple>()) {
        for (auto op : tuple->ops()) {
            depends_on_loads(op, set, loads);
        }
    }
}*/

DefSet CacheOptimizer::optimize(DefSet defs) {
    DefSet required_caches;
    for (auto def : defs) {
        auto target = alias.get(def);
        if (needs_cache(target)) {
            required_caches.insert(target);
        } else {
            def->dump(1);
            def->dump(1);
        }
    }

    DefSet no_deps;
    for (auto def : required_caches) {
        if (depends_on_cache(def, required_caches)) { continue; }

        /*if (auto app = def->isa<App>()) {
            auto arg = app->arg();
            if (arg->num_projs() == 1 && depends_on_cache(arg, required_caches)){
                continue;
            }
        }*/

        no_deps.insert(def);
    }

    /*
        DefSet loads;
        for( auto def : defs ){
            depends_on_loads(def, no_deps, loads);
        }
    */
    return no_deps;

    /*canonicalize(filtered2);

    auto size = groups.size();
    size_t test = 0;

    for (DefSet& group : groups) {
        auto size2 = group.size();
        for( auto def : group ){
            def->dump(1);
            def->dump(1);
        }
    }

    DefSet result;
    for (DefSet& group : groups) {
        best = std::make_shared<CacheState>();
        for (auto def : group) {
            if (needs_cache(def)) {
                best->cached_defs.insert(def);

                if (!requires_cache(def)) { best->queue.push(def); }
            }
        }
        search(best);

        result.insert(best->cached_defs.begin(), best->cached_defs.end());
    }

    return result;*/
}

} // namespace thorin::autodiff
