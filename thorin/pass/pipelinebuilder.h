#pragma once

#include <functional>
#include <vector>

#include "thorin/pass/pass.h"

namespace thorin {

const int DEFAULT_PRIORITY = 100;
typedef std::function<void(PassMan&)> PassBuilder;
typedef std::pair<int, PassBuilder> PrioPassBuilder;
typedef std::vector<PrioPassBuilder> PassList;

struct passCmp {
    constexpr bool operator()(PrioPassBuilder const& a, PrioPassBuilder const& b) const noexcept {
        return a.first < b.first;
    }
};

class PipelineBuilder {
public:
    explicit PipelineBuilder() {}

    void extend_opt_phase(int i, std::function<void(PassMan&)>, int priority = DEFAULT_PRIORITY);
    void extend_opt_phase(std::function<void(PassMan&)>);
    void add_opt(int i);
    void extend_codegen_prep_phase(std::function<void(PassMan&)>);
    // void extend_opt_prep_phase1(std::function<void(PassMan&)>);
    // void extend_opt_prep_phase2(std::function<void(PassMan&)>);

    std::unique_ptr<PassMan> opt_phase(int i, World& world);
    // std::unique_ptr<PassMan> opt_phase2(World& world);
    // std::unique_ptr<PassMan> codegen_prep_phase(World& world);
    // std::unique_ptr<PassMan> opt_prep_phase1(World& world);
    // std::unique_ptr<PassMan> opt_prep_phase2(World& world);
    void add_opt(PassMan man);

    std::vector<int> passes();

private:
    std::map<int, PassList> phase_extensions_;
    // std::vector<std::function<void(PassM#an&)>> codegen_prep_phase_extensions_;
    // std::vector<std::function<void(PassMan&)>> opt_prep_phase1_extensions_;
    // std::vector<std::function<void(PassMan&)>> opt_prep_phase2_extensions_;
};

} // namespace thorin
