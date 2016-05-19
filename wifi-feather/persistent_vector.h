#ifndef PERSISTENT_VECTOR
#define PERSISTENT_VECTOR

#include <EEPROM.h>

template<class T>
class persistent_vector {
public:
  typedef T value_type;
  typedef std::size_t size_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  
  persistent_vector(int base_address, size_type capacity)
    : capacity_{capacity}
  {
    uint8_t* data = EEPROM.getDataPtr() + base_address;
    storage_ = reinterpret_cast<storage_area*>(data);
            
    if (storage_->signature != SIGNATURE) {
      storage_->signature = SIGNATURE;
      storage_->size = 0;
    }
  }

  static constexpr size_type storage_size(size_type capacity) {
    return sizeof(storage_area::signature)
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

  reference operator[](size_type pos) {
    return storage_->data()[pos];
  }

  const_reference operator[](size_type pos) const {
    return storage_->data()[pos];
  }

  bool push_back(const value_type& value) {
    if (full())
      return false;
    
    storage_->data()[storage_->size] = value;
    ++(storage_->size);
    return true;
  }

  bool push_back(value_type&& value) {
    if (full())
      return false;

    storage_->data()[storage_->size] = std::move(value);
    ++(storage_->size);
    return true;
  }

  bool pop_back() {
    if (empty())
      return false;

    --(storage_->size);
    return true;
  }

private:
  struct storage_area {
    unsigned signature;
    size_type size;
    
    value_type* data() {
      uint8_t* ptr_to_data =
          reinterpret_cast<uint8_t*>(&signature)
          + sizeof(signature)
          + sizeof(size);
      return reinterpret_cast<value_type*>(ptr_to_data);
    }
  };

  static constexpr unsigned SIGNATURE { 0xa2bedef9 };
  
  const size_type capacity_;
  storage_area* storage_;

  persistent_vector() = delete;
  
  persistent_vector(const persistent_vector& other) = delete;
  persistent_vector& operator=(const persistent_vector& other) = delete;
  
  persistent_vector(persistent_vector&& other) = delete;
  persistent_vector& operator=(persistent_vector&& other) = delete;
};

#endif // PERSISTENT_VECTOR

