#include "socket_api.h"
#include "common/config.h"
#include "common/logging.h"
#include <endian.h>
#include <string.h>
#include <zlib.h>
#include "utils/net_utils.h"

int SocketInit()
{
    auto conf = Singleton<Config>::instance();
    std::string listen_ip = conf->listen_ip();
    in_port_t listen_port = conf->listen_port();
    auto socket_manager = Singleton<SocketManager>::instance();
    auto ret = socket_manager->Listen(listen_ip, listen_port);
    if (ret < 0)
    {
        return ret - 10000;
    }
    ret = socket_manager->Listen(conf->unix_domain_path());
    if (ret < 0)
    {
        return ret - 20000;
    }
    socket_manager->ThreadStart();
    Singleton<ProtobufProcess>::instance()->ThreadStart(conf->work_thread_num());
    return 0;
}

void SocketDestory()
{
    Singleton<SocketManager>::instance()->ThreadStop();
    Singleton<ProtobufProcess>::instance()->ThreadStop();
}

static bool ZlibCompressor(const std::string &bytes, std::string &out)
{
    uint64_t datalen = (bytes.size() + 12) * 1.001 + 2;
    char *pressdata = new char[datalen]{0};
    int err = compress((Bytef *)pressdata, &datalen, (const Bytef *)bytes.c_str(), bytes.size());
    if (err != Z_OK)
    {
        delete[] pressdata;
        return false;
    }
    out = std::string(pressdata, datalen);
    delete[] pressdata;
    return true;
}

static bool ZlibUnCompressor(const std::string &bytes, std::string &out)
{
    size_t uncompress_len = bytes.size() * 10;
    char *uncompress_data = new char[uncompress_len]{0};
    int err = uncompress((Bytef *)uncompress_data, &uncompress_len, (const Bytef *)bytes.c_str(), bytes.size());
    if (err != Z_OK)
    {
        delete[] uncompress_data;
        return false;
    }
    out = std::string(uncompress_data, uncompress_len);
    delete[] uncompress_data;
    return true;
}

int Bytes2Proto(const std::string &bytes, Priority &priority, std::shared_ptr<google::protobuf::Message> &out_msg)
{
    uint32_t length = 0;
    int pos = sizeof(length);
    if (bytes.size() < pos)
    {
        return 0;
    }
    memcpy(&length, bytes.data(), sizeof(length));
    //字节序转换
    length = le32toh(length);
    if (pos + length > bytes.size())
    {
        return 0;
    }
    std::string data = std::string(bytes.begin() + pos, bytes.begin() + pos + (length - sizeof(uint32_t) * 3));
    pos += (length - sizeof(uint32_t) * 3);

    uint32_t checksum = 0;
    memcpy(&checksum, bytes.data() + pos, sizeof(checksum));
    checksum = le32toh(checksum);
    if(checksum != GetAdler32(data))
    {
        return -1;
    }
    pos += sizeof(checksum);

    uint32_t flag = 0;
    memcpy(&flag, bytes.data() + pos, sizeof(flag));
    priority = (Priority)(flag & 0xF);
    pos = pos + sizeof(flag);

    uint32_t end = 0;
    memcpy(&end, bytes.data() + pos, sizeof(end));
    end = le32toh(end);

    CommonMsg common_msg;
    if (!common_msg.ParseFromString(data))
    {
        return -2;
    }
    std::string type = common_msg.type();
    if (type.empty())
    {
        return -3;
    }
    const google::protobuf::Descriptor *des = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type);
    if (nullptr == des)
    {
        return -4;
    }
    const google::protobuf::Message *proto = google::protobuf::MessageFactory::generated_factory()->GetPrototype(des);
    if (nullptr == proto)
    {
        return -5;
    }
    std::string sub_data = common_msg.data();
    if (Compress::kCompress_True == common_msg.compress())
    {
        if (!ZlibUnCompressor(common_msg.data(), sub_data))
        {
            return -6;
        }
        sub_data = common_msg.data();
    }
    out_msg.reset(proto->New());
    if (!out_msg->ParseFromString(sub_data))
    {
        return -7;
    }
    return sizeof(length) + length;
}

void Proto2Bytes(const std::string &msg_byte, const std::string &type, Priority priority, Compress compress, Encrypt encrypt, std::string &out_bytes)
{
    CommonMsg common_msg;
    common_msg.set_version(g_msg_version);
    common_msg.set_type(type);
    common_msg.set_encrypt(encrypt);
    std::string comp_data;
    if (Compress::kCompress_True == common_msg.compress() && ZlibCompressor(msg_byte, comp_data) && comp_data.size() < msg_byte.size())
    {
        common_msg.set_compress(Compress::kCompress_True);
        common_msg.set_data(comp_data);
    }
    else
    {
        common_msg.set_compress(Compress::kCompress_False);
        common_msg.set_data(msg_byte);
    }
    std::string data = common_msg.SerializeAsString();
    uint32_t length = data.size() + sizeof(uint32_t) * 3;
    length = htole32(length);
    out_bytes.append((char *)&length, sizeof(length));

    out_bytes.append(data);

    uint32_t checksum = htole32(GetAdler32(data));
    out_bytes.append((char *)&checksum, sizeof(checksum));

    uint32_t flag = ((uint8_t)priority & 0xF);
    flag = htole32(flag);
    out_bytes.append((char *)&flag, sizeof(flag));

    uint32_t end = 7777777;
    end = htole32(end);
    out_bytes.append((char *)&end, sizeof(end));
}
int WriteMessage(std::shared_ptr<SocketConnection> connection, const std::string &msg_byte, const std::string &type, Priority priority, Compress compress, Encrypt encrypt)
{
    if (nullptr == connection)
    {
        return -1;
    }
    if(!connection->IsConnected())
    {
        return -2;
    }
    std::string msg;
    Proto2Bytes(msg_byte, type, priority, compress, encrypt, msg);
    auto ret = connection->WriteMsg(msg);
    if(ret < 0)
    {
        return ret - 100;
    }
    return ret;
}
