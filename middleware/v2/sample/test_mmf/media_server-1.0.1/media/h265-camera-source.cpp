#include "h265-camera-source.h"
#include "cstringext.h"
#include "base64.h"
#include "rtp-profile.h"
#include "rtp-payload.h"
#include <assert.h>

#define FRAME_TIME_MS (33)
#define H265_NAL(v)	 ((v>> 1) & 0x3f)

enum { NAL_IDR_W_RADL = 19, NAL_IDR_N_LP= 20, NAL_VPS = 32, NAL_SPS = 33, NAL_PPS = 34, NAL_SEI = 39};

extern "C" uint32_t rtp_ssrc(void);

H265CameraSource::H265CameraSource(const char *file)
{
	m_speed = 1.0;
	m_status = 0;
	m_rtp_clock = 0;
	m_rtcp_clock = 0;
	m_timestamp = 0;
	m_last_push_frame = 0;

	(void)file;
	uint32_t ssrc = rtp_ssrc();
	static struct rtp_payload_t s_rtpfunc = {
		H265CameraSource::RTPAlloc,
		H265CameraSource::RTPFree,
		H265CameraSource::RTPPacket,
	};
	m_rtppacker = rtp_payload_encode_create(RTP_PAYLOAD_H265, "H265", (uint16_t)ssrc, ssrc, &s_rtpfunc, this);

	struct rtp_event_t event;
	event.on_rtcp = OnRTCPEvent;
	m_rtp = rtp_create(&event, this, ssrc, m_timestamp, 90000, 4*1024, 1);
	rtp_set_info(m_rtp, "RTSPServer", "szj.h265");

	pthread_mutex_init(&m_lock, NULL);
	pthread_mutex_unlock(&m_lock);
}

H265CameraSource::~H265CameraSource()
{
	if(m_rtp)
		rtp_destroy(m_rtp);

	if(m_rtppacker)
		rtp_payload_encode_destroy(m_rtppacker);

	std::list<std::pair<const uint8_t*, size_t>>::iterator sps_iter;
	for (sps_iter = m_sps.begin(); sps_iter != m_sps.end(); sps_iter ++) {
		if (sps_iter->first) {
			free((void *)sps_iter->first);
		}
	}

	pthread_mutex_destroy(&m_lock);
}

int H265CameraSource::SetTransport(const char* /*track*/, std::shared_ptr<IRTPTransport> transport)
{
	m_transport = transport;
	return 0;
}

int H265CameraSource::Play()
{
	m_status = 1;
//
	//uint32_t timestamp = 0;
	time64_t clock = time64_now();
	if (0 == m_rtp_clock)
		m_rtp_clock = clock;

	if(m_rtp_clock + 10 < clock)
	{
		size_t bytes;
		const uint8_t* ptr;

		pthread_mutex_lock(&m_lock);
		if(0 == GetNextFrame(m_pos, ptr, bytes))
		{
			// for(int i=0;i<bytes;i++)
			// 	printf("%02x ",ptr[i]);
			// printf("nalu over\n\n\n");
			rtp_payload_encode_input(m_rtppacker, ptr, bytes, m_timestamp * 90 /*kHz*/);
			m_rtp_clock += 10;
			m_timestamp += FRAME_TIME_MS;

			SendRTCP();

			FreeNextFrame();
			pthread_mutex_unlock(&m_lock);
			return 1;
		}
		pthread_mutex_unlock(&m_lock);
	}

	return 0;
}


static inline const uint8_t* search_start_code(const uint8_t* ptr, const uint8_t* end)
{
    for(const uint8_t *p = ptr; p + 3 < end; p++)
    {
        if(0x00 == p[0] && 0x00 == p[1] && (0x01 == p[2] || (0x00==p[2] && 0x01==p[3])))
            return p;
    }
	return end;
}

static inline int h265_nal_type(const unsigned char* ptr)
{
    int i = 2;
    assert(0x00 == ptr[0] && 0x00 == ptr[1]);
    if(0x00 == ptr[2])
        ++i;
    assert(0x01 == ptr[i]);
    return H265_NAL(ptr[i+1]);
}


int H265CameraSource::SetNextFrame(const uint8_t* data, size_t bytes)
{
	pthread_mutex_lock(&m_lock);
	const uint8_t* p = data;
	if(!(0x00 == p[0] && 0x00 == p[1] && (0x01 == p[2] || (0x00==p[2] && 0x01==p[3])))) {
		printf("[%s][%d] invalid data\n", __func__, __LINE__);
		pthread_mutex_unlock(&m_lock);
		return 0;
	}

	int nal_unit_type = h265_nal_type(p);
	if (nal_unit_type < 0) {
		printf("[%s][%d] invalid data\n", __func__, __LINE__);
		pthread_mutex_unlock(&m_lock);
		return 0;
	}

	if(m_sps.size() == 0) {
		if(NAL_VPS == nal_unit_type || NAL_SPS == nal_unit_type || NAL_PPS == nal_unit_type)
        {
			// size_t n = 0x01 == p[2] ? 3 : 4;
			std::pair<const uint8_t*, size_t> pr;
			pr.second = bytes;
			pr.first = (const uint8_t*)malloc(bytes);
			assert(pr.first);
			memcpy((uint8_t *)pr.first, p, bytes);
			m_sps.push_back(pr);
		} else {
			pthread_mutex_unlock(&m_lock);
			return 0;
		}
	}

	time64_t curr_ms = time64_now();
	time64_t frame_time;
	if (m_last_push_frame == 0) {
		frame_time = 0;
	} else {
		frame_time = curr_ms - m_last_push_frame;
	}
	m_last_push_frame = curr_ms;

	vframe_t frame;
	vframe_t last_frame = m_videos_list.back();
	frame.bytes = bytes;
	frame.idr = (NAL_IDR_N_LP == nal_unit_type || NAL_IDR_W_RADL == nal_unit_type); // IDR-frame;
	frame.time = frame_time + last_frame.time;
	frame.nalu = (const uint8_t*)malloc(bytes);
	assert(frame.nalu);
	memcpy((void *)frame.nalu, data, bytes);
	m_videos_list.push_back(frame);

	pthread_mutex_unlock(&m_lock);
	return 0;
}

