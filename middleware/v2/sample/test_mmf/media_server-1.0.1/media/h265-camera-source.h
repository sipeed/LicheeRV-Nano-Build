#ifndef _h265_camera_source_h_
#define _h265_camera_source_h_

#include "media-source.h"
#include "sys/process.h"
#include "time64.h"
#include "rtp.h"
#include <string>
#include "pthread.h"
#include <list>

class H265CameraSource : public IMediaSource
{
public:
	H265CameraSource(const char *file);
	virtual ~H265CameraSource();

public:
	virtual int Play();
	virtual int Pause();
	virtual int Seek(int64_t pos);
	virtual int SetSpeed(double speed);
	virtual int GetDuration(int64_t& duration) const;
	virtual int GetSDPMedia(std::string& sdp) const;
	virtual int GetRTPInfo(const char* uri, char *rtpinfo, size_t bytes) const;
	virtual int SetTransport(const char* track, std::shared_ptr<IRTPTransport> transport);
	int SetNextFrame(const uint8_t* ptr, size_t bytes);

private:
	int GetNextFrame(int64_t &dts, const uint8_t* &ptr, size_t &bytes);
	int FreeNextFrame();

	static void OnRTCPEvent(void* param, const struct rtcp_msg_t* msg);
	void OnRTCPEvent(const struct rtcp_msg_t* msg);
	int SendRTCP();

	static void* RTPAlloc(void* param, int bytes);
	static void RTPFree(void* param, void *packet);
	static int RTPPacket(void* param, const void *packet, int bytes, uint32_t timestamp, int flags);

private:
	void* m_rtp;
	uint32_t m_timestamp;
	time64_t m_rtp_clock;
	time64_t m_rtcp_clock;
	time64_t m_last_push_frame;

    // H265CameraReader m_reader;
	std::shared_ptr<IRTPTransport> m_transport;

	struct vframe_t
	{
		const uint8_t* nalu;
		int64_t time;
		long bytes;
		bool idr; // IDR frame

		bool operator < (const struct vframe_t &v) const
		{
			return time < v.time;
		}
	};
	typedef std::list<vframe_t> lframes_t;
	lframes_t m_videos_list;
	lframes_t::iterator m_video;
	pthread_mutex_t m_lock;

	std::list<std::pair<const uint8_t*, size_t> > m_sps;

	int m_status;
	int64_t m_pos;
	double m_speed;

	void *m_rtppacker;
	unsigned char m_packet[MAX_UDP_PACKET+14];
};

#endif /* !_h265_camera_source_h_ */
