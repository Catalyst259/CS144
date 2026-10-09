#include "reassembler.hh"

#include <algorithm>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( output_.writer().is_closed() ) {
    return;
  }

  // 先记住原始结束位置，即使这次的数据超出了接收窗口。
  if ( is_last_substring ) {
    end_index_ = first_index + data.size();
  }

  uint64_t next = output_.writer().bytes_pushed();
  const uint64_t begin = max( first_index, next );
  const uint64_t end = min( first_index + data.size(), next + output_.writer().available_capacity() );

  // 只保存窗口内的字节；相同索引再次到达时，map 自动去重。
  for ( uint64_t index = begin; index < end; ++index ) {
    pending_.try_emplace( index, data[index - first_index] );
  }

  // 从下一个待写入的位置开始收集连续字节，再一次性写入。
  string ready;
  while ( !pending_.empty() && pending_.begin()->first == next ) {
    ready += pending_.begin()->second;
    pending_.erase( pending_.begin() );
    ++next;
  }
  if ( !ready.empty() ) {
    output_.writer().push( move( ready ) );
  }

  if ( end_index_.has_value() && output_.writer().bytes_pushed() == end_index_.value() ) {
    output_.writer().close();
  }
}

uint64_t Reassembler::count_bytes_pending() const
{
  return pending_.size();
}
