#include "stdinc.hpp"
#include "scoped_handle.hpp"
#include "crypt.hpp"

namespace
{
struct BioTraits
{
  typedef BIO* HandleType;
  static void close_fcn( HandleType handle ) { BIO_free_all( handle ); }
  static bool is_valid( HandleType handle ) { return handle != 0; };
  static HandleType invalid() { return 0; }
};

typedef scoped_handle< BioTraits > BioHandle;

typedef boost::error_info< struct tag_errno,int > ErrNo;
typedef boost::error_info< struct tag_errstr,char* > ErrStr;
}

namespace Crypt
{

std::string Base64Encode( const std::vector< uint8_t >& buffer )
{
  std::string ret;
  if ( buffer.empty() ) {
    // Nothing to encode
    return ret;
  }

  BIO* bmem = 0;

  BioHandle b64( BIO_new( BIO_f_base64() ) ); // Auto-releases
  if ( !b64.get() ) {
    throw Exception() << ErrStr( "Base64Encode: Can't BIO_new, BIO_f_base64" ) << ErrNo( ERR_get_error() );
  }

  bmem = BIO_new( BIO_s_mem() ); // Will free when b64 frees
  if ( !bmem ) {
    throw Exception() << ErrStr( "Base64Encode: Can't BIO_new, BIO_s_mem") << ErrNo( ERR_get_error() );
  }

  b64.get() = BIO_push( b64.get(), bmem );

  if ( !BIO_write( b64.get(), &buffer[0], static_cast< int > ( buffer.size() ) ) ) {
    throw Exception() << ErrStr( "Base64Encode: Can't BIO_write" ) << ErrNo( ERR_get_error() );
  }
  if (!BIO_flush( b64.get() ) ) {
    throw Exception() << ErrStr( "Base64Encode: Can't BIO_flush" ) << ErrNo( ERR_get_error() );
  }

  BUF_MEM* bptr = 0;
  BIO_get_mem_ptr( b64.get(), &bptr );

  // Copy to return buffer but omit trailing ASCII code 10
  ret.resize( bptr->length - 1 );
  ret.assign( bptr->data, bptr->data + bptr->length - 1 );

  return ret;
}

std::vector< uint8_t > Base64Decode( const std::string& in )
{
  std::vector< uint8_t > ret;
  if ( in.empty() ) {
    // Nothing to decode
    return ret;
  }

  // Must copy string to vector but string is not guaranteed to be
  // stored in a contiguos buffer
  std::vector< uint8_t > buffer;
  buffer.assign( in.begin(), in.end() );
  ret.resize( buffer.size() );
  buffer.push_back( 10 ); // Add trailing ascii code 10

  BioHandle bmem( BIO_new_mem_buf( const_cast< uint8_t* >( &buffer[0] ), static_cast< int > ( buffer.size() ) ) );

  BIO* b64 = 0;
  b64 = BIO_new( BIO_f_base64() );
  bmem.get() = BIO_push( b64, bmem.get() );

  int read = BIO_read( bmem.get(),&ret[0], static_cast< int >( ret.size() ) );
  if ( !read ) {
    throw Exception() << ErrStr( "Base64Decode: Can't BIO_read" ) << ErrNo( ERR_get_error() );
  }

  ret.resize( read );

  return ret;
}

Sha1HashValue Sha1Hash( const std::vector< uint8_t >& buffer )
{
  if ( buffer.empty() ) {
    throw std::invalid_argument( "Can't hash an empty buffer" );
  }
  Sha1HashValue ret;

  SHA_CTX ctx;
  if ( !SHA1_Init( &ctx ) ) {
    throw Exception() << ErrStr( "Sha1Hash: SHA1_Init" ) << ErrNo( ERR_get_error() );
  }

  if ( !SHA1_Update( &ctx, &buffer[0], buffer.size() ) ) {
    throw Exception() << ErrStr( "Sha1Hash: SHA1_Update" ) << ErrNo( ERR_get_error() );
  }

  if ( !SHA1_Final( ret.c_array(), &ctx ) ) {
    throw Exception() << ErrStr( "Sha1Hash: SHA1_Update" ) << ErrNo( ERR_get_error() );
  }

  return ret;
}


void Rc4EncryptDecrypt( const std::vector< uint8_t >& key,  std::vector< uint8_t >& buffer )
{
  if ( key.empty() || buffer.empty() ) {
    throw std::invalid_argument( "Rc4EncryptDecrypt, empty key or buffer" );
  }
  RC4_KEY rc4Key;
  RC4_set_key( &rc4Key, static_cast< int >(  key.size() ), static_cast< const unsigned char* >( &key[0] ) );
  RC4( &rc4Key, static_cast< int >( buffer.size() ), static_cast< const unsigned char* > (&buffer[0]), static_cast< unsigned char* > (&buffer[0]) );
}

std::string EncodeFilenameFromBuffer( const std::vector< uint8_t > buffer, const std::string& fileExtension )
{
  std::ostringstream ss;
  ss << std::setbase(16);
  for( std::vector< uint8_t >::const_iterator it = buffer.begin(); it != buffer.end(); ++it ) {
    ss << std::setw(2) << std::setfill('0') << ((unsigned int) *it );
  }
  return ss.str() + fileExtension;
}

//bool DecodeBufferFromFilename( const std::string& filename, std::vector< uint8_t >& buffer, const std::string& fileExtension )
//{
//      if ( filename.size() < fileExtension.size() + 1 ) {
//              return false;
//      }
//
//      if ( filename.substr( filename.size() - fileExtension.size(), fileExtension.size() ) != fileExtension ) {
//              return false;
//      }
//
//      std::string filenameNoExtension( filename.substr( 0, filename.size() - fileExtension.size() ) );
//      if ( filenameNoExtension.size() % 2 ) {
//              return false; // Should be even number of chars
//      }
//      for ( size_t i = 0; i < filenameNoExtension.size(); i = i + 2 ) {
//              std::string byte( filenameNoExtension.substr( i, 2 ) );
//              int byteNo = strtol( byte.c_str(), 0, 16 );
//              buffer.push_back( byteNo );
//      }
//      return true;
//}

}
