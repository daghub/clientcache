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

/**
 */
class OsFileException : public boost::exception, public std::exception {};
class OsEnsureDirectoryException : public OsFileException {};
class OsWriteFileException : public OsFileException {};
class OsReadFileException : public OsFileException {};
class OsDeleteFileException : public OsFileException{};
class OsGetFileInfoException : public OsFileException{};


#endif // __OS_HPP__
