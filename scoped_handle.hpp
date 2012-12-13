#ifndef __SCOPED_HANDLE_HPP__
#define __SCOPED_HANDLE_HPP__

template< class HandleTraits >
class scoped_handle
{
 public:
  explicit scoped_handle( typename HandleTraits::HandleType handle = HandleTraits::invalid() ) : handle_(handle) {}
  ~scoped_handle() { if ( is_valid() ) HandleTraits::close_fcn(handle_); };
  typename HandleTraits::HandleType get() const { return handle_; }
  typename HandleTraits::HandleType& get() { return handle_; }
  bool is_valid() const { return HandleTraits::is_valid(handle_); }
  void reset(typename HandleTraits::HandleType handle = HandleTraits::invalid() ) {
    if (is_valid()) HandleTraits::close_fcn(handle_);
    handle_ = handle;
  }
  typename HandleTraits::HandleType release() {
    typename HandleTraits::HandleType old = handle_;
    handle_ = HandleTraits::invalid();
    return old;
  }

 private:
  typename HandleTraits::HandleType handle_;
  scoped_handle(const scoped_handle&);     // not copyable
  bool operator=(const scoped_handle&); // not assignable
};

#endif // __SCOPED_HANDLE_HPP__
