#include "stdinc.hpp"
#include "crypt.hpp"
#include "cache.hpp"
#include "cacheimpl.hpp"
#include "os.hpp"

#define BOOST_TEST_MODULE CacheTest
#include <boost/test/unit_test.hpp>


typedef std::vector< uint8_t > BinaryBuffer;

// Number of test buffers that will be written as objects
const size_t noOfBuffers = 100; // 100

// Maximum number of bytes of objects stored in the cache
const uint64_t maxSize = 200000; // 200000
const uint64_t reducedMaxSize = 100000; // 100000


struct BuffersFixture
{
  BuffersFixture() {
    const std::string path( "C:\\temp\\test" );
    BOOST_TEST_MESSAGE( "Creating a test directory: " << path );
    bool ret = OsEnsureDirectory( path );
    if ( ret ) {
      BOOST_TEST_MESSAGE( "Directory was created" );
    } else {
      BOOST_TEST_MESSAGE( "Directory already existed" );
    }
    BOOST_TEST_MESSAGE( "Generate object id:s" );
    for ( size_t i = 0; i < noOfBuffers; ++i ) {
      // Generate object id:s between 10 and 29 bytes.
      // Some object id:s may get the same value!
      size_t objectIdSize = ( rand() % 20 + 10 );

      Cache::ObjectId objId( objectIdSize );
      for ( size_t j = 0; j < objectIdSize; ++j ) {
        // Generate numbers between 0 and 0xFF
        objId[j] = ( rand() % 0x100 );
      }
      objectIds_.push_back( objId );
    }

    BOOST_TEST_MESSAGE( "Creating som buffers to play around with" );
    for ( size_t i = 0; i < noOfBuffers; ++i ) {
      // Generate buffers between 1 and 10 000 bytes.
      size_t bufferSize = ( rand() % 10000 + 1 );

      BinaryBuffer buf( bufferSize );
      for ( size_t j = 0; j < bufferSize; ++j ) {
        // Generate numbers between 0 and 0xFF
        buf[j] = ( rand() % 0x100 );
      }
      buffers_.push_back( buf );
    }
  }
  std::vector< BinaryBuffer > buffers_;
  std::vector< Cache::ObjectId > objectIds_;
};

struct CacheFixture : public BuffersFixture
{
  CacheFixture() : objWritten_(0), currSize_(0)
  {
    std::vector< uint8_t > key;

    const std::string dummykey( "dummykey" );
    std::copy( dummykey.begin(), dummykey.end(), back_inserter( key ) );
    BOOST_TEST_MESSAGE( "Creating Cache instance" );
    cache_ = createCache( "c:\\temp\\cache", key );;
    BOOST_REQUIRE( cache_ );
    cache_->setMaxSize( maxSize );
  }

  void WriteObjects() {
    BOOST_TEST_MESSAGE( "Writing objects. Stop writing when cache is full." );

    int n = 0;
    for ( std::vector< BinaryBuffer >::const_iterator it = objectIds_.begin(); it != objectIds_.end(); ++ it, ++n )
    {
      if ( cache_->getCurrentSize() + buffers_[n].size() > maxSize ) {
        // Next object will not fit. Save that for the next test case.
        BOOST_TEST_MESSAGE( "Cache is now filled with " << cache_->getCurrentSize() << " bytes." );
        BOOST_TEST_MESSAGE( "Maximum size of cache is " << maxSize << "." );
        break;
      }
      BOOST_REQUIRE( cache_->writeObject( *it, buffers_[n] ) );
      objWritten_ ++;

      BOOST_REQUIRE( cache_->hasObject( *it ) );
    }

    BOOST_REQUIRE( cache_->getCurrentSize() != 0 );
    currSize_ = cache_->getCurrentSize();
  }

  void ReadObjects() {
    BOOST_TEST_MESSAGE( "Reading objects previously written. Tampering with files, re-reading to prove is is no longer possible. Write them back again." );

    size_t n = 0;
    for ( std::vector< BinaryBuffer >::const_iterator it = objectIds_.begin(); ( it != objectIds_.end() ) && ( n < objWritten_ ) ; ++ it, ++n )
    {
      BinaryBuffer buffer;
      BOOST_REQUIRE( cache_->readObject( *it, buffer ) );
      BOOST_REQUIRE( buffer == buffers_[n] );
    }
    currSize_ =  cache_->getCurrentSize();
  }
  void EraseObjects() {
    BOOST_TEST_MESSAGE( "Erasing objects. Of course we can only erase objects that have not been pruned." );

    for ( std::vector< BinaryBuffer >::const_iterator it = objectIds_.begin(); it != objectIds_.end(); ++ it )
    {
      BinaryBuffer buffer;
      BOOST_REQUIRE( cache_->eraseObject( *it ) );
      BOOST_REQUIRE( !cache_->hasObject( *it ) );
      BOOST_REQUIRE( !OsFileExists( OsConcatPath( "c:\\temp\\cache", Crypt::EncodeFilenameFromBuffer( *it, ".CDF" ) ) ) );
    }
    BOOST_REQUIRE( cache_->getCurrentSize() == 0 );
    currSize_ =  cache_->getCurrentSize();
  }

