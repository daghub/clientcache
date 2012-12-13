#ifndef __CRYPT_HPP__
#define __CRYPT_HPP__

namespace Crypt
{
std::string EncodeFilenameFromBuffer( const std::vector< uint8_t > buffer, const std::string& fileExtension );
// bool DecodeBufferFromFilename( const std::string& filename, std::vector< uint8_t >& buffer, const std::string& fileExtension );

typedef boost::array< uint8_t, 20 > Sha1HashValue; // SHA1 is 160 bit
Sha1HashValue Sha1Hash( const std::vector< uint8_t >& buffer );

// Does a hash "in-place" into a buffer
template < typename InIter, typename OutIter >
void Sha1Hash( InIter begin, InIter end, OutIter out )
{
  // TODO: Possible improvement is to create hash in chunks
  // to avoid having to allocate a large buffer.
  std::vector< uint8_t > buffer( begin, end );
  Sha1HashValue value( Sha1Hash( buffer ) );
  std::copy( value.begin(), value.end(), out );
}


void Rc4EncryptDecrypt( const std::vector< uint8_t >& key,  std::vector< uint8_t >& buffer );

class Exception: public boost::exception, public std::exception {};
std::string Base64Encode( const std::vector< uint8_t >& buffer );
std::vector< uint8_t > Base64Decode( const std::string& in );


}

#endif // __CRYPT_HPP__
