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

enum ConnStage {
    SC_NCON = 0,
    SC_CHSENT,
    SC_PASSED,
};

struct Session {
    uint8_t     stage_;
    uint8_t     rev_;
    uint32_t    agentId_;
    uint32_t    passwd_[4];
};

enum CommandAgt {
    CA_UNKNOWN  = 0,

    CAS_WELCOME = 1,
    CAA_LOGANS  = 2,
    CAS_LOGRES  = 3,    // Not necessary?

    CAA_KALIVE  = 8,
    CAS_KALIVE  = 9,

    CAA_DEVSTA  = 0x10,
    CAA_DEVLOST = 0x12,
    CAA_DEVHIS  = 0x14,

    CAS_DEVANS  = 0x11,

    CAA_DATA    = 0x20,
    CAS_DACK    = 0x21,


};

struct PacketHeader {
//    uint8_t     verHi_;
//    uint8_t     verLo_;
//    uint8_t     lenHi_; // Length = < command ... body >, without CRC
//    uint8_t     LenLo_;
    uint8_t     cmd_;
    uint8_t     cmdA_;
    uint8_t     ver_;
    uint8_t     rev_;   // Used as encryption

    uint32_t    saltHi_;
    uint32_t    saltLo_;

    char        body_[0];
};
void FillPacketHeader(PacketHeader *pH, enum CommandAgt cmd, bool genSalt = true);

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

#endif  // ZECODEC_H
