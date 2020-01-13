# UNP卷1：套接字联网API

## 第2章 传输层：TCP、UDP和SCTP

### 2.1 概述

- SCTP（Stream Control Transmission Protocol，流控制传输协议）与TCP类似之处在于它也是一个可靠的传输协议，但它还提供消息边界、传输级别多宿（multihoming）支持以及将投毒案阻塞（head-of-line blocking）减少到最小的一种方法

### 2.2 总图

- TCP/IP协议概况

  ![image-20200113002024936](images/TCPIP协议概况.png)

### 2.6 TCP连接的建立和终止

#### 2.6.1 三次握手

<img src="images/TCP三次握手.png" alt="image-20200113182818037" style="zoom:50%;" />

#### 2.6.2 TCP选项

- 每一个SYN可以有多个TCP选项
  - MSS（Maximum Segment Size）选项。可以使用TCP_MAXSEG套接字选项提取和设置这个TCP选项
  - 窗口规模选项。这个新选项指定TCP首部中的通告窗口必须扩大的位数。在一个TCP连接上使用窗口规模的前提是它的两个断系统必须都支持这个选项。可以使用SO_RCVBUF套接字选项设置这个TCP选项
  - 时间戳选项

#### 2.6.3 四次挥手

<img src="images/TCP四次挥手.png" alt="image-20200113183835624" style="zoom:50%;" />

#### 2.6.4 TCP状态转换图

<img src="images/TCP状态转换图.png" alt="image-20200113184952280" style="zoom:45%;" />

#### 2.6.5 观察分组

- TCP连接的分组交换

  <img src="images/TCP连接的分组交换.png" alt="image-20200113190803952" style="zoom:45%;" />

  - 服务器对客户端请求的确认是伴随其应答发送的，这种做法称为捎带（piggybacking），它通常在服务器处理请求并产生应答的时间少于20ms时发生

### 2.7 TIME_WAIT状态

- 停留在TIME_WAIT状态的持续时间是MSL（Maximum Segment Lifetime）的两倍，有时候称之为2MSL

- TIME_WAIT状态有两个存在的理由
  - 可靠地实现TCP全双工连接的终止
    - 执行主动关闭的那一断是处于TIME_WAIT状态的那一断，因为可能不得不重传最终那个ACK
  - 允许老的重复分节在网络中消逝
    - 解决这样的问题：A主机的P1端口和B主机的P2端口之间有一个连接，在这个连接关闭后，在这两个端口之间建立了一个新的连接。为了避免来自上一个连接的老的重复分组在上一个连接终止后再出现，从而被误认为是新连接的分组
    - 为了做到这一点，TCP将不给处于TIME_WAIT状态的连接发起新的连接
    - 我们假设，具有最大跳限（hop limit）（255）的分组在网络中存在的时间不可能超过MSL秒。因此2MSL足以让某个方向上的分组最多存活MSL秒即被丢弃，另一个方向上的应答最多存活MSL秒也被丢弃

### 2.8 SCTP关联的建立和终止

#### 2.8.1 四路握手

<img src="images/SCTP四路握手.png" alt="image-20200113205745102" style="zoom:50%;" />

#### 2.8.2 关联终止

<img src="images/SCTP关联关闭时的分组交换.png" alt="image-20200113210038084" style="zoom:50%;" />