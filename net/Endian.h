//
// Created by chen on 2022/11/1.
//

/*
 *      封装各种字节序转换函数（Host --> Net; Net --> Host）
 */

#ifndef MYMUDUO_ENDIAN_H
#define MYMUDUO_ENDIAN_H

#include <stdint.h>
#include <endian.h>

namespace muduo{
    namespace net{
        namespace sockets{

// 在此处禁用编译时类型转化相关的warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

            /*
             *  注：be表示大端字节序（网络字节序）; le表示小端字节序（host字节序）
             */

            inline uint64_t hostToNetwork64(uint64_t host64){
                return htobe64(host64);
            }

            inline uint32_t hostToNetwork32(uint32_t host32){
                return htobe32(host32);
            }

            inline uint16_t hostToNetwork16(uint16_t host16){
                return htobe16(host16);
            }

            inline uint64_t networkToHost64(uint64_t net64){
                return be64toh(net64);
            }

            inline uint32_t networkToHost32(uint32_t net32){
                return be32toh(net32);
            }

            inline uint16_t networkToHost16(uint16_t net16){
                return be16toh(net16);
            }

#pragma GCC diagnostic pop
        }
    }
}


#endif //MYMUDUO_ENDIAN_H
