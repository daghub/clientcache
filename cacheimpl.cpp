#include "stdinc.hpp"
#include "os.hpp"
#include "cacheimpl.hpp"
#include "crypt.hpp"

#define CATCH_RETURN()                                                  \
  catch( boost::exception& ex ) {                                       \
    std::clog << "boost::exception caught in " << __FILE__ << " line " << __LINE__ << "\n" <<  diagnostic_information( ex ) << std::endl; \
    return false;                                                       \
  } catch( std::exception& ex ) {                                       \
    std::clog << "std::exception caught in " << __FILE__ << " line " << __LINE__ << "\n" <<  ex.what() << std::endl; \
    return false;                                                       \
  }                                                                     \
  return true;

#define CATCH()                                                         \
  catch( boost::exception& ex ) {                                       \
    std::clog << "boost::exception caught in " << __FILE__ << " line " << __LINE__ << "\n" <<  diagnostic_information( ex ) << std::endl; \
  } catch( std::exception& ex ) {                                       \
    std::clog << "std::exception caught in " << __FILE__ << " line " << __LINE__ << "\n" <<  ex.what() << std::endl; \
  }

CacheImpl::CacheImpl( const std::string& path, const std::vector< uint8_t >& encryption_key )
    : path_( path ), encryptionKey_( encryption_key ), maxSize_( 500000000 ), currSize_( 0 )
{
  // Create the cache directory
  OsEnsureDirectory( path );
  LoadMetaData();
}


CacheImpl::~CacheImpl()
{
  try {
    SaveMetaData();
  } CATCH();
}

bool CacheImpl::hasObject( const ObjectId& obj_id )
{
  try {
    return objects_.find( obj_id ) != objects_.end();
  } CATCH_RETURN();
}


bool CacheImpl::readObject( const ObjectId& obj_id, std::vector< uint8_t >& result )
{
  try {
    if ( !hasObject( obj_id ) ) {
      return false;
    }
    std::string filename( OsConcatPath( path_, Crypt::EncodeFilenameFromBuffer( obj_id, fileExtension ) ) );

    std::vector< uint8_t > rawBuffer;
    OsReadFile( filename, rawBuffer );

    // Now, decrypt the buffer
    Crypt::Rc4EncryptDecrypt( encryptionKey_, rawBuffer );

    // The raw buffer should contain a heading that
    // contains the hash value and the object id.
    size_t headerSize( sizeof( Crypt::Sha1HashValue ) + obj_id.size() );
    if ( rawBuffer.size() <= headerSize ) {
      // The buffer is too small to event hold the hash code.
      // Invalid.
      RemoveFromObjects( obj_id );
      return false;
    }
    // First we should have the hash
    Crypt::Sha1HashValue hash;
    std::vector< uint8_t >::iterator iter;
    iter = rawBuffer.begin() + sizeof( Crypt::Sha1HashValue );
    std::copy( rawBuffer.begin(), iter , hash.begin() );

    // Then we should have the object id
    ObjectId bufferObjectId( obj_id.size() );
    std::copy( iter, iter + bufferObjectId.size(), bufferObjectId.begin() );

    // Now check if the object id:s match
    if ( bufferObjectId != bufferObjectId ) {
      // Hmm object may be tampered with.
      RemoveFromObjects( obj_id );
      return false;
    }

    // Set the correct size of the resulting object
    result.resize( rawBuffer.size() - headerSize );
    iter = iter + bufferObjectId.size();
    std::copy( iter, rawBuffer.end(), result.begin() );

    // Now check hash
    if ( Crypt::Sha1Hash( result ) != hash ) {
      RemoveFromObjects( obj_id );
      return false;
    }

    return true;
  } CATCH_RETURN();
}

void CacheImpl::AddToObjects( const ObjectId& obj_id, CacheObject& cacheObject )
{
  RemoveFromObjects( obj_id ); // Remove it in case it is aleady there

  // Make sure it will fit
  PruneObjects( maxSize_ - cacheObject.size_ );
  currSize_ += cacheObject.size_;

  objects_[ obj_id ] = cacheObject;
  // Reach into the unordered map to get a reference to actual element stored
  HashMap::value_type &elem = *objects_.find( obj_id );
  // Store a pointer to that element inside the element itself. Thay way
  // we can find the element (key + value) from the value (CacheObject)
  // that is stored inside the prune list
  elem.second.mapElement_ = &elem;
  // Add to prune list
  pruneList_.push_back( elem.second );

  // Make sure structures are in sync
  assert( objects_.size() == pruneList_.size() );
}

bool CacheImpl::writeObject( const ObjectId& obj_id, const std::vector< uint8_t >& value )
{
  try {
    if ( value.size() > maxSize_ ) {
      // There is no way this object will fit in the cache
      throw std::invalid_argument( "Too large object" );
    }

    std::string filename( OsConcatPath( path_, Crypt::EncodeFilenameFromBuffer( obj_id, fileExtension ) ) );

    // Calculate hash signature
    Crypt::Sha1HashValue hash( Crypt::Sha1Hash( value ) );

    // Write a buffer that contains the signature + the object
    std::vector< uint8_t >::iterator iter;
    std::vector< uint8_t > buffer( sizeof( Crypt::Sha1HashValue ) + obj_id.size() + value.size() );
    iter = std::copy( hash.begin(), hash.end(), buffer.begin() );
    iter = std::copy( obj_id.begin(), obj_id.end(), iter );
    iter = std::copy( value.begin(), value.end(), iter );

    // Now, encrypt the buffer
    Crypt::Rc4EncryptDecrypt( encryptionKey_, buffer );

    // And write the file
    OsWriteFile( filename, buffer );

    // Update internal structures after the write, because
    // we don't want them updated in case the write throws
    // an exception

    CacheObject obj;
    obj.size_ = static_cast< uint32_t > ( value.size() );

    AddToObjects( obj_id, obj );

    return true;
  } CATCH_RETURN();
}

