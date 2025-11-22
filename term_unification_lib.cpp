// include header for term interfaces
#include "term_unification.h"

// include standard exception header
#include <stdexcept>

// begin unify function definition
std::optional<Unifier::Substitution> Unifier::unify(const Term<std::string>& t1,
                                                    const Term<std::string>& t2) {
    // create a working substitution map
    Substitution working;
    // attempt to unify internally and check for failure
    if (!unifyInternal(t1, t2, working)) {
        // return no value on failure
        return std::nullopt;
    }
    // return successful substitution
    return working;
}

// start substitute helper
std::unique_ptr<Term<std::string>> Unifier::substitute(const Term<std::string>& term,
                                                       const Substitution& sub) const {
    // delegate to cloneWithSubstitution to apply bindings
    return cloneWithSubstitution(term, sub);
}

// begins occurs check implementation
bool Unifier::occurs(const std::string& varName,
                     const Term<std::string>& term,
                     const Substitution& sub) const {
    // handle variable detection
    if (term.isVariable()) {
        // cast to variable type
        const auto* v = dynamic_cast<const Variable*>(&term);
        // check direct name match
        if (v->name() == varName) {
            // occurs directly
            return true;
        }
        // search for existing binding
        auto it = sub.find(v->name());
        // if binding exists follow it recursively
        if (it != sub.end()) {
            // recursive occurs check on binding
            return occurs(varName, *it->second, sub);
        }
        // unbound variable does not contain target
        return false;
    }

    // constants cannot contain variables
    if (term.isConstant()) {
        // no occurrence inside constant
        return false;
    }

    // treat as compound term when neither variable nor constant
    const auto* comp = dynamic_cast<const Compound<std::string>*>(&term);
    // iterate through arguments for recursive search
    for (std::size_t i = 0; i < comp->arity(); ++i) {
        // if any argument contains the variable return true
        if (occurs(varName, comp->arg(i), sub)) {
            // occurrence found in argument
            return true;
        }
    }
    // no occurrence detected
    return false;
}

// clone while applying substitution
std::unique_ptr<Term<std::string>> Unifier::cloneWithSubstitution(
    const Term<std::string>& term, const Substitution& sub) const {
    // handle variable cloning
    if (term.isVariable()) {
        // cast to variable pointer
        const auto* v = dynamic_cast<const Variable*>(&term);
        // check existing substitution binding
        auto it = sub.find(v->name());
        // follow binding if present
        if (it != sub.end()) {
            // recursively clone bound value
            return cloneWithSubstitution(*it->second, sub);
        }
        // otherwise return new variable clone
        return std::make_unique<Variable>(v->name());
    }

    // handle constant cloning
    if (term.isConstant()) {
        // cast to constant pointer
        const auto* c = dynamic_cast<const Constant*>(&term);
        // return fresh constant copy
        return std::make_unique<Constant>(c->value());
    }

    // handle compound cloning
    const auto* comp = dynamic_cast<const Compound<std::string>*>(&term);
    // prepare argument container
    std::vector<std::unique_ptr<Term<std::string>>> clonedArgs;
    // reserve for efficiency
    clonedArgs.reserve(comp->arity());
    // iterate through each argument
    for (std::size_t i = 0; i < comp->arity(); ++i) {
        // clone argument with substitutions applied
        clonedArgs.push_back(cloneWithSubstitution(comp->arg(i), sub));
    }
    // return new compound with cloned arguments
    return std::make_unique<Compound<std::string>>(comp->functor(), std::move(clonedArgs));
}

// internal unification driver
bool Unifier::unifyInternal(const Term<std::string>& a,
                            const Term<std::string>& b,
                            Substitution& working) {
    // apply current substitutions to both inputs
    auto aReduced = cloneWithSubstitution(a, working);
    // apply to second term
    auto bReduced = cloneWithSubstitution(b, working);

    // references to reduced terms
    const Term<std::string>& lhs = *aReduced;
    // right-hand reduced reference
    const Term<std::string>& rhs = *bReduced;

    // handle variable-variable cases
    if (lhs.isVariable() && rhs.isVariable()) {
        // cast left variable
        const auto* lv = dynamic_cast<const Variable*>(&lhs);
        // cast right variable
        const auto* rv = dynamic_cast<const Variable*>(&rhs);
        // identical variables unify trivially
        if (lv->name() == rv->name()) {
            // success for same variable
            return true;
        }
        // choose deterministic binding order
        const auto* first = (lv->name() < rv->name()) ? lv : rv;
        // identify second variable
        const auto* second = (first == lv) ? rv : lv;
        // bind first variable to second
        working[first->name()] = std::make_unique<Variable>(second->name());
        // success after binding
        return true;
    }

    // handle left variable
    if (lhs.isVariable()) {
        // cast left variable pointer
        const auto* lv = dynamic_cast<const Variable*>(&lhs);
        // occurs check prevents cycles
        if (occurs(lv->name(), rhs, working)) {
            // fail if occurs
            return false;
        }
        // bind variable to substituted rhs
        working[lv->name()] = cloneWithSubstitution(rhs, working);
        // success after binding
        return true;
    }

    // handle right variable
    if (rhs.isVariable()) {
        // cast right variable pointer
        const auto* rv = dynamic_cast<const Variable*>(&rhs);
        // occurs check for right variable
        if (occurs(rv->name(), lhs, working)) {
            // fail when occurs
            return false;
        }
        // bind right variable to substituted lhs
        working[rv->name()] = cloneWithSubstitution(lhs, working);
        // success after binding
        return true;
    }

    // handle constant comparison
    if (lhs.isConstant() && rhs.isConstant()) {
        // cast left constant
        const auto* lc = dynamic_cast<const Constant*>(&lhs);
        // cast right constant
        const auto* rc = dynamic_cast<const Constant*>(&rhs);
        // compare constant values
        return lc->value() == rc->value();
    }

    // handle compound comparison
    if (lhs.isCompound() && rhs.isCompound()) {
        // cast left compound
        const auto* lc = dynamic_cast<const Compound<std::string>*>(&lhs);
        // cast right compound
        const auto* rc = dynamic_cast<const Compound<std::string>*>(&rhs);
        // ensure functor and arity match
        if (lc->functor() != rc->functor() || lc->arity() != rc->arity()) {
            // mismatch results in failure
            return false;
        }
        // iterate over arguments to unify
        for (std::size_t i = 0; i < lc->arity(); ++i) {
            // recursive unification for each pair
            if (!unifyInternal(lc->arg(i), rc->arg(i), working)) {
                // propagate failure
                return false;
            }
        }
        // success when all arguments unify
        return true;
    }

    // fallback failure for incompatible types
    return false;
}
