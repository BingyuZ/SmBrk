/*
 * zecodec.h
 *
 *  Created on: May 25, 2017
 *      Author: Bingyu
 */

#ifndef ZECODEC_H
#define ZECODEC_H

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <cassert>

using namespace muduo;
using namespace muduo::net;

#define MAXDEV  16
#define ZE_VER  1

#define AU_ENCRY    ENCRY_NONE

#define ENCRY_NONE  0

enum ConnStage {
    SC_NCON = 0,
    SC_CHSENT,
    SC_PASSED,
};

enum CommandAgt {
    CA_UNKNOWN  = 0,

    CAS_WELCOME = 1,
    CAA_LOGANS  = 2,
    CAS_LOGRES  = 3,

    CAA_KALIVE  = 8,
    CAS_KALIVE  = 9,

    CAA_DEVSTA  = 0x10,
    CAS_DEVANS  = 0x11,
//  CAA_DEVLOST = 0x12,
//  CAA_DEVHIS  = 0x14,

    CAA_DATA    = 0x20,
    CAS_DACK    = 0x21,

    CAS_COMMAND = 0x30,
    CAA_CMDRESP = 0x31,
};

struct PacketHeader {
//    uint8_t     verHi_;
//    uint8_t     verLo_;
//    uint8_t     lenHi_; // Length = < command ... body >, without CRC
//    uint8_t     lenLo_;

    uint8_t     cmd_;
    uint8_t     cmdA_;
    uint8_t     ver_;
    uint8_t     rev_;   // Used as encryption

    union {
        struct {
            uint32_t    saltHi_;
            uint32_t    saltLo_;
        };
        uint64_t    salt_;
    };

//    uint32_t    body_[0];
    char        body_[0];
};

extern uint32_t GetMyRand(bool t=true);

class Session {
public:
    explicit Session(const TcpConnectionPtr& conn)
        : stage_(SC_CHSENT), conn_(conn)
    {
        rev_ = AU_ENCRY;
        for (int i=0; i<MAXDEV; i++)
            devId_[i] = 0xFFFFFFFFUL;
        agentId_ = 0;
        passwd_[0] = GetMyRand();
        passwd_[1] = GetMyRand();
    }

    ~Session(){}

    void GeneratePass(void)
    {
        //stage_ = SC_PASSED;
        for (int i=0; i<4; i++)
            passwd_[i] = GetMyRand();
    }

	uint32_t Chksum(int head, const char *ptr, int32_t length)
	{
		uint32_t res = head;
		const uint32_t *p = reinterpret_cast<const uint32_t *>(ptr);
		assert(length % 4 == 0);

		for (int i=0; i < length/4; ++i)
			res ^= *p++;
		return res;
	}

	bool CheckPass(const muduo::string& s)
	{
	    const PacketHeader *pPack = reinterpret_cast<const PacketHeader *> (s.data());

	    // TODO: Check the password here

	    const uint32_t ul = *reinterpret_cast<const uint32_t *>(pPack->body_);
	    agentId_ = muduo::net::sockets::networkToHost32(ul);
	    stage_ = SC_PASSED;

        return true;
	}

	void sendWelcome(void)
	{
	    PacketHeader header;

	    header.cmd_ = CAS_WELCOME;
	    header.cmdA_ = 0;
	    header.ver_ = ZE_VER;
	    header.rev_ = rev_;

	    header.saltHi_ = passwd_[0];
	    header.saltLo_ = passwd_[1];

		uint32_t first = kSpec + 12;

		muduo::net::Buffer buf;
		buf.append(&header, 12);

		uint32_t be32 = muduo::net::sockets::hostToNetwork32(first);
		buf.prependInt32(first);

		buf.appendInt32(Chksum(be32, reinterpret_cast<const char *>(&header), 12));
		conn_->send(&buf);
	}

	void sendPacket(CommandAgt type, const void *pBuf, uint32_t length,
                    const uint32_t *pwd = NULL)
	{
	    const char *ptr = (const char *)pBuf;

		assert(length % 8 == 0);
		assert(length < 65501);

		PacketHeader header;
		header.cmd_ = type;
		header.cmdA_ = 0;
		header.ver_ = ZE_VER;
	    header.rev_ = rev_;

	    header.saltHi_ = GetMyRand(false);
	    header.saltLo_ = GetMyRand(false);

		uint32_t first = kSpec + 12 + length;

		muduo::net::Buffer buf;
		buf.append(&header, 12);
		buf.prependInt32(first);

		// encrypt here
		// If pwd == NULL, using passwd_, else pwd
		for (uint32_t i=0; i<length; i+=8) {
            buf.append(ptr, 8);
            ptr += 8;
		}

		uint32_t crc = 0;   // TODO: Real CRC
		buf.appendInt32(crc);

		conn_->send(&buf);
	}

