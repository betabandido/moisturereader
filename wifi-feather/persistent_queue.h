#ifndef PERSISTENT_QUEUE
#define PERSISTENT_QUEUE

#include <EEPROM.h>

template<class T>
class persistent_queue {
public:
  typedef T value_type;
  typedef std::size_t size_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  
  persistent_queue(int base_address, size_type capacity)
    : capacity_{capacity}
  {
    uint8_t* data = EEPROM.getDataPtr() + base_address;
    storage_ = reinterpret_cast<storage_area*>(data);
            
    if (storage_->signature != SIGNATURE) {
      storage_->signature = SIGNATURE;
      storage_->begin = 0;
      storage_->end = 0;
      storage_->size = 0;
    }
  }

  static constexpr size_type storage_size(size_type capacity) {
    return sizeof(storage_area::signature)
        + sizeof(storage_area::begin)
        + sizeof(storage_area::end)
        + sizeof(storage_area::size)
        + capacity * sizeof(value_type);
  }

  bool empty() const {
    return size() == 0;
  }

  bool full() const {
    return size() == capacity();
  }

  size_type size() const {
    return storage_->size;
  }

  size_type capacity() const {
    return capacity_;
  }

  reference front() {
    return storage_->data()[storage_->begin];
  }

  const_reference front() const {
    return storage_->data()[storage_->begin];
  }

  bool push(const value_type& value) {
    if (full())
      return false;
    
    storage_->data()[storage_->end] = value;
    increment(storage_->end);
    ++(storage_->size);
    return true;
  }

  bool push(value_type&& value) {
    if (full())
      return false;

    storage_->data()[storage_->end] = std::move(value);
    increment(storage_->end);
    ++(storage_->size);
    return true;
  }

  bool pop() {
    if (empty())
      return false;

    increment(storage_->begin);
    --(storage_->size);
    return true;
  }

private:
  struct storage_area {
    unsigned signature;
    unsigned begin;
    unsigned end;
    size_type size;
    
    value_type* data() {
      uint8_t* ptr_to_data =
          reinterpret_cast<uint8_t*>(&signature)
          + sizeof(signature)
          + sizeof(begin)
          + sizeof(end)
          + sizeof(size);
      return reinterpret_cast<value_type*>(ptr_to_data);
    }
  };

  static constexpr unsigned SIGNATURE { 0xa2bedef9 };
  
  const size_type capacity_;
  storage_area* storage_;

  void increment(unsigned& idx) const {
    if (++idx == capacity_)
      idx = 0;
  }

  persistent_queue() = delete;
  
  persistent_queue(const persistent_queue& other) = delete;
  persistent_queue& operator=(const persistent_queue& other) = delete;
  
  persistent_queue(persistent_queue&& other) = delete;
  persistent_queue& operator=(persistent_queue&& other) = delete;
};

#endif // PERSISTENT_QUEUE

