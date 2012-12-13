#ifndef __CACHEIMPL_HPP__
#define __CACHEIMPL_HPP__

#include "cache.hpp"

const std::string fileExtension = ".CDF";
const std::string metaDataFilename = "cache.db";

class CacheImpl : public Cache
{
 public:
  CacheImpl( const std::string& path, const std::vector< uint8_t >& encryption_key );
  virtual ~CacheImpl();
  virtual bool hasObject( const ObjectId& obj_id );
  virtual bool readObject( const ObjectId& obj_id, std::vector< uint8_t >& result );
  virtual bool writeObject( const ObjectId& obj_id, const std::vector< uint8_t >& value );
  virtual bool eraseObject( const ObjectId& obj_id );
  virtual void setMaxSize( uint64_t max_size );
  virtual uint64_t getCurrentSize();

 private:
  struct CacheObject
  {
    uint32_t size_;
    boost::posix_time::ptime accessTime_;
  };

  void LoadMetaData();
  void SaveMetaData();

  std::auto_ptr< CacheObject > RemoveFromObjects( const ObjectId& obj_id );

  bool RemoveFromObjectsAndAccessTimes( const ObjectId& obj_id );
  void AddToObjectsAndAccessTimes( const ObjectId& obj_id, const CacheObject& cacheObject );

  void PruneObjects( uint64_t maxCacheSize );

  const std::string path_;
  const std::vector< uint8_t > encryptionKey_;

  typedef boost::unordered_map< ObjectId, CacheObject > HashMap;
  HashMap objects_;

  typedef std::multimap< boost::posix_time::ptime, ObjectId > TimeMap;
  TimeMap accessTimes_;

  uint64_t maxSize_;
  uint64_t currSize_;
};

#endif // __CACHEIMPL_HPP__
