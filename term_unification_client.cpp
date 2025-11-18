#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "term_unification.h"

/* Sample output (after you implement Unifier):
Test 1 (var-const): success => {X -> a}
Test 2 (const-var): success => {X -> b}
Test 3 (const mismatch): failure
Test 4 (compound match): success => {X -> a}
Test 5 (functor mismatch): failure
Test 6 (arity mismatch): failure
Test 7 (occurs check): failure
Test 8 (deep cons): success => {H -> 1, T -> cons(2, nil)}
Test 9 (var-compound): success => {X -> g(a, Y)}
Test 10 (two vars): success => {X -> Y}
Test 11 (pair mismatch): failure
*/

using TermPtr = std::unique_ptr<Term<std::string>>;

namespace builders {

TermPtr var(std::string name) {
    return std::make_unique<Variable>(std::move(name));
}

TermPtr constant(std::string value) {
    return std::make_unique<Constant>(std::move(value));
}

TermPtr compound(std::string functor, std::vector<TermPtr> args) {
    return std::make_unique<Compound<std::string>>(std::move(functor), std::move(args));
}

}  // namespace builders

void printTerm(const Term<std::string>& term, std::ostream& out) {
    if (term.isVariable()) {
        const auto* v = dynamic_cast<const Variable*>(&term);
        out << v->name();
    } else if (term.isConstant()) {
        const auto* c = dynamic_cast<const Constant*>(&term);
        out << c->value();
    } else if (term.isCompound()) {
        const auto* comp = dynamic_cast<const Compound<std::string>*>(&term);
        out << comp->functor() << "(";
        for (std::size_t i = 0; i < comp->arity(); ++i) {
            if (i > 0) {
                out << ", ";
            }
            printTerm(comp->arg(i), out);
        }
        out << ")";
    }
}

void printTerm(const Term<std::string>& term) {
    printTerm(term, std::cout);
}

void printSubstitution(const Unifier::Substitution& sub) {
    bool first = true;
    for (const auto& [varName, term] : sub) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << varName << " -> ";
        printTerm(*term, std::cout);
        first = false;
    }
}

struct TestCase {
    std::string name;
    TermPtr t1;
    TermPtr t2;
    bool expectSuccess;
};

std::vector<TestCase> buildTests() {
    using namespace builders;
    std::vector<TestCase> tests;

    tests.push_back({"var-const", var("X"), constant("a"), true});
    tests.push_back({"const-var", constant("b"), var("X"), true});
    tests.push_back({"const mismatch", constant("a"), constant("b"), false});

    {
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(var("X"));
        lhsArgs.push_back(constant("b"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(constant("a"));
        rhsArgs.push_back(constant("b"));
        tests.push_back({"compound match",
                         compound("f", std::move(lhsArgs)),
                         compound("f", std::move(rhsArgs)),
                         true});
    }

    {
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(var("X"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(var("X"));
        tests.push_back({"functor mismatch",
                         compound("f", std::move(lhsArgs)),
                         compound("g", std::move(rhsArgs)),
                         false});
    }

    {
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(var("X"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(var("X"));
        rhsArgs.push_back(var("Y"));
        tests.push_back({"arity mismatch",
                         compound("f", std::move(lhsArgs)),
                         compound("f", std::move(rhsArgs)),
                         false});
    }

    {
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(var("X"));
        tests.push_back({"occurs check",
                         var("X"),
                         compound("f", std::move(rhsArgs)),
                         false});
    }

    {
        // cons(H, T) vs cons(1, cons(2, nil))
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(var("H"));
        lhsArgs.push_back(var("T"));

        std::vector<TermPtr> innerArgs;
        innerArgs.push_back(constant("2"));
        innerArgs.push_back(constant("nil"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(constant("1"));
        rhsArgs.push_back(compound("cons", std::move(innerArgs)));
        tests.push_back({"deep cons",
                         compound("cons", std::move(lhsArgs)),
                         compound("cons", std::move(rhsArgs)),
                         true});
    }

    {
        // X vs g(a, Y)
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(constant("a"));
        rhsArgs.push_back(var("Y"));
        tests.push_back(
            {"var-compound", var("X"), compound("g", std::move(rhsArgs)), true});
    }

    {
        tests.push_back({"two vars", var("X"), var("Y"), true});
    }

    {
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(constant("a"));
        lhsArgs.push_back(constant("b"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(constant("a"));
        rhsArgs.push_back(constant("c"));
        tests.push_back({"pair mismatch",
                         compound("pair", std::move(lhsArgs)),
                         compound("pair", std::move(rhsArgs)),
                         false});
    }

    return tests;
}

int main() {
    Unifier unifier;
    auto tests = buildTests();

    int passed = 0;
    for (std::size_t i = 0; i < tests.size(); ++i) {
        const auto& test = tests[i];
        auto result = unifier.unify(*test.t1, *test.t2);
        bool success = result.has_value();
        std::cout << "Test " << (i + 1) << " (" << test.name << "): ";
        printTerm(*test.t1);
        std::cout << "  ~  ";
        printTerm(*test.t2);
        std::cout << " => " << (success ? "success" : "failure");
        if (success && result) {
            std::cout << " {";
            printSubstitution(*result);
            std::cout << "}";
        }
        std::cout << "\n";
        passed += (success == test.expectSuccess) ? 1 : 0;
    }

    std::cout << "Summary: " << passed << "/" << tests.size()
              << " outcomes matched expectations.\n";
    return 0;
}
