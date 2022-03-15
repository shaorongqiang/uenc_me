#ifndef UENC_UTILS_NET_UTILS_H_
#define UENC_UTILS_NET_UTILS_H_

#include <arpa/inet.h>
#include <string>
#include <vector>

uint32_t GetAdler32(const std::string &bytes);

bool GetLocalIpv4(std::vector<uint64_t> &ips);
bool GetIpv4AndPortByFd(int fd, in_addr_t &ip, in_port_t &port);
bool Int2StrIPv4(in_addr_t ip, std::string &out);
bool Str2IntIPv4(const std::string &ip, in_addr_t &out);

bool Int2StrIPv6(uint8_t ip[16], std::string &out);
bool Str2IntIPv6(const std::string &ip, uint8_t out[16]);
#endif
