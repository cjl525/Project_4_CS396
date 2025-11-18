#include "term_unification.h"

#include <stdexcept>

// TODO: Implement full unification logic with occurs check.
std::optional<Unifier::Substitution> Unifier::unify(const Term<std::string>& t1,
                                                    const Term<std::string>& t2) {
    Substitution working;
    if (!unifyInternal(t1, t2, working)) {
        return std::nullopt;
    }
    return working;
}

// TODO: Apply substitutions recursively to produce a fully bound deep copy.
std::unique_ptr<Term<std::string>> Unifier::substitute(const Term<std::string>& term,
                                                       const Substitution& sub) const {
    return cloneWithSubstitution(term, sub);
}

// TODO: Implement occurs check to prevent circular bindings.
bool Unifier::occurs(const std::string& varName,
                     const Term<std::string>& term,
                     const Substitution& sub) const {
    (void)varName;
    (void)term;
    (void)sub;
    return false;
}

// TODO: Clone a term while applying the provided substitution map.
std::unique_ptr<Term<std::string>> Unifier::cloneWithSubstitution(
    const Term<std::string>& term, const Substitution& sub) const {
    (void)sub;
    return term.clone();
}

// TODO: Recursive unification helper. Mutates working substitution on success.
bool Unifier::unifyInternal(const Term<std::string>& a,
                            const Term<std::string>& b,
                            Substitution& working) {
    (void)a;
    (void)b;
    (void)working;
    return false;
}
