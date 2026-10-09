In check0 there are four main tasks:
1. Use telnet to send an http request to whatever website you can visit
2. Use netcat to start a local server and connect to it in another terminal
3. Implement `get_URL` function in webget.cc
4. Simulate the byte_stream transmission in byte_stream.cc

收获：
1. 一次服务器请求包括：建立连接，构造请求，等待响应三步
    1. 建立连接这一步需要做 DNS 解析并与服务器 IP 地址进行 TCP 三次握手
    2. 构造请求这一步需要包含方法，路由，HTTP协议版本，地址，以及结束标志：
    ```
    GET /hello HTTP/1.1
    Host: cs144.keithw.org
    Connection: close

    ```
    3. 用循环可以反复读取缓冲区，已读内容不会重复消费，读到 EOF 说明服务器已经关闭连接（Connection: close）