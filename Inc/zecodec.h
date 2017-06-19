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
#include <boost/noncopyable.hpp>

#include <cassert>


typedef boost::function<void (	const muduo::net::TcpConnectionPtr&,
        						const muduo::string& message,
								muduo::Timestamp)> MBlockMessageCallback;

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
		while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
		{
			// FIXME: use Buffer::peekInt32()
			const char* pC1 = (const char *) buf->peek();

			if (*pC1 != 'Z' && *(pC1+1) != 'E') {
				LOG_ERROR << "Wrong packet head " << *pC1 << " " << *(pC1+1);
				conn->shutdown(); // FIXME: disable reading
				break;
			}

			pC1 += 2;
			int32_t len = *pC1 * 256 + *(pC1+1);

			if (len < kMinLength)
			{
				LOG_ERROR << "Invalid packet length " << len;
				conn->shutdown();  // FIXME: disable reading
				break;
			}
			else if (buf->readableBytes() >= len + kHeaderLen)
			{
				buf->retrieve(kHeaderLen);
				muduo::string message(buf->peek(), len-4);
				messageCallback_(conn, message, receiveTime);
				buf->retrieve(len);
			}
			else break;	// Haven't got enough bytes
		}
	}

	// use simple XOR in this version
	// ptr should be 4x, otherwise SIGBUS
	uint32_t ChkSum(int head, const char *ptr, int32_t length)
	{
		uint32_t res = head;
		const uint32_t *p = reinterpret_cast<const uint32_t *>(ptr);
		assert(length % 4 == 0);

		for (int i=0; i < length /4; ++i)
			res ^= *p++;
		return res;
	}

	// FIXME: TcpConnectionPtr
	void send(muduo::net::TcpConnection* conn,
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
	void sendwChksum(muduo::net::TcpConnection* conn,
			  const void *ptr, int32_t length)
	{
		assert(length %4 == 0);
		assert(length < 65501);

		muduo::net::Buffer buf;
		buf.append(ptr, length);
		int32_t head = kSpec + length + 4;
		int32_t be32 = muduo::net::sockets::hostToNetwork32(head);
		buf.prepend(&be32, sizeof be32);
//		uint32_t chksum = Chksum(head, ptr, length+4);
//		be32 = muduo::net::sockets::hostToNetwork32(chksum);
		buf.appendInt32(ChkSum(head, static_cast<const char *>(ptr), length+4));
		conn->send(&buf);
	}

	private:
		MBlockMessageCallback messageCallback_;
		const static size_t kHeaderLen = sizeof(int32_t);
		const static int32_t kSpec = static_cast<int32_t>(('Z'<<24) | ('E'<<16));
		const static int32_t kMinLength = 0x10;
};


#endif  // ZECODEC_H