  ~CacheFixture()
  {
    delete cache_;
  }
  Cache *cache_;
  size_t objWritten_;
  uint64_t currSize_;
};

BinaryBuffer GetTestBuffer()
{
  const std::string dummybuffer( "23\nfad&hAppd" );
  BinaryBuffer buffer;
  std::copy( dummybuffer.begin(), dummybuffer.end(), back_inserter( buffer ) );
  buffer[4] = 0; // Test a null byte
  return buffer;
}

BOOST_FIXTURE_TEST_SUITE(BuffersTests, BuffersFixture);

BOOST_AUTO_TEST_CASE( TestSha1Hash )
{
  BOOST_TEST_MESSAGE( "Sha1Hash testing. Generated modified buffers and detecting changes." );
  // Do slight modifications to buffers and see that we can detect all changes
  for ( std::vector< BinaryBuffer >::const_iterator it = buffers_.begin(); it != buffers_.end(); ++ it ) {
    const BinaryBuffer& buffer = *it;
    // Copy buffer
    BinaryBuffer modifyBuffer( buffer );
    // Do between 1 to 16 changes
    int noChanges = rand() % 0x10 + 1;
    for ( int i = 0; i < noChanges; i++ ) {
      size_t modifyIndex = rand() % modifyBuffer.size();
      ++ modifyBuffer[ modifyIndex ];
    }
    // Make sure the hashes are different
    BOOST_REQUIRE( Crypt::Sha1Hash( buffer ) != Crypt::Sha1Hash( modifyBuffer ) );
  }
}

BOOST_AUTO_TEST_CASE( TestWriteAndReadFiles )
{
  const std::string path( "C:\\temp\\test\\" );
  BOOST_TEST_MESSAGE( "Writing " << buffers_.size() << " files to " << path );

  int n = 0;
  for ( std::vector< BinaryBuffer >::const_iterator it = buffers_.begin(); it != buffers_.end(); ++ it, ++n ) {
    std::ostringstream ss;
    ss << "testfile" << n;

    std::string filename( OsConcatPath( path, ss.str() ) );
    OsWriteFile( filename, *it );

    BOOST_REQUIRE( OsFileExists( filename ) );
  }

  BOOST_TEST_MESSAGE( "Reading " << buffers_.size() << " files and comparing with original buffers." );
  for ( size_t n = 0; n < buffers_.size(); ++n ) {
    std::ostringstream ss;
    ss << "testfile" << n;
    BinaryBuffer buf;
    std::string filename( OsConcatPath( path, ss.str() ) );
    OsReadFile( filename, buf );
    BOOST_REQUIRE( buf == buffers_[n] );
  }

  BOOST_TEST_MESSAGE( "Deleting files" );
  for ( size_t n = 0; n < buffers_.size(); ++n ) {
    std::ostringstream ss;
    ss << "testfile" << n;
    std::string filename( OsConcatPath( path, ss.str() ) );
    OsDeleteFile( filename );

    BOOST_REQUIRE( !OsFileExists( filename ) );
  }
}

