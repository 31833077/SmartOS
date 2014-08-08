#include "TinyIP.h"

TinyIP::TinyIP(Enc28j60* enc, byte ip[4], byte mac[6])
{
	_enc = enc;
	memcpy(IP, ip, 4);
	memcpy(Mac, mac, 6);

	Buffer = NULL;
	BufferSize = 1500;

	seqnum = 0xa;
}

TinyIP::~TinyIP()
{
    _enc = NULL;
	if(Buffer) delete Buffer;
}

void TinyIP::TcpClose(byte* buf, uint size)
{
    // fill the header:
    // This code requires that we send only one data packet
    // because we keep no state information. We must therefore set
    // the fin here:
    buf[TCP_FLAGS_P] = TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V|TCP_FLAGS_FIN_V;

    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp (when no options) + len of data
    uint j = IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + size;
    buf[IP_TOTLEN_H_P] = j >> 8;
    buf[IP_TOTLEN_L_P] = j & 0xff;
    fill_ip_hdr_checksum(buf);
    // zero the checksum
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;
    // calculate the checksum, len=8 (start from ip.src)  +  TCP_HEADER_LEN_PLAIN  +  data len
    j = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + size,2);
    buf[TCP_CHECKSUM_H_P] = j >> 8;
    buf[TCP_CHECKSUM_L_P] = j & 0xff;
    _enc->PacketSend(buf, IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + size + ETH_HEADER_LEN);
}

void TinyIP::TcpSend(byte* buf, uint size)
{
    // fill the header:
    // This code requires that we send only one data packet
    // because we keep no state information. We must therefore set
    // the fin here:
    buf[TCP_FLAGS_P] = TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V;

    // total length field in the IP header must be set:
    // 20 bytes IP  +  20 bytes tcp (when no options)  +  len of data
    uint j = IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + size;
    buf[IP_TOTLEN_H_P] = j >> 8;
    buf[IP_TOTLEN_L_P] = j & 0xff;
    fill_ip_hdr_checksum(buf);
    // zero the checksum
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;
    // calculate the checksum, len=8 (start from ip.src)  +  TCP_HEADER_LEN_PLAIN  +  data len
    j = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + size, 2);
    buf[TCP_CHECKSUM_H_P] = j >> 8;
    buf[TCP_CHECKSUM_L_P] = j & 0xff;
		//debug_printf("len=%d\r\n",tcp_d_len);
    _enc->PacketSend(buf, IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + size + ETH_HEADER_LEN);
}

void TinyIP::Start()
{
	// ���仺�������Ƚϴ�С��ջ�ռ䲻��
	if(!Buffer) Buffer = new byte[BufferSize + 1];
	assert_param(Buffer);
	assert_param(Sys.CheckMemory());
	byte* buf = Buffer;

    // ��ʼ�� enc28j60 ��MAC��ַ(�����ַ),�����������Ҫ����һ��
    _enc->Init((string)Mac);

    // ��enc28j60�������ŵ�ʱ�������Ϊ��from 6.25MHz to 12.5MHz(�����̸�����NC,û�õ�)
    _enc->ClockOut(2);

    while(1)
    {
        // ��ȡ�������İ�
        uint len = _enc->PacketReceive(buf, BufferSize);

        // �������������û��������ת����һ��ѭ��
        if(!len) continue;

        if(eth_type_is_arp_and_my_ip(buf, len))
        {
            make_arp_answer_from_request(buf);
            continue;
        }

        debug_printf("Packet: %d\r\n", len);

        for(int i=0; i<4; i++)
        {
            debug_printf("begin:\r\n");
            debug_printf("TCP_FLAGS_P %d\r\n", buf[TCP_FLAGS_P]);
            debug_printf("\r\n");
        }

        // �ж��Ƿ�Ϊ���͸�����ip�İ�
        if(!eth_type_is_ip_and_my_ip(buf, len)) continue;

        // ICMPЭ���������Ƿ�ΪICMP���� ping
        if(buf[IP_PROTO_P] == IP_PROTO_ICMP_V)
        {
			ProcessICMP(buf, len);
            continue;
        }

        //���ڲ鿴���ݰ��ı�־λ
        if (buf[IP_PROTO_P] == IP_PROTO_TCP_V)
        {
			ProcessTcp(buf, len);
			continue;
        }
        if (buf[IP_PROTO_P] == IP_PROTO_UDP_V /*&& buf[UDP_DST_PORT_H_P] == 4*/)
        {
			ProcessUdp(buf, len);
			continue;
        }

        debug_printf("Something accessed me....\r\n");
    }
}

