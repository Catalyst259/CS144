# Checkpoint 1：本地回环实验操作指南

用同一台 Linux 电脑的不同终端模拟通信双方，完成实验 2.1 和 2.2。

- 目标地址：`127.0.0.1`，表示本机。
- 抓包接口：`lo`，替代讲义中的 `wg0`。
- 不需要加入斯坦福课程专网。
- 回环不经过物理网卡；延迟、丢包率不能代表真实两机通信。
- 本文不涉及 `Reassembler` 的实现。

## 0. 准备终端

打开三个终端，分别记为 A、B、C。所有终端都在同一台 Linux 电脑或同一台虚拟机中运行。

每个终端都进入项目目录：

```bash
cd ~/Personal-Files/Courses/CS144/minnow
```

本机已有 `tcpdump`、`nc` 和 `tshark`。若要用图形界面查看抓包，安装 Wireshark：

```bash
sudo apt install wireshark
```

没有图形界面时，可以使用后面的 `tshark` 命令。

## 1. 实验 2.1：ping、抓包和统计

### 1.1 终端 A：先开始抓包

```bash
sudo tcpdump -i lo -nn -s 0 -U --print -w /tmp/check1-ping.pcap icmp
```

看到 `listening on lo` 后，再进行下一步。

| 参数 | 作用 |
|---|---|
| `-i lo` | 在回环接口抓包 |
| `-nn` | 用数字显示地址和端口 |
| `-s 0` | 捕获完整数据包 |
| `-U` | 每收到一个包就将它写入抓包文件 |
| `--print` | 保存文件的同时在终端显示包摘要 |
| `-w ...` | 指定抓包文件 |
| `icmp` | 只抓 ICMP |

### 1.2 终端 B：发送 1000 次 ping

```bash
ping -c 1000 -i 0.2 127.0.0.1 | tee /tmp/check1-ping.txt
```

- `-c 1000`：发送 1000 次请求。
- `-i 0.2`：每隔约 0.2 秒发送一次，总共约 200 秒。
- `tee`：同时显示输出并保存文本记录。

预期：终端 A 能看到 `echo request` 和 `echo reply`；终端 B 能看到每次回复及最终统计。

等待 ping 自动结束，然后在终端 A 按 `Ctrl+C` 停止抓包。

### 1.3 记录报告中的 (a)、(b)、(c)

从本次 1000 次 ping 的实际输出中填写，不要直接使用之前 5 次 ping 的结果。

| 项目 | 读取方法 | 实测值 |
|---|---|---|
| 平均 RTT | `min/avg/max/mdev` 中的 `avg` | __0.074__ ms |
| 发出的请求数 | `packets transmitted` | __1000__ |
| 收到回复的请求数 | `received`，重复回复不重复计入 | __1000__ |
| 交付率 | 收到回复的请求数 / 发出的请求数 × 100% | __100__ % |
| 丢包率 | `packet loss` | __0__ % |
| 重复数据报 | 查看输出中是否出现 `DUP!` | __无__ |

### 1.4 打开抓包，完成 (d)、(e)

有图形界面时运行：

```bash
wireshark /tmp/check1-ping.pcap
```

读取已保存的抓包不需要 `sudo`。在 Wireshark 显示过滤器中输入：

```text
icmp
```

