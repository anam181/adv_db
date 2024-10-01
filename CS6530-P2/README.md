## Project Overview

The goal of this project is to implement a swap space (Buffer Manager) that moves objects between memory and disk as required. A swap space is used by databases to manage datasets larger than main memory by using the disk as a backing store. 

<!-- This project uses a bit of advanced C++ features such as operator overloading and templatized code as part of its infrastructure, but any new code you write should not need to use these features. You are encouraged to use C++ STL library for data structures needed in your implementation.  -->

Your task is to implement a swap space pointer (`swap_space::pointer`) that fetches objects from disk on demand, and writes updates back to the disk. The swap space is allowed to keep `N` objects in memory, and must use LRU (Least Recently Used) eviction policy when it needs to remove objects. The changes in this project you make should be limited to `pointer.hpp` (bookkeeping, using swap\_space to retrieve objects from disk)  and `swap_space.hpp` (eviction policies, hash table, LRU pqueue).

This project will be done in two stages: first implement a disk backed pointer (swap space size `N` as 0), and then implement a caching policy for these pointers in the swap space using the LRU eviction policy.

## Building and Testing

Below is an overview of how the project code is organized:

* `backing_store.hpp`, `backing_store.cpp`: Backing store that enables store/load of serialized objects on disk. 
* `swap_store.hpp`, `pointer.hpp`: Buffer manager related classes. 
* `be_tree.hpp`: A write-optimized key-value store implementation based on the B^e-tree data structure.
* `test.cpp`, `unit_test.cpp`: Performance and unit tests.

To build and run the performance test,

```bash
$ mkdir tmpdir
$ make clean test
$ ./test -m benchmark-queries -d tmpdir
```

If `swap_space::pointer` is implemented correctly, the benchmark should output throughput numbers. For example:

```bash
$ ./test -m benchmark-queries -d tmpdir
num_ops: 4096 time(usec): 5896434, thput(ops/sec): 694.657144
```

Running the benchmark without implementing `swap_space::pointer` will result in a SegFault.

```bash
$ ./test -m benchmark-queries -d tmpdir
Received a heap pointer, convert it to a swap space pointer here
Book-keeping for referent object here
Dereferencing swap space pointer, returning a pin of this object
Should return a pin, Not implemented yet!
Segmentation fault (core dumped)
```

The print statements are left as hints to get you started. 

We also provide a `unit_test.cpp` that contains a simple verification test for `swap_space::pointer`. However, this is the not the final test. The project report should be written based on the results from  `test.cpp`.

```bash
$ make bs_dir
$ make unit_test
$ ./unit_test
BackingStore/Serialization Test Passed!
Received a heap pointer, convert it to a swap space pointer here
Book-keeping for referent object here
Dereferencing swap space pointer, returning a pin of this object
Should return a pin, Not implemented yet!
Segmentation fault (core dumped)
```

The unit test also contains other optional starter tasks. You do not have to implement and fix the tests in `unit_test.cpp` (you will not be graded on this). However starting with the unit tests can help build familiarity with the project code. You are also encouraged to add more unit tests to test the correctness of your implementation during development.


## Swap Space Details

### Serialization

Objects that implement the below serialization methods can be used with the swap space. Serialization methods constructs a string represent of the object which is stored on disk. The Deserialization methods can reconstruct the object from the string stored on disk.

You DO NOT have to implement serialization methods. The `BeTree` node implementation provided implements these methods i.e., they are already serializable. *In case you do update your pointer implementation, be sure to update your serialization methods*.

```cpp
class C {
... // Class Fields, Methods

    // Used by swap_space::pointer to write this object back to Disk.
    // You don't have to implement these, but its good to understand this for debugging.
    // (Exact implementation details vary, code below takes a few liberties).
    void _serialize(std::iostream &fs, serialization_context &context) {
      std::string serialized_representation = this->to_string();
      serialize(fs, context, serialized_representation); 
    }

    void _deserialize(std::iostream &fs, serialization_context &context) {
      std::string serialized_representation 
      deserialize(fs, context, &serialized_representation);
      fill_values_from_serialized_representation(serialized_representation);
    }

}
```

### Backing Store

