#ifndef TERM_UNIFICATION_H
#define TERM_UNIFICATION_H

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Base interface for all term types.
// T is the stored value type (string for this assignment).
template <typename T>
class Term {
public:
    virtual ~Term() = default;

    // True when this term is a logic variable placeholder (e.g., "X").
    virtual bool isVariable() const = 0;

    // True when this term is a ground symbol with no structure (e.g., "a").
    virtual bool isConstant() const = 0;

    // True when this term is a structured functor with arguments (e.g., f(X, Y)).
    virtual bool isCompound() const = 0;

    // Clone enables deep copies during substitution/unification.
    virtual std::unique_ptr<Term<T>> clone() const = 0;
};

// ------------------------------- Variable ---------------------------------
class Variable : public Term<std::string> {
private:
    std::string name_;

public:
    explicit Variable(std::string name);

    // Returns the variable identifier (e.g., "X").
    const std::string& name() const noexcept;

    bool isVariable() const override;
    bool isConstant() const override;
    bool isCompound() const override;

    std::unique_ptr<Term<std::string>> clone() const override;
};

// ------------------------------- Constant ---------------------------------
class Constant : public Term<std::string> {
private:
    std::string value_;

public:
    explicit Constant(std::string value);

    // Returns the stored symbol (e.g., "a").
    const std::string& value() const noexcept;

    bool isVariable() const override;
    bool isConstant() const override;
    bool isCompound() const override;

    std::unique_ptr<Term<std::string>> clone() const override;
};

// ------------------------------- Compound ---------------------------------
template <typename T>
class Compound : public Term<T> {
private:
    std::string functor_;
    std::vector<std::unique_ptr<Term<T>>> args_;

public:
    using TermPtr = std::unique_ptr<Term<T>>;

    Compound(std::string functor, std::vector<TermPtr> args);
    Compound(const Compound& other);
    Compound& operator=(const Compound& other);
    Compound(Compound&& other) noexcept = default;
    Compound& operator=(Compound&& other) noexcept = default;
    ~Compound() override = default;

    // Functor name (e.g., "f" in f(X, Y)).
    const std::string& functor() const noexcept;

    // Number of child terms.
    std::size_t arity() const noexcept;

    // Access the i-th child term (throws std::out_of_range on bad index).
    const Term<T>& arg(std::size_t index) const;

    bool isVariable() const override;
    bool isConstant() const override;
    bool isCompound() const override;

    std::unique_ptr<Term<T>> clone() const override;

private:
    static std::vector<TermPtr> cloneArgs(const std::vector<TermPtr>& src);
};

// ------------------------------- Unifier ----------------------------------
class Unifier {
public:
    using Substitution = std::map<std::string, std::unique_ptr<Term<std::string>>>;

    // Attempts to unify t1 and t2. Returns std::nullopt on failure.
    // On success, returns variable bindings such that applying them makes t1 and t2 identical.
    std::optional<Substitution> unify(const Term<std::string>& t1,
                                      const Term<std::string>& t2);

    // Applies a substitution to a term, returning a deep copy with bindings applied.
    // Input: any term plus a substitution map. Output: fully substituted term (new heap object).
    std::unique_ptr<Term<std::string>> substitute(const Term<std::string>& term,
                                                  const Substitution& sub) const;

private:
    // Occurs check: returns true if varName appears anywhere inside term after applying sub.
    // Used to prevent circular bindings.
    bool occurs(const std::string& varName,
                const Term<std::string>& term,
                const Substitution& sub) const;

    // Helper: clone term while applying sub recursively.
    std::unique_ptr<Term<std::string>> cloneWithSubstitution(const Term<std::string>& term,
                                                             const Substitution& sub) const;

    // Recursive unification driver; returns true on success and mutates 'working'.
    // Expects both terms to be partially substituted already.
    bool unifyInternal(const Term<std::string>& a,
                       const Term<std::string>& b,
                       Substitution& working);
};

// ------------------------- Inline Implementations ------------------------

inline Variable::Variable(std::string name) : name_(std::move(name)) {}

inline const std::string& Variable::name() const noexcept {
    return name_;
}

inline bool Variable::isVariable() const {
    return true;
}

inline bool Variable::isConstant() const {
    return false;
}

inline bool Variable::isCompound() const {
    return false;
}

inline std::unique_ptr<Term<std::string>> Variable::clone() const {
    return std::make_unique<Variable>(name_);
}

inline Constant::Constant(std::string value) : value_(std::move(value)) {}

inline const std::string& Constant::value() const noexcept {
    return value_;
}

inline bool Constant::isVariable() const {
    return false;
}

inline bool Constant::isConstant() const {
    return true;
}

inline bool Constant::isCompound() const {
    return false;
}

inline std::unique_ptr<Term<std::string>> Constant::clone() const {
    return std::make_unique<Constant>(value_);
}

template <typename T>
inline Compound<T>::Compound(std::string functor, std::vector<TermPtr> args)
    : functor_(std::move(functor)), args_(std::move(args)) {}

template <typename T>
inline Compound<T>::Compound(const Compound& other)
    : functor_(other.functor_), args_(cloneArgs(other.args_)) {}

template <typename T>
inline Compound<T>& Compound<T>::operator=(const Compound& other) {
    if (this != &other) {
        functor_ = other.functor_;
        args_ = cloneArgs(other.args_);
    }
    return *this;
}

template <typename T>
inline const std::string& Compound<T>::functor() const noexcept {
    return functor_;
}

template <typename T>
inline std::size_t Compound<T>::arity() const noexcept {
    return args_.size();
}

template <typename T>
inline const Term<T>& Compound<T>::arg(std::size_t index) const {
    return *args_.at(index);
}

template <typename T>
inline bool Compound<T>::isVariable() const {
    return false;
}

template <typename T>
inline bool Compound<T>::isConstant() const {
    return false;
}

template <typename T>
inline bool Compound<T>::isCompound() const {
    return true;
}

template <typename T>
inline std::unique_ptr<Term<T>> Compound<T>::clone() const {
    return std::make_unique<Compound<T>>(functor_, cloneArgs(args_));
}

template <typename T>
inline std::vector<typename Compound<T>::TermPtr> Compound<T>::cloneArgs(
    const std::vector<TermPtr>& src) {
    std::vector<TermPtr> copy;
    copy.reserve(src.size());
    for (const auto& arg : src) {
        copy.push_back(arg->clone());
    }
    return copy;
}

#endif // TERM_UNIFICATION_H
