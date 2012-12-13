#include "stdinc.hpp"
#include "scoped_handle.hpp"
#include "os.hpp"


namespace
{
struct FileTraits
{
  typedef HANDLE HandleType;
  static void close_fcn( HandleType handle ) { CloseHandle( handle ); }
  static bool is_valid( HandleType handle ) { return handle != 0; };
  static HandleType invalid() { return 0; }
};

typedef scoped_handle< FileTraits > FileHandle;
typedef boost::error_info< struct tag_errno, int > ErrNo;
typedef boost::error_info< struct tag_errstr, std::string > ErrStr;

}

// Note requires shell32.dll version 5.0 or later
bool OsEnsureDirectory( const std::string& path )
{
  // Ok, lets attempt to create the directory
  int res = SHCreateDirectoryExA( 0, path.c_str(), 0 );
  if ( res == ERROR_SUCCESS ) {
    return true;
  } else if ( res == ERROR_ALREADY_EXISTS || res == ERROR_FILE_EXISTS ) {
    // Ok, the directory already exists
    // See if it is a file
    WIN32_FILE_ATTRIBUTE_DATA data;
    if ( !GetFileAttributesExA( path.c_str(), GetFileExInfoStandard, &data ) ) {
      throw OsEnsureDirectoryException() << ErrStr( "GetFileAttributesEx" ) << ErrNo( GetLastError() );
    }
    if ( data.dwFileAttributes &= FILE_ATTRIBUTE_DIRECTORY ) {
      // Ok, it is a directory. Just return false
      return false;
    } else {
      // There is a file. Can't create directory
      throw OsEnsureDirectoryException() << ErrStr( "File exists" ) << ErrNo( GetLastError() );
    }
  } else {
    // A file exists where the directory should be created.
    throw OsEnsureDirectoryException() << ErrStr( "ShCreateDirectoryExA" ) << ErrNo( res );
  }
}

void OsWriteFile( const std::string& filename, const std::vector< uint8_t >& buffer )
{
  // Create (or open) the file with permission to write
  FileHandle handle( CreateFileA( filename.c_str(), FILE_WRITE_DATA, 0, 0, CREATE_ALWAYS, 0, 0 ) ); // Will auto-close
  if ( handle.get() == INVALID_HANDLE_VALUE ) {
    throw OsWriteFileException() << ErrStr( "CreateFileA" ) << ErrNo( GetLastError() );
  }
  // Write the buffer (if it isn't empty)
  if ( !buffer.empty() ) {
    DWORD dwWritten = 0;
    if  ( !WriteFile( handle.get(), &buffer[0], static_cast< DWORD > ( buffer.size() ), &dwWritten, 0 ) ) {
      throw OsWriteFileException() << ErrStr( "WriteFile" ) << ErrNo( GetLastError() );
    }
  }
}

std::string OsConcatPath( const std::string& path, const std::string& filename )
{
  if ( path.size() < 2 || filename.empty() ) {
    throw std::invalid_argument( "Invalid path" );
  }
  std::string ret( path );
  if ( *( path.end()-1 ) != '\\' ) {
    ret.push_back( '\\' );
  }
  return ret + filename;
}

bool OsFileExists( const std::string& filename )
{
  WIN32_FILE_ATTRIBUTE_DATA data;
  BOOL ret = GetFileAttributesExA( filename.c_str(), GetFileExInfoStandard, &data );
  if ( !ret ) {
    // Does not exist
    return false;
  } else if ( data.dwFileAttributes &= FILE_ATTRIBUTE_DIRECTORY  ) {
    return false; // Is a directory
  } else {
    return true; // Ok exists
  }
}

void OsDeleteFile( const std::string& filename )
{
  if ( !DeleteFileA( filename.c_str() ) ) {
    throw OsDeleteFileException() << ErrStr( "DeleteFile" ) << ErrNo( GetLastError() );
  }
}

void OsReadFile( const std::string& filename, std::vector< uint8_t >& buffer )
{
  // Open the existing file for reading
  FileHandle handle( CreateFileA( filename.c_str(), FILE_READ_DATA, 0, 0, OPEN_EXISTING, 0, 0 ) ); // Will auto-close
  if ( handle.get() == INVALID_HANDLE_VALUE ) {
    throw OsReadFileException() << ErrStr( "CreateFileA" ) << ErrNo( GetLastError() );
  }
  // Get file size
  LARGE_INTEGER liSize;
  if (!GetFileSizeEx( handle.get(), &liSize ) ) {
    throw OsReadFileException() << ErrStr( "GetFileSizeEx" ) << ErrNo( GetLastError() );
  }

  if ( liSize.QuadPart >> 32 ) {
    // File must be bigger then 4G. Blow up.
    throw OsReadFileException() << ErrStr( "Too large file" );
  }
  size_t fileSize = static_cast< size_t >( liSize.QuadPart );
  if ( fileSize == 0 ) {
    // Nothing to read
    buffer.clear();
  } else {
    // Resize the buffer
    buffer.resize( fileSize );
    DWORD dwBytesRead = 0;
    if ( !ReadFile( handle.get(), &buffer[0], static_cast< DWORD > ( buffer.size() ), &dwBytesRead, 0 ) ) {
      throw OsReadFileException() << ErrStr( "ReadFile" ) << ErrNo( GetLastError() );
    }
  }
}