The backing store is the interface that stores serialized version of objects on disk. Think of it as a disk based hashmap where the key is `object_id, version_id`  and the value is the serialized version of that version of the object, and the serialized version is backed on disk. 

You DO NOT have to implement backing store. You can use provided implementation, `one_file_per_object_backing_store` which implements the below API. 

```cpp
class backing_store {
  public:
  virtual void allocate(uint64_t obj_id, uint64_t version) = 0;
  virtual void deallocate(uint64_t obj_id, uint64_t version) = 0;
  virtual std::iostream* get(uint64_t obj_id, uint64_t version) = 0;
  virtual void put(std::iostream* ios) = 0;
};
```

You can see `swap_space::backstore_load` and `swap_space::backstore_store` to see how to interface with the backing store.

### Swap space pointers

```cpp
swap_space *s; 
//.... Initialize swap space

{
  swap_space::pointer<C> p = s->allocate(new C); 

  // Do work on p.
  p->some_field = 5;
  p->some_method();

  // pointer destroyed when it goes out of scope.
}

// Contrast the above with a normal C pointer.

{
  C *p = new C();
  p->some_field = 5;
  delete p;
}

```

## Tasks/Swap Space

```cpp
{
swap_space::pointer<C> p = s->allocate(new C);
p->some_field = 5
p->some_method();
}

```

The key to successfully completing this project is to deconstruct what is intended to happen in the above code snippet. The swap\_space code depends on operator overloading and templating, and heavily uses three templated classes - `pointer`, `pin` and `object`.

To understand this better, look at the comments at the top of `swap_space.hpp`, and try to work on the simplified TODOs in `unit_test.cpp`. You are encouraged to write more unit tests to test your implementation.

### Task 1: Creating and backing objects

```
swap_space::pointer<C> p = s->allocate(new C);
```

When `s->allocate` is called on `new C`, The C++ heap allocates space for `C`, calls the constructor to `C()` and returns a pointer to that object to `s->allocate`. 

Your Task: Do the necessary bookkeeping to back this object to disk. The book keeping needs to ensure that this object can be loaded back into memory when needed. 

### Task 2: Retrieving/Writing objects from/to disk

```cpp
p->some_field = 5
p->some_method()
```

`p` which is a type of `swap_space::pointer`, needs to return a type of object `C` when the `->` operator is called on this. The operator overloading infrastructure to complete this is setup in `pointer.hpp` (these classes are actually child classes of `swap_space`) in the `pin<Referent> operator ->(void)` method. The `pin` class overloads `->` again to finally return a pointer of `Referent`, which is the object on which you want to access.

#### Differences between `pin<Referrent>`, `pointer<Referent>`, `object`

The `pin` lives for the lifetime of an object access. Once the pin is destroyed, the object can be freed from memory.

The `pointer` is for garbage collection. An object for which no `pointer` instance exists can be garbage-collected and reclaimed from disk.

An `object` stores the pointer to the `Referent` object (which must subclass `serializable`) and other object properties such as refcounts and if it has been modified.

### Task 3: Eviction Policy

If you complete Task 1 and Task 2 correctly, you should have a pointer type that is disk backed.
Going to disk is costly, and you would like to cache some items in memory. Implement an LRU policy in swap\_space that stores a maximum of `N` objects in memory.


### C++ Tips/Reading material

1. C++ constructor and destructor on scope exit.
2. Operator overloading
3. Friend Classes
4. C++ Templates
5. C++ STL Classes

### Submission
You need to submit a `.zip` file of your source code to canvas.

You should also include a `report.pdf` in your submission that contains:

1. A design document on how to implement the swap\_space and pointer classes. 
2. A brief description of the Bε-tree and how it differs from the standard B+-tree.
3. A brief description describing your implementation and how it will speed up operations.
4. A plot showing the performance of the key-value store with changing swap\_space sizes `N`.
5. A list of contributions made by individual students in the group. For parts that multiple students contributed equally should mention "equal contribution".
6. We will evaluate the correctness and the performance of your implementation off-line after the project due date.

### Collaboration Policy
Students will work in the same group as they specified in the Project2 team quiz in Canvas.
Students are allowed to discuss high-level details about the project with others.
Students are not allowed to copy the contents of a white-board after a group meeting with other students.
Students are not allowed to copy the solutions from another colleague.

