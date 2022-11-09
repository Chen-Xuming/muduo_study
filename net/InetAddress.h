//
// Created by chen on 2022/11/5.
//

/*
 *      InetAddress
 *      封装了sockaddr_in/sockaddr_in6
 *
 */

#ifndef MYMUDUO_INETADDRESS_H
#define MYMUDUO_INETADDRESS_H

#include "../base/copyable.h"
#include "../base/StringPiece.h"
#include <netinet/in.h>

namespace muduo{
    namespace net{
        namespace sockets{
            // sockaddr_in6 转 sockaddr
            const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
        }

        class InetAddress: public copyable{
        public:
            // 使用本机ip和指定port创建
            explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

            // 使用指定ip和指定port创建
            InetAddress(StringArg ip, uint16_t port = 0, bool ipv6 = false);

            // 使用sockaddr_in / sockaddr_in6直接创建
            explicit InetAddress(const struct sockaddr_in &addr): addr_(addr){}
            explicit InetAddress(const struct sockaddr_in6 &addr): addr6_(addr){}

            /// FIXME 如果是ipv6怎么处理？
            sa_family_t family() const {
                return addr_.sin_family;
            }

            string toIp() const;
            string toIpPort() const;
            uint16_t port() const;

            const struct sockaddr *getSockAddr() const{
                return sockets::sockaddr_cast(&addr6_);
            }

            void setSockAddrInet6(const struct sockaddr_in6 &addr6){
                addr6_ = addr6;
            }

            uint32_t ipv4NetEndian() const;
            uint16_t portNetEndian() const {
                return addr_.sin_port;
            }

            // 根据主机名解析ip地址
            static bool resolve(StringArg hostname, InetAddress *result);

            // set IPv6 scopeID
            void setScopedId(uint32_t scope_id);

        private:
            union{
                struct sockaddr_in addr_;
                struct sockaddr_in6 addr6_;
            };
        };

    }   // net
}// muduo








#endif //MYMUDUO_INETADDRESS_H



























