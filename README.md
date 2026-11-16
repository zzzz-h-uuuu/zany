# Type Erasure for Nearly All Types — zany

## Why zany, and when to use?

This project aims to address the lack of support for move-only types in `std::any` since C++17. In everyday C++ programming, when you need to perform type erasure on move-only types, you might consider using `std::variant` as an alternative. However, `std::variant` has two drawbacks that can be rather unfriendly to developers:

- **Fixed set of types:** You must enumerate all the types to be erased by `std::variant` at compile time;
- **Copy restrictions:** If any of the alternatives in a `std::variant` is a move-only type, the entire `std::variant` becomes non-copyable, regardless of whether the actual object it currently holds is copyable.

These limitations mean that `std::variant` is not always an ideal choice.

Another approach is to wrap a move-only object in a `std::shared_ptr` and then pass it to `std::any`. However, considering the **SBO** (Small Buffer Optimization) adopted by standard library implementations, wrapping every move-only type in a `std::shared_ptr` introduces unnecessary overhead and is clearly not an elegant solution. For example, **in common 64-bit GCC/libstdc++ implementations,** a `std::shared_ptr<T>` object stored in `std::any` is always allocated on the heap.

And that's why `zany` was created — it retains almost **all** the operations supported by `std::any` while addressing most of the limitations of type erasure. You can initialize a `zany` object with either copyable or move-only types, and (in most cases) you don't have to worry about performance overhead.

## How to use

`zany` supports almost all operations provided by `std::any`. Once RTTI support is further refined in a future release (trust me, this will happen very soon), we can finally drop the word "almost". This means you can work with `zany` in much the same way as you would with `std::any`.

One thing worth noting is how `zany` handles move-only types. Since I'm a firm believer in SBO, `zany` distinguishes between move-only types based on their size and alignment, resulting in different copy behaviors:

- For move-only objects smaller than 16 bytes, `zany` does not allow them to be copied. If you attempt to copy-construct another `zany` from one of these objects, or perform a similar operation, an exception will be thrown. The error message will look something like this:

```
try copying move-only type: "Only_move_small" from zany(const zany& __other)
terminate called after throwing an instance of 'std::runtime_error'
  what():  try copying move-only type: "Only_move_small" from zany(const zany& __other)
```

- For move-only objects larger than 16 bytes, copying a `zany` behaves similarly to copying a `std::shared_ptr`: multiple `zany` instances share ownership of the same underlying object. Its memory management has passed `ASan`/`TSan` testing prior to release，but you need to take care of the resource just like what you usually do with `shared_ptr`.

## Changelog

- v1.0.0: Supports GCC compilers only, requires C++20 or later. No known operating system restrictions.
- v1.1.0: Supports GCC compilers only, but remove all demands about `concept`, so now it requires C++17.