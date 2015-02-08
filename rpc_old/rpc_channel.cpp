/***************************************************************************
 * 
 * Copyright (c) 2014 Aishuyu. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file rpc_channel.cpp
 * @author aishuyu(asy5178@163.com)
 * @date 2014/11/23 16:25:13
 * @brief 
 *  
 **/


#include "rpc_channel.h"

#include <string>

#include "rpc_util.h"
#include "rpc_msg.pb.h"
#include "../log/ds_log.h"

namespace dist_storage {

using std::string;

Channel::Channel(const char* addr, const char* port) {
    strcpy(addr_ = (char*)malloc(strlen(addr) + 1), addr);
    strcpy(port_ = (char*)malloc(strlen(port) + 1), port);
}

Channel::~Channel() {
    //close(connect_fd_);
    delete addr_;
    delete port_;
}

// used by CallMethod
bool FormatSendMsg(
        const MethodDescriptor* method, const Message* request, string& send_str);
// used by CallMethod
bool FormatRecvMsg(
        const string& recv_str, Message* response);

void Channel::CallMethod(const MethodDescriptor* method,
                         RpcController* control,
                         const Message* request,
                         Message* response,
                         Closure* done) {

    connect_fd_ = connection_manager_.TcpConnect(addr_, port_);
    if (connect_fd_ < 0) {
        DS_LOG(ERROR, "rpc connect server failed!");
        return;
    }

    string send_str;
    if (!FormatSendMsg(method, request, send_str)) {
        return;
    }
  
    if (connection_manager_.SendMsg(connect_fd_, send_str) < 0) {
        DS_LOG(ERROR, "send msg error!");
    }

    string recv_str;
    if (connection_manager_.RecvMsg(connect_fd_, recv_str) < 0) {
        DS_LOG(ERROR, "rcv msg error!");
    }
    close(connect_fd_);

    if (!FormatRecvMsg(recv_str, response)) {
        DS_LOG(ERROR, "format recv msg failed!");
    }
}

void Channel::Close() {
}

bool FormatSendMsg(
        const MethodDescriptor* method, const Message* request, string& send_str) {

    uint32_t hash_code = BKDRHash(method->full_name().c_str());

    if (NULL == request) {
        DS_LOG(ERROR, "request is null ptr!");
        return false;
    }

    string request_str;
    if (!request->SerializeToString(&request_str)) {
        DS_LOG(ERROR, "request SerializeToString has failed!");
        return false;
    }
    RpcMessage rpc_msg;
    rpc_msg.set_head_code(hash_code);
    rpc_msg.set_body_msg(request_str);


    if (!rpc_msg.SerializeToString(&send_str)) {
        DS_LOG(ERROR, "request SerializeToString has failed!");
        return false;
    }
    return true;
}

bool FormatRecvMsg(const string& recv_str, Message* response) {
    RpcMessage recv_rpc_msg;
    if (!recv_rpc_msg.ParseFromString(recv_str)) {
        DS_LOG(ERROR, "parse recv msg error! %s", recv_str.c_str());
        return false;
    }

    if (500 == recv_rpc_msg.head_code()) {
        DS_LOG(ERROR, "server internal error!");
    }

    if ("0" != recv_rpc_msg.body_msg() && 
        !response->ParseFromString(recv_rpc_msg.body_msg())) {
        DS_LOG(ERROR, "parse recv body msg error!");
        return false;
    }
    return true;
}


}  // end of namespace dist_storage



/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
