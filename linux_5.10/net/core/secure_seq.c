#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cryptohash.h>
#include <linux/module.h>
#include <linux/cache.h>
#include <linux/random.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/string.h>

#include <net/secure_seq.h>

static u32 net_secret[MD5_MESSAGE_BYTES / 4] ____cacheline_aligned;

static int __init net_secret_init(void)
{
	get_random_bytes(net_secret, sizeof(net_secret));
	return 0;
}
late_initcall(net_secret_init);

static u32 seq_scale(u32 seq)
{
	/*
	 *	As close as possible to RFC 793, which
	 *	suggests using a 250 kHz clock.
	 *	Further reading shows this assumes 2 Mb/s networks.
	 *	For 10 Mb/s Ethernet, a 1 MHz clock is appropriate.
	 *	For 10 Gb/s Ethernet, a 1 GHz clock should be ok, but
	 *	we also need to limit the resolution so that the u32 seq
	 *	overlaps less than one time per MSL (2 minutes).
	 *	Choosing a clock of 64 ns period is OK. (period of 274 s)
	 */
	return seq + (ktime_to_ns(ktime_get_real()) >> 6);
}

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
__u32 secure_tcpv6_sequence_number(__be32 *saddr, __be32 *daddr,
				   __be16 sport, __be16 dport)
{
	u32 secret[MD5_MESSAGE_BYTES / 4];
	u32 hash[MD5_DIGEST_WORDS];
	u32 i;

	memcpy(hash, saddr, 16);
	for (i = 0; i < 4; i++)
		secret[i] = net_secret[i] + daddr[i];
	secret[4] = net_secret[4] +
		(((__force u16)sport << 16) + (__force u16)dport);
	for (i = 5; i < MD5_MESSAGE_BYTES / 4; i++)
		secret[i] = net_secret[i];

	md5_transform(hash, secret);

	return seq_scale(hash[0]);
}
EXPORT_SYMBOL(secure_tcpv6_sequence_number);

u32 secure_ipv6_port_ephemeral(const __be32 *saddr, const __be32 *daddr,
			       __be16 dport)
{
	u32 secret[MD5_MESSAGE_BYTES / 4];
	u32 hash[MD5_DIGEST_WORDS];
	u32 i;

	memcpy(hash, saddr, 16);
	for (i = 0; i < 4; i++)
		secret[i] = net_secret[i] + (__force u32) daddr[i];
	secret[4] = net_secret[4] + (__force u32)dport;
	for (i = 5; i < MD5_MESSAGE_BYTES / 4; i++)
		secret[i] = net_secret[i];

	md5_transform(hash, secret);

	return hash[0];
}
#endif

#ifdef CONFIG_INET
__u32 secure_ip_id(__be32 daddr)
{
	u32 hash[MD5_DIGEST_WORDS];

	hash[0] = (__force __u32) daddr;
	hash[1] = net_secret[13];
	hash[2] = net_secret[14];
	hash[3] = net_secret[15];

	md5_transform(hash, net_secret);

	return hash[0];
}

__u32 secure_ipv6_id(const __be32 daddr[4])
{
	__u32 hash[4];

	memcpy(hash, daddr, 16);
	md5_transform(hash, net_secret);

	return hash[0];
}

__u32 secure_tcp_sequence_number(__be32 saddr, __be32 daddr,
				 __be16 sport, __be16 dport)
{
	u32 hash[MD5_DIGEST_WORDS];

	hash[0] = (__force u32)saddr;
	hash[1] = (__force u32)daddr;
	hash[2] = ((__force u16)sport << 16) + (__force u16)dport;
	hash[3] = net_secret[15];

	md5_transform(hash, net_secret);

	return seq_scale(hash[0]);
}

u32 secure_ipv4_port_ephemeral(__be32 saddr, __be32 daddr, __be16 dport)
{
	u32 hash[MD5_DIGEST_WORDS];

	hash[0] = (__force u32)saddr;
	hash[1] = (__force u32)daddr;
	hash[2] = (__force u32)dport ^ net_secret[14];
	hash[3] = net_secret[15];

	md5_transform(hash, net_secret);

	return hash[0];
}
EXPORT_SYMBOL_GPL(secure_ipv4_port_ephemeral);
#endif

