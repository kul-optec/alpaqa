#pragma once

#include <alpaqa/export.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/noop-delete.hpp>
#include <alpaqa/util/type-traits.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace alpaqa::util {

#if ALPAQA_WITH_RTTI
class ALPAQA_EXPORT bad_type_erased_type : public std::logic_error {
  public:
    bad_type_erased_type(const std::type_info &actual_type,
                         const std::type_info &requested_type,
                         const std::string &message = "")
        : std::logic_error{message}, actual_type{actual_type},
          requested_type{requested_type} {}

    [[nodiscard]] const char *what() const noexcept override {
        message = "";
        if (const char *w = std::logic_error::what(); w && *w) {
            message += w;
            message += ": ";
        }
        message = "Type requested: " + demangled_typename(requested_type) +
                  ", type contained: " + demangled_typename(actual_type);
        return message.c_str();
    }

  private:
    const std::type_info &actual_type;
    const std::type_info &requested_type;
    mutable std::string message;
};
#endif

class ALPAQA_EXPORT bad_type_erased_constness : public std::logic_error {
  public:
    bad_type_erased_constness()
        : std::logic_error{"Non-const method called on a TypeErased object "
                           "that references a const object"} {}
};

/// Struct that stores the size of a polymorphic object, as well as pointers to
/// functions to copy, move or destroy the object.
/// Inherit from this struct to add useful functions.
struct BasicVTable {

    template <class>
    struct required_function; // undefined
    template <class R, class... Args>
    struct required_function<R(Args...)> {
        using type = R (*)(void *self, Args...);
    };
    template <class R, class... Args>
    struct required_function<R(Args...) const> {
        using type = R (*)(const void *self, Args...);
    };
    template <class, class VTable = BasicVTable>
    struct optional_function; // undefined
    template <class R, class... Args, class VTable>
    struct optional_function<R(Args...), VTable> {
        using type = R (*)(void *self, Args..., const VTable &);
    };
    template <class R, class... Args, class VTable>
    struct optional_function<R(Args...) const, VTable> {
        using type = R (*)(const void *self, Args..., const VTable &);
    };
    /// A required function includes a void pointer to self, in addition to the
    /// arguments of @p F.
    template <class F>
    using required_function_t = typename required_function<F>::type;
    /// An optional function includes a void pointer to self, the arguments of
    /// @p F, and an additional reference to the VTable, so that it can be
    /// implemented in terms of other functions.
    template <class F, class VTable = BasicVTable>
    using optional_function_t = typename optional_function<F, VTable>::type;

    /// Copy-construct a new instance into storage.
    required_function_t<void(void *storage) const> copy = nullptr;
    /// Move-construct a new instance into storage.
    required_function_t<void(void *storage)> move = nullptr;
    /// Destruct the given instance.
    required_function_t<void()> destroy = nullptr;
#if ALPAQA_WITH_RTTI
    /// The original type of the stored object.
    const std::type_info *type = &typeid(void);
#endif

    BasicVTable() = default;

    template <class T>
    BasicVTable(std::in_place_t, T &) noexcept {
        copy = [](const void *self, void *storage) {
            new (storage) T(*std::launder(reinterpret_cast<const T *>(self)));
        };
        // TODO: require that move constructor is noexcept?
        move = [](void *self, void *storage) noexcept {
            if constexpr (std::is_const_v<T>)
                std::terminate();
            else
                new (storage)
                    T(std::move(*std::launder(reinterpret_cast<T *>(self))));
        };
        destroy = [](void *self) {
            if constexpr (std::is_const_v<T>)
                std::terminate();
            else
                std::launder(reinterpret_cast<T *>(self))->~T();
        };
#if ALPAQA_WITH_RTTI
        type = &typeid(T);
#endif
    }
};

