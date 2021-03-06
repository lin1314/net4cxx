//
// Created by yuwenyong on 17-9-20.
//

#ifndef NET4CXX_CORE_NETWORK_BASE_H
#define NET4CXX_CORE_NETWORK_BASE_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/algorithm/string.hpp>
#include "net4cxx/common/utilities/errors.h"
#include "net4cxx/common/utilities/messagebuffer.h"

NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(AlreadyCancelled, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorNotRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorAlreadyRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(NotConnectingError, Exception);
NET4CXX_DECLARE_EXCEPTION(ConnectionDone, IOError);
NET4CXX_DECLARE_EXCEPTION(ConnectionAbort, IOError);
NET4CXX_DECLARE_EXCEPTION(UserAbort, Exception);


class Reactor;
class Protocol;
using ProtocolPtr = std::shared_ptr<Protocol>;
class SSLOption;
using SSLOptionPtr = std::shared_ptr<SSLOption>;


enum class SSLVerifyMode {
    CERT_NONE,
    CERT_OPTIONAL,
    CERT_REQUIRED,
};


class SSLParams {
public:
    explicit SSLParams(bool serverSide= false)
            : _serverSide(serverSide) {
    }

    void setCertFile(const std::string &certFile) {
        _certFile = certFile;
    }

    const std::string& getCertFile() const {
        return _certFile;
    }

    void setKeyFile(const std::string &keyFile) {
        _keyFile = keyFile;
    }

    const std::string& getKeyFile() const {
        return _keyFile;
    }

    void setPassword(const std::string &password) {
        _password = password;
    }

    const std::string& getPassword() const {
        return _password;
    }

    void setVerifyMode(SSLVerifyMode verifyMode) {
        _verifyMode = verifyMode;
    }

    SSLVerifyMode getVerifyMode() const {
        return _verifyMode;
    }

    void setVerifyFile(const std::string &verifyFile) {
        _verifyFile = verifyFile;
    }

    const std::string& getVerifyFile() const {
        return _verifyFile;
    }

    void setCheckHost(const std::string &hostName) {
        _checkHost = hostName;
    }

    const std::string& getCheckHost() const {
        return _checkHost;
    }

    bool isServerSide() const {
        return _serverSide;
    }

protected:
    bool _serverSide;
    SSLVerifyMode _verifyMode{SSLVerifyMode::CERT_NONE};
    std::string _certFile;
    std::string _keyFile;
    std::string _password;
    std::string _verifyFile;
    std::string _checkHost;
};


class NET4CXX_COMMON_API SSLOption: public boost::noncopyable {
public:
    typedef boost::asio::ssl::context SSLContextType;

    bool isServerSide() const {
        return _serverSide;
    }

    SSLContextType &context() {
        return _context;
    }

    static SSLOptionPtr create(const SSLParams &sslParams);
protected:
    explicit SSLOption(const SSLParams &sslParams);

    void setCertFile(const std::string &certFile) {
        _context.use_certificate_chain_file(certFile);
    }

    void setKeyFile(const std::string &keyFile) {
        _context.use_private_key_file(keyFile, boost::asio::ssl::context::pem);
    }

    void setPassword(const std::string &password) {
        _context.set_password_callback([password](size_t, boost::asio::ssl::context::password_purpose) {
            return password;
        });
    }

    void setVerifyMode(SSLVerifyMode verifyMode) {
        if (verifyMode == SSLVerifyMode::CERT_NONE) {
            _context.set_verify_mode(boost::asio::ssl::verify_none);
        } else if (verifyMode == SSLVerifyMode::CERT_OPTIONAL) {
            _context.set_verify_mode(boost::asio::ssl::verify_peer);
        } else {
            _context.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
        }
    }

    void setVerifyFile(const std::string &verifyFile) {
        _context.load_verify_file(verifyFile);
    }

    void setDefaultVerifyPath() {
        _context.set_default_verify_paths();
    }

    void setCheckHost(const std::string &hostName) {
        _context.set_verify_callback(boost::asio::ssl::rfc2818_verification(hostName));
    }

    bool _serverSide;
    SSLContextType _context;
};


class NET4CXX_COMMON_API NetUtil {
public:
    static bool isValidIP(const std::string &ip) {
        boost::system::error_code ec;
        boost::asio::ip::address::from_string(ip, ec);
        return !ec;
    }

    static bool isValidPort(const std::string &port) {
        if (port.empty()) {
            return false;
        }
        return boost::all(port, boost::is_digit());
    }
};

class NET4CXX_COMMON_API Address {
public:
    Address() = default;

    Address(std::string address, unsigned short port)
            : _address{std::move(address)}
            , _port(port) {

    }

    void setAddress(std::string &&address) {
        _address = std::move(address);
    }

    void setAddress(const std::string &address) {
        _address = address;
    }

    const std::string& getAddress() const {
        return _address;
    }

    void setPort(unsigned short port) {
        _port = port;
    }

    unsigned short getPort() const {
        return _port;
    }
protected:
    std::string _address;
    unsigned short _port{0};
};


class NET4CXX_COMMON_API Timeout: public std::enable_shared_from_this<Timeout> {
public:
    friend Reactor;
    friend class DelayedCall;
    typedef boost::asio::steady_timer TimerType;

    explicit Timeout(Reactor *reactor);

    Timeout(const Timeout&) = delete;

    Timeout& operator=(const Timeout&) = delete;
protected:
    template <typename CallbackT>
    void start(const Timestamp &deadline, CallbackT &&callback) {
        _timer.expires_at(deadline);
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void start(const Duration &deadline, CallbackT &&callback) {
        _timer.expires_from_now(deadline);
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void start(double deadline, CallbackT &&callback) {
        _timer.expires_from_now(std::chrono::milliseconds(int64_t(deadline * 1000)));
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void wait(CallbackT &&callback) {
        _timer.async_wait([callback = std::forward<CallbackT>(callback), timeout = shared_from_this()](
                const boost::system::error_code &ec) {
            if (!ec) {
                callback();
            }
        });
    }

    void cancel() {
        _timer.cancel();
    }

    TimerType _timer;
};

typedef std::weak_ptr<Timeout> TimeoutHandle;

class NET4CXX_COMMON_API DelayedCall {
public:
    DelayedCall() = default;

    explicit DelayedCall(TimeoutHandle timeout)
            : _timeout(std::move(timeout)) {

    }

    bool cancelled() const {
        return _timeout.expired();
    }

    void cancel();
protected:
    TimeoutHandle _timeout;
};


class NET4CXX_COMMON_API Connection {
public:
    Connection(const ProtocolPtr &protocol, Reactor *reactor)
            : _protocol(protocol)
            , _reactor(reactor) {

    }

    virtual ~Connection() = default;

    virtual void write(const Byte *data, size_t length) = 0;

    virtual void loseConnection() = 0;

    virtual void abortConnection() = 0;

    virtual bool getNoDelay() const = 0;

    virtual void setNoDelay(bool enabled) = 0;

    virtual bool getKeepAlive() const = 0;

    virtual void setKeepAlive(bool enabled) = 0;

    virtual std::string getLocalAddress() const = 0;

    virtual unsigned short getLocalPort() const = 0;

    virtual std::string getRemoteAddress() const = 0;

    virtual unsigned short getRemotePort() const = 0;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    void dataReceived(Byte *data, size_t length);

    void connectionLost(std::exception_ptr reason);

    std::weak_ptr<Protocol> _protocol;
    Reactor *_reactor{nullptr};
    MessageBuffer _readBuffer;
    std::deque<MessageBuffer> _writeQueue;
    bool _reading{false};
    bool _writing{false};
    bool _connected{false};
    bool _disconnected{false};
    bool _disconnecting{false};
};

using ConnectionPtr = std::shared_ptr<Connection>;

class NET4CXX_COMMON_API Listener {
public:
    explicit Listener(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Listener() = default;

    virtual void startListening() = 0;

    virtual void stopListening() = 0;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    Reactor *_reactor{nullptr};
};

using ListenerPtr = std::shared_ptr<Listener>;

class NET4CXX_COMMON_API Connector {
public:
    explicit Connector(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Connector() = default;

    virtual void startConnecting() = 0;

    virtual void stopConnecting() = 0;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    Reactor *_reactor{nullptr};
};

using ConnectorPtr = std::shared_ptr<Connector>;

NS_END

#endif //NET4CXX_CORE_NETWORK_BASE_H
