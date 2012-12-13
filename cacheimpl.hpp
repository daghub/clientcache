#ifndef __CACHEIMPL_HPP__
#define __CACHEIMPL_HPP__

#include "cache.hpp"

const std::string fileExtension = ".CDF";
const std::string metaDataFilename = "cache.db";

namespace intrusive = boost::intrusive;

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
  typedef intrusive::list_base_hook<
   intrusive::link_mode< intrusive::auto_unlink> > auto_unlink_hook;

  struct CacheObject : public auto_unlink_hook
  {
    boost::unordered_map< ObjectId, CacheObject >::pointer mapElement_;
    uint32_t size_;
  };

  void LoadMetaData();
  void SaveMetaData();

  bool RemoveFromObjects( const ObjectId& obj_id );
  void AddToObjects( const ObjectId& obj_id, CacheObject& cacheObject );

  void PruneObjects( uint64_t maxCacheSize );

  const std::string path_;
  const std::vector< uint8_t > encryptionKey_;

  typedef boost::unordered_map< ObjectId, CacheObject > HashMap;
  HashMap objects_;

  typedef intrusive::list< CacheObject, intrusive::constant_time_size< false > > PruneList;
  PruneList pruneList_;

  uint64_t maxSize_;
  uint64_t currSize_;
};

#endif // __CACHEIMPL_HPP__
