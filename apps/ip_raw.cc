// #include "socket.hh"

// using namespace std;

// class RawSocket : public DatagramSocket
// {
// public:
//   RawSocket() : DatagramSocket( AF_INET, SOCK_RAW, IPPROTO_RAW ) {}
// };

// int main()
// {
//   // construct an Internet or user datagram here, and send using the RawSocket as in the Jan. 10 lecture

//   return 0;
// }

#include "socket.hh"

#include <cstdint>
#include <string>

using namespace std;

class RawSocket : public DatagramSocket
{
public:
  RawSocket() : DatagramSocket( AF_INET, SOCK_RAW, IPPROTO_RAW ) {}
};

// 网络字节序：16 位整数的高字节在前。
void append_u16( string& data, uint16_t value )
{
  data.push_back( static_cast<char>( ( value >> 8 ) & 0xff ) );
  data.push_back( static_cast<char>( value & 0xff ) );
}

string make_ip( char protocol, const string& payload )
{
  string packet( 20, '\0' ); // 20 字节 IPv4 首部，无选项
  packet[0] = 0x45;          // Version=4，IHL=5
  packet[8] = 64;            // TTL
  packet[9] = protocol;      // IP 协议号

  // 源地址 127.0.0.1，占偏移 12～15。
  packet[12] = 127;
  packet[15] = 1;

  // 目的地址 127.0.0.1，占偏移 16～19。
  packet[16] = 127;
  packet[19] = 1;

  // Linux 会填写 IP 总长度、首部校验和及当前为零的 ID。
  return packet + payload;
}

int main()
{
  RawSocket sock {};
  const Address destination { "127.0.0.1", 0 };

  // 第一个包：协议号 5。
  sock.sendto( destination, make_ip( 5, "hello protocol 5\n" ) );

  // 第二个包：手工构造 UDP 首部和负载。
  const string message = "hello UDP\n";
  string udp;
  append_u16( udp, 40000 ); // 源端口
  append_u16( udp, 9090 );  // 目的端口
  append_u16( udp, static_cast<uint16_t>( 8 + message.size() ) );
  append_u16( udp, 0 ); // IPv4 UDP：不使用 UDP 校验和
  udp += message;

  sock.sendto( destination, make_ip( 17, udp ) );
}