void TinyIP::ProcessICMP(byte* buf, uint len)
{
	if(buf[ICMP_TYPE_P] != ICMP_TYPE_ECHOREQUEST_V) return;

	debug_printf("from "); // ��ӡ������ip
	ShowIP(&buf[IP_SRC_P]);
	debug_printf(" ICMP package.\r\n");
	make_echo_reply_from_request(buf, len);
}

void TinyIP::ProcessTcp(byte* buf, uint len)
{
	//��ȡĿ�����TCP�˿�
	//ushort tcp_port = buf[TCP_DST_PORT_H_P] << 8 | buf[TCP_DST_PORT_L_P];

	// ��һ��ͬ��Ӧ��
	if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) // SYN���������־λ��Ϊ1��ʾ�������ӵ��������ݰ�
	{
		debug_printf("One TCP request from "); // ��ӡ���ͷ���ip
		ShowIP(&buf[IP_SRC_P]);
		debug_printf("\r\n");

		//�ڶ���ͬ��Ӧ��
		make_tcp_synack_from_syn(buf);

		return;
	}
	// ������ͬ��Ӧ��,����Ӧ��󷽿ɴ�������
	if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V) // ACKȷ�ϱ�־λ��Ϊ1��ʾ�����ݰ�ΪӦ�����ݰ�
	{
		init_len_info(buf);
		uint dat_p = get_tcp_data_pointer();

		//�����ݷ���ACK
		if (dat_p == 0)
		{
			if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V)      //FIN�������������־λ��Ϊ1��ʾ�ǽ������ӵ��������ݰ�
			{
				make_tcp_ack_from_any(buf);
			}
			return;
		}
		///////////////////////////��ӡTCP����/////////////////
		debug_printf("Data from TCP:");
		uint i = 0;
		while(i < tcp_d_len)
		{
			debug_printf("%c", buf[dat_p + i]);
			i++;
		}
		debug_printf("\r\n");
		///////////////////////////////////////////////////////
		make_tcp_ack_from_any(buf);       // ����ACK��֪ͨ���յ�
		TcpSend(buf, len);

		// tcp_close(buf,len);
		// for(;reset<BufferSize + 1;reset++)
		// 	buf[BufferSize + 1] = 0;
	}
}

void TinyIP::ProcessUdp(byte* buf, uint len)
{
	//��ȡĿ���UDP�Ķ˿�
	//ushort udp_port = buf[UDP_DST_PORT_H_P] << 8 | buf[UDP_DST_PORT_L_P];

	debug_printf("Data from UDP:");
	//UDP���ݳ���
	uint payloadlen = buf[UDP_LEN_H_P];
	payloadlen = payloadlen << 8;
	payloadlen = (payloadlen + buf[UDP_LEN_L_P]) - UDP_HEADER_LEN;

	byte* buf2 = new byte[payloadlen];
	for(int i=0; i<payloadlen; i++)
	{
		buf2[i] = buf[UDP_DATA_P + i];
		debug_printf("%c", buf2[i]);
	}
	debug_printf("\r\n");

	//��ȡ����Դ�˿�
	ushort pc_port = buf[UDP_SRC_PORT_H_P] << 8 | buf[UDP_SRC_PORT_L_P];
	make_udp_reply_from_request(buf, buf2, payloadlen, pc_port);
}

