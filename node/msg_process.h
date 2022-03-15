#ifndef UENC_NODE_MSG_PROCESS_H_
#define UENC_NODE_MSG_PROCESS_H_

#include "proto/node.pb.h"
#include "socket/socket_api.h"

int SendRegisterNodeReq(std::string addr, uint16_t port);

void SendRegisterNodeAck(const std::shared_ptr<SocketConnection> &connection, bool get_subnode);

void SendSyncNodeReq(const std::string &base58addr);

void SendSyncNodeAck(const std::string &base58addr);

void SendConnectNodeReq(std::shared_ptr<SocketConnection> connection);

void SendBroadcaseMsgReq(const std::string &msg, Priority priority);

void SendTransMsgReq(const std::string &dest_base58addr, const std::string &msg, Priority priority, Compress compress, Encrypt encrypt);

void SendPingReq(const std::string &base58addr);

void SendPongReq(const std::string &base58addr);

void SendEchoReq(const std::string &base58addr);

void SendEchoAck(const std::string &base58addr);

void SendUpdateFeeReq();

void SendUpdatePackageFeeReq();

void SendGetNodeCacheReq(const std::string &base58addr, bool is_fetch_public);

void SendGetNodeCacheAck(const std::string &base58addr, bool is_fetch_public);

void SendNodeHeightChangedReq();

int HandlerRegisterNodeReq(const std::shared_ptr<RegisterNodeReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerRegisterNodeAck(const std::shared_ptr<RegisterNodeAck> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerSyncNodeReq(const std::shared_ptr<SyncNodeReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerSyncNodeAck(const std::shared_ptr<SyncNodeAck> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerConnectNodeReq(const std::shared_ptr<ConnectNodeReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerTransMsgReq(const std::shared_ptr<TransMsgReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerBroadcaseMsgReq(const std::shared_ptr<BroadcaseMsgReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerPingReq(const std::shared_ptr<PingReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerPongReq(const std::shared_ptr<PongReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerEchoReq(const std::shared_ptr<EchoReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerEchoAck(const std::shared_ptr<EchoAck> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerUpdateFeeReq(const std::shared_ptr<UpdateFeeReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerUpdatePackageFeeReq(const std::shared_ptr<UpdatePackageFeeReq> &msg, std::shared_ptr<SocketConnection> connection);

int HandlerNodeHeightChangedReq(const std::shared_ptr<NodeHeightChangedReq> &msg, std::shared_ptr<SocketConnection> connection);

#endif
