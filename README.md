# Key-agreement-system（密钥协商系统）

[1. 项目简介](#1.项目简介)
- [四大功能](###四大功能)

[2. 主要知识点](##2.主要知识点)

[3. 项目架构](##3.项目架构)
- [3.1 项目简介](###3.1项目架构图)
- [3.2 系统运作流程](###3.2系统运作流程)
- [3.3 五大模块](###3.3五大模块)

[4. 项目部署](##4.项目部署)
- [4.1 环境](###4.1环境)
- [4.2 编译运行](###4.2编译运行)
- [4.3 项目演示](###4.3项目演示)

## 1. 项目简介

[密钥协商](https://blog.csdn.net/m0_37433111/article/details/115466668)系统通过协商生成**对称加密**所需的密钥，提供为第三方应用使用，保障点与点之间的通信安全。

### 四大功能

1. 密钥协商
2. 密钥校验
3. 密钥查看
4. 密钥注销

## 2. 主要知识点
- C++基础
- 对称加密和非对称加密
- 单向散列函数（哈希函数）和消息认证码
- 编解码技术[ASN.1](https://blog.csdn.net/dongdong7_77/article/details/113072896)

- 工厂模式
- 网络通信
  - TCP/IP协议
  - 三次握手和四次挥手
  - socket API函数: socket  setsockopt  bind  listen  accept  connect  |  read  recv  send  write       
  - 多进程并发服务器
  - 多线程并发服务器
  - I/O多路复用技术: select poll epoll
  - I/O多路复用与多线程或者多进程结合使用
  - 第三方库: libevent
- 进程间通信 
  - pipe fifo mmap 本地套接字 网络套接字 共享内存
- 数据库相关
  - C++操作MySQL
- 守护进程相关

## 3. 项目架构

### 3.1 项目架构图

<img align=center src="https://github.com/March225/Key-agreement-system/blob/main/%E9%A1%B9%E7%9B%AE%E6%9E%B6%E6%9E%84%E5%9B%BE.png" alt="01" style="zoom:100%；" />

<p align="center">
	图1 项目架构图
</p>

### 3.2 系统运作流程

系统的核心功能是密钥协商，因此这里以密钥协商为例，讲述系统的运作流程。（ps：在进行密钥协商之前，必须要先进行网点注册，否则会被拒绝服务）

密钥协商流程：

1. 客户端发起密钥协商请求

   客户端生成一个随机数r1， 同时使用openssl中哈希函数对r1进行哈希运算，得到一个哈希值（消息认证码）；然后将随机数和消息认证码等要发送的数据进行编码，发送给服务端。

2. 服务端收到请求后进行处理，并给予客户端应答

   第一阶段：服务端收到请求数据之后， 首先对数据进行解码；然后根据客户端ID+服务端ID查询数据库， 检验客户端是否合法；之后，通过消息认证码验证客户端发送的随机数r1是否被篡改。

   第二阶段：服务端生成随机数r2，将r1和r2进行拼接，然后使用与客户端相同的哈希算法进行哈希运算， 得到一个哈希值（密钥seckey），并将密钥信息写入共享内存和数据库；之后，服务端对应答消息进行编码发送给客户端。

3. 客户端接收服务端应答，成功生成密钥信息后，密钥协商结束

   客户端对接收到的数据进行解码，然后判断rv的值， 若rv为0表示生成密钥成功；获得服务端发来的随机字符串r2， 将r2和r1进行拼接， 之后进行哈希运算，得到密钥seckey；最后，将密钥信息写入共享内存。

密钥校验、密钥查看和密钥注销流程和密钥协商类似，这里不做赘述。

### 3.3 五大模块

1. 数据编解码模块 
   数据编解码模块使用的编解码技术是[**ASN.1**](https://blog.csdn.net/dongdong7_77/article/details/113072896)
	- ASN.1定义
    ASN.1（抽象语法标记）描述了一种对数据进行表示，编码，传输和解码的数据格式，提供了一套完整的描述对象的结构。
	- 进行数据编解码的原因
    由于跨平台传输过程中可能存在操作系统不同，大小端模式不同，字节对齐不同，开发语言不同等各种原因，所以需要对数据进行序列化处理。
	- 选择ANS.1进行编解码，而不是JSON或XML的原因
    ANS.1是比JSON、XML更高级的语言，相较于它们更为规范。
	- 将ASN.1编解码函数封装成类方便调用

2. TCP通信模块
   TCP通信模块使用的是**多线程socket编程**
	- socket API函数：socket  setsockopt  bind  listen  accept  connect  |  read  recv  send  write 
	- 单进程socket编程存在的问题及解决办法
		- 服务端：accept() 和 read() 互相阻塞，通信效率低；解决办法是使用多线程技术：主线程调用accept接受新的客户端连接， 子线程负责处理通信。
		- 客户端：单进程只能处理一个连接，效率低；解决办法是使用多线程技术。	
	- 如果是高并发场景，考虑使用I/O多路复用技术（select poll epoll），线程池，连接池，第三方库（libevent）等。
	- 理解TCP通信服务端类和客户端类的封装思想（服务端类和客户端类是依赖关系，服务端类的成员函数返回一个客户端类的对象用于通信）


3. 共享内存模块

	- 共享内存[底层原理](https://blog.csdn.net/jnu_simba/article/details/9097419)
	- 共享内存操作函数：shmget(创建)  shmat(关联)   shmdt(将shm和当前进程分离)   shmctl(共享内存操作) 
	- 将共享内存封装成类方便调用

4. 数据库模块
   数据库使用的是**MySQL**
	- MySQL数据库基础
	- 将SQL语句封装成类方便调用
  
5. 自定义API

	- 将API函数制作成动态链接库(.so)，提供给第三方通信平台使用，调用API即可完成数据的加解密操作


## 4. 项目部署
### 4.1 环境

- 操作系统 ubuntu 16.04

  ```bash
  # uname -a 查看详细信息
  $ uname -a
  # 输出：Linux ubuntu 4.15.0-140-generic #144~16.04.1-Ubuntu SMP Fri Mar 19 21:24:12 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux
  ```
  
- 安装相关库

  ```bash
  $ sudo apt install openssh-server
  $ sudo apt-get install libssl-dev
  $ sudo apt-get install mysql-server mysql-client
  $ sudo apt-get install libmysqlclient-dev
  ```

- 创建数据库
    ```bash
  # 登录MySQL
  $ mysql -u root -p
  # 执行 .sql 文件，创建数据库（默认生成了一个根网点和两个子网点）
  $ source ~/KeyAgree_mysql.sql
  ```

### 4.2 编译运行

- 编译服务端

  ```bash
  # 打开密钥协商服务端文件夹
  $ cd ~/Key-agreement-system/KeyAgreeClient
  # 编译
  $ make
  # 清理编译生成的文件
  $ make clean
  ```

- 编译客户端

  方法同服务端

- 运行

```bash
  # 运行服务端（默认为守护进程，所以启动后在后台运行）
  $ ./svrMain
  # 运行服务端（命令行参数有一个整数，表示客户端id）
  $ ./cltMain 2
```

### 4.3 项目演示

- 密钥协商过程

  <img src="https://github.com/March225/Key-agreement-system/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE1.png" alt="演示图" style="zoom:100%;" />

  

- 简单测试自定义API

  调用自定义API，先从客户端的共享内存中取出密钥进行数据加密，然后从服务端的共享内存中取出密钥进行解密，观察数据是否发生变化。

  ![演示图2](https://github.com/March225/Key-agreement-system/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE2.png)
