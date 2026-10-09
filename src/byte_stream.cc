#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer {} {}

void Writer::push( string data )
{
  // Your code here.
  // 保留到 available_capacity() 的长度
  if ( data.length() > available_capacity() ) {
    data = data.substr( 0, available_capacity() );
  }
  // 将数据添加到缓冲区
  Writer::buffer.append( data );
  // 更新 total_pushed
  Writer::total_pushed += data.length();
}

void Writer::close()
{
  // Your code here.
  closed = true;
}

bool Writer::is_closed() const
{
  return closed; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - ( total_pushed - total_popped ); // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed; // Your code here.
}

string_view Reader::peek() const
{
  return buffer; // Your code here.
}

void Reader::pop( uint64_t len )
{
  if ( len > buffer.length() ) {
    len = buffer.length();
  }
  buffer.erase( 0, len );
  total_popped += len; // Your code here.
}

bool Reader::is_finished() const
{
  return closed && ( buffer.length() == 0 ); // Your code here.
}

uint64_t Reader::bytes_buffered() const
{
  return buffer.length(); // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return total_popped; // Your code here.
}
