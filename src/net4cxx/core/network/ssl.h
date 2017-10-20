//
// Created by yuwenyong on 17-10-10.
//

#ifndef NET4CXX_CORE_NETWORK_SSL_H
#define NET4CXX_CORE_NETWORK_SSL_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/base.h"


NS_BEGIN

class Factory;
class ClientFactory;
class SSLConnector;


class NET4CXX_COMMON_API SSLConnection: public Connection, public std::enable_shared_from_this<SSLConnection> {
public:
    using SocketType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

    SSLConnection(const ProtocolPtr &protocol, SSLOptionPtr sslOption, Reactor *reactor);

    SocketType& getSocket() {
        return _socket;
    }

    void write(const Byte *data, size_t length) override;

    void loseConnection() override;

    void abortConnection() override;

    bool getTcpNoDely() const {
        boost::asio::ip::tcp::no_delay option;
        _socket.lowest_layer().get_option(option);
        return option.value();
    }

    void setTcpNoDelay(bool enabled) {
        boost::asio::ip::tcp::no_delay option(enabled);
        _socket.lowest_layer().set_option(option);
    }

    bool getTcpKeepAlive() const {
        boost::asio::socket_base::keep_alive option;
        _socket.lowest_layer().get_option(option);
        return option.value();
    }

    void setTcpKeepAlive(bool enabled) {
        boost::asio::socket_base::keep_alive option(enabled);
        _socket.lowest_layer().set_option(option);
    }

    std::string getLocalAddress() const {
        auto endpoint = _socket.lowest_layer().local_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getLocalPort() const {
        auto endpoint = _socket.lowest_layer().local_endpoint();
        return endpoint.port();
    }

    std::string getRemoteAddress() const {
        auto endpoint = _socket.lowest_layer().remote_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getRemotePort() const {
        auto endpoint = _socket.lowest_layer().remote_endpoint();
        return endpoint.port();
    }
protected:
    void doClose();

    void doAbort();

    virtual void closeSocket();

    void startHandshake() {
        if (!_sslAccepting) {
            doHandshake();
        }
    }

    void doHandshake();

    void cbHandshake(const boost::system::error_code &ec) {
        _sslAccepting = false;
        handleHandshake(ec);
        if (!_disconnecting && !_disconnected) {
            startReading();
            if (!_writeQueue.empty()) {
                startWriting();
            }
        }
    }

    void handleHandshake(const boost::system::error_code &ec);

    void startReading() {
        if (!_reading && _sslAccepted) {
            doRead();
        }
    }

    void doRead();

    void cbRead(const boost::system::error_code &ec, size_t transferredBytes) {
        _reading = false;
        handleRead(ec, transferredBytes);
        if (!_disconnecting && !_disconnected) {
            doRead();
        }
    }

    void handleRead(const boost::system::error_code &ec, size_t transferredBytes);

    void startWriting() {
        if (!_writing && _sslAccepted) {
            doWrite();
        }
    }

    void doWrite();

    void cbWrite(const boost::system::error_code &ec, size_t transferredBytes) {
        _writing = false;
        handleWrite(ec, transferredBytes);
        if (!_disconnecting && !_disconnected && !_writeQueue.empty()) {
            doWrite();
        }
    }

    void handleWrite(const boost::system::error_code &ec, size_t transferredBytes);

    void startShutdown() {
        if (!_sslShutting) {
            doShutdown();
        }
    }

    void doShutdown();

    void cbShutdown(const boost::system::error_code &ec) {
        _sslShutting = false;
        handleShutdown(ec);
    }

    void handleShutdown(const boost::system::error_code &ec);

    bool _sslAccepting{false};
    bool _sslAccepted{false};
    bool _sslShutting{false};
    SSLOptionPtr _sslOption;
    SocketType _socket;
    std::exception_ptr _error;
};


class NET4CXX_COMMON_API SSLServerConnection: public SSLConnection {
public:
    explicit SSLServerConnection(SSLOptionPtr sslOption, Reactor *reactor)
            : SSLConnection({}, std::move(sslOption), reactor) {
#ifndef NET4CXX_NDEBUG
        NET4CXX_Watcher->inc(NET4CXX_SSLServerConnection_COUNT);
#endif
    }

#ifndef NET4CXX_NDEBUG
    ~SSLServerConnection() override {
        NET4CXX_Watcher->dec(NET4CXX_SSLServerConnection_COUNT);
    }
#endif

    void cbAccept(const ProtocolPtr &protocol);
};


class NET4CXX_COMMON_API SSLClientConnection: public SSLConnection {
public:
    explicit SSLClientConnection(SSLOptionPtr sslOption, Reactor *reactor)
            : SSLConnection({}, std::move(sslOption), reactor) {
#ifndef NET4CXX_NDEBUG
        NET4CXX_Watcher->inc(NET4CXX_SSLClientConnection_COUNT);
#endif
    }

#ifndef NET4CXX_NDEBUG
    ~SSLClientConnection() override {
        NET4CXX_Watcher->dec(NET4CXX_SSLClientConnection_COUNT);
    }
#endif

    void cbConnect(const ProtocolPtr &protocol, std::shared_ptr<SSLConnector> connector);
protected:
    void closeSocket() override;

    std::shared_ptr<SSLConnector> _connector;
};


class NET4CXX_COMMON_API SSLListener: public Listener, public std::enable_shared_from_this<SSLListener> {
public:
    using AddressType = boost::asio::ip::address;
    using AcceptorType = boost::asio::ip::tcp::acceptor;
    using SocketType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
    using ResolverType = boost::asio::ip::tcp::resolver ;
    using EndpointType = boost::asio::ip::tcp::endpoint ;
    using ResolverIterator = ResolverType::iterator;

    SSLListener(std::string port, std::unique_ptr<Factory> &&factory, SSLOptionPtr sslOption, std::string interface,
                Reactor *reactor);

#ifndef NET4CXX_NDEBUG
    ~SSLListener() override {
        NET4CXX_Watcher->dec(NET4CXX_SSLListener_COUNT);
    }
#endif

    void startListening() override;

    void stopListening() override;

    std::string getLocalAddress() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getLocalPort() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.port();
    }
protected:
    void cbAccept(const boost::system::error_code &ec);

    void handleAccept(const boost::system::error_code &ec);

    void doAccept() {
        _connection = std::make_shared<SSLServerConnection>(_sslOption, _reactor);
        _acceptor.async_accept(_connection->getSocket().lowest_layer(),
                               std::bind(&SSLListener::cbAccept, shared_from_this(), std::placeholders::_1));
    }

    std::string _port;
    std::unique_ptr<Factory> _factory;
    SSLOptionPtr _sslOption;
    std::string _interface;
    AcceptorType _acceptor;
    bool _connected{false};
    std::shared_ptr<SSLServerConnection> _connection;
};


class NET4CXX_COMMON_API SSLConnector: public Connector, public std::enable_shared_from_this<SSLConnector> {
public:
    using AddressType = boost::asio::ip::address;
    using SocketType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
    using ResolverType = boost::asio::ip::tcp::resolver ;
    using ResolverIterator = ResolverType::iterator;
    using EndpointType = boost::asio::ip::tcp::endpoint ;

