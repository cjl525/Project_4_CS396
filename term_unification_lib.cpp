#include "term_unification.h"

#include <stdexcept>

// TODO: Implement full unification logic with occurs check.

std::optional<Unifier::Substitution> Unifier::unify(const Term<std::string>& t1,
                                                    const Term<std::string>& t2) {
    // create a working sub map
    Substitution working;
    // attempt to unify internally and check for failure
    if (!unifyInternal(t1, t2, working)) {
        // return no value on failure
        return std::nullopt;
    }
    // return successful sub
    return working;
}

// TODO: Apply substitutions recursively to produce a fully bound deep copy.

std::unique_ptr<Term<std::string>> Unifier::substitute(const Term<std::string>& term,
                                                       const Substitution& sub) const {
    // apply the bindings
    return cloneWithSubstitution(term, sub);
}

// TODO: Implement occurs check to prevent circular bindings.

bool Unifier::occurs(const std::string& varName,
                     const Term<std::string>& term,
                     const Substitution& sub) const {
    if (term.isVariable()) {
        const auto* v = dynamic_cast<const Variable*>(&term);
        if (v->name() == varName) {
            return true;
        }
        auto it = sub.find(v->name());
        if (it != sub.end()) {
            return occurs(varName, *it->second, sub);
        }
        return false;
    }

    if (term.isConstant()) {
        return false;
    }

    const auto* comp = dynamic_cast<const Compound<std::string>*>(&term);
    for (std::size_t i = 0; i < comp->arity(); ++i) {
        if (occurs(varName, comp->arg(i), sub)) {
            return true;
        }
    }
    return false;
}

// TODO: Clone a term while applying the provided substitution map.

std::unique_ptr<Term<std::string>> Unifier::cloneWithSubstitution(
    const Term<std::string>& term, const Substitution& sub) const {
    if (term.isVariable()) {
        const auto* v = dynamic_cast<const Variable*>(&term);
        auto it = sub.find(v->name());
        if (it != sub.end()) {
            return cloneWithSubstitution(*it->second, sub);
        }
        return std::make_unique<Variable>(v->name());
    }

    if (term.isConstant()) {
        const auto* c = dynamic_cast<const Constant*>(&term);
        return std::make_unique<Constant>(c->value());
    }

    const auto* comp = dynamic_cast<const Compound<std::string>*>(&term);
    std::vector<std::unique_ptr<Term<std::string>>> clonedArgs;
    clonedArgs.reserve(comp->arity());
    for (std::size_t i = 0; i < comp->arity(); ++i) {
        clonedArgs.push_back(cloneWithSubstitution(comp->arg(i), sub));
    }
    return std::make_unique<Compound<std::string>>(comp->functor(), std::move(clonedArgs));
}

// TODO: Recursive unification helper. Mutates working substitution on success.

bool Unifier::unifyInternal(const Term<std::string>& a,
                            const Term<std::string>& b,
                            Substitution& working) {
    //current substitutions to both terms to work with their reduced forms.
    auto aReduced = cloneWithSubstitution(a, working);
    auto bReduced = cloneWithSubstitution(b, working);

    const Term<std::string>& lhs = *aReduced;
    const Term<std::string>& rhs = *bReduced;

    // the variable cases
    if (lhs.isVariable() && rhs.isVariable()) {
        const auto* lv = dynamic_cast<const Variable*>(&lhs);
        const auto* rv = dynamic_cast<const Variable*>(&rhs);
        if (lv->name() == rv->name()) {
            return true;
        }
        // bind lexicographically smaller variable to other
        const auto* first = (lv->name() < rv->name()) ? lv : rv;
        const auto* second = (first == lv) ? rv : lv;
        working[first->name()] = std::make_unique<Variable>(second->name());
        return true;
    }

    if (lhs.isVariable()) {
        const auto* lv = dynamic_cast<const Variable*>(&lhs);
        if (occurs(lv->name(), rhs, working)) {
            return false;
        }
        working[lv->name()] = cloneWithSubstitution(rhs, working);
        return true;
    }

    if (rhs.isVariable()) {
        const auto* rv = dynamic_cast<const Variable*>(&rhs);
        if (occurs(rv->name(), lhs, working)) {
            return false;
        }
        working[rv->name()] = cloneWithSubstitution(lhs, working);
        return true;
    }

    // the const case
    if (lhs.isConstant() && rhs.isConstant()) {
        const auto* lc = dynamic_cast<const Constant*>(&lhs);
        const auto* rc = dynamic_cast<const Constant*>(&rhs);
        return lc->value() == rc->value();
    }

    // compound case
    if (lhs.isCompound() && rhs.isCompound()) {
        const auto* lc = dynamic_cast<const Compound<std::string>*>(&lhs);
        const auto* rc = dynamic_cast<const Compound<std::string>*>(&rhs);
        if (lc->functor() != rc->functor() || lc->arity() != rc->arity()) {
            return false;
        }
        for (std::size_t i = 0; i < lc->arity(); ++i) {
            if (!unifyInternal(lc->arg(i), rc->arg(i), working)) {
                return false;
            }
        }
        return true;
    }

    return false;
}