namespace detail {
template <class Class, class... ExtraArgs>
struct Launderer {
  private:
    template <auto M, class V, class C, class R, class... Args>
    [[gnu::always_inline]] static constexpr auto
    do_invoke(V *self, Args... args, ExtraArgs...) -> R {
        return std::invoke(M, *std::launder(reinterpret_cast<C *>(self)),
                           std::forward<Args>(args)...);
    }
    template <auto M, class T, class R, class... Args>
        requires std::is_base_of_v<T, Class>
    [[gnu::always_inline]] static constexpr auto invoker_ovl(R (T::*)(Args...)
                                                                 const) {
        return do_invoke<M, const void, const Class, R, Args...>;
    }
    template <auto M, class T, class R, class... Args>
        requires std::is_base_of_v<T, Class>
    [[gnu::always_inline]] static constexpr auto
    invoker_ovl(R (T::*)(Args...)) {
        if constexpr (std::is_const_v<Class>)
            return +[](void *, Args..., ExtraArgs...) {
                throw bad_type_erased_constness{};
            };
        else
            return do_invoke<M, void, Class, R, Args...>;
    }

  public:
    /// Returns a function that accepts a void pointer, casts it to the class
    /// type of the member function @p Method, launders it, and then invokes
    /// @p Method with it, passing on the arguments to @p Method. The function
    /// can also accept additional arguments at the end, of type @p ExtraArgs.
    template <auto Method>
    [[gnu::always_inline]] static constexpr auto invoker() {
        return invoker_ovl<Method>(Method);
    }
};
} // namespace detail

/// @copydoc detail::Launderer::invoker
template <class Class, auto Method, class... ExtraArgs>
[[gnu::always_inline]] constexpr auto type_erased_wrapped() {
    return detail::Launderer<Class, ExtraArgs...>::template invoker<Method>();
}

template <class VTable, class Allocator>
inline constexpr size_t default_te_buffer_size() {
    struct S {
        [[no_unique_address]] Allocator allocator;
        void *self = nullptr;
        VTable vtable;
    };
    const size_t max_size = 128;
    return max_size - std::min(max_size, sizeof(S));
}

template <class... Types>
inline constexpr size_t required_te_buffer_size_for() {
    constexpr size_t sizes[] = {sizeof(Types)...};
    return *std::max_element(std::begin(sizes), std::end(sizes));
}

/// Class for polymorphism through type erasure. Saves the entire vtable, and
/// uses small buffer optimization.
///
/// @todo   Decouple allocation/small buffer optimization.
template <class VTable           = BasicVTable,
          class Allocator        = std::allocator<std::byte>,
          size_t SmallBufferSize = default_te_buffer_size<VTable, Allocator>()>
class TypeErased {
  public:
    static constexpr size_t small_buffer_size = SmallBufferSize;
    using allocator_type                      = Allocator;

  private:
    using allocator_traits = std::allocator_traits<allocator_type>;
    using buffer_type      = std::array<std::byte, small_buffer_size>;
    [[no_unique_address]] alignas(std::max_align_t) buffer_type small_buffer;
    [[no_unique_address]] allocator_type allocator;

  private:
    /// True if @p T is not a child class of @ref TypeErased.
    template <class T>
    static constexpr auto no_child_of_ours =
        !std::is_base_of_v<TypeErased, std::remove_cvref_t<T>>;

