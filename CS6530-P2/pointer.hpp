template <class Referent>
class pin;

template <class Referent>
class pointer;

class object;

// This pins an object in memory for the duration of a member
// access.  It's sort of an instance of the "resource aquisition is
// initialization" paradigm.
//
template <class Referent>
class pin {
  public:
  const Referent* operator->(void) const
  {
    // TODO: We are dereferencing a READ-ONLY version of this pinned object.
    return NULL;
  }

  Referent* operator->(void)
  {
    // TODO: We are dereferencing a READ-WRITE version of this pinned object.
    // std::cout<<"Should return a pin, Not implemented yet!"<<std::endl;
    Referent* r;
    if(ptr->is_in_memory())
    {
      r = dynamic_cast<Referent*>(ptr->ss->ptrMap[ptr->target]->target);
    }
    else
    {
      r = ptr->ss->retrieve_obj_from_disk(ptr);
    }

    auto now = std::chrono::system_clock::now(); 
    ptr->ss->ptrMap[ptr->target]->last_access = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                                        now.time_since_epoch()).count());
    return r;
  }

  pin(const pointer<Referent>* p)
  {
    // TODO: Create a 'pinned' wrapper of the object pointed to by p
    // A 'pinned' wrapper object live in memory until it is destroyed (i.e , when ~pin is called).
    // DESIGN CONSIDERATION: How does the swap space know not to evict this object?
    ptr = p;
    ptr->ss->ptrMap[ptr->target]->pincount += 1;
  }

  ~pin(void)
  {
    // TODO: It is now safe to remove a 'pinned' wrapper of the object pointed to by p from memory.
  }

  pin& operator=(const pin& other)
  {
    // TODO: Update this pin to be wrapper of the 'other'.
    // HINT: What happens to the 'pinned object' this object was pointing to?
    // HINT: What if other == this?
  }

  private:
  // TODO: Add fields here.
  const pointer<Referent>* ptr;
  
};

template <class Referent>
class pointer : public serializable {
  // These class declarations are to allow you to access
  // private and protected members of swap_space and pin<Referrent>
  friend class swap_space;
  friend class pin<Referent>;

  public:
  pointer(void)
      : ss(NULL)
      , target(0)
  {
  }

  pointer(const pointer& other)
  {
    // TODO: Initilize this to be a copy of the pointer pointed to by other
  }

  ~pointer(void)
  {
    // TODO: Destroy this pointer.
    // DESIGN CONSIDERATION: (What happens to the object pointed to by this pointer?)
  }

  pointer& operator=(const pointer& other)
  {
    // TODO: Initilize this to be a copy of the pointer pointed to by other.
    // DESIGN CONSIDERATION: What happens to the object pointed to by this pointer?
    return *this;
  }

  bool operator==(const pointer& other) const
  {
    return ss == other.ss && target == other.target;
  }

  bool operator!=(const pointer& other) const
  {
    return !operator==(other);
  }

  const pin<Referent> operator->(void) const
  {
    std::cout<<"Dereferencing swap space pointer, returning a pin of this object"<<std::endl;
    return pin<Referent>(this);
  }

  pin<Referent> operator->(void)
  {
    std::cout<<"Dereferencing swap space pointer, returning a pin of this object"<<std::endl;
    return pin<Referent>(this);
  }

  pin<Referent> get_pin(void)
  {
    return pin<Referent>(this);
  }

  const pin<Referent> get_pin(void) const
  {
    return pin<Referent>(this);
  }

  bool is_in_memory(void) const
  {
    // Implement
    return ss->memory_store.find(target) != ss->memory_store.end();
  }

  bool is_dirty(void) const
  {
    // TODO: Implement.
    return false;
  }

  void _serialize(std::iostream& fs, serialization_context& context)
  {
    fs << target << " ";
    target = 0;
  }

  void _deserialize(std::iostream& fs, serialization_context& context)
  {
    ss = &context.ss;
    fs >> target;
  }

  private:
  swap_space* ss;
  uint64_t target;

  // Don't call this directly, only swap_space::allocate should use this.
  pointer(swap_space* sspace, Referent* tgt)
  {
    // TODO: Create a 'swap space' pointer for the object pointed to by tgt in that swap space.

    // Assume obj is new
    std::cout<<"Book-keeping for referent object here"<<std::endl;
    uint64_t id = static_cast<uint64_t>(sspace->ptrMap.size());
    ss = sspace;
    target = id;
    object* obj = new object(sspace, tgt);
    obj->id = id;
    obj->refcount = 1;
    sspace->ptrMap.insert(std::make_pair(id, obj));
    sspace->add_object_to_memory(obj);
  }
};

class object {
  public:
  object(swap_space* sspace, serializable* tgt)
  : target(tgt), id(0), version(0), refcount(0), last_access(0), target_is_dirty(false), pincount(0) {};

  serializable* target;
  uint64_t id; // id to fetch from backing store.
  uint64_t version; // optional version field. 
  uint64_t refcount; // Count of pointers pointing to this object
  uint64_t last_access; // Timestamp of last read/write. Use for LRU queue.
  bool target_is_dirty; // Dirty bit marker (Was the object modified after loading into memory?)
  uint64_t pincount; // Count of threads requiring this object to be pinned. 
	// The object must be kept in memory when pinned.
	// (You do not have to support concurrent accesses for this project).
};
