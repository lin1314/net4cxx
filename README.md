
# net4cxx

## Tutorial

### 开发基于字节流协议的服务器(tcp,ssl,unix)


```c++
#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class Echo: public Protocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void dataReceived(Byte *data, size_t length) override {
        write(data, length);
    }
};

class EchoFactory: public Factory {
public:
    ProtocolPtr buildProtocol(const Address &address) override {
        return std::make_shared<Echo>();
    }
};

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    TCPServerEndpoint endpoint(&reactor, "28001");
    endpoint.listen(std::make_unique<EchoFactory>());
    reactor.run();
    return 0;
}

```

* 以上实现了一个最简单的例子,将客户端发送过来的信息回写;
* 将TCPServerEndPoint替换成SSLServerEndPoint将会启动一个基于sslSocket的服务器;
* 将TCPServerEndPoint替换成UNIXServerEndPoint将会启动一个基于unixSocket的服务器;
* 后面会展示一种更好的服务器启动方式,切换协议无需修改任何代码.

```c++

class Echo: public Protocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void connectionMade() override {
        NET4CXX_INFO(gAppLog, "Connection made");
    }
    
    void connectionLost(std::exception_ptr reason) override {
        NET4CXX_INFO(gAppLog, "Connection lost");
    }

    void dataReceived(Byte *data, size_t length) override {
        write(data, length);
        loseConnection();
    }
};

```

* 连接建立时将会回调connectionMade;
* 连接销毁时将会回调connectionLost;
* 调用loseConnection安全的关闭连接.


```c++

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    serverFromString(&reactor, "tcp:28001")->listen(std::make_unique<EchoFactory>());
    serverFromString(&reactor, "ssl:28002:privateKey=test.key:certKey=test.crt")->listen(std::make_unique<EchoFactory>());
    serverFromString(&reactor, "unix:/var/foo/bar")->listen(std::make_unique<MyFactory>());
    reactor.run();
    return 0;
}

```

* 以上代码展示了在一个进程内同时启动了一个tcp服务器,ssl服务器,unix服务器;
* 观察服务器的启动方式,发现只有字符串参数的值不同,如果我们从配置中读取这个字符串的话,切换协议无须更改一行代码,这也是推荐的方式;

### 开发基于字节流协议的客户端(tcp,ssl,unix)

```c++
#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class WelcomeMessage: public Protocol, public std::enable_shared_from_this<WelcomeMessage> {
public:
    void connectionMade() override {
        write("Hello server, I am the client!");
        loseConnection();
    }
};

class WelcomeFactory: public ClientFactory {
public:
    std::shared_ptr<Protocol> buildProtocol(const Address &address) override {
        return std::make_shared<WelcomeMessage>();
    }
};

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.connectTCP("localhost", "28001", std::make_unique<WelcomeFactory>());
    reactor.run();
    return 0;
}

```

* 以上实现了一个最简单的例子,客户端向服务器打了个招乎,随后关闭连接;
* 将connectTCP替换成connectSSL或者connectUNIX能分别建立ssl或者unix客户端连接;
* 与服务器一样支持从字符串构建客户端连接,调用clientFromString即可,不再赘述;


```c++
int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.connectTCP("localhost", "28001", std::make_unique<OneShotFactory>(std::make_shared<WelcomeMessage>()));
    reactor.run();
    return 0;
}
```

* 使用内置的OneShotFactory可以指定总是返回某个固定的protocol,用户也无需创建自己的factory,这在服务器之间互联很有用;

```c++

class WelcomeFactory: public ReconnectingClientFactory {
public:
    std::shared_ptr<Protocol> buildProtocol(const Address &address) override {
        resetDelay();
        return std::make_shared<WelcomeMessage>();
    }
};

```

* 当继承自内建的ReconnectingClientFactory,连接断开时,会自动启用指数避让原则进行重连;


## Reference

### 核心模块

#### class Reactor

##### 默认构造函数

```c++
Reactor();
```

##### 启动反应器循环

```c++
void run(bool installSignalHandlers=true);
```

* installSignalHandlers： 是否安装信号退出处理器

##### 定时调用某个函数