#if defined(CONFIG_IP_DCCP) || defined(CONFIG_IP_DCCP_MODULE)
u64 secure_dccp_sequence_number(__be32 saddr, __be32 daddr,
				__be16 sport, __be16 dport)
{
	u32 hash[MD5_DIGEST_WORDS];
	u64 seq;

	hash[0] = (__force u32)saddr;
	hash[1] = (__force u32)daddr;
	hash[2] = ((__force u16)sport << 16) + (__force u16)dport;
	hash[3] = net_secret[15];

	md5_transform(hash, net_secret);

	seq = hash[0] | (((u64)hash[1]) << 32);
	seq += ktime_to_ns(ktime_get_real());
	seq &= (1ull << 48) - 1;

	return seq;
}
EXPORT_SYMBOL(secure_dccp_sequence_number);

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
u64 secure_dccpv6_sequence_number(__be32 *saddr, __be32 *daddr,
				  __be16 sport, __be16 dport)
{
	u32 secret[MD5_MESSAGE_BYTES / 4];
	u32 hash[MD5_DIGEST_WORDS];
	u64 seq;
	u32 i;

	memcpy(hash, saddr, 16);
	for (i = 0; i < 4; i++)
		secret[i] = net_secret[i] + daddr[i];
	secret[4] = net_secret[4] +
		(((__force u16)sport << 16) + (__force u16)dport);
	for (i = 5; i < MD5_MESSAGE_BYTES / 4; i++)
		secret[i] = net_secret[i];

	md5_transform(hash, secret);

	seq = hash[0] | (((u64)hash[1]) << 32);
	seq += ktime_to_ns(ktime_get_real());
	seq &= (1ull << 48) - 1;

	return seq;
}
EXPORT_SYMBOL(secure_dccpv6_sequence_number);
#endif
#endif
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2016 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cache.h>
#include <linux/random.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/string.h>
#include <linux/net.h>
#include <linux/siphash.h>
#include <net/secure_seq.h>

#if IS_ENABLED(CONFIG_IPV6) || IS_ENABLED(CONFIG_INET)
#include <linux/in6.h>
#include <net/tcp.h>

static siphash_key_t net_secret __read_mostly;
static siphash_key_t ts_secret __read_mostly;

static __always_inline void net_secret_init(void)
{
	net_get_random_once(&net_secret, sizeof(net_secret));
}

static __always_inline void ts_secret_init(void)
{
	net_get_random_once(&ts_secret, sizeof(ts_secret));
}
#endif

#ifdef CONFIG_INET
static u32 seq_scale(u32 seq)
{
	/*
	 *	As close as possible to RFC 793, which
	 *	suggests using a 250 kHz clock.
	 *	Further reading shows this assumes 2 Mb/s networks.
	 *	For 10 Mb/s Ethernet, a 1 MHz clock is appropriate.
	 *	For 10 Gb/s Ethernet, a 1 GHz clock should be ok, but
	 *	we also need to limit the resolution so that the u32 seq
	 *	overlaps less than one time per MSL (2 minutes).
	 *	Choosing a clock of 64 ns period is OK. (period of 274 s)
	 */
	return seq + (ktime_get_real_ns() >> 6);
}
#endif

#if IS_ENABLED(CONFIG_IPV6)
u32 secure_tcpv6_ts_off(const struct net *net,
			const __be32 *saddr, const __be32 *daddr)
{
	const struct {
		struct in6_addr saddr;
		struct in6_addr daddr;
	} __aligned(SIPHASH_ALIGNMENT) combined = {
		.saddr = *(struct in6_addr *)saddr,
		.daddr = *(struct in6_addr *)daddr,
	};

	if (net->ipv4.sysctl_tcp_timestamps != 1)
		return 0;

	ts_secret_init();
	return siphash(&combined, offsetofend(typeof(combined), daddr),
		       &ts_secret);
}
EXPORT_SYMBOL(secure_tcpv6_ts_off);

