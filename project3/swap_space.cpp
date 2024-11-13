#include "swap_space.hpp"
#include <fstream>


//Methods to serialize/deserialize different kinds of objects.
//You shouldn't need to touch these.
void serialize(std::iostream &fs, serialization_context &context, uint64_t x)
{
  fs << x << " ";
  assert(fs.good());
}

void deserialize(std::iostream &fs, serialization_context &context, uint64_t &x)
{
  fs >> x;
  assert(fs.good());
}

void serialize(std::iostream &fs, serialization_context &context, int64_t x)
{
  fs << x << " ";
  assert(fs.good());
}

void deserialize(std::iostream &fs, serialization_context &context, int64_t &x)
{
  fs >> x;
  assert(fs.good());
}

void serialize(std::iostream &fs, serialization_context &context, std::string x)
{
  fs << x.size() << ",";
  assert(fs.good());
  fs.write(x.data(), x.size());
  assert(fs.good());
}

void deserialize(std::iostream &fs, serialization_context &context, std::string &x)
{
  size_t length;
  char comma;
  fs >> length >> comma;
  assert(fs.good());
  char *buf = new char[length];
  assert(buf);
  fs.read(buf, length);
  assert(fs.good());
  x = std::string(buf, length);
  delete buf;
}

bool swap_space::cmp_by_last_access(swap_space::object *a, swap_space::object *b) {
  return a->last_access < b->last_access;
}

swap_space::swap_space(backing_store *bs, uint64_t n) :
  backstore(bs),
  max_in_memory_objects(n),
  objects(),
  lru_pqueue(cmp_by_last_access)
{}

//construct a new object. Called by ss->allocate() via pointer<Referent> construction
//Does not insert into objects table - that's handled by pointer<Referent>()
swap_space::object::object(swap_space *sspace, serializable * tgt) {
  target = tgt;
  id = sspace->next_id++;
  version = 0;
  is_leaf = false;
  refcount = 1;
  last_access = sspace->next_access_time++;
  target_is_dirty = true;
  pincount = 0;
}

//set # of items that can live in ss.
void swap_space::set_cache_size(uint64_t sz) {
  assert(sz > 0);
  max_in_memory_objects = sz;
  maybe_evict_something();
}

//write an object that lives on disk back to disk
//only triggers a write if the object is "dirty" (target_is_dirty == true)
void swap_space::write_back(swap_space::object *obj)
{
  assert(objects.count(obj->id) > 0);

  debug(std::cout << "Writing back " << obj->id
	<< " (" << obj->target << ") "
	<< "with last access time " << obj->last_access << std::endl);

  // This calls _serialize on all the pointers in this object,
  // which keeps refcounts right later on when we delete them all.
  // In the future, we may also use this to implement in-memory
  // evictions, i.e. where we first "evict" an object by
  // compressing it and keeping the compressed version in memory.
  serialization_context ctxt(*this);
  std::stringstream sstream;
  serialize(sstream, ctxt, *obj->target);
  obj->is_leaf = ctxt.is_leaf;

  if (obj->target_is_dirty) {
    // std::cout << "WRITING BACK" << std::endl;
    std::string buffer = sstream.str();


    //modification - ss now controls BSID - split into unique id and version.
    //version increments linearly based uniquely on this version counter.

    uint64_t new_version_id = obj->version+1;

    backstore->allocate(obj->id, new_version_id);
    std::iostream *out = backstore->get(obj->id, new_version_id);
    out->write(buffer.data(), buffer.length());
    backstore->put(out);

    //version 0 is the flag that the object exists only in memory.
    // if (obj->version > 0)
    //   backstore->deallocate(obj->id, obj->version);
    obj->version = new_version_id;
    objects_to_versions[obj->id] = new_version_id;
    obj->target_is_dirty = false;
  }
}


//attempt to evict an unused object from the swap space
//objects in swap space are referenced in a priority queue
//pull objects with low counts first to try and find an object with pincount 0.
void swap_space::maybe_evict_something(void)
{
  while (current_in_memory_objects > max_in_memory_objects) {
    object *obj = NULL;
    for (auto it = lru_pqueue.begin(); it != lru_pqueue.end(); ++it)
      if ((*it)->pincount == 0) {
	obj = *it;
	break;
      }
    if (obj == NULL)
      return;
    lru_pqueue.erase(obj);

    write_back(obj);
    
    delete obj->target;
    obj->target = NULL;
    current_in_memory_objects--;
  }
}

void swap_space::write_back_dirty_pages_info_to_disk(void)
{
    object *obj = NULL;
    for (auto it = lru_pqueue.begin(); it != lru_pqueue.end();) {
        obj = *it;
        if (obj == NULL || !obj->target_is_dirty) {
            ++it;
            continue;
        }

        auto next_it = std::next(it);  // Save the next iterator
        lru_pqueue.erase(it);          // Erase the current element
        write_back(obj);               // Write back the object
        
        delete obj->target;           // Deallocate the target memory
        obj->target = NULL;           // Nullify the target pointer
        current_in_memory_objects--; // Decrement the count of in-memory objects

        it = next_it; // Move the iterator to the next element
    }
}


void swap_space::write_version_map_to_disk(void) {
  std::string version_map_filename = "version_map.txt";
  std::string temp_version_map_filename = "tmp_version_map.txt";
  
  // Open new temp file
  std::ofstream temp_version_map_file;
  temp_version_map_file.open(temp_version_map_filename, std::ofstream::out | std::ofstream::trunc);

  // Write each log record to the file
  temp_version_map_file << root_id << std::endl;
  for (const auto& pair : objects_to_versions) {
      temp_version_map_file << pair.first << ":" << pair.second << std::endl;
  }
  temp_version_map_file.close();

  // Delete old file
  remove(version_map_filename.c_str());

  // Rename temp file
  rename(temp_version_map_filename.c_str(), version_map_filename.c_str());

    
}