	void sendDevAck(struct DevInfoHeader *);
	void sendLogRes(const char *);

	uint64_t getEnc(uint64_t salt)
	{
	    return 0;
	}

	bool decrypt(const char *src, muduo::string *msg, int length)
	{
        const struct PacketHeader *pH = (const struct PacketHeader *)src;
        if (pH->ver_ != ZE_VER || pH->rev_ != AU_ENCRY) return false;

	    uint64_t    salt = pH->salt_;

	    msg->reserve(length);
	    src += 12;
	    for (int i=0; i<((length-5)&0xfff8); i+=8) {
            salt = getEnc(salt);
            msg->append(src, 8);
            src += 8;
	    }
	    LOG_DEBUG << msg->length()-12 << " bytes decrypted";

	    return true;
    }

    void ResetConn(void){
	    conn_.reset();
	}

    uint8_t     stage_;
    uint8_t     rev_;
    uint32_t    passwd_[4];
    uint64_t    agentId_;
    uint64_t    devId_[MAXDEV]; // 12 BCD

 private:
    TcpConnectionPtr conn_;     // Must do! An assignment
    const static int32_t kSpec = static_cast<int32_t>(('Z'<<24) | ('E'<<16));
};

#if 0
typedef boost::function<void (	const muduo::net::TcpConnectionPtr&,
        						const muduo::string& message,
								muduo::Timestamp) > MBlockMessageCallback;

class MLengthHeaderCodec : boost::noncopyable
{
public:
	explicit MLengthHeaderCodec(const MBlockMessageCallback& cb)
		: messageCallback_(cb)
	{
	}

	// Checksum not returned
	// And Checksum is ignored
	void onMessagewC( const muduo::net::TcpConnectionPtr& conn,
					  muduo::net::Buffer* buf,
					  muduo::Timestamp receiveTime)
	{
		while (buf->readableBytes() >= kMinLength + 4) // kMinLength == 16  exclude CRC
		{
			const char* pC1 = (const char *) buf->peek();

			if (*pC1 != 'Z' && *(pC1+1) != 'E') {
				LOG_ERROR << "Wrong packet head " << *pC1 << " " << *(pC1+1);
				conn->shutdown(); // FIXME: disable reading
				break;
			}

			pC1 += 2;
			uint32_t len = *pC1 * 256 + *(pC1+1);

			if (len < kMinLength - 4)   // Length = 12+
			{
				LOG_ERROR << "Invalid packet length " << len;
				conn->shutdown();  // FIXME: disable reading
				break;
			}
			else if (buf->readableBytes() >= len + 8)   // Header + CRC
			{
			    // TODO: Check CRC here
				buf->retrieve(kHeaderLen);  // 4
				muduo::string message(buf->peek(), len);
				messageCallback_(conn, message, receiveTime);
				buf->retrieve(len + 4);
			}
			else break;	// Haven't got enough bytes
		}
	}

	// use simple XOR in this version
	// ptr should be 4x, otherwise SIGBUS
	uint32_t Chksum(int head, const char *ptr, int32_t length)
	{
		uint32_t res = head;
		const uint32_t *p = reinterpret_cast<const uint32_t *>(ptr);
		assert(length % 4 == 0);

		for (int i=0; i < length/4; ++i)
			res ^= *p++;
		return res;
	}

	void send(const muduo::net::TcpConnectionPtr& conn,
			  const muduo::StringPiece& message)
	{
		muduo::net::Buffer buf;
		buf.append(message.data(), message.size());	// COPY Here!
		int32_t head = kSpec + static_cast<int32_t>(message.size());
		int32_t be32 = muduo::net::sockets::hostToNetwork32(head);
		buf.prepend(&be32, sizeof be32);
		conn->send(&buf);
	}

	// SBY: Checksum added
	// Length must be 4x
	// Chksum untested
	void sendwChksum(const muduo::net::TcpConnectionPtr& conn,
			  const void *ptr, int32_t length)
	{
		assert(length % 4 == 0);
		assert(length < 65501);

		muduo::net::Buffer buf;
		buf.append(ptr, length);

		int32_t head = kSpec + length;
		int32_t be32 = muduo::net::sockets::hostToNetwork32(head);
		buf.prependInt32(head);

		//head = Chksum(be32, static_cast<const char *>(ptr), length);

		buf.appendInt32(Chksum(be32, static_cast<const char *>(ptr), length));
		conn->send(&buf);
	}

	private:
		MBlockMessageCallback messageCallback_;
		const static size_t kHeaderLen = sizeof(int32_t);
		const static int32_t kSpec = static_cast<int32_t>(('Z'<<24) | ('E'<<16));
		const static int32_t kMinLength = 0x10;
};
#endif

#endif  // ZECODEC_H
