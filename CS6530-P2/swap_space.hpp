// A scheme for transparently swapping data structures in and out of
// memory.

// WARNING: this is very incomplete.  It's just enough functionality
//          for the betree.cpp.  In particular, the current system
//          does not handle cycles in the pointer graph or pointers
//          into the middle of objects (such as into an array).

// The goal of this code is to enable users to write complex in-memory
// data structures and have a separate layer (i.e. this code) manage
// I/O.  Users should be able to define their data structures as they
// see fit (i.e. they can use pointers, etc) but still control the
// granularity at which items are swapped to/from memory.

// Therefore, we define a swap_space::pointer type that represents a
// pointer from one swappable unit to another.  When the swapper elects
// to swap out an object X, it will swap out all the objects that X
// points to through regular C++ pointers.  All these objects will be
// written to a single place on the backing store, so this will be
// I/O-efficient.  The swapper does not traverse swap_space::pointers
// -- they point to separate things that should be swapped out
// independently of the thing pointing to them.

// The betree code provides an example of how this is used.  We want
// each node to be swapped in/out as a single unit, but separate nodes
// in the tree should be able to be swapped in/out independently of
// eachother.  Therefore, nodes use swap_space::pointers to point to
// eachother.  They use regular C++ pointers to point to internal
// items that should be serialized as part of the node.

// The swap_space needs to manage all pointers to swappable objects.
// New swappable objects should be created like this:
//      swap_space ss;
//      swap_space::pointer<T> p = ss.allocate(new T(constructor args));

// You can then use operator-> as normal, e.g.
//      p->some_field
//      p->some_method(args)
// Although no operator* is not defined, it should be straightforward
// to do so.

// Invoking p->some_method() pins the object referred to by p in
// memory.  Thus, during the execution of some_method(), it is safe to
// dereference "this" and any other plain C++ pointers in the object.

// Objects are automatically garbage collected.  The garbage collector
// uses reference counting.

// The current system uses LRU to select items to swap.  The swap
// space has a user-specified in-memory cache size it.  The cache size
// can be adjusted dynamically.

// Don't try to get your hands on an unwrapped pointer to the object
// or anything that is swapped in/out as part of the object.  It can
// only lead to trouble.  Casting is also probably a bad idea.  Just
// write nice, clean, type-safe, well-encapsulated code and everything
// should work just fine.

// Objects managed by this system must be sub-types of class
// serializable.  This basically defines two methods for serializing
// and deserializing the object.  See the betree for examples of
// implementing these methods.  We provide default implementations for
// a few basic types and STL containers.  Feel free to add more and
// submit patches as you need them.

// The current implementation serializes to a textual file format.
// This is just a convenience.  It would be nice to be able to swap in
// different formats.

#ifndef SWAP_SPACE_HPP
#define SWAP_SPACE_HPP

#include "backing_store.hpp"
#include "debug.hpp"
#include "serialization.hpp"
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <chrono>

class swap_space {
  public:
  swap_space(backing_store *bs, uint64_t max_in_memory_objects) :
    backstore(bs),
    max_in_memory_objects(max_in_memory_objects) {}

  template <class Referent>
  class pointer;

  // Given a heap pointer, construct a ss object around it.
  template <class Referent>
  pointer<Referent> allocate(Referent* tgt)
  {
    std::cout<<"Received a heap pointer, convert it to a swap space pointer here"<<std::endl;
    return pointer<Referent>(this, tgt);
  }

#include "pointer.hpp"

  private:
  struct ObjectComparator {
    bool operator()(const object* lhs, const object* rhs) const {
        return lhs->last_access < rhs->last_access || (lhs->last_access == rhs->last_access && lhs->id < rhs->id);
    }
  };

// TODO: Design and add the required fields and methods to completely implement the swap space.
  backing_store* backstore;
  uint64_t max_in_memory_objects;
  std::map<uint64_t, object*> ptrMap;
  std::unordered_map<uint64_t, object*> memory_store;


  void add_object_to_memory(object* obj) 
  {
    if(memory_store.size() == max_in_memory_objects)
    {
      evict_object_from_memory();
    }

    // Add object
    memory_store[obj->id] = obj;
    std::cout << "ADD:"<< std::endl;
    print_ptrMap();
    print_MemoryStore();
  }

  void evict_object_from_memory() 
  {
    uint64_t oldest_timestamp = UINT64_MAX;
    uint64_t oldest_obj_id;
    for (const auto& pair : memory_store) {
        uint64_t curr_timestamp = pair.second->last_access;
        if(curr_timestamp < oldest_timestamp)
        {
          oldest_timestamp = curr_timestamp;
          oldest_obj_id = pair.second->id;
        }
    }

    backstore_store(ptrMap[oldest_obj_id]);

    delete ptrMap[oldest_obj_id]->target;
    ptrMap[oldest_obj_id]->target = nullptr;
    memory_store.erase(oldest_obj_id);
    std::cout << "EVICT:"<< std::endl;
    print_ptrMap();
    print_MemoryStore();
  }

  void print_ptrMap() {
    std::cout << "PtrMap" << std::endl;
    for (const auto& pair : ptrMap) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second->target << std::endl;
    }
    std::cout << std::endl;
  }

  void print_MemoryStore() {
    std::cout << "Memory Store" << std::endl;
    for (const auto& pair : memory_store) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second->target << std::endl;
    }
    std::cout << std::endl;
  }

  template <class Referent>
  Referent* retrieve_obj_from_disk(const pointer<Referent>* ptr)
  {
    object* obj = ptrMap[ptr->target];

    if(memory_store.size() == max_in_memory_objects)
    {
      evict_object_from_memory();
    }

    Referent* disk_obj = backstore_load<Referent>(obj->id, obj->version);

    obj->target = disk_obj;
    memory_store[obj->id] = obj;
    std::cout << "RETRIEVE:"<< std::endl;
    print_ptrMap();
    print_MemoryStore();

    return disk_obj;

  }

  // Below are Helper methods provided to you to interface with the backing store.
  template <class Referent>
  Referent* backstore_load(uint64_t obj_id, uint64_t obj_version)
  {
    std::iostream* in = backstore->get(obj_id, obj_version);
    Referent* r = new Referent();
    serialization_context ctxt(*this);
    deserialize(*in, ctxt, *r);
    backstore->put(in);
    return r;
  }

  //store a referent into the backstore.
  void backstore_store(object* obj)
  {
    serialization_context ctxt(*this);
    std::stringstream sstream;
    serialize(sstream, ctxt, *obj->target);
    std::string buffer = sstream.str();
    uint64_t new_version_id = obj->version + 1;
    backstore->allocate(obj->id, new_version_id);
    std::iostream* out = backstore->get(obj->id, new_version_id);
    out->write(buffer.data(), buffer.length());
    backstore->put(out);

    //version 0 is the flag that the object exists only in memory.
    if (obj->version > 0)
      backstore->deallocate(obj->id, obj->version);

    obj->version = new_version_id;
    obj->target_is_dirty = false;
  }
};

#endif // SWAP_SPACE_HPP