void TinyIP::ShowIP(byte* ip)
{
	debug_printf("%d", *ip++);
	for(int i=1; i<4; i++)
		debug_printf(".%d", *ip++);
}

void TinyIP::ShowMac(byte* mac)
{
	debug_printf("%02X", *mac++);
	for(int i=1; i<6; i++)
		debug_printf("-%02X", *mac++);
}

// The Ip checksum is calculated over the ip header only starting
// with the header length field and a total length of 20 bytes
// unitl ip.dst
// You must set the IP checksum field to zero before you start
// the calculation.
// len for ip is 20.
//
// For UDP/TCP we do not make up the required pseudo header. Instead we 
// use the ip.src and ip.dst fields of the real packet:
// The udp checksum calculation starts with the ip.src field
// Ip.src=4bytes,Ip.dst=4 bytes,Udp header=8bytes + data length=16+len
// In other words the len here is 8 + length over which you actually
// want to calculate the checksum.
// You must set the checksum field to zero before you start
// the calculation.
// len for udp is: 8 + 8 + data length
// len for tcp is: 4+4 + 20 + option len + data length
//
// For more information on how this algorithm works see:
// http://www.netfor2.com/checksum.html
// http://www.msc.uky.edu/ken/cs471/notes/chap3.htm
// The RFC has also a C code example: http://www.faqs.org/rfcs/rfc1071.html
uint TinyIP::checksum(byte* buf, uint len,byte type)
{
    // type 0=ip 
    //      1=udp
    //      2=tcp
    unsigned long sum = 0;
    
    //if(type==0){
    //        // do not add anything
    //}
    if(type==1)
    {
        sum+=IP_PROTO_UDP_V; // protocol udp
        // the length here is the length of udp (data+header len)
        // =length given to this function - (IP.scr+IP.dst length)
        sum+=len-8; // = real tcp len
    }
    if(type==2)
    {
        sum+=IP_PROTO_TCP_V; 
        // the length here is the length of tcp (data+header len)
        // =length given to this function - (IP.scr+IP.dst length)
        sum+=len-8; // = real tcp len
    }
    // build the sum of 16bit words
    while(len >1)
    {
        sum += 0xFFFF & (*buf<<8|*(buf+1));
        buf+=2;
        len-=2;
    }
    // if there is a byte left then add it (padded with zero)
    if (len)
    {
        sum += (0xFF & *buf)<<8;
    }
    // now calculate the sum over the bytes in the sum
    // until the result is only 16bit long
    while (sum>>16)
    {
        sum = (sum & 0xFFFF)+(sum >> 16);
    }
    // build 1's complement:
    return( (uint) sum ^ 0xFFFF);
}