BOOST_AUTO_TEST_CASE( TestEncryptBuffer )
{
  BOOST_TEST_MESSAGE( "Encrypting and decrypting " << buffers_.size() << " buffers." );

  BinaryBuffer key( GetTestBuffer() );
  for ( std::vector< BinaryBuffer >::const_iterator it = buffers_.begin(); it != buffers_.end(); ++ it ) {

    // Encrypt buffer
    BinaryBuffer buffer( *it );
    Crypt::Rc4EncryptDecrypt( key, buffer );
    BOOST_REQUIRE( buffer != *it );


    // Decrypt buffer. Compare with original buffer
    Crypt::Rc4EncryptDecrypt( key, buffer );
    BOOST_REQUIRE( buffer == *it );
  }
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_FIXTURE_TEST_SUITE(CacheTestSuite, CacheFixture);

BOOST_AUTO_TEST_CASE(TestReadWriteEraseObjects)
{
  WriteObjects();
  ReadObjects();
  EraseObjects();
}

BOOST_AUTO_TEST_CASE( TestTamperObjects )
{
  WriteObjects();

  size_t n = 0;
  for ( std::vector< BinaryBuffer >::const_iterator it = objectIds_.begin(); ( it != objectIds_.end() ) && ( n < objWritten_ ) ; ++ it, ++n )
  {
    // Tamper with the file
    std::string filename( OsConcatPath( "c:\\temp\\cache", Crypt::EncodeFilenameFromBuffer( *it, ".CDF" ) ) );

    BinaryBuffer origBuffer;
    OsReadFile( filename, origBuffer );

    BinaryBuffer modifyBuffer( origBuffer );
    // Do between 1 to 16 changes
    int noChanges = rand() % 0x10 + 1;
    for ( int i = 0; i < noChanges; i++ ) {
      size_t modifyIndex = rand() % modifyBuffer.size();
      ++ modifyBuffer[ modifyIndex ];
    }

    BOOST_REQUIRE( modifyBuffer != origBuffer );

    // Write modified buffer
    OsWriteFile( filename, modifyBuffer );

    // See if we can read the object. It is (remotely) possible
    // that a modification goes undetected.
    BOOST_WARN( !cache_->readObject( *it, origBuffer ) );
    BOOST_WARN( !cache_->hasObject( *it ) );

    // And current size of the cache should be reduced.
    BOOST_WARN( cache_->getCurrentSize() == currSize_ - origBuffer.size() );

    // OK, write the original object again
    BOOST_REQUIRE( cache_->writeObject( *it, origBuffer ) );
    BOOST_REQUIRE( cache_->getCurrentSize() == currSize_ );
  }
  BOOST_MESSAGE( "Have read, tampered with, re-read and re-written " << n << " objects." );
}

size_t nPruneNext = 0;
/*
  BOOST_AUTO_TEST_CASE( TestWritePruning )
  {
  BOOST_MESSAGE( "Now we start over-filling the cache to check that files are being pruned." );

  size_t nBuffer = objWritten;
  size_t nPrunedBegin = 0;
  size_t nPrunedEnd = 0;

  for ( std::vector< BinaryBuffer >::const_iterator it = objectIds_.begin() + objWritten; it != objectIds_.end(); ++it, ++nBuffer )
  {
  size_t objSize = buffers_[nBuffer].size();

  BOOST_MESSAGE( "Storing object with size " << objSize );

  uint64_t mustPruneSize = 0;
  if ( cache_->getCurrentSize() + buffers_[nBuffer].size() > maxSize ) {
  mustPruneSize = cache_->getCurrentSize() + buffers_[nBuffer].size() - maxSize;
  BOOST_MESSAGE( "Need to free " << mustPruneSize );
  }
  uint64_t willPruneSize = 0;

  if ( mustPruneSize > 0 ) {
  nPrunedBegin = nPruneNext;
  // Check which objects will be pruned.
  while ( mustPruneSize > 0 ) {
  size_t bufferSize = buffers_[ nPruneNext ++ ].size();
  BOOST_MESSAGE( "Pruning object " << nPruneNext << " will free " << bufferSize );
  willPruneSize += bufferSize;
  if ( mustPruneSize < bufferSize ) {
  mustPruneSize = 0;
  } else {
  mustPruneSize -= bufferSize;
  }
  }
  nPrunedEnd = nPruneNext;

  }
  uint64_t prevSize = cache_->getCurrentSize();

  // Write an object that will prune the first object that was written to the cache
  BOOST_REQUIRE( cache_->writeObject( *it, buffers_[nBuffer] ) );

  // Check that the pruned objects are missing
  if ( willPruneSize > 0 ) {
  for ( size_t i = nPrunedBegin; i < nPrunedEnd; ++ i ) {
  const Cache::ObjectId& prunedObjectId = objectIds_[ i ];
  BOOST_REQUIRE( !cache_->hasObject( prunedObjectId ) );

  // Check that the file is gone from the file system
  BOOST_REQUIRE( !OsFileExists( OsConcatPath( "c:\\temp\\cache", Crypt::EncodeFilenameFromBuffer( prunedObjectId, ".CDF" ) ) ) );
  }

  // Check that the new cache size is correct
  BOOST_REQUIRE( prevSize + buffers_[nBuffer].size() - willPruneSize == cache_->getCurrentSize() );
  }

  BOOST_MESSAGE( "Cache size after writing object " << cache_->getCurrentSize() );

  BOOST_REQUIRE( cache_->getCurrentSize() < maxSize );
  }

  }

  BOOST_AUTO_TEST_CASE( ReduceMaxSize )
  {
  BOOST_TEST_MESSAGE( "Reducing the cache size from " << maxSize << " to " << reducedMaxSize );

  uint64_t mustPruneSize = 0;
  if ( cache_->getCurrentSize() > reducedMaxSize ) {
  mustPruneSize = cache_->getCurrentSize() - reducedMaxSize;
  }
  BOOST_MESSAGE( "Need to free " << mustPruneSize );

  uint64_t willPruneSize = 0;

  size_t nPrunedBegin = nPruneNext;
  size_t nPrunedEnd = nPruneNext;

  // Check which objects will be pruned.
  while ( mustPruneSize > 0 ) {
  size_t bufferSize = buffers_[ nPruneNext ++ ].size();
  BOOST_MESSAGE( "Pruning object " << nPruneNext << " will free " << bufferSize );
  willPruneSize += bufferSize;
  if ( mustPruneSize < bufferSize ) {
  mustPruneSize = 0;
  } else {
  mustPruneSize -= bufferSize;
  }
  }
  nPrunedEnd = nPruneNext;

  uint64_t prevSize = cache_->getCurrentSize();
  cache_->setMaxSize( reducedMaxSize );

  BOOST_TEST_MESSAGE( "Cache size after reduction " << cache_->getCurrentSize() );
  BOOST_REQUIRE( prevSize - willPruneSize == cache_->getCurrentSize() );
  }
*/
BOOST_AUTO_TEST_SUITE_END();

/*
  BOOST_AUTO_TEST_CASE( TestDestroy )
  {
  BOOST_TEST_MESSAGE( "Destroying Cache instance. Instance will save meta data." );

  delete cache_;
  cache_s = 0;

  // Maybe not much to check here.
  BOOST_REQUIRE( cache_s == 0 );
  }

  /*
  BOOST_AUTO_TEST_CASE( TestReCreateWrongKey )
  {
  BOOST_TEST_MESSAGE( "ReCreating Cache instance with the wrong encryption key. Cache should appear empty." );

  const std::string dummykey( "dummykey2" );

  std::vector< uint8_t > key;
  key.assign( dummykey.begin(), dummykey.end() );

  cache_s = createCache( "c:\\temp\\cache", key );
  BOOST_REQUIRE( cache_s );

  BOOST_REQUIRE( cache_s->getCurrentSize() == 0 );

  // Leak this instance to avoid it destroying the old cache. We will try
  // to re-create a new instance and load the cache again.
  // NOTE: This may cause a leak to be reported by CRT when running the
  // unit test
  }

  BOOST_AUTO_TEST_CASE( TestReCreate )
  {
  BOOST_TEST_MESSAGE( "ReCreating Cache instance. Will load meta data from file" );

  const std::string dummykey( "dummykey" );

  std::vector< uint8_t > key;
  std::copy( dummykey.begin(), dummykey.end(), back_inserter( key ) );

  cache_s = createCache( "c:\\temp\\cache", key );
  BOOST_REQUIRE( cache_s );
  cache_s->setMaxSize( reducedMaxSize );

  BOOST_REQUIRE( cache_s->getCurrentSize() );

  BOOST_TEST_MESSAGE( "The cachse size is now " << cache_s->getCurrentSize() );
  }

  BOOST_AUTO_TEST_CASE( TestEraseObject )
  {
  BOOST_TEST_MESSAGE( "Erasing objects. Of course we can only erase objects that have not been pruned." );

  for ( std::vector< BinaryBuffer >::const_iterator it = objectIds_s.begin() + nPruneNext; it != objectIds_s.end(); ++ it )
  {
  BinaryBuffer buffer;
  BOOST_REQUIRE( cache_s->eraseObject( *it ) );
  BOOST_REQUIRE( !cache_s->hasObject( *it ) );
  BOOST_REQUIRE( !OsFileExists( OsConcatPath( "c:\\temp\\cache", Crypt::EncodeFilenameFromBuffer( *it, ".CDF" ) ) ) );
  }
  BOOST_REQUIRE( cache_s->getCurrentSize() == 0 );
  }

*/