u32 secure_tcpv6_seq(const __be32 *saddr, const __be32 *daddr,
		     __be16 sport, __be16 dport)
{
	const struct {
		struct in6_addr saddr;
		struct in6_addr daddr;
		__be16 sport;
		__be16 dport;
	} __aligned(SIPHASH_ALIGNMENT) combined = {
		.saddr = *(struct in6_addr *)saddr,
		.daddr = *(struct in6_addr *)daddr,
		.sport = sport,
		.dport = dport
	};
	u32 hash;

	net_secret_init();
	hash = siphash(&combined, offsetofend(typeof(combined), dport),
		       &net_secret);
	return seq_scale(hash);
}
EXPORT_SYMBOL(secure_tcpv6_seq);

u32 secure_ipv6_port_ephemeral(const __be32 *saddr, const __be32 *daddr,
			       __be16 dport)
{
	const struct {
		struct in6_addr saddr;
		struct in6_addr daddr;
		__be16 dport;
	} __aligned(SIPHASH_ALIGNMENT) combined = {
		.saddr = *(struct in6_addr *)saddr,
		.daddr = *(struct in6_addr *)daddr,
		.dport = dport
	};
	net_secret_init();
	return siphash(&combined, offsetofend(typeof(combined), dport),
		       &net_secret);
}
EXPORT_SYMBOL(secure_ipv6_port_ephemeral);
#endif

#ifdef CONFIG_INET
u32 secure_tcp_ts_off(const struct net *net, __be32 saddr, __be32 daddr)
{
	if (net->ipv4.sysctl_tcp_timestamps != 1)
		return 0;

	ts_secret_init();
	return siphash_2u32((__force u32)saddr, (__force u32)daddr,
			    &ts_secret);
}

/* secure_tcp_seq_and_tsoff(a, b, 0, d) == secure_ipv4_port_ephemeral(a, b, d),
 * but fortunately, `sport' cannot be 0 in any circumstances. If this changes,
 * it would be easy enough to have the former function use siphash_4u32, passing
 * the arguments as separate u32.
 */
u32 secure_tcp_seq(__be32 saddr, __be32 daddr,
		   __be16 sport, __be16 dport)
{
	u32 hash;

	net_secret_init();
	hash = siphash_3u32((__force u32)saddr, (__force u32)daddr,
			    (__force u32)sport << 16 | (__force u32)dport,
			    &net_secret);
	return seq_scale(hash);
}
EXPORT_SYMBOL_GPL(secure_tcp_seq);

u32 secure_ipv4_port_ephemeral(__be32 saddr, __be32 daddr, __be16 dport)
{
	net_secret_init();
	return siphash_3u32((__force u32)saddr, (__force u32)daddr,
			    (__force u16)dport, &net_secret);
}
EXPORT_SYMBOL_GPL(secure_ipv4_port_ephemeral);
#endif

#if IS_ENABLED(CONFIG_IP_DCCP)
u64 secure_dccp_sequence_number(__be32 saddr, __be32 daddr,
				__be16 sport, __be16 dport)
{
	u64 seq;
	net_secret_init();
	seq = siphash_3u32((__force u32)saddr, (__force u32)daddr,
			   (__force u32)sport << 16 | (__force u32)dport,
			   &net_secret);
	seq += ktime_get_real_ns();
	seq &= (1ull << 48) - 1;
	return seq;
}
EXPORT_SYMBOL(secure_dccp_sequence_number);

#if IS_ENABLED(CONFIG_IPV6)
u64 secure_dccpv6_sequence_number(__be32 *saddr, __be32 *daddr,
				  __be16 sport, __be16 dport)
{
	const struct {
		struct in6_addr saddr;
		struct in6_addr daddr;
		__be16 sport;
		__be16 dport;
	} __aligned(SIPHASH_ALIGNMENT) combined = {
		.saddr = *(struct in6_addr *)saddr,
		.daddr = *(struct in6_addr *)daddr,
		.sport = sport,
		.dport = dport
	};
	u64 seq;
	net_secret_init();
	seq = siphash(&combined, offsetofend(typeof(combined), dport),
		      &net_secret);
	seq += ktime_get_real_ns();
	seq &= (1ull << 48) - 1;
	return seq;
}
EXPORT_SYMBOL(secure_dccpv6_sequence_number);
#endif
#endif
