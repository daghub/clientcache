#ifndef __CACHE_HPP__
#define __CACHE_HPP__

class Cache
{
 public:
  typedef std::vector< uint8_t > ObjectId;
  virtual ~Cache() {}
  virtual bool hasObject( const ObjectId& obj_id ) = 0;
  virtual bool readObject( const ObjectId& obj_id, std::vector< uint8_t >& result ) = 0;
  virtual bool writeObject( const ObjectId& obj_id, const std::vector< uint8_t >& value ) = 0;
  virtual bool eraseObject( const ObjectId &obj_id ) = 0;
  virtual void setMaxSize( uint64_t max_size ) = 0;
  virtual uint64_t getCurrentSize() = 0;

  static Cache* createCache( const std::string&path, const std::vector< uint8_t >& encryption_key );

};

Cache* createCache( const std::string& path, const std::vector< uint8_t >& encryption_key );


#endif // __CACHE_HPP__
