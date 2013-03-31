#ifndef ANYDSL2_ANALYSES_SCOPE_H
#define ANYDSL2_ANALYSES_SCOPE_H

#include <vector>

#include "anydsl2/lambda.h"
#include "anydsl2/util/array.h"
#include "anydsl2/util/autoptr.h"

namespace anydsl2 {

class DomTree;
class LoopForestNode;
class LoopInfo;

typedef std::vector<const Def*> FreeVariables;

class Scope {
public:

    explicit Scope(Lambda* entry);
    ~Scope();

    bool contains(Lambda* lambda) const { return lambda->scope() == this; }
    ArrayRef<Lambda*> rpo() const { return rpo_; }
    Lambda* rpo(size_t i) const { return rpo_[i]; }
    Lambda* operator [] (size_t i) const { return rpo(i); }
    ArrayRef<Lambda*> preds(Lambda* lambda) const;
    ArrayRef<Lambda*> succs(Lambda* lambda) const;
    size_t num_preds(Lambda* lambda) const { return preds(lambda).size(); }
    size_t num_succs(Lambda* lambda) const { return succs(lambda).size(); }
    Lambda* entry() const { return rpo_[0]; }
    size_t size() const { return rpo_.size(); }
    void reassign_sids();
    World& world() const { return entry()->world(); }
    FreeVariables free_variables() const;

    Lambda* clone(const GenericMap& generic_map = GenericMap());
    Lambda* drop(ArrayRef<const Def*> with);
    Lambda* drop(ArrayRef<size_t> to_drop, ArrayRef<const Def*> drop_with, 
                 const GenericMap& generic_map = GenericMap());
    Lambda* lift(ArrayRef<const Def*> to_lift, 
                 const GenericMap& generic_map = GenericMap());
    Lambda* mangle(ArrayRef<size_t> to_drop, ArrayRef<const Def*> drop_with, 
                   ArrayRef<const Def*> to_lift, 
                   const GenericMap& generic_map = GenericMap());

    const DomTree& domtree() const;
    const LoopForestNode* loopforest() const;
    const LoopInfo& loopinfo() const;
    void dump() const;

private:

    void jump_to_param_users(const size_t pass, Lambda* lambda);
    void up(const size_t pass, Lambda* lambda);
    void find_user(const size_t pass, const Def* def);
    size_t number(const size_t pass, Lambda* cur, size_t i);
    void insert(const size_t pass, Lambda* lambda) { 
        lambda->visit_first(pass); 
        lambda->scope_ = this; 
        rpo_.push_back(lambda); 
    }
    template<class T>
    void fill_succ_pred(const Lambdas& lsp, T& sp);

    std::vector<Lambda*> rpo_;
    Array< Array<Lambda*> > preds_;
    Array< Array<Lambda*> > succs_;
    mutable AutoPtr<DomTree> domtree_;
    mutable AutoPtr<LoopForestNode> loopforest_;
    mutable AutoPtr<LoopInfo> loopinfo_;

    friend class ScopeBuilder;
};

} // namespace anydsl2

#endif