    SSLConnector(std::string host, std::string port, std::unique_ptr<ClientFactory> &&factory, SSLOptionPtr sslOption,
                 double timeout, Address bindAddress, Reactor *reactor);

#ifndef NET4CXX_NDEBUG
    ~SSLConnector() override {
        NET4CXX_Watcher->dec(NET4CXX_SSLConnector_COUNT);
    }
#endif

    void startConnecting() override;

    void stopConnecting() override;

    void connectionFailed(std::exception_ptr reason={});

    void connectionLost(std::exception_ptr reason={});
protected:
    ProtocolPtr buildProtocol(const Address &address);

    void cancelTimeout() {
        if (!_timeoutId.cancelled()) {
            _timeoutId.cancel();
        }
    }

    void doResolve();

    void cbResolve(const boost::system::error_code &ec, ResolverIterator iterator) {
        handleResolve(ec, iterator);
        if (_state == kConnecting) {
            doConnect(std::move(iterator));
        }
    }

    void handleResolve(const boost::system::error_code &ec, ResolverIterator iterator);

    void doConnect();

    void doConnect(ResolverIterator iterator);

    void cbConnect(const boost::system::error_code &ec) {
        handleConnect(ec);
    }

    void handleConnect(const boost::system::error_code &ec);

    void cbTimeout() {
        handleTimeout();
    }

    void handleTimeout();

    void makeTransport();

    enum State {
        kDisconnected,
        kConnecting,
        kConnected,
    };

    std::string _host;
    std::string _port;
    std::unique_ptr<ClientFactory> _factory;
    SSLOptionPtr _sslOption;
    double _timeout{0.0};
    Address _bindAddress;
    ResolverType _resolver;
    std::shared_ptr<SSLClientConnection> _connection;
    State _state{kDisconnected};
    DelayedCall _timeoutId;
    bool _factoryStarted{false};
    std::exception_ptr _error;
};

NS_END

#endif //NET4CXX_CORE_NETWORK_SSL_H
