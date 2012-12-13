#ifndef __OS_HPP__
#define __OS_HPP__

#include "scoped_handle.hpp"


/**
   This function checks to see if a directory exists. If it does
   it returns without further action. If a file exists with the
   directory name then an OsFileException is thrown. If neither
   a file nor a directory exists, a new directory is created.
   Will create all intermediate directories as well.
   @param path The path to ensure a directory
   @return true if a directory was created, otherwise false
*/
bool OsEnsureDirectory( const std::string& path );

void OsWriteFile( const std::string& filename, const std::vector< uint8_t >& buffer );
void OsReadFile( const std::string& filename, std::vector< uint8_t >& buffer );
std::string OsConcatPath( const std::string& path, const std::string& filename );
bool OsFileExists( const std::string& filename );
void OsDeleteFile( const std::string& filename );
void OsGetFileInfo( const std::string& filename, boost::posix_time::ptime& accessTime, uint64_t& size );
//boost::posix_time::ptime OsGetFileAccessedTime( const std::string& filename );

//namespace
//{
//struct FindTraits
//{
//    typedef HANDLE HandleType;
//    static void close_fcn( HandleType handle ) { FindClose( handle ); }
//    static bool is_valid( HandleType handle ) { return handle != 0; };
//    static HandleType invalid() { return 0; }
//};
//
//typedef scoped_handle< FindTraits > FindHandle;
//}
//
//template < typename CallBack >
//void IterateFiles( const std::string& path, const std::string& extension, CallBack cb )
//{
//      std::string filePattern( OsConcatPath( path, "*" + extension ) );
//      WIN32_FIND_DATAA data;
//      FindHandle handle( FindFirstFileA( filePattern.c_str(), &data ) );
//      if ( handle.get() ) {
//              cb( data.cFileName );
//              while ( FindNextFileA( handle.get(), &data ) ) {
//                      cb( data.cFileName );
//              }
//      }
//}

/**
 */

class OsFileException : public boost::exception, public std::exception {};
class OsEnsureDirectoryException : public OsFileException {};
class OsWriteFileException : public OsFileException {};
class OsReadFileException : public OsFileException {};
class OsDeleteFileException : public OsFileException{};
class OsGetFileInfoException : public OsFileException{};


#endif // __OS_HPP__