//����Ƿ�Ϊ�Ϸ���eth������ֻ���շ���������arp����
byte TinyIP::eth_type_is_arp_and_my_ip(byte* buf, uint len)
{
    //arp���ĳ����жϣ�������arp���ĳ���Ϊ42byts
    if (len < 41) return false;

	//����Ƿ�Ϊarp����
    if(buf[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V || buf[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V) return false;

    for(int i=0; i<4; i++)
    {
        if(buf[ETH_ARP_DST_IP_P+i] != IP[i]) return false;
    }
    return true;
}

byte TinyIP::eth_type_is_ip_and_my_ip(byte* buf, uint len)
{
    byte i=0;
    //eth+ip+udp header is 42
    if (len < 42) return false;

    if(buf[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V || buf[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V) return false;

    if (buf[IP_HEADER_LEN_VER_P] != 0x45)
    {
        // must be IP V4 and 20 byte header
        return false;
    }
    while(i<4)
    {
        if(buf[IP_DST_P+i] != IP[i])
        {
            return false;
        }
        i++;
    }
    return true;
}
// make a return eth header from a received eth packet
void TinyIP::make_eth(byte* buf)
{
    //copy the destination mac from the source and fill my mac into src
    for(int i=0; i<6; i++)
    {
        buf[ETH_DST_MAC +i] = buf[ETH_SRC_MAC +i];
        buf[ETH_SRC_MAC +i] = Mac[i];
    }
}
void TinyIP::fill_ip_hdr_checksum(byte* buf)
{
    uint ck;
    // clear the 2 byte checksum
    buf[IP_CHECKSUM_P]=0;
    buf[IP_CHECKSUM_P+1]=0;
    buf[IP_FLAGS_P]=0x40; // don't fragment
    buf[IP_FLAGS_P+1]=0;  // fragement offset
    buf[IP_TTL_P]=64; // ttl
    // calculate the checksum:
    ck=checksum(&buf[IP_P], IP_HEADER_LEN,0);
    buf[IP_CHECKSUM_P]=ck>>8;
    buf[IP_CHECKSUM_P+1]=ck& 0xff;
}

// make a return ip header from a received ip packet
void TinyIP::make_ip(byte* buf)
{
    for(int i=0; i<4; i++)
    {
        buf[IP_DST_P+i] = buf[IP_SRC_P+i];
        buf[IP_SRC_P+i] = IP[i];
    }
    fill_ip_hdr_checksum(buf);
}

// make a return tcp header from a received tcp packet
// rel_ack_num is how much we must step the seq number received from the
// other side. We do not send more than 255 bytes of text (=data) in the tcp packet.
// If mss=1 then mss is included in the options list
//
// After calling this function you can fill in the first data byte at TCP_OPTIONS_P+4
// If cp_seq=0 then an initial sequence number is used (should be use in synack)
// otherwise it is copied from the packet we received
void TinyIP::make_tcphead(byte* buf, uint rel_ack_num, byte mss, byte cp_seq)
{
    byte i=0;
    byte tseq;
    while(i<2)
    {
        buf[TCP_DST_PORT_H_P + i] = buf[TCP_SRC_PORT_H_P + i];
        buf[TCP_SRC_PORT_H_P + i] = 0; // clear source port
        i++;
    }
    // set source port  (http):
   // buf[TCP_SRC_PORT_L_P]=wwwport;                //Դ��//////////////////////////////////////////////
		//debug_printf("%d\r\n",src_port_H<<8 | src_port_L);  
	buf[TCP_SRC_PORT_H_P] = RemotePort >> 8;								//�Լ����
	buf[TCP_SRC_PORT_L_P] = RemotePort & 0xFF;								//

    i=4;
    // sequence numbers:
    // add the rel ack num to SEQACK
    while(i>0)
    {
        rel_ack_num=buf[TCP_SEQ_H_P+i-1]+rel_ack_num;
        tseq=buf[TCP_SEQACK_H_P+i-1];
        buf[TCP_SEQACK_H_P+i-1]=0xff&rel_ack_num;
        if (cp_seq)
        {
            // copy the acknum sent to us into the sequence number
            buf[TCP_SEQ_H_P+i-1]=tseq;
        }
        else
        {
            buf[TCP_SEQ_H_P+i-1]= 0; // some preset vallue
        }
        rel_ack_num=rel_ack_num>>8;
        i--;
    }
    if (cp_seq==0)
    {
        // put inital seq number
        buf[TCP_SEQ_H_P+0]= 0;
        buf[TCP_SEQ_H_P+1]= 0;
        // we step only the second byte, this allows us to send packts 
        // with 255 bytes or 512 (if we step the initial seqnum by 2)
        buf[TCP_SEQ_H_P+2]= seqnum; 
        buf[TCP_SEQ_H_P+3]= 0;
        // step the inititial seq num by something we will not use
        // during this tcp session:
        seqnum+=2;
    }
    // zero the checksum
    buf[TCP_CHECKSUM_H_P]=0;
    buf[TCP_CHECKSUM_L_P]=0;
    
    // The tcp header length is only a 4 bit field (the upper 4 bits).
    // It is calculated in units of 4 bytes. 
    // E.g 24 bytes: 24/4=6 => 0x60=header len field
    //buf[TCP_HEADER_LEN_P]=(((TCP_HEADER_LEN_PLAIN+4)/4)) <<4; // 0x60
    if (mss)
    {
        // the only option we set is MSS to 1408:
        // 1408 in hex is 0x580
        buf[TCP_OPTIONS_P]=2;
        buf[TCP_OPTIONS_P+1]=4;
        buf[TCP_OPTIONS_P+2]=0x05; 
        buf[TCP_OPTIONS_P+3]=0x80;
        // 24 bytes:
        buf[TCP_HEADER_LEN_P]=0x60;
    }
    else
    {
        // no options:
        // 20 bytes:
        buf[TCP_HEADER_LEN_P]=0x50;
    }
}

void TinyIP::make_arp_answer_from_request(byte* buf)
{
    make_eth(buf);
    buf[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;   //arp ��Ӧ
    buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
    // fill the mac addresses:
    for(int i=0; i<6; i++)
    {
        buf[ETH_ARP_DST_MAC_P+i] = buf[ETH_ARP_SRC_MAC_P+i];
        buf[ETH_ARP_SRC_MAC_P+i] = Mac[i];
    }
    for(int i=0; i<4; i++)
    {
        buf[ETH_ARP_DST_IP_P+i] = buf[ETH_ARP_SRC_IP_P+i];
        buf[ETH_ARP_SRC_IP_P+i] = IP[i];
    }
    // eth+arp is 42 bytes:
    _enc->PacketSend(buf, 42); 
}

void TinyIP::make_echo_reply_from_request(byte* buf, uint len)
{
    make_eth(buf);
    make_ip(buf);
    buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;	  //////����Ӧ��////////////////////////////////////////////////////////////////////////////
    // we changed only the icmp.type field from request(=8) to reply(=0).
    // we can therefore easily correct the checksum:
    if (buf[ICMP_CHECKSUM_P] > (0xff-0x08))
    {
        buf[ICMP_CHECKSUM_P+1]++;
    }
    buf[ICMP_CHECKSUM_P]+=0x08;
    //
    _enc->PacketSend(buf, len);
}

// you can send a max of 220 bytes of data
void TinyIP::make_udp_reply_from_request(byte* buf,byte *data, uint datalen, uint port)
{
    unsigned int i=0;
    uint ck;
    make_eth(buf);
    //if (datalen>220)
    //	{
    //    datalen=220;
    //	}
    
    // total length field in the IP header must be set:
    i= IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
    buf[IP_TOTLEN_H_P]=i>>8;
    buf[IP_TOTLEN_L_P]=i;
    make_ip(buf);
    buf[UDP_DST_PORT_H_P]=port>>8;
    buf[UDP_DST_PORT_L_P]=port & 0xff;
    // source port does not matter and is what the sender used.
    // calculte the udp length:
    buf[UDP_LEN_H_P]=datalen>>8;
    buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
    // zero the checksum
    buf[UDP_CHECKSUM_H_P]=0;
    buf[UDP_CHECKSUM_L_P]=0;
    // copy the data:
    while(i<datalen)
    {
        buf[UDP_DATA_P+i]=data[i];
        i++;
    }
		//�����16�ֽ���UDP��α�ײ�����IP��Դ��ַ-0x1a+Ŀ���ַ-0x1e
		//+UDP�ײ�=4+4+8=16
    ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
    buf[UDP_CHECKSUM_H_P]=ck>>8;
    buf[UDP_CHECKSUM_L_P]=ck& 0xff;
    _enc->PacketSend(buf, UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen);
}

void TinyIP::make_tcp_synack_from_syn(byte* buf)
{
    uint ck;
    make_eth(buf);
    // total length field in the IP header must be set:
    // 20 bytes IP + 24 bytes (20tcp+4tcp options)
    buf[IP_TOTLEN_H_P]=0;
    buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4;
    make_ip(buf);
    buf[TCP_FLAGS_P]=TCP_FLAGS_SYNACK_V;
    make_tcphead(buf,1,1,0);
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + 4 (one option: mss)
    ck=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+4,2);
    buf[TCP_CHECKSUM_H_P]=ck>>8;
    buf[TCP_CHECKSUM_L_P]=ck& 0xff;
    // add 4 for option mss:
    _enc->PacketSend(buf, IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4+ETH_HEADER_LEN);
}

// get a pointer to the start of tcp data in buf
// Returns 0 if there is no data
// You must call init_len_info once before calling this function
uint TinyIP::get_tcp_data_pointer(void)
{
    if (info_data_len)
    {
        return((uint)TCP_SRC_PORT_H_P+info_hdr_len);
    }
    else
    {
        return false;
    }
}

// do some basic length calculations and store the result in static varibales
void TinyIP::init_len_info(byte* buf)
{
	  //IP������
    info_data_len=(buf[IP_TOTLEN_H_P]<<8)|(buf[IP_TOTLEN_L_P]&0xff);
	  buf_len=info_data_len;
	  //��ȥIP�ײ�����
    info_data_len-=IP_HEADER_LEN;
	  //TCP�ײ����ȣ���ΪTCPЭ��涨��ֻ����λ����ʾ���ȣ�������Ҫ���´���,4*6=24
    info_hdr_len=(buf[TCP_HEADER_LEN_P]>>4)*4; // generate len in bytes;
	  //��ȥTCP�ײ�����
    info_data_len-=info_hdr_len;
	tcp_d_len=info_data_len;
    if (info_data_len<=0)
    {
        info_data_len=0;
    }
	
}

// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint TinyIP::fill_tcp_data_p(byte* buf, uint pos, const byte* progmem_s)
{
    byte c;
    // fill in tcp data at position pos
    //
    // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
    while ((c = *progmem_s++)) 
    {
        buf[TCP_CHECKSUM_L_P + 3 + pos]=c;
        pos++;
    }
    return(pos);
}

// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint TinyIP::fill_tcp_data(byte* buf, uint pos, const byte* s)
{
    // fill in tcp data at position pos
    //
    // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
    while (*s) 
    {
        buf[TCP_CHECKSUM_L_P+3+pos]=*s;
        pos++;
        s++;
    }
    return(pos);
}

// Make just an ack packet with no tcp data inside
// This will modify the eth/ip/tcp header 
void TinyIP::make_tcp_ack_from_any(byte* buf)
{
    uint j;
    make_eth(buf);
    // fill the header:
    buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V;
    if (info_data_len==0)
    {
        // if there is no data then we must still acknoledge one packet
        make_tcphead(buf,1,0,1); // no options
    }
    else
    {
        make_tcphead(buf,info_data_len,0,1); // no options
    }
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp (when no options) 
    j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN;
    buf[IP_TOTLEN_H_P]=j>>8;
    buf[IP_TOTLEN_L_P]=j& 0xff;
    make_ip(buf);
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN,2);
    buf[TCP_CHECKSUM_H_P]=j>>8;
    buf[TCP_CHECKSUM_L_P]=j& 0xff;
    _enc->PacketSend(buf, IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+ETH_HEADER_LEN);
}

// you must have called init_len_info at some time before calling this function
// dlen is the amount of tcp data (http data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
void TinyIP::make_tcp_ack_with_data(byte* buf, uint dlen)
{
    uint j;
    // fill the header:
    // This code requires that we send only one data packet
    // because we keep no state information. We must therefore set
    // the fin here:
    buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V;//|TCP_FLAGS_FIN_V;
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp (when no options) + len of data
    j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen;
    buf[IP_TOTLEN_H_P]=j>>8;
    buf[IP_TOTLEN_L_P]=j& 0xff;
    fill_ip_hdr_checksum(buf);
    // zero the checksum
    buf[TCP_CHECKSUM_H_P]=0;
    buf[TCP_CHECKSUM_L_P]=0;
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dlen,2);
    buf[TCP_CHECKSUM_H_P]=j>>8;
    buf[TCP_CHECKSUM_L_P]=j& 0xff;
    _enc->PacketSend(buf, IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen+ETH_HEADER_LEN);
}
