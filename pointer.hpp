#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <set>
#include "swap_space.hpp" 

class serializable;  // Forward declaration


template <class Referent>
class pin {
public:
    const Referent* operator->(void) const
    {
        // TODO: We are dereferencing a READ-ONLY version of this pinned object.
        return referent_;
    }

    Referent* operator->(void)
    {
        // TODO: We are dereferencing a READ-WRITE version of this pinned object.
        // Return a read-write version of this pinned object
        return referent_;
    }

    pin(const pointer<Referent>* p)
        : referent_(nullptr), ss_(p->ss_), target_(p->target_) // Use p->ss_ and p->target_
    {
        // TODO: Create a 'pinned' wrapper of the object pointed to by p
        // A 'pinned' wrapper object lives in memory until it is destroyed (i.e., when ~pin is called).
        // DESIGN CONSIDERATION: How does the swap space know not to evict this object?
        // Pin the object in memory (prevent eviction)
        referent_ = ss_->load<Referent>(target_); // Specify template argument
        ss_->pin_object(target_);
    }

    ~pin(void)
    {
        // TODO: It is now safe to remove a 'pinned' wrapper of the object pointed to by p from memory.
        // Unpin the object (allow eviction)
        ss_->unpin_object(target_);
    }

    pin& operator=(const pin& other)
    {
        // TODO: Update this pin to be wrapper of the 'other'.
        // HINT: What happens to the 'pinned object' this object was pointing to?
        // HINT: What if other == this?
        if (this != &other) {
            // Unpin the current object
            ss_->unpin_object(target_);
            // Assign new values and pin the new object
            referent_ = other.referent_;
            ss_ = other.ss_;
            target_ = other.target_;
            ss_->pin_object(target_);
        }
        return *this;
    }

private:
    // TODO: Add fields here.
    Referent* referent_; // Pointer to the pinned object
    swap_space* ss_;     // Pointer to the swap space
    uint64_t target_;     // Object ID
};

template <class Referent>
class pointer : public serializable {
    // These class declarations are to allow you to access
    // private and protected members of swap_space and pin<Referent>
    friend class swap_space;
    friend class pin<Referent>;

public:
     pointer(swap_space* ss, uint64_t target)
        : ss_(ss), target_(target), referent_(ss_->load<Referent>(target)) { // Initializing target_
        ss_->pin_object(target); // Pin the object using the initialized target
    }

    ~pointer() {
        ss_->unpin_object(target_); // Use target_ in the destructor
    }

    pointer(void) : ss_(nullptr), target_(0) {}

    pointer(const pointer& other)
    {
        // TODO: Initialize this to be a copy of the pointer pointed to by other
        ss_ = other.ss_;
        target_ = other.target_;
    }

    ~pointer(void)
    {
        // TODO: Destroy this pointer.
        // DESIGN CONSIDERATION: (What happens to the object pointed to by this pointer?)
        // No special cleanup is needed, but reference counting and memory management
        // can be handled in swap_space.
    }

    pointer& operator=(const pointer& other)
    {
        // TODO: Initialize this to be a copy of the pointer pointed to by other.
        // DESIGN CONSIDERATION: What happens to the object pointed to by this pointer?
        if (this != &other) {
            ss_ = other.ss_;
            target_ = other.target_;
        }
        return *this;
    }

    bool operator==(const pointer& other) const
    {
        return ss_ == other.ss_ && target_ == other.target_;
    }

    bool operator!=(const pointer& other) const
    {
        return !operator==(other);
    }

    pin<Referent> operator->(void) 
    {
        std::cout << "Dereferencing swap space pointer, returning a pin of this object" << std::endl;
        return pin<Referent>(this);
    }

    const pin<Referent> operator->(void) const
    {
        std::cout << "Dereferencing swap space pointer, returning a pin of this object" << std::endl;
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
        // TODO: Implement.
        return ss_->is_in_memory(target_);
    }

    bool is_dirty(void) const
    {
        // TODO: Implement.
        return ss_->is_dirty(target_);
    }

    void _serialize(std::iostream& fs, serialization_context& context)
    {
        fs << target_ << " "; // Serialize the target_
        target_ = 0;          // Reset the target_ after serialization
    }

    void _deserialize(std::iostream& fs, serialization_context& context)
    {
        ss_ = &context.ss;    // Point to the current swap space
        fs >> target_;        // Deserialize the target_
    }

private:
    swap_space* ss_; // Pointer to the swap space
    uint64_t target_; // Object ID

    // Don't call this directly, only swap_space::allocate should use this.
    pointer(swap_space* sspace, Referent* tgt)
    {
        // TODO: Create a 'swap space' pointer for the object pointed to by tgt in that swap space.
        std::cout << "Book-keeping for referent object here" << std::endl;
        ss_ = sspace;
        target_ = ss_->allocate(tgt); // Allocate and set target_
    }
};

class object {
public:
    object(swap_space* sspace, serializable* tgt);

    serializable* target;       // Pointer to the actual object
    uint64_t id;                // ID to fetch from the backing store.
    uint64_t version;           // Optional version field. 
    uint64_t refcount;          // Count of pointers pointing to this object
    uint64_t last_access;       // Timestamp of last read/write. Use for LRU queue.
    bool target_is_dirty;       // Dirty bit marker (Was the object modified after loading into memory?)
    uint64_t pincount;          // Count of threads requiring this object to be pinned. 
                                // The object must be kept in memory when pinned.
                                // (You do not have to support concurrent accesses for this project).
};
