#ifndef _M_IP_H__
#define _M_IP_H__

#define CLICK_BYTE_ORDER 1234 //FIXME: hard coded
#define CLICK_LITTLE_ENDIAN 1234
#define CLICK_BIG_ENDIAN 4321

struct click_ip {
#if CLICK_BYTE_ORDER == CLICK_BIG_ENDIAN
    unsigned	ip_v : 4;		/* 0     version == 4		     */
    unsigned	ip_hl : 4;		/*       header length		     */
#elif CLICK_BYTE_ORDER == CLICK_LITTLE_ENDIAN
    unsigned	ip_hl : 4;		/* 0     header length		     */
    unsigned	ip_v : 4;		/*       version == 4		     */
#else
#   error "unknown byte order"
#endif
    uint8_t	ip_tos;			/* 1     type of service	     */
#define IP_DSCPMASK	0xFC		/*         diffserv code point	     */
#define IP_ECNMASK	0x03		/*	   ECN code point	     */
#define   IP_ECN_NOT_ECT  0x00		/*         not ECN capable transport */
#define   IP_ECN_ECT1	  0x01		/*         ECN capable transport     */
#define   IP_ECN_ECT2	  0x02		/*         ECN capable transport     */
#define   IP_ECN_CE	  0x03		/*         ECN congestion exp'd	     */
    uint16_t	ip_len;			/* 2-3   total length		     */
    uint16_t	ip_id;			/* 4-5   identification		     */
    uint16_t	ip_off;			/* 6-7   fragment offset field	     */
#define	IP_RF		0x8000		/*         reserved fragment flag    */
#define	IP_DF		0x4000		/*         don't fragment flag	     */
#define	IP_MF		0x2000		/*         more fragments flag	     */
#define	IP_OFFMASK	0X1FFF		/*         mask for fragmenting bits */
    uint8_t	ip_ttl;			/* 8     time to live		     */
    uint8_t	ip_p;			/* 9     protocol		     */
    uint16_t	ip_sum;			/* 10-11 checksum		     */
    struct in_addr ip_src;		/* 12-15 source address		     */
    struct in_addr ip_dst;		/* 16-19 destination address	     */
};

/* ip_protocol */
#define IP_PROTO_ICMP		1
#define IP_PROTO_IGMP		2
#define IP_PROTO_GGP		3
#define IP_PROTO_IPIP		4
#define IP_PROTO_ST		5
#define IP_PROTO_TCP		6
#define IP_PROTO_UCL		7
#define IP_PROTO_EGP		8
#define IP_PROTO_IGP		9
#define IP_PROTO_BBN		10
#define IP_PROTO_NVPII		11
#define IP_PROTO_PUP		12
#define IP_PROTO_ARGUS		13
#define IP_PROTO_EMCON		14
#define IP_PROTO_XNET		15
#define IP_PROTO_CHAOS		16
#define IP_PROTO_UDP		17
#define IP_PROTO_MUX		18
#define IP_PROTO_DCN		19
#define IP_PROTO_HMP		20
#define IP_PROTO_PRM		21
#define IP_PROTO_XNS		22
#define IP_PROTO_TRUNK1		23
#define IP_PROTO_TRUNK2		24
#define IP_PROTO_LEAF1		25
#define IP_PROTO_LEAF2		26
#define IP_PROTO_RDP		27
#define IP_PROTO_IRTP		28
#define IP_PROTO_ISOTP4		29
#define IP_PROTO_NETBLT		30
#define IP_PROTO_MFENSP		31
#define IP_PROTO_MERIT		32
#define IP_PROTO_SEP		33
#define IP_PROTO_ICMP6          58
#define IP_PROTO_CFTP		62
#define IP_PROTO_SATNET		64
#define IP_PROTO_MITSUBNET	65
#define IP_PROTO_RVD		66
#define IP_PROTO_IPPC		67
#define IP_PROTO_SATMON		69
#define IP_PROTO_IPCV		71
#define IP_PROTO_BRSATMON	76
#define IP_PROTO_WBMON		78
#define IP_PROTO_WBEXPAK	79

#define IP_PROTO_NONE		257
#define IP_PROTO_TRANSP		258
#define IP_PROTO_TCP_OR_UDP	256
#define IP_PROTO_PAYLOAD	259

#define	IPOPT_EOL		0	/* end of option list */
#define	IPOPT_NOP		1	/* no operation */
#define IPOPT_RR		7	/* record packet route */
#define IPOPT_TS		68	/* timestamp */
#define IPOPT_SECURITY		130	/* provide s,c,h,tcc */
#define IPOPT_LSRR		131	/* loose source route */
#define IPOPT_SATID		136	/* satnet id */
#define IPOPT_SSRR		137	/* strict source route */
#define IPOPT_RA		148	/* router alert */

#define IP_ISFRAG(iph)	  (((iph)->ip_off & htons(IP_MF | IP_OFFMASK)) != 0)
#define IP_FIRSTFRAG(iph) (((iph)->ip_off & htons(IP_OFFMASK)) == 0)


#endif
