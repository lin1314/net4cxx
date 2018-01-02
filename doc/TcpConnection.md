### `TCPConnection`





## base `Connection` class

```c++
    Connection(const ProtocolPtr &protocol, Reactor *reactor)
            : _protocol(protocol), _reactor(reactor) {
    }  
	virtual ~Connection() = default;
    virtual void write(const Byte *data, size_t length) = 0;
    virtual void loseConnection() = 0;
    virtual void abortConnection() = 0;
    virtual void setKeepAlive(bool enabled) = 0;
// ....
protected:
    void dataReceived(Byte *data, size_t length);
    void connectionLost(std::exception_ptr reason); 
    std::weak_ptr<Protocol> _ protocol;
    Reactor *_reactor{nullptr};
    MessageBuffer _readBuffer;
    std::deque<MessageBuffer> _writeQueue;
};
using ConnectionPtr = std::shared_ptr<Connection>;

```



## `TCPConnection ` class

- public base: Connection std::enable_shared_from_this<TCPConnection>

#### write(const Byte *data, size_t length)

#### `loseConnection()`

#### `abortConnection()`

#### `setKeepAlive(bool enabled)`

### protect:

```c++
SocketType _socket;
std::exception_ptr _error;
```

#### `doClose()`

####`doAbort()`

#### `virtual void closeSocket()`

#### `startReading()` 

- `doRead();`
- `cbRead(const boost::system::error_code &ec, size_t transferredBytes) `
- `handleRead(const boost::system::error_code &ec, size_t transferredBytes)`

#### `startWriting()`

- `doWrite();`
- `cbWrite(const boost::system::error_code &ec, size_t transferredBytes) `
- `handleWrite(const boost::system::error_code &ec, size_t transferredBytes)`