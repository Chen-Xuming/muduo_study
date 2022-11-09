//
// Created by chen on 2022/11/5.
//

#include "InetAddress.h"
#include "../base/Logging.h"
#include "Endian.h"
#include "SocketsOps.h"

#include <netdb.h>
#include <netinet/in.h>


#pragma GCC diagnostic ignored "-Wold-style-cast"

/*
 *      INADDR_ANY: 泛指本机地址（0.0.0.0），能够收到任意一张网卡的连接
 *      INADDR_LOOPBACK: 回环地址（127.0.0.1）
 */
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

#pragma GCC diagnostic error "-Wold-style-cast"

using namespace muduo;
using namespace muduo::net;

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");


/*
 *  loopbackOnly: 仅监听回环地址上的连接
 */
InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6) {
    // InetAddress要么是sockaddr_in，要么是sockaddr_in6
    static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
    static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");

    if(ipv6){
        memZero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = sockets::hostToNetwork16(port);
    }else{
        memZero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
        addr_.sin_port = sockets::networkToHost16(port);
    }
}

InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6) {
    if(ipv6 || strchr(ip.c_str(), ':')){
        memZero(&addr6_, sizeof addr6_);
        sockets::fromIpPort(ip.c_str(), port, &addr6_);
    }else{
        memZero(&addr_, sizeof addr_);
        sockets::fromIpPort(ip.c_str(), port, &addr_);
    }
}

string InetAddress::toIpPort() const {
    char buf[64] = "";
    sockets::toIpPort(buf, sizeof buf, getSockAddr());
    return buf;
}

string InetAddress::toIp() const {
    char buf[64] = "";
    sockets::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

/*
 *     获取ipv4地址字节序（仅当InetAddress是ipv4地址时）
 */
uint32_t InetAddress::ipv4NetEndian() const {
    assert(family() == AF_INET);
    return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::port() const {
    return sockets::networkToHost16(portNetEndian());
}

/*
 *      根据域名或者ip获得一个主机的信息（存放在hostent），
 *      然后为addr_.sin_addr赋值
 *
 *      仅支持解析ipv4
 *
 *      struct hostent {
             char *h_name;                    // official name of host
             char **h_aliases;                // alias list
             int h_addrtype;                  // host address type——AF_INET || AF_INET6
             int h_length;                    // length of address
             char **h_addr_list;              // list of addresses
        };
 *
 */
static __thread char t_resolveBuffer[64 * 1024];    // 用于解析过程中存储中间结果
bool InetAddress::resolve(StringArg hostname, InetAddress *result) {
    assert(result != nullptr);
    struct hostent hent;
    struct hostent *he = nullptr;
    int herrno = 0;
    memZero(&hent, sizeof hent);

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
    if(ret == 0 && he != nullptr){
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        result->addr_.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
        return true;
    }else{
        if(ret){
            LOG_SYSERR << "InetAddress::resolve";
        }
        return false;
    }
}

/*
 *     仅IPv6使用
 */
void InetAddress::setScopedId(uint32_t scope_id) {
    if(family() == AF_INET6){
        addr6_.sin6_scope_id = scope_id;
    }
}

















