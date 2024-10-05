#include <iostream>
#include "swap_space.hpp"
#include "backing_store.hpp"

const  std::string backing_store_dir = "simple_bs";


struct TestClass : public serializable {
  uint64_t x, y;
  TestClass(): x(0), y(0) {}
  TestClass(uint64_t a, uint64_t b): x(a), y(b) {}
  void _serialize(std::iostream& fs, serialization_context& context) {
    serialize(fs, context, x);
    serialize(fs, context, y);
  }
  void _deserialize(std::iostream& fs, serialization_context& context) {
    deserialize(fs, context, x);
    deserialize(fs, context, y);
  }
};

void test_backing_store_api() {
  one_file_per_object_backing_store bs("bs_dir"); // Make sure this directory exists before testing.
  swap_space ss(&bs, 0);
  serialization_context ctx(ss);

  TestClass tc1;
  tc1.x = 12; tc1.y = 24;
  int id = 123, version_1 = 10;

  // Write the object out to backing store.
  bs.allocate(id, version_1);
  std::iostream* out = bs.get(id, version_1);
  serialize(*out, ctx, tc1);
  bs.put(out);

  // Read the object out to backing store.
  TestClass tc2;
  std::iostream* in = bs.get(id, version_1);
  deserialize(*in, ctx, tc2);

  if (tc1.x != tc2.x && tc1.y != tc2.y) {
    printf("BackingStore/Serialization Test Failed!\n");
  } else {
    printf("BackingStore/Serialization Test Passed!\n");
  }
}

void test_swap_space_pointers() {
  one_file_per_object_backing_store bs("testdir");
  swap_space ss(&bs, 1);
  serialization_context ctx(ss);

  swap_space::pointer<TestClass> tc = ss.allocate(new TestClass());
  // This test will seg fault as the operator overloads for pointer-> and pin-> are not implemented.
  // To get this working, you will have to:
  //    In allocate, do the swapspace book-keeping and (optionally) free the pointer from heap, 
  //    In pointer and pin, use the swapspace and backing store to get your object from the disk.
  tc->x = 5; 
  tc->y = 6;
}

void test_multiple_pointers() {
  one_file_per_object_backing_store bs("testdir");
  swap_space ss(&bs, 1);
  serialization_context ctx(ss);

  swap_space::pointer<TestClass> tc = ss.allocate(new TestClass());
  tc->x = 5; 
  tc->y = 6;
  swap_space::pointer<TestClass> tc2 = ss.allocate(new TestClass());
  tc2->x = 2;
  tc->x = 6;

  uint64_t x_val = tc->x;
  uint64_t y_val = tc->y;

  std::cout << "X: " << x_val << " Y: " << y_val << std::endl;
}

void test_multiple_pointers_multiple_versions() {
  one_file_per_object_backing_store bs("testdir");
  swap_space ss(&bs, 1);
  serialization_context ctx(ss);

  swap_space::pointer<TestClass> tc = ss.allocate(new TestClass());
  tc->x = 5; 
  tc->y = 6;
  swap_space::pointer<TestClass> tc2 = ss.allocate(new TestClass());
  tc2->x = 2;
  tc->x = 6;
  tc2->x = 3;
  tc->x = 10;

  uint64_t x_val = tc->x;
  uint64_t y_val = tc->y;

  std::cout << "X: " << x_val << " Y: " << y_val << std::endl;
}


void test_pinning() {
  one_file_per_object_backing_store bs("testdir");
  swap_space ss(&bs, 2);
  serialization_context ctx(ss);

  swap_space::pointer<TestClass> tc = ss.allocate(new TestClass());
  swap_space::pointer<TestClass> tc2;
  {
    swap_space::pin<TestClass> pin1 = tc.get_pin();
    tc2 = ss.allocate(new TestClass());
  }

  swap_space::pin<TestClass> pin2 = tc2.get_pin();
  swap_space::pointer<TestClass> tc3 = ss.allocate(new TestClass());
}

void test_small_pinning() {
  one_file_per_object_backing_store bs("testdir");
  swap_space ss(&bs, 2);
  serialization_context ctx(ss);

  swap_space::pointer<TestClass> tc = ss.allocate(new TestClass());
  tc->y = 10;
  tc->x = tc->y;
  
  
}


struct LinkedList {
  struct Node : public serializable {
    uint64_t x;
    uint64_t has_next; // A bit inefficient, but works.
    swap_space::pointer<Node> next;
    void _serialize(std::iostream& fs, serialization_context& context) {
      serialize(fs, context, x);
      serialize(fs, context, has_next);
      serialize(fs, context, next);
    }
    void _deserialize(std::iostream& fs, serialization_context& context) {
      deserialize(fs, context, x);
      deserialize(fs, context, has_next);
      deserialize(fs, context, next);
    }
  };
};

void test_linked_list() {
  // TODO: Implement a linked list that is backed on disk. 
  // HINT: Is your swap space pointer serialization/deserialization correct? 
}

void basic_test() {

}

int main() {
  // test_backing_store_api();
  // test_swap_space_pointers();
  // test_multiple_pointers();
  // test_multiple_pointers_multiple_versions();
  // test_pinning();
  test_small_pinning();
}