  protected:
    static constexpr size_t invalid_size =
        static_cast<size_t>(0xDEAD'BEEF'DEAD'BEEF);
    static constexpr size_t mut_ref_size =
        static_cast<size_t>(0xFFFF'FFFF'FFFF'FFFF);
    static constexpr size_t const_ref_size =
        static_cast<size_t>(0xFFFF'FFFF'FFFF'FFFE);
    [[nodiscard]] static bool size_indicates_ownership(size_t size) {
        return size != const_ref_size && size != mut_ref_size;
    }
    [[nodiscard]] static bool size_indicates_const(size_t size) {
        return size == const_ref_size;
    }

    /// Pointer to the stored object.
    void *self = nullptr;
    /// Size required to store the object.
    size_t size = invalid_size;
    VTable vtable;

  public:
    /// Default constructor.
    TypeErased() noexcept(noexcept(allocator_type()) && noexcept(VTable())) =
        default;
    /// Default constructor (allocator aware).
    template <class Alloc>
    TypeErased(std::allocator_arg_t, const Alloc &alloc) : allocator{alloc} {}

    /// Copy constructor.
    TypeErased(const TypeErased &other)
        : allocator{allocator_traits::select_on_container_copy_construction(
              other.allocator)},
          vtable{other.vtable} {
        do_copy_assign<false>(other);
    }
    /// Copy constructor (allocator aware).
    TypeErased(const TypeErased &other, const allocator_type &alloc)
        : allocator{alloc}, vtable{other.vtable} {
        do_copy_assign<false>(other);
    }
    /// Copy constructor (allocator aware).
    TypeErased(std::allocator_arg_t, const allocator_type &alloc,
               const TypeErased &other)
        : TypeErased{other, alloc} {}

    /// Copy assignment.
    TypeErased &operator=(const TypeErased &other) {
        // Check for self-assignment
        if (&other == this)
            return *this;
        // Delete our own storage before assigning a new value
        cleanup();
        vtable = other.vtable;
        do_copy_assign<true>(other);
        return *this;
    }

    /// Move constructor.
    TypeErased(TypeErased &&other) noexcept
        : allocator{std::move(other.allocator)},
          vtable{std::move(other.vtable)} {
        size = other.size;
        // If not owned, or if dynamically allocated, simply steal storage,
        // simply move the pointer.
        // TODO: is it safe to assume that we can simply move the pointer
        // without performing a move if we moved the allocator? What if the
        // allocator has a small buffer?
        if (!other.owns_referenced_object() || size > small_buffer_size) {
            // We stole the allocator, so we can steal the storage as well
            self = std::exchange(other.self, nullptr);
        }
        // Otherwise, use the small buffer and do an explicit move
        else if (other.self) {
            self = small_buffer.data();
            vtable.move(other.self, self); // assumed not to throw
            vtable.destroy(other.self);    // nothing to deallocate
            other.self = nullptr;
        }
        other.size = invalid_size;
    }
    /// Move constructor (allocator aware).
    TypeErased(TypeErased &&other, const allocator_type &alloc) noexcept
        : allocator{alloc}, vtable{std::move(other.vtable)} {
        // Only continue if other actually contains a value
        if (other.self == nullptr)
            return;
        size = other.size;
        // If not owned, simply move the pointer
        if (!other.owns_referenced_object()) {
            self = std::exchange(other.self, nullptr);
        }
        // If dynamically allocated, simply steal other's storage
        else if (size > small_buffer_size) {
            // Can we steal the storage because of equal allocators?
            if (allocator == other.allocator) {
                self = std::exchange(other.self, nullptr);
            }
            // If the allocators are not the same, we cannot steal the
            // storage, so do an explicit move
            else {
                self = allocator.allocate(size);
                vtable.move(other.self, self);
                // Cannot call other.cleanup() here because we stole the vtable
                vtable.destroy(other.self);
                other.deallocate();
            }
        }
        // Otherwise, use the small buffer and do an explicit move
        else if (other.self) {
            self = small_buffer.data();
            vtable.move(other.self, self);
            // Cannot call other.cleanup() here because we stole the vtable
            vtable.destroy(other.self); // nothing to deallocate
            other.self = nullptr;
        }
        other.size = invalid_size;
    }
    /// Move constructor (allocator aware).
    TypeErased(std::allocator_arg_t, const allocator_type &alloc,
               TypeErased &&other) noexcept
        : TypeErased{std::move(other), alloc} {}

    /// Move assignment.
    TypeErased &operator=(TypeErased &&other) noexcept {
        // Check for self-assignment
        if (&other == this)
            return *this;
        // Delete our own storage before assigning a new value
        cleanup();
        // Check if we are allowed to steal the allocator
        static constexpr bool prop_alloc =
            allocator_traits::propagate_on_container_move_assignment::value;
        if constexpr (prop_alloc)
            allocator = std::move(other.allocator);
        // Only assign if other contains a value
        if (other.self == nullptr)
            return *this;

        size   = other.size;
        vtable = std::move(other.vtable);
        // If not owned, simply move the pointer
        if (!other.owns_referenced_object()) {
            self = std::exchange(other.self, nullptr);
        }
        // If dynamically allocated, simply steal other's storage
        else if (size > small_buffer_size) {
            // Can we steal the storage because of equal allocators?
            // TODO: is it safe to assume that we can simply move the pointer
            // without performing a move if we moved the allocator? What if the
            // allocator has a small buffer?
            if (prop_alloc || allocator == other.allocator) {
                self = std::exchange(other.self, nullptr);
            }
            // If the allocators are not the same, we cannot steal the
            // storage, so do an explicit move
            else {
                self = allocator.allocate(size);
                vtable.move(other.self, self); // assumed not to throw
                vtable.destroy(other.self);
                // Careful, we might have moved other.allocator!
                auto &deallocator    = prop_alloc ? allocator : other.allocator;
                using pointer_t      = typename allocator_traits::pointer;
                auto &&other_pointer = static_cast<pointer_t>(other.self);
                deallocator.deallocate(other_pointer, size);
                other.self = nullptr;
            }
        }
        // Otherwise, use the small buffer and do an explicit move
        else if (other.self) {
            self = small_buffer.data();
            vtable.move(other.self, self);
            vtable.destroy(other.self); // nothing to deallocate
            other.self = nullptr;
        }
        other.size = invalid_size;
        return *this;
    }

    /// Destructor.
    ~TypeErased() { cleanup(); }

    /// Main constructor that type-erases the given argument.
    template <class T, class Alloc>
        requires no_child_of_ours<T>
    explicit TypeErased(std::allocator_arg_t, const Alloc &alloc, T &&d)
        : allocator{alloc} {
        construct_inplace<std::remove_cvref_t<T>>(std::forward<T>(d));
    }
    /// Main constructor that type-erases the object constructed from the given
    /// argument.
    template <class T, class Alloc, class... Args>
    explicit TypeErased(std::allocator_arg_t, const Alloc &alloc,
                        std::in_place_type_t<T>, Args &&...args)
        : allocator{alloc} {
        construct_inplace<std::remove_cvref_t<T>>(std::forward<Args>(args)...);
    }
    /// @copydoc TypeErased(std::allocator_arg_t, const Alloc &, T &&)
    /// Requirement prevents this constructor from taking precedence over the
    /// copy and move constructors.
    template <class T>
        requires no_child_of_ours<T>
    explicit TypeErased(T &&d) {
        construct_inplace<std::remove_cvref_t<T>>(std::forward<T>(d));
    }
    /// Main constructor that type-erases the object constructed from the given
    /// argument.
    template <class T, class... Args>
    explicit TypeErased(std::in_place_type_t<T>, Args &&...args) {
        construct_inplace<std::remove_cvref_t<T>>(std::forward<Args>(args)...);
    }

    /// Construct a type-erased wrapper of type Ret for an object of type T,
    /// initialized in-place with the given arguments.
    template <class Ret, class T, class Alloc, class... Args>
        requires std::is_base_of_v<TypeErased, Ret>
    static Ret make(std::allocator_arg_t tag, const Alloc &alloc,
                    Args &&...args) {
        Ret r{tag, alloc};
        r.template construct_inplace<T>(std::forward<Args>(args)...);
        return r;
    }
    /// Construct a type-erased wrapper of type Ret for an object of type T,
    /// initialized in-place with the given arguments.
    template <class Ret, class T, class... Args>
        requires no_leading_allocator<Args...>
    static Ret make(Args &&...args) {
        return make<Ret, T>(std::allocator_arg, allocator_type{},
                            std::forward<Args>(args)...);
    }

    /// Check if this wrapper wraps an object. False for default-constructed
    /// objects.
    explicit operator bool() const noexcept { return self != nullptr; }

    /// Check if this wrapper owns the storage of the wrapped object, or
    /// whether it simply stores a reference to an object that was allocated
    /// elsewhere.
    [[nodiscard]] bool owns_referenced_object() const noexcept {
        return size_indicates_ownership(size);
    }

    /// Check if the wrapped object is const.
    [[nodiscard]] bool referenced_object_is_const() const noexcept {
        return size_indicates_const(size);
    }

    /// Get a copy of the allocator.
    allocator_type get_allocator() const noexcept { return allocator; }

    /// Query the contained type.
    [[nodiscard]] const std::type_info &type() const noexcept {
        return *vtable.type;
    }

    /// Convert the type-erased object to the given type.
    /// @throws alpaqa::util::bad_type_erased_type
    ///         If T does not match the stored type.
    template <class T>
        requires(!std::is_const_v<T>)
    [[nodiscard]] T &as() & {
#if ALPAQA_WITH_RTTI
        if (typeid(T) != type())
            throw bad_type_erased_type(type(), typeid(T));
#endif
        if (referenced_object_is_const())
            throw bad_type_erased_constness();
        return *reinterpret_cast<T *>(self);
    }
    /// @copydoc as()
    template <class T>
        requires(std::is_const_v<T>)
    [[nodiscard]] T &as() const & {
#if ALPAQA_WITH_RTTI
        if (typeid(T) != type())
            throw bad_type_erased_type(type(), typeid(T));
#endif
        return *reinterpret_cast<T *>(self);
    }
    /// @copydoc as()
    template <class T>
    [[nodiscard]] T &&as() && {
#if ALPAQA_WITH_RTTI
        if (typeid(T) != type())
            throw bad_type_erased_type(type(), typeid(T));
#endif
        if (!std::is_const_v<T> && referenced_object_is_const())
            throw bad_type_erased_constness();
        return std::move(*reinterpret_cast<T *>(self));
    }

    /// Get a type-erased pointer to the wrapped object.
    /// @throws alpaqa::util::bad_type_erased_constness
    ///         If the wrapped object is const.
    /// @see @ref get_const_pointer()
    [[nodiscard]] void *get_pointer() const {
        if (referenced_object_is_const())
            throw bad_type_erased_constness{};
        return self;
    }
    /// Get a type-erased pointer to the wrapped object.
    [[nodiscard]] const void *get_const_pointer() const { return self; }

    /// @see @ref derived_from_TypeErased
    template <std::derived_from<TypeErased> Child>
    friend void derived_from_TypeErased_helper(const Child &) noexcept {
        static constexpr bool False = sizeof(Child) != sizeof(Child);
        static_assert(False, "not allowed in an evaluated context");
    }

  private:
    /// Deallocates the storage when destroyed.
    struct Deallocator {
        TypeErased *instance;
        Deallocator(TypeErased *instance) noexcept : instance{instance} {}
        Deallocator(const Deallocator &)            = delete;
        Deallocator &operator=(const Deallocator &) = delete;
        Deallocator(Deallocator &&o) noexcept
            : instance{std::exchange(o.instance, nullptr)} {}
        Deallocator &operator=(Deallocator &&) noexcept = delete;
        void release() noexcept { instance = nullptr; }
        ~Deallocator() { instance ? instance->deallocate() : void(); }
    };

    /// Ensure that storage is available, either by using the small buffer if
    /// it is large enough, or by calling the allocator.
    /// Returns a RAII wrapper that deallocates the storage unless released.
    Deallocator allocate(size_t size) {
        assert(!self);
        assert(size != invalid_size);
        assert(size > 0);
        assert(size_indicates_ownership(size));
        self       = size <= small_buffer_size ? small_buffer.data()
                                               : allocator.allocate(size);
        this->size = size;
        return {this};
    }

    /// Deallocate the memory without invoking the destructor.
    void deallocate() {
        assert(size != invalid_size);
        assert(size > 0);
        assert(size_indicates_ownership(size));
        using pointer_t = typename allocator_traits::pointer;
        if (size > small_buffer_size)
            allocator.deallocate(reinterpret_cast<pointer_t>(self), size);
        self = nullptr;
    }

    /// Destroy the type-erased object (if not empty), and deallocate the memory
    /// if necessary.
    void cleanup() {
        if (!owns_referenced_object()) {
            self = nullptr;
        } else if (self) {
            vtable.destroy(self);
            deallocate();
        }
    }

    template <bool CopyAllocator>
    void do_copy_assign(const TypeErased &other) {
        constexpr bool prop_alloc =
            allocator_traits::propagate_on_container_copy_assignment::value;
        if constexpr (CopyAllocator && prop_alloc)
            allocator = other.allocator;
        if (!other)
            return;
        if (!other.owns_referenced_object()) {
            // Non-owning: simply copy the pointer.
            size = other.size;
            self = other.self;
        } else {
            auto storage_guard = allocate(other.size);
            // If copy constructor throws, storage should be deallocated and
            // self set to null, otherwise the TypeErased destructor will
            // attempt to call the contained object's destructor, which is
            // undefined behavior if construction failed.
            vtable.copy(other.self, self);
            storage_guard.release();
        }
    }

  protected:
    /// Ensure storage and construct the type-erased object of type T in-place.
    template <class T, class... Args>
    void construct_inplace(Args &&...args) {
        static_assert(std::is_same_v<T, std::remove_cvref_t<T>>);
        if constexpr (std::is_pointer_v<T>) {
            T ptr{args...};
            using Tnp = std::remove_pointer_t<T>;
            size      = std::is_const_v<Tnp> ? const_ref_size : mut_ref_size;
            vtable    = VTable{std::in_place, *ptr};
            self      = const_cast<std::remove_const_t<Tnp> *>(ptr);
        } else {
            // Allocate memory
            auto storage_guard = allocate(sizeof(T));
            // Construct the stored object
            using destroyer = std::unique_ptr<T, noop_delete<T>>;
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 160000
            // TODO: remove when we drop libc++ 15 support
            destroyer obj_guard{new (self) T{std::forward<Args>(args)...}};
#else
            destroyer obj_guard{std::uninitialized_construct_using_allocator(
                reinterpret_cast<T *>(self), allocator,
                std::forward<Args>(args)...)};
#endif
            vtable = VTable{std::in_place, static_cast<T &>(*obj_guard.get())};
            obj_guard.release();
            storage_guard.release();
        }
    }

    /// Call the vtable function @p f with the given arguments @p args,
    /// implicitly passing the @ref self pointer and @ref vtable reference if
    /// necessary.
    template <class Ret, class... FArgs, class... Args>
    [[gnu::always_inline]] decltype(auto) call(Ret (*f)(const void *, FArgs...),
                                               Args &&...args) const {
        assert(f);
        assert(self);
        using LastArg = util::last_type_t<FArgs...>;
        if constexpr (std::is_same_v<LastArg, const VTable &>)
            return f(self, std::forward<Args>(args)..., vtable);
        else
            return f(self, std::forward<Args>(args)...);
    }
    /// @copydoc call
    template <class Ret, class... FArgs, class... Args>
    [[gnu::always_inline]] decltype(auto) call(Ret (*f)(void *, FArgs...),
                                               Args &&...args) {
        assert(f);
        assert(self);
        if (referenced_object_is_const())
            throw bad_type_erased_constness{};
        using LastArg = util::last_type_t<FArgs...>;
        if constexpr (std::is_same_v<LastArg, const VTable &>)
            return f(self, std::forward<Args>(args)..., vtable);
        else
            return f(self, std::forward<Args>(args)...);
    }
    /// @copydoc call
    template <class Ret>
    [[gnu::always_inline]] decltype(auto) call(Ret (*f)(const void *)) const {
        assert(f);
        assert(self);
        return f(self);
    }
    /// @copydoc call
    template <class Ret>
    [[gnu::always_inline]] decltype(auto) call(Ret (*f)(void *)) {
        assert(f);
        assert(self);
        if (referenced_object_is_const())
            throw bad_type_erased_constness{};
        return f(self);
    }
    /// @copydoc call
    template <class Ret>
    [[gnu::always_inline]] decltype(auto) call(Ret (*f)(const void *,
                                                        const VTable &)) const {
        assert(f);
        assert(self);
        return f(self, vtable);
    }
    /// @copydoc call
    template <class Ret>
    [[gnu::always_inline]] decltype(auto) call(Ret (*f)(void *,
                                                        const VTable &)) {
        assert(f);
        assert(self);
        if (referenced_object_is_const())
            throw bad_type_erased_constness{};
        return f(self, vtable);
    }
};

template <class Child>
concept derived_from_TypeErased =
    requires(Child c) { derived_from_TypeErased_helper(c); };

} // namespace alpaqa::util