bool CacheImpl::RemoveFromObjects( const ObjectId& obj_id )
{
  // Make sure structures are in sync
  assert( objects_.size() == pruneList_.size() );
  HashMap::iterator it = objects_.find( obj_id );
  if ( it == objects_.end() ) {
    return false;
  }

  currSize_ -= it->second.size_;
  // Remove object from linked list
  it->second.unlink();
  // Remove object from unordered map
  objects_.erase( it );

  return true;
}

void CacheImpl::PruneObjects( uint64_t maxCacheSize )
{
  // Prune objects, oldest first.
  PruneList::iterator it = pruneList_.begin();

  while ( ( it != pruneList_.end() ) && ( maxCacheSize < getCurrentSize() ) ) {
    std::string filename( OsConcatPath( path_, Crypt::EncodeFilenameFromBuffer( it->mapElement_->first, fileExtension ) ) );

    PruneList::iterator removeIter( it );
    ++ it;

    RemoveFromObjects( removeIter->mapElement_->first );

    // The following could thrown an exception if the file is no longer available
    // The user could have restarted the cache after removing a file manually.
    // This is an "ok" error case
    try {
      OsDeleteFile( filename );
    } catch ( OsDeleteFileException& )
    {
    }
  }

  // Make sure structures are in sync
  assert( objects_.size() == pruneList_.size() );
}

bool CacheImpl::eraseObject( const ObjectId& obj_id )
{
  try {
    if ( !hasObject( obj_id ) ) {
      return false;
    }
    std::string filename( OsConcatPath( path_, Crypt::EncodeFilenameFromBuffer( obj_id, fileExtension ) ) );
    try {
      OsDeleteFile( filename );
    } catch ( OsDeleteFileException& ) {
    }
    RemoveFromObjects( obj_id );

    // Make sure structures are in sync
    assert( objects_.size() == pruneList_.size() );

    return true;
  } CATCH_RETURN();
}

void CacheImpl::setMaxSize( uint64_t max_size )
{
  try {
    PruneObjects( max_size );
    maxSize_ = max_size;
  } CATCH();
}

uint64_t CacheImpl::getCurrentSize()
{
  return currSize_;
}

void CacheImpl::SaveMetaData()
{
  std::vector< uint8_t > out;
  if ( !pruneList_.empty() ) {
    // Allocate space for hash
    out.resize( sizeof ( Crypt::Sha1HashValue ) );

    std::ostringstream oss;
    for ( PruneList::const_iterator it = pruneList_.begin(); it != pruneList_.end(); ++ it ) {
      CacheObject& cacheObject( it->mapElement_->second );
      // Write size and object id
      oss <<  cacheObject.size_ << " " << Crypt::Base64Encode( it->mapElement_->first ) << " ";
    }

    const std::string& metaData( oss.str() );
    std::copy( metaData.begin(), metaData.end(), back_inserter( out ) );

    // Calculate hash. Put it first in the out buffer.
    std::vector< uint8_t >::iterator objectDataBegin( out.begin() + sizeof ( Crypt::Sha1HashValue ) );
    Crypt::Sha1Hash( objectDataBegin , out.end(), out.begin() );

    // Encrypt
    Crypt::Rc4EncryptDecrypt( encryptionKey_, out );
  }

  // Save to file
  OsWriteFile( OsConcatPath( path_, metaDataFilename ), out );
}

void CacheImpl::LoadMetaData()
{
  // Clear previous data
  objects_.clear();
  pruneList_.clear();

  std::vector< uint8_t > in;
  std::string fullPath( OsConcatPath( path_, metaDataFilename ) );

  if ( OsFileExists( fullPath ) ) {
    OsReadFile( fullPath, in );
    if ( in.size() > sizeof( Crypt::Sha1HashValue ) ) {
      // Decrypt
      Crypt::Rc4EncryptDecrypt( encryptionKey_, in );

      std::vector< uint8_t >::iterator objectDataBegin( in.begin() + sizeof( Crypt::Sha1HashValue ) );

      Crypt::Sha1HashValue hash;
      std::copy( in.begin(), objectDataBegin, hash.begin() );

      // First, check the has to see that the file is intact.
      Crypt::Sha1HashValue calculatedHash;
      Crypt::Sha1Hash( objectDataBegin, in.end(), calculatedHash.begin() );

      if ( hash == calculatedHash ) {
        // Ok, go on
        std::string metaData( objectDataBegin, in.end() );
        std::istringstream is( metaData );

        while ( is ) {
          CacheObject cacheObj;
          std::string encodedObjId;

          if ( ( is >> cacheObj.size_ ) &&
               ( is >> encodedObjId ) ) {
            ObjectId objId( Crypt::Base64Decode( encodedObjId ) );

            if ( cacheObj.size_ <= maxSize_ ) {
              // This object will fit, at least after pruning.
              AddToObjects( objId, cacheObj );
            }
          }
        }
      }
    }
  }
}


static CacheImpl::Cache* cache_s = 0;
Cache* createCache( const std::string& path, const std::vector< uint8_t >& encryption_key )
{
  return cache_s = new CacheImpl( path, encryption_key );
}