选择一个 Echo Request，展开 `Internet Protocol Version 4`。对照 [RFC 791 的首部图](https://www.rfc-editor.org/rfc/rfc791.html#page-11) 查看：

| 字段 | 含义或本实验中的常见值 |
|---|---|
| Version | IPv4，值为 4 |
| Header Length / IHL | 通常为 20 字节，即 5 个 32 位字 |
| Differentiated Services Field | 原 IPv4 Type of Service 字段 |
| Total Length | 默认 ping 通常为 84 字节 |
| Identification | IP 数据报标识 |
| Flags / Fragment Offset | 分片控制信息 |
| Time to Live | TTL |
| Protocol | ICMP 为 1 |
| Header Checksum | IP 首部校验和 |
| Source / Destination | 本实验中均为 127.0.0.1 |

继续展开 ICMP，查看请求类型、标识符和序号，再找到对应的 Echo Reply。

也可以用命令行查看第一个包的全部字段：

```bash
tshark -r /tmp/check1-ping.pcap -c 1 -V
```

### 1.5 如何回答 (f)

讲义要求比较“同一个数据报在两台机器上的抓包记录”。本地回环不能原样复现这个条件。

报告中可以注明：

> 本实验使用单机 loopback，完成了 ICMP 收发、统计和首部分析。未进行两台主机对同一个数据报的抓包对比。

Echo Request 和 Echo Reply 是两个不同的数据报，不要把它们当作同一个包在发送端与接收端的记录。

## 2. 实验 2.2：手工构造 IP 和 UDP 数据报

本节先发一个协议号为 5 的 IP 包，再发一个协议号为 17 的 UDP 包：

```text
协议号 5：[ IPv4 首部 ][ 测试数据 ]
UDP：    [ IPv4 首部 ][ UDP 首部 ][ 测试数据 ]
```

协议号写在 IPv4 首部中。协议号 17 表示 UDP，不是“端口 17”。

### 2.1 编辑 apps/ip_raw.cc

将 [apps/ip_raw.cc](../apps/ip_raw.cc) 改为下面的本地实验示例。它每次运行发送两个包，然后退出。

```cpp
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
  packet[0] = 0x45;         // Version=4，IHL=5
  packet[8] = 64;           // TTL
  packet[9] = protocol;     // IP 协议号

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
  append_u16( udp, 0 );    // IPv4 UDP：不使用 UDP 校验和
  udp += message;

  sock.sendto( destination, make_ip( 17, udp ) );
}
```

简单说明：

- `string` 保存二进制字节，允许包含 `\0`。
- UDP 首部共 8 字节：源端口、目的端口、长度、校验和各占 2 字节。
- UDP 长度为 `8 + 负载长度`；上面的 `hello UDP\n` 共 10 字节，因此 UDP 长度为 18，IP 总长度为 38。
- 本示例使用 IPv4，UDP 校验和为零表示不使用校验和；不要直接套用到 IPv6。
- `Address` 中填端口 0；真正的 UDP 目的端口写在你构造的 UDP 首部中。
- 填写源端口不会创建接收 socket；这个发送程序本身不监听回复。

首部格式参考 [RFC 768](https://www.rfc-editor.org/rfc/rfc768.html)。Linux `IPPROTO_RAW` 会启用 `IP_HDRINCL`，并补填上述 IP 字段，详见 [raw(7)](https://man7.org/linux/man-pages/man7/raw.7.html)。

### 2.2 编译

```bash
cmake -S . -B build
cmake --build build --target ip_raw
```

### 2.3 终端 A：开始抓包

```bash
sudo tcpdump -i lo -nn -s 0 -U --print -vv -X \
  -w /tmp/check1-raw.pcap \
  'ip proto 5 or udp port 9090 or udp port 40000'
```

等到 `listening on lo`。这次先保持抓包运行，直到反方向练习也完成。

### 2.4 终端 B：启动 UDP 接收程序

```bash
nc -u -l 127.0.0.1 9090
```

不需要 `sudo`。保持这个终端运行，它等待发往 UDP 端口 9090 的数据。

### 2.5 终端 C：发送数据报

```bash
sudo ./build/apps/ip_raw
```

预期结果：

- 终端 A 抓到一个 IP 协议号为 5 的包，以及一个 `40000 → 9090` 的 UDP 包。
- 终端 B 显示 `hello UDP`。
- 终端 C 的发送程序正常退出。

这说明你手工写出的 UDP 首部被内核识别，负载被送到了监听端口 9090 的普通 UDP socket。`nc` 只显示负载，不显示 IP 和 UDP 首部。

### 2.6 模拟反方向发送

终端 B 按 `Ctrl+C` 停止原来的 `nc`，再改为监听端口 40000：

```bash
nc -u -l 127.0.0.1 40000
```

在 `apps/ip_raw.cc` 中，仅将 UDP 源端口与目的端口对调：

```cpp
append_u16( udp, 9090 );  // 源端口
append_u16( udp, 40000 ); // 目的端口
```

终端 C 重新编译并发送：

```bash
cmake --build build --target ip_raw
sudo ./build/apps/ip_raw
```

预期：终端 A 看到 `9090 → 40000`，终端 B 再次显示 `hello UDP`。

程序也会再次发送协议号为 5 的包。对于协议号 5，可交换负责发包和抓包的终端角色，再执行一次。这仍是单机模拟，报告中应注明没有真实同学参与两机收发。

完成后，在终端 A、B 分别按 `Ctrl+C` 停止抓包和接收。

### 2.7 查看手工构造的包

```bash
wireshark /tmp/check1-raw.pcap
```

分别使用下面的显示过滤器：

```text
ip.proto == 5
```

```text
udp.port == 9090 || udp.port == 40000
```

展开 IPv4 和 UDP，确认协议号、地址、端口、长度和负载。

无图形界面时：

```bash
tshark -r /tmp/check1-raw.pcap -Y 'ip.proto == 5 || udp' -V
```

## 3. 保存结果并填写报告

`/tmp` 中的文件可能在重启后被清理。完成实验后，在项目目录保存一份：

```bash
mkdir -p Report/check1-artifacts
cp /tmp/check1-ping.txt /tmp/check1-ping.pcap /tmp/check1-raw.pcap \
  Report/check1-artifacts/
```

将实际结果记录到 [writeups/check1.md](../writeups/check1.md) 或自己的实验笔记中：

- [ ] 说明实验环境为 Linux 本地回环：`127.0.0.1` / `lo`。
- [ ] 记录至少 1000 次 ping 的平均 RTT、交付率、丢包率及重复情况。
- [ ] 保存 ICMP 抓包，记录观察到的 IPv4 首部字段。
- [ ] 说明未完成真实两机对同一个数据报的抓包比较。
- [ ] 抓到协议号为 5 的手工 IP 数据报。
- [ ] 普通 `nc -u` 在不使用 `sudo` 的情况下收到手工 UDP 包的负载。
- [ ] 对调端口，完成反方向的本地模拟。
- [ ] 保留 `apps/ip_raw.cc` 源码及抓包文件。

## 4. 常见问题

| 现象 | 检查方法 |
|---|---|
| 抓不到 ping 包 | 确认抓包接口为 `lo`，目标为 `127.0.0.1`，并且先抓包再 ping |
| `ip_raw` 提示权限不足 | 使用 `sudo ./build/apps/ip_raw`，原始套接字需要相应权限 |
| `nc` 没有显示内容 | 先启动 `nc`，核对 UDP 目的端口与监听端口，修改代码后重新编译 |
| `Address already in use` | 停止自己之前启动的、占用相同端口的 `nc` |
| 运行 `ip_raw` 后没有包 | 确认已替换空的 `main()`，并编译了最新代码 |
| 终端里只有协议号 5 的包，没有 UDP | 检查过滤器、IP Protocol=17、UDP 长度和端口字节序 |
| Wireshark 无法打开图形界面 | 使用本文的 `tshark -r ... -V` 命令读取抓包 |

# reassembler
由于数据包的可能乱序到达，丢失，或重复，因此需要一个 reassembler 统一处理并把数据包以正确的顺序送入缓冲区
以 string 为单位需要处理复杂的合并逻辑，因此最好的方式是以 byte 为单位进行处理，reassembler 只需要处理 byte 的合并逻辑即可