```c++
template <typename CallbackT>
DelayedCall callLater(double deadline, CallbackT &&callback);

template <typename CallbackT>
DelayedCall callLater(const Duration &deadline, CallbackT &&callback);
```

* deadline: 延迟多少秒触发
* callback: 回调函数，满足签名void ()

```c++
template <typename CallbackT>
DelayedCall callAt(time_t deadline, CallbackT &&callback);

template <typename CallbackT>
DelayedCall callAt(const Timestamp &deadline, CallbackT &&callback);
```

* deadline: 触发的绝对时间
* callback: 回调函数，满足签名void ()

##### 在服务器的下一帧触发回调

```c++
template <typename CallbackT>
void addCallback(CallbackT &&callback);
```

* callback: 回调函数，满足签名void ()

##### 添加一个反应器退出时的回调函数

```c++
template <typename CallbackT>
void addStopCallback(CallbackT &&callback);
```

* callback: 回调函数，满足签名void ()

##### 启动一个tcp服务器

```c++
ListenerPtr listenTCP(const std::string &port, std::unique_ptr<Factory> &&factory, const std::string &interface={});
```

* port: 端口号或服务名称
* factory： 协议工厂
* interface: 指定监听的ip地址或域名

##### 启动一个tcp客户端

```c++
ConnectorPtr connectTCP(const std::string &host, const std::string &port, std::unique_ptr<ClientFactory> &&factory,
                        double timeout=30.0, const Address &bindAddress={});
```

* host: 连接的服务器的ip地址或域名
* port: 连接的服务器的端口或服务名
* factory： 协议工厂
* timeout： 连接超时时间
* bindAddress: 为客户端套接字绑定一个指定的地址和端口

##### 启动一个ssl服务器

```c++
ListenerPtr listenSSL(const std::string &port, std::unique_ptr<Factory> &&factory, SSLOptionPtr sslOption,
                      const std::string &interface={});
```

* port: 端口号或服务名称
* factory： 协议工厂
* sslOption: ssl选项
* interface: 指定监听的ip地址或域名

##### 启动一个ssl客户端

```c++
ConnectorPtr connectSSL(const std::string &host, const std::string &port, std::unique_ptr<ClientFactory> &&factory,
                        SSLOptionPtr sslOption, double timeout=30.0, const Address &bindAddress={});
```

* host: 连接的服务器的ip地址或域名
* port: 连接的服务器的端口或服务名
* factory： 协议工厂
* sslOption: ssl选项
* timeout： 连接超时时间
* bindAddress: 为客户端套接字绑定一个指定的地址和端口

##### 启动一个unix服务器

```c++
ListenerPtr listenUNIX(const std::string &path, std::unique_ptr<Factory> &&factory);
```

* path: 监听的文件路经
* factory： 协议工厂

##### 启动一个unix客户端

```c++
ConnectorPtr connectUNIX(const std::string &path, std::unique_ptr<ClientFactory> &&factory, double timeout=30.0);
```

* path: 服务器的文件路经
* factory： 协议工厂
* timeout： 连接超时时间

##### 获取reactor是否正在运行

```c++
bool running() const;
```

##### 停止reactor

```c++
void stop();
```

#### class DelayedCall

##### 获取计时器是否过期

```c++
bool cancelled() const;
```

##### 取消该计时器

```c++
void cancel();
```

#### class Address

##### 构造函数

```c++
Address();
Address(std::string address, unsigned short port);
```

* address: ip地址或文件路经(unix)
* port: 端口号, 对unix域地址无效

##### 设置地址

```c++
void setAddress(std::string &&address);
void setAddress(const std::string &address);
```

* address: ip地址或文件路经(unix)

##### 获取地址

```c++
const std::string& getAddress() const
```

##### 设置端口号

```c++
void setPort(unsigned short port);
```

* port: 端口号, 对unix域地址无效

##### 获取端口号

```c++
unsigned short getPort() const;
```

### 基础流协议模块

### 基础数据报协议模块(待实现)

### http核心模块(待实现)

### web模块(待实现)

### webSocket模块(待实现)

### 基础工具模块(待实现)

### 多线程支持(待实现)

### 单元测试模块(待实现)

### 电子邮件模块(待实现)


