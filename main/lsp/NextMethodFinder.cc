#include "NextMethodFinder.h"
#include "core/GlobalState.h"

using namespace std;

namespace sorbet::realmain::lsp {

unique_ptr<ast::MethodDef> NextMethodFinder::preTransformMethodDef(core::Context ctx,
                                                                   unique_ptr<ast::MethodDef> methodDef) {
    ENFORCE(methodDef->symbol.exists());
    ENFORCE(methodDef->symbol != core::Symbols::todo());

    auto currentMethod = methodDef->symbol;

    auto &currentMethodLocs = currentMethod.data(ctx)->locs();
    auto inFileOfQuery = [&](const auto &loc) { return loc.file() == this->queryLoc.file(); };
    auto maybeCurrentLoc = absl::c_find_if(currentMethodLocs, inFileOfQuery);
    if (maybeCurrentLoc == currentMethodLocs.end()) {
        return methodDef;
    }

    auto currentLoc = *maybeCurrentLoc;
    if (!currentLoc.exists()) {
        // Defensive in case location information is disabled (e.g., certain fuzzer modes)
        return methodDef;
    }

    ENFORCE(currentLoc.file() == this->queryLoc.file());

    if (currentLoc.beginPos() < queryLoc.beginPos()) {
        // Current method is before query, not after.
        return methodDef;
    }

    // Current method starts at or after query loc. Starting 'at' is fine, because it can happen in cases like this:
    //   |def foo; end

    if (this->result_.exists()) {
        // Method defs are not guaranteed to be sorted in order by their declLocs
        auto &resultLocs = this->result_.data(ctx)->locs();
        auto maybeResultLoc = absl::c_find_if(resultLocs, inFileOfQuery);
        ENFORCE(maybeResultLoc != resultLocs.end(), "Must exist, because otherwise it wouldn't have been the result_");
        auto resultLoc = *maybeResultLoc;
        if (currentLoc.beginPos() < resultLoc.beginPos()) {
            // Found a method defined after the query but earlier than previous result: overwrite previous result
            this->result_ = currentMethod;
            return methodDef;
        } else {
            // We've already found an earlier result, so the current is not the first
            return methodDef;
        }
    } else {
        // Haven't found a result yet, so this one is the best so far.
        this->result_ = currentMethod;
        return methodDef;
    }
}

const core::SymbolRef NextMethodFinder::result() const {
    return this->result_;
}

}; // namespace sorbet::realmain::lsp
