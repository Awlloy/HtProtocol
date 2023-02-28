# HT Protocol

2023-02-27

---

基于不可靠信道(需要字符串传输和读取功能)下的可靠通信实现
例如支持：串口通信，网络 UDP 协议，蓝牙 BLE 等

该协议为精简协议

为了实现可靠通信，采用了以下几个方案：
1. 接收者 `ACK` 接收回复机制，发送者等待 `ACK` 反馈
2. 接收者 `NAK` 请求重发机制，
3. 发送者超时自动重传
4. 字节填充法避免误判头部,预留解决粘包导致的flag和数据无法区分

为了能够传输更大的数据包： 
- 对于大的数据包自动拆分为多个小数据包，理论上可拆出无限个包，但不建议超过255个包
- 宏定义 `PACK_SIZE` 可设置, 默认 `20`, 蓝牙 BLE 一次只能传送20个字节

为了增加信道利用率：
- 采用了滑动窗口，允许发送者发送多个窗口数据，避免了发1等1的情况

**!!!注意:**
- 该程序实现可靠通信,读写接口均为阻塞接口.
- 用户提供的`read`的接口函数也必须是无阻塞(否则会导致发送方接收不到接收方的反馈信息时程序阻塞), `write`接口也尽可能为无阻塞(但不是必须的)
- 为了提高代码移植能力,使得总多硬件比如单片机内也可以部署，整个代码里边没有依赖任何系统接口，所有变量均采用静态内存。
- 提供了`write`, `read`函数指针让用户接入不同的接口，还提供了定时器更新接口，只需要开启一个线程或者在单片机内的定时器中断调用并传入经过时间。
- 时间单位可自定义，与超时时间对应，可自定义阻塞发送的超时时间和每帧数据发送的回应超时


目前传送结束后的挥手逻辑是:
1. 发送者收到接收者的最后一个包的 `ACK` (即所有包的 ACK 均收到),向接收者发布一个 `RF`,收到 `RF` 和 `ACK` 后,进入超时关闭,
期间若收到接收者的 `RF`, 则反馈 `RF` 和 `ACK`,  随后立即关闭发送
2. 接收者收到发布者发布的 `RF` 后,反馈 `RF` 和 `ACK` 给发送者,并发送一个 `RF` 给发送者,进入超时关闭等待,期间若收到 `RF` 和 `ACK`, 随后立即关闭

发送者和接收者的滑动窗口一致移动,从而提高了数据缓存效率



完成随机丢包测试,测试代码 client_drop.cpp 和 sender_drop.cpp


未来计划:
    1. 支持粘包处理,粘包自动拆分
    2. 提供各个语言版本和平台接口(c/c++,python,java),平台(linux,windows,android,STM系列单片机)等
    3. 测试协议性能
    