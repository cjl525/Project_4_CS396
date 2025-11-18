// include iostream for output
#include <iostream>
// include map for substitution storage
#include <map>
// include memory for smart pointers
#include <memory>
// include optional for optional results
#include <optional>
// include string for text handling
#include <string>
// include utility for move operations
#include <utility>
// include vector for argument lists
#include <vector>

// include unification interface definitions
#include "term_unification.h"

/* Expected Output:
Test 1 (var-const): X  ~  a => success {X -> a}
Test 2 (const-var): b  ~  X => success {X -> b}
Test 3 (const mismatch): a  ~  b => failure
Test 4 (compound match): f(X, b)  ~  f(a, b) => success {X -> a}
Test 5 (functor mismatch): f(X)  ~  g(X) => failure
Test 6 (arity mismatch): f(X)  ~  f(X, Y) => failure
Test 7 (occurs check): X  ~  f(X) => failure
Test 8 (deep cons): cons(H, T)  ~  cons(1, cons(2, nil)) => success {H -> 1, T -> cons(2, nil)}
Test 9 (var-compound): X  ~  g(a, Y) => success {X -> g(a, Y)}
Test 10 (two vars): X  ~  Y => success {X -> Y}
Test 11 (pair mismatch): pair(a, b)  ~  pair(a, c) => failure
Test 12 (repeated var mismatch): f(X, X)  ~  f(a, b) => failure
Test 13 (symmetric binding): f(X, Y)  ~  f(Y, a) => success {X -> a, Y -> a}
Test 14 (occurs through alias): f(X, Y)  ~  f(Y, g(X)) => failure
Test 15 (nested success): h(g(X), X)  ~  h(g(a), a) => success {X -> a}

Summary: 15/15 outcomes matched expectations.
*/

// alias for pointer to term
using TermPtr = std::unique_ptr<Term<std::string>>;

// namespace to build convenience constructors
namespace builders {

// build a variable term
TermPtr var(std::string name) {
    // create variable pointer
    return std::make_unique<Variable>(std::move(name));
}

// build a constant term
TermPtr constant(std::string value) {
    // create constant pointer
    return std::make_unique<Constant>(std::move(value));
}

// build a compound term
TermPtr compound(std::string functor, std::vector<TermPtr> args) {
    // create compound pointer
    return std::make_unique<Compound<std::string>>(std::move(functor), std::move(args));
}

}  // namespace builders

// print a term to a stream
void printTerm(const Term<std::string>& term, std::ostream& out) {
    // check variable case
    if (term.isVariable()) {
        // cast to variable pointer
        const auto* v = dynamic_cast<const Variable*>(&term);
        // output variable name
        out << v->name();
    } else if (term.isConstant()) {
        // cast to constant pointer
        const auto* c = dynamic_cast<const Constant*>(&term);
        // output constant value
        out << c->value();
    } else if (term.isCompound()) {
        // cast to compound pointer
        const auto* comp = dynamic_cast<const Compound<std::string>*>(&term);
        // print functor and opening parenthesis
        out << comp->functor() << "(";
        // iterate over arguments
        for (std::size_t i = 0; i < comp->arity(); ++i) {
            // add comma separator for subsequent args
            if (i > 0) {
                // print separator
                out << ", ";
            }
            // recursively print argument
            printTerm(comp->arg(i), out);
        }
        // close compound representation
        out << ")";
    }
}

// convenience overload to print to cout
void printTerm(const Term<std::string>& term) {
    // delegate to stream overload
    printTerm(term, std::cout);
}

void printSubstitution(const Unifier& unifier, const Unifier::Substitution& sub) {
    bool first = true;
    // iterate over substitution entries
    for (const auto& [varName, term] : sub) {
        // add comma when not first
        if (!first) {
            // print comma separator
            std::cout << ", ";
        }
        // print variable arrow
        std::cout << varName << " -> ";
        auto substituted = unifier.substitute(*term, sub);
        printTerm(*substituted, std::cout);
        first = false;
    }
}

// define test case structure
struct TestCase {
    // descriptive name
    std::string name;
    // first term pointer
    TermPtr t1;
    // second term pointer
    TermPtr t2;
    // expected success flag
    bool expectSuccess;
};

// build the collection of test cases
std::vector<TestCase> buildTests() {
    // use builder helpers
    using namespace builders;
    // container for tests
    std::vector<TestCase> tests;

    // add variable to constant test
    tests.push_back({"var-const", var("X"), constant("a"), true});
    // add constant to variable test
    tests.push_back({"const-var", constant("b"), var("X"), true});
    // add mismatch constant test
    tests.push_back({"const mismatch", constant("a"), constant("b"), false});

    {
        // prepare left and right compound arguments
        std::vector<TermPtr> lhsArgs;
        // push variable X
        lhsArgs.push_back(var("X"));
        // push constant b
        lhsArgs.push_back(constant("b"));
        // prepare right arguments
        std::vector<TermPtr> rhsArgs;
        // push constant a
        rhsArgs.push_back(constant("a"));
        // push constant b
        rhsArgs.push_back(constant("b"));
        // add compound match test
        tests.push_back({"compound match",
                         compound("f", std::move(lhsArgs)),
                         compound("f", std::move(rhsArgs)),
                         true});
    }

    {
        // prepare functor mismatch case
        std::vector<TermPtr> lhsArgs;
        // push variable X
        lhsArgs.push_back(var("X"));
        // prepare right arguments
        std::vector<TermPtr> rhsArgs;
        // push variable X
        rhsArgs.push_back(var("X"));
        // add mismatch test
        tests.push_back({"functor mismatch",
                         compound("f", std::move(lhsArgs)),
                         compound("g", std::move(rhsArgs)),
                         false});
    }

    {
        // prepare arity mismatch case
        std::vector<TermPtr> lhsArgs;
        // push variable X
        lhsArgs.push_back(var("X"));
        // prepare right arguments
        std::vector<TermPtr> rhsArgs;
        // push variable X
        rhsArgs.push_back(var("X"));
        // push variable Y
        rhsArgs.push_back(var("Y"));
        // add arity mismatch test
        tests.push_back({"arity mismatch",
                         compound("f", std::move(lhsArgs)),
                         compound("f", std::move(rhsArgs)),
                         false});
    }

    {
        // prepare occurs check case
        std::vector<TermPtr> rhsArgs;
        // push variable X
        rhsArgs.push_back(var("X"));
        // add occurs check test
        tests.push_back({"occurs check",
                         var("X"),
                         compound("f", std::move(rhsArgs)),
                         false});
    }

    {
        // prepare cons structure test
        std::vector<TermPtr> lhsArgs;
        // push head variable
        lhsArgs.push_back(var("H"));
        // push tail variable
        lhsArgs.push_back(var("T"));

        // inner list arguments
        std::vector<TermPtr> innerArgs;
        // push element 2
        innerArgs.push_back(constant("2"));
        // push nil terminator
        innerArgs.push_back(constant("nil"));
        // build right-hand side arguments
        std::vector<TermPtr> rhsArgs;
        // push head constant 1
        rhsArgs.push_back(constant("1"));
        // push constructed tail
        rhsArgs.push_back(compound("cons", std::move(innerArgs)));
        // add deep cons test
        tests.push_back({"deep cons",
                         compound("cons", std::move(lhsArgs)),
                         compound("cons", std::move(rhsArgs)),
                         true});
    }

    {
        // build variable versus compound case
        std::vector<TermPtr> rhsArgs;
        // push constant a
        rhsArgs.push_back(constant("a"));
        // push variable y
        rhsArgs.push_back(var("Y"));
        // add var-compound test
        tests.push_back(
            {"var-compound", var("X"), compound("g", std::move(rhsArgs)), true});
    }

    {
        // add two variables binding test
        tests.push_back({"two vars", var("X"), var("Y"), true});
    }

    {
        // prepare pair mismatch
        std::vector<TermPtr> lhsArgs;
        // push constant a
        lhsArgs.push_back(constant("a"));
        // push constant b
        lhsArgs.push_back(constant("b"));
        // prepare right arguments
        std::vector<TermPtr> rhsArgs;
        // push constant a
        rhsArgs.push_back(constant("a"));
        // push constant c
        rhsArgs.push_back(constant("c"));
        // add pair mismatch test
        tests.push_back({"pair mismatch",
                         compound("pair", std::move(lhsArgs)),
                         compound("pair", std::move(rhsArgs)),
                         false});
    }

    {
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(var("X"));
        lhsArgs.push_back(var("X"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(constant("a"));
        rhsArgs.push_back(constant("b"));
        tests.push_back({"repeated var mismatch",
                         compound("f", std::move(lhsArgs)),
                         compound("f", std::move(rhsArgs)),
                         false});
    }

    {
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(var("X"));
        lhsArgs.push_back(var("Y"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(var("Y"));
        rhsArgs.push_back(constant("a"));
        tests.push_back({"symmetric binding",
                         compound("f", std::move(lhsArgs)),
                         compound("f", std::move(rhsArgs)),
                         true});
    }

    {
        std::vector<TermPtr> lhsArgs;
        lhsArgs.push_back(var("X"));
        lhsArgs.push_back(var("Y"));
        std::vector<TermPtr> rhsArgs;
        rhsArgs.push_back(var("Y"));
        {
            std::vector<TermPtr> inner;
            inner.push_back(var("X"));
            rhsArgs.push_back(compound("g", std::move(inner)));
        }
        tests.push_back({"occurs through alias",
                         compound("f", std::move(lhsArgs)),
                         compound("f", std::move(rhsArgs)),
                         false});
    }

    {
        std::vector<TermPtr> lhsArgs;
        {
            std::vector<TermPtr> inner;
            inner.push_back(var("X"));
            lhsArgs.push_back(compound("g", std::move(inner)));
        }
        lhsArgs.push_back(var("X"));
        std::vector<TermPtr> rhsArgs;
        {
            std::vector<TermPtr> inner;
            inner.push_back(constant("a"));
            rhsArgs.push_back(compound("g", std::move(inner)));
        }
        rhsArgs.push_back(constant("a"));
        tests.push_back({"nested success",
                         compound("h", std::move(lhsArgs)),
                         compound("h", std::move(rhsArgs)),
                         true});
    }

    return tests;
}

// program entry point
int main() {
    // create unifier instance
    Unifier unifier;
    // build all tests
    auto tests = buildTests();

    // track passed count
    int passed = 0;
    // iterate through tests
    for (std::size_t i = 0; i < tests.size(); ++i) {
        // reference current test
        const auto& test = tests[i];
        // attempt unification
        auto result = unifier.unify(*test.t1, *test.t2);
        // determine success flag
        bool success = result.has_value();
        // print test header
        std::cout << "Test " << (i + 1) << " (" << test.name << "): ";
        // print left term
        printTerm(*test.t1);
        // print separator
        std::cout << "  ~  ";
        // print right term
        printTerm(*test.t2);
        // output result state
        std::cout << " => " << (success ? "success" : "failure");
        // if success print substitution
        if (success && result) {
            // open brace
            std::cout << " {";
            printSubstitution(unifier, *result);
            std::cout << "}";
        }
        // end line
        std::cout << "\n";
        // tally pass or fail
        passed += (success == test.expectSuccess) ? 1 : 0;
    }

    // print summary line
    std::cout << "Summary: " << passed << "/" << tests.size()
              << " outcomes matched expectations.\n";
    // return success code
    return 0;
}
