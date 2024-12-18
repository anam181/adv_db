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
  // Write back all dirty pages and remove them from the queue
  object *obj = NULL;
  for (auto it = lru_pqueue.begin(); it != lru_pqueue.end();) {
      obj = *it;
      if (obj == NULL || !obj->target_is_dirty) {
          ++it;
          continue;
      }
      auto next_it = std::next(it);
      lru_pqueue.erase(it);
      write_back(obj);
      
      delete obj->target;
      obj->target = NULL;
      current_in_memory_objects--;

      it = next_it;
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
  temp_version_map_file << next_id << std::endl;
  for (const auto& pair : objects_to_versions) {
      temp_version_map_file << pair.first << ":" << pair.second << std::endl;
  }
  temp_version_map_file.close();

  // Rename temp file
  rename(temp_version_map_filename.c_str(), version_map_filename.c_str());

}

void swap_space::delete_old_version(void) {
    for (const auto& entry : objects_to_versions) {
        uint64_t object_id = entry.first;
        uint64_t current_version = entry.second;

        // Check for and delete all older versions
        if (current_version > 1) { // Versions start at 1
            for (uint64_t old_version = 1; old_version < current_version; ++old_version) {
                // Debug message
                debug(std::cout << "Deleting old version: Object ID " << object_id 
                                << ", Version " << old_version << std::endl);

                // Delete the old version from the backing store
                if (backstore) {
                    backstore->deallocate(object_id, old_version);
                }
            }
        }
    }

}


int swap_space::rebuildVersionMap(std::string filename, uint64_t& root_id, uint64_t& next) {
  // Read in file and parse out the data
  std::ifstream file(filename);
  if (!file.is_open()) {
      std::cerr << "Error: Could not open file.\n";
      assert(false);
  }

  std::string line;
  int lineCount = 0;
  while (std::getline(file, line)) {
      if(lineCount == 0) {
        root_id = std::stoull(line);
      }
      else if(lineCount == 1) {
        next = std::stoull(line);
      }
      else {
          size_t pos = line.find(':');

          if (pos != std::string::npos) {
              std::string key = line.substr(0, pos);
              std::string value = line.substr(pos + 1);
              objects_to_versions[std::stoull(key)] = std::stoull(value);
          } else {
              std::cerr << "Delimiter ':' not found!" << std::endl;
              assert(false);
          }
      }
      lineCount++;
  }

  file.close();

  return 0;
}


int swap_space::rebuildObjectMap(uint64_t next) {
  // Loop through all keys in map
  for (const auto& pair : objects_to_versions) {
    // Make object for the key
    object *newObj = new object(this, nullptr);

    newObj->id = pair.first;
    newObj->version = pair.second;
    newObj->target_is_dirty = false;
    newObj->refcount = 1;
    newObj->last_access = UINT64_MAX;
    newObj->pincount = 0;

    // Make filename
    std::string filepath = "./tmpdir/" + std::to_string(pair.first) + '_' + std::to_string(pair.second);

    // Open the file
    std::ifstream inputFile(filepath);
    if (!inputFile) {
        std::cerr << "Failed to open the file. id:" << pair.first << " version: " << pair.second << std::endl;
        assert(false);
    }

    std::string line;
    int lineCount = 0;
    int pivotCount = -1;

    // Read the file line by line
    while (std::getline(inputFile, line)) {
        lineCount++;

        if (lineCount == 2) { 
            std::istringstream iss(line);
            std::string word;
            // Check if line starts with map
            if (iss >> word && word == "map") {
              // Get number out after map
                if (iss >> pivotCount) {} 
                else {
                    std::cerr << "Failed to extract the number after 'map'." << std::endl;
                    assert(false);
                }
            }
            break;
        }
    }

    inputFile.close();

    // Check if file has pivots
    if(pivotCount == -1) {
      std::cerr << "Error: Could not find pivot count for . id:" << pair.first << " version: " << pair.second << "\n";
      assert(false);
    }
    // Set isleaf
    newObj->is_leaf = pivotCount == 0;

    // Set new object in map
    objects[pair.first] = newObj;
  }

  if (next != UINT64_MAX) {
    next_id = next;
  }

  if (objects_to_versions.size() == 0) {
    return 1;
  }

  return 0;

}

void swap_space::print_LRU(void) {
  object *obj = NULL;
  std::cout << "PRINTING LRU: " << std::endl;
    for (auto it = lru_pqueue.begin(); it != lru_pqueue.end();) {
        obj = *it;
        std::cout << obj->id << std::endl;
        ++it;
    }
}

void swap_space::print_ref_counts(void) {
  std::cout << "PRINTING REFCOUNTS" << std::endl;
  for (const auto& pair : objects) {
    std::cout << "ID: " << pair.first << " Ref: " << pair.second->refcount << std::endl;
  }
}