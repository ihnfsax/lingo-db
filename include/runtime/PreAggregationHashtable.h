#ifndef RUNTIME_PREAGGREGATIONHASHTABLE_H
#define RUNTIME_PREAGGREGATIONHASHTABLE_H
#include "runtime/Buffer.h"
#include "runtime/ThreadLocal.h"
#include <cstddef>
#include <cstdint>
namespace runtime {
class PreAggregationHashtableFragment {
   public:
   struct Entry {
      Entry* next;
      size_t hashValue;
      uint8_t content[];
      //kv follows
   };
   static constexpr size_t numOutputs = 64;
   static constexpr size_t hashtableSize = 1024;
   Entry* ht[hashtableSize];
   size_t typeSize;
   size_t len;
   runtime::FlexibleBuffer* outputs[numOutputs];
   PreAggregationHashtableFragment(size_t typeSize) : ht(), typeSize(typeSize), len(0), outputs() {}
   static PreAggregationHashtableFragment* create(runtime::ExecutionContext* context, size_t typeSize);
   Entry* insert(size_t hash);
};
class PreAggregationHashtable {
   using Entry=PreAggregationHashtableFragment::Entry;
   struct PartitionHt{
      Entry** ht;
      size_t hashMask;
   };
   PartitionHt ht[PreAggregationHashtableFragment::numOutputs];
   runtime::FlexibleBuffer buffer;
   PreAggregationHashtable() : ht(), buffer(1, sizeof(PreAggregationHashtableFragment::Entry*)) {

   }

   public:
   static runtime::PreAggregationHashtable* merge(runtime::ExecutionContext* context,ThreadLocal*, bool (*eq)(uint8_t*, uint8_t*), void (*combine)(uint8_t*, uint8_t*));
   Entry* lookup(size_t hash);
   static void lock(Entry* entry,size_t subtract);
   static void unlock(Entry* entry,size_t subtract);
   runtime::BufferIterator* createIterator();
   ~PreAggregationHashtable();
};

} // end namespace runtime

#endif //RUNTIME_PREAGGREGATIONHASHTABLE_H
