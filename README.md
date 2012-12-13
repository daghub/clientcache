======================================================================
Client Cache demonstration code
======================================================================
This was the assignment that this demostration code solves:

======== Client-side cache
Background
The client is built in C++. There are both platform-specific and shared
parts of the code. You have been tasked with improving and implementing a
client-side cache which will be used to store objects of several different types
(e.g. images, tracks, artist biographies).

Since your code should be usable across our supported operating systems, you
will need to make the code portable. The way we do this is to add a thin
wrapper around non-portable functions, where the build process will compile
the code with the correct implementation linked in. For instance, you may
assume the function OsDeleteFile() exists if you include "os.h" and that it takes
a string containing the file to delete.

The cache should scale well for at least 50000 items.

Interface
This is the interface you should implement:

class Cache {
 public:
  typedef vector<uint8_t> ObjectId;
  virtual ~Cache() {}
  // Check for existance of an object. This should be very fast
  // since it might be called multiple times for the same ID
  // quite often. Avoid OS calls if possible.
  virtual bool hasObject(const ObjectId &obj_id) = 0;
  // Read an object from the cache. This should fail if the object has become corrupt,
  // if the contents has been tampered with by a user, or if the file
  // has been deleted.
  virtual bool readObject(const ObjectId &obj_id, vector<uint8_t> &result) = 0;
  // Write an object to the cache
  virtual bool writeObject(const ObjectId &obj_id, const vector<uint8_t>
  &value) = 0;
  // Erase an object from the cache
  virtual bool eraseObject(const ObjectId &obj_id) = 0;
  // Set a maximum number of bytes that the cache may occupy.
  // If the cache grows larger than this, old objects must be pruned.
  virtual void setMaxSize(uint64_t max_size) = 0;
  // Return the current size (in bytes) of the cache
  virtual uint64_t getCurrentSize() = 0;
};

// Creates an instance of the cache.
// The encryption key should be used to encrypt all files in the cache.
// If the encryption key is wrong, it should behave identically to as if
// the cache was empty. Use any suitable encryption algorithm. Don't think
// too much about cryptographic weaknesses, that is not the focus of this
test.
Cache *createCache(const string &path, const vector<uint8_t>
&encryption_key);


========== Windows build instructions

Download boost 1.52.0
unpack boost into clientcache\

Open a Visual Studio command prompt
cd clientcache\boost_1_52_0
bootstrap
.\b2

Have a nice cup of coffee

I have included openssl 1.0.1 c which I have already built binaries for,
as the process is a bit buggy.

Generate a Visual Studio 2010 project (adapt to your desired version):

mkdir clientcache\build
cd clientcache\build
cmake .. -G"Visual Studio 10"

Open the generated solution and build.

Don't forget to add openssl-1.0.1c/out32dll to the PATH to able to run the unit tests.

If you would like to see errors being reported for the unit tests, add this command line
paramater: --log_level=error.

Other options are documented in
http://www.boost.org/doc/libs/1_52_0/libs/test/doc/html/utf/user-guide/runtime-config/reference.html