int H265CameraSource::GetNextFrame(int64_t &dts, const uint8_t* &ptr, size_t &bytes)
{
	lframes_t::iterator frame = m_videos_list.begin();
	if (frame == m_videos_list.end()) {
		return -1;
	}

	ptr = frame->nalu;
	dts = frame->time;
	bytes = frame->bytes;

	return 0;
}

int H265CameraSource::FreeNextFrame()
{
	lframes_t::iterator frame = m_videos_list.begin();
	if (frame != m_videos_list.end()) {
		free((void *)frame->nalu);
		m_videos_list.pop_front();
	}
	// printf("FreeNextFrame remain size:%ld\n", m_videos_list.size());
	return 0;
}


int H265CameraSource::Pause()
{
	m_status = 2;
	m_rtp_clock = 0;
	return 0;
}

int H265CameraSource::Seek(int64_t pos)
{
	m_pos = pos;
	m_rtp_clock = 0;
	return 0;
}

int H265CameraSource::SetSpeed(double speed)
{
	m_speed = speed;
	return 0;
}

int H265CameraSource::GetDuration(int64_t& duration) const
{
	(void)duration;
	return 0;
}

int H265CameraSource::GetSDPMedia(std::string& sdp) const
{
    static const char* pattern =
        "m=video 0 RTP/AVP %d\n"
        "a=rtpmap:%d H265/90000\n"
        "a=fmtp:%d profile-level-id=%02X%02X%02X;"
    			 "packetization-mode=1;"
    			 "sprop-parameter-sets=";

    char base64[512] = {0};
    std::string parameters;

    const std::list<std::pair<const uint8_t*, size_t> >& sps = m_sps;
    std::list<std::pair<const uint8_t*, size_t> >::const_iterator it;
    for(it = sps.begin(); it != sps.end(); ++it)
    {
        if(parameters.empty())
        {
            snprintf(base64, sizeof(base64), pattern, 
				RTP_PAYLOAD_H265, RTP_PAYLOAD_H265,RTP_PAYLOAD_H265, 
				(unsigned int)(it->first[1]), (unsigned int)(it->first[2]), (unsigned int)(it->first[3]));
            sdp = base64;
        }
        else
        {
            parameters += ',';
        }

        size_t bytes = it->second;
        assert((bytes+2)/3*4 + bytes/57 + 1 < sizeof(base64));
        bytes = base64_encode(base64, it->first, bytes);
		base64[bytes] = '\0';
        assert(strlen(base64) > 0);
        parameters += base64;
    }

    sdp += parameters;
    sdp += '\n';
    return sps.empty() ? -1 : 0;
}

int H265CameraSource::GetRTPInfo(const char* uri, char *rtpinfo, size_t bytes) const
{
	uint16_t seq;
	uint32_t timestamp;
	rtp_payload_encode_getinfo(m_rtppacker, &seq, &timestamp);

	// url=rtsp://video.example.com/twister/video;seq=12312232;rtptime=78712811
	snprintf(rtpinfo, bytes, "url=%s;seq=%hu;rtptime=%u", uri, seq, timestamp);
	return 0;
}

void H265CameraSource::OnRTCPEvent(const struct rtcp_msg_t* msg)
{
	(void)msg;
}

void H265CameraSource::OnRTCPEvent(void* param, const struct rtcp_msg_t* msg)
{
	H265CameraSource *self = (H265CameraSource *)param;
	self->OnRTCPEvent(msg);
}

int H265CameraSource::SendRTCP()
{
	// make sure have sent RTP packet

	time64_t clock = time64_now();
	int interval = rtp_rtcp_interval(m_rtp);
	if(0 == m_rtcp_clock || m_rtcp_clock + interval < clock)
	{
		char rtcp[1024] = {0};
		size_t n = rtp_rtcp_report(m_rtp, rtcp, sizeof(rtcp));

		// send RTCP packet
		m_transport->Send(true, rtcp, n);

		m_rtcp_clock = clock;
	}
	
	return 0;
}

void* H265CameraSource::RTPAlloc(void* param, int bytes)
{
	H265CameraSource *self = (H265CameraSource*)param;
	assert(bytes <= (int)sizeof(self->m_packet));
	return self->m_packet;
}

void H265CameraSource::RTPFree(void* param, void *packet)
{
	H265CameraSource *self = (H265CameraSource*)param;
	assert(self->m_packet == packet);
}

int H265CameraSource::RTPPacket(void* param, const void *packet, int bytes, uint32_t /*timestamp*/, int /*flags*/)
{
	H265CameraSource *self = (H265CameraSource*)param;
	assert(self->m_packet == packet);
	// const char* ptr = (const char*)packet;
	// for(int i=0;i<bytes;i++)
	// 	printf("%02x ",ptr[i]);
	// printf("RTPPacket over\n\n\n");
	// if(i++ > 4){
	// 	exit(-1);
	// }
	int r = self->m_transport->Send(false, packet, bytes);
	if (r != bytes)
		return -1;

	return rtp_onsend(self->m_rtp, packet, bytes/*, time*/);
}
