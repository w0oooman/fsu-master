#include "fsu.h"

//=====================================================================
// const para
//=====================================================================
//const FSU_UINT FSU_INIT_MTU = 1400;   //1500


//=====================================================================
// define
//=====================================================================
#define safe_free(p) if((p)){free((p));(p) = NULL;}

#define fsu_queue_isnull(queue) \
	((queue)->next == queue)

#define fsu_queue_init(queue) \
	(queue)->head = queue,(queue)->tail = queue

#define fsu_queue_add(queue,p) \
	(p)->prev = (queue)->tail,(p)->next = (queue); \
    (queue)->tail->next = (p),(queue)->tail = (p)

#define fsu_queue_del(p) \
	(p)->prev->next = (p)->next; \
    (p)->next->prev = (p)->prev; \
	(p)->prev = NULL,(p)->next = NULL

#define fsu_queue_free_one(head,next) \
	fsup *p = fsu_queue_entry(head, fsup, node); \
	next = head->next; \
	fsu_queue_del(head); \
	safe_free(p); \
	head = next

#define fsu_queue_free_all(head,queue,next) \
	for (head = queue.head; head != &queue;){ \
	    fsu_queue_free_one(head,next); \
	}

#ifdef __FSU_WINDOWS_SYSTEM__
#define fsu_online_time_get(useed) \
	useed = GetTickCount() //不准，还是用QueryPerformanceFrequency 和 QueryPerformanceCounter吧
#else
#define fsu_online_time_get(useed) \
    struct timespec ts; \
	clock_gettime(CLOCK_MONOTONIC, &ts); \
	useed = ts.tv_sec * 1000 + ts.tv_nsec / 1000000 
#endif

////////////////////////////////////////////////////////////////////////////////////////////
static union { char buf[4]; unsigned long value; } fsu_endian_test = { '1', '0', '0', 'a' };
#define fsu_is_smallendian() ((char)(fsu_endian_test.value) == '1')

#define fsu_encode_u32(p,v,a,b,c,d) \
	*(unsigned char*)((p)+a) = (unsigned char)(((v) >> 0) & 0XFF);  \
	*(unsigned char*)((p)+b) = (unsigned char)(((v) >> 8) & 0XFF);  \
	*(unsigned char*)((p)+c) = (unsigned char)(((v) >> 16) & 0XFF); \
	*(unsigned char*)((p)+d) = (unsigned char)(((v) >> 24) & 0XFF);

#define fsu_encode_u16(p,v,a,b) \
	*(unsigned char*)((p)+a) = (unsigned char)(((v) >> 0) & 0XFF);  \
	*(unsigned char*)((p)+b) = (unsigned char)(((v) >> 8) & 0XFF);

#define fsu_encode_u8(p,v,a) \
	*(unsigned char*)((p)+a) = (unsigned char)(((v) >> 0) & 0XFF);

#define fsu_decode_u32(p,v,a,b,c,d) \
    *(unsigned long*)(v) = *(unsigned char*)((p)+a); \
	*(unsigned long*)(v) = ((*((unsigned char*)(p)+b)) << 8) + *(unsigned long*)(v);  \
	*(unsigned long*)(v) = ((*((unsigned char*)(p)+c)) << 16) + *(unsigned long*)(v); \
	*(unsigned long*)(v) = ((*((unsigned char*)(p)+d)) << 24) + *(unsigned long*)(v); 

#define fsu_decode_u16(p,v,a,b) \
    *(unsigned short*)(v) = *(unsigned char*)((p)+a); \
	*(unsigned short*)(v) = ((*((unsigned char*)(p)+b)) << 8) + *(unsigned short*)(v);

#define fsu_decode_u8(p,v,a) \
    *(unsigned char*)(v) = *(unsigned char*)((p)+a);

#define fsu_encode_sign(p,v) \
    if (fsu_is_smallendian()){ *(FSU_UINT*)(p) = (v);} \
		    else{ fsu_encode_u32(p,v,0,1,2,3)}

#define fsu_encode_seq(p,v) \
    if (fsu_is_smallendian()){ *(FSU_UINT*)((unsigned char*)(p)+4) = (v);} \
			else{ fsu_encode_u32(p,v,4,5,6,7)}

#define fsu_encode_ack(p,v) \
	if (fsu_is_smallendian()){ *(FSU_UINT*)((unsigned char*)(p)+8) = (v);} \
			else{ fsu_encode_u32(p,v,8,9,10,11)}

#define fsu_encode_len(p,v) \
	if (fsu_is_smallendian()){ *(FSU_USHORT*)((unsigned char*)(p)+12) = (v);} \
			else{ fsu_encode_u16(p,v,12,13)}

#define fsu_encode_wnd(p,v) \
	if (fsu_is_smallendian()){ *(FSU_USHORT*)((unsigned char*)(p)+14) = (v);} \
			else{ fsu_encode_u16(p,v,14,15)}

#define fsu_encode_cmd(p,v) \
	fsu_encode_u8(p,v,16)

#define fsu_encode_ackn(p,v) \
	fsu_encode_u8(p,v,17)

#define fsu_decode_sign(p,v) \
	if (fsu_is_smallendian()){ *(FSU_UINT*)(v) = *(FSU_UINT*)((unsigned char*)(p)+0);} \
			else{fsu_decode_u32(p,v,0,1,2,3)}

#define fsu_decode_seq(p,v) \
	if (fsu_is_smallendian()){ *(FSU_UINT*)(v) = *(FSU_UINT*)((unsigned char*)(p)+4);} \
			else{fsu_decode_u32(p,v,4,5,6,7)}

#define fsu_decode_ack(p,v) \
	if (fsu_is_smallendian()){ *(FSU_UINT*)(v) = *(FSU_UINT*)((unsigned char*)(p)+8);} \
			else{fsu_decode_u32(p,v,8,9,10,11)}

#define fsu_decode_len(p,v) \
	if (fsu_is_smallendian()){ *(FSU_USHORT*)(v) = *(FSU_USHORT*)((unsigned char*)(p)+12);} \
			else{fsu_decode_u16(p,v,12,13)}

#define fsu_decode_wnd(p,v) \
	if (fsu_is_smallendian()){ *(FSU_USHORT*)(v) = *(FSU_USHORT*)((unsigned char*)(p)+14);} \
			else{fsu_decode_u16(p,v,14,15)}

#define fsu_decode_cmd(p,v) \
	fsu_decode_u8(p,v,16)

#define fsu_decode_ackn(p,v) \
	fsu_decode_u8(p,v,17)

#define fsu_encode_segment(ptr) \
	fsu_encode_sign((ptr)->fsuhead, (ptr)->sign); \
    fsu_encode_seq((ptr)->fsuhead, (ptr)->seq);   \
	fsu_encode_ack((ptr)->fsuhead, (ptr)->ack);   \
    fsu_encode_len((ptr)->fsuhead, (ptr)->len);   \
	fsu_encode_wnd((ptr)->fsuhead, (ptr)->wnd);   \
    fsu_encode_cmd((ptr)->fsuhead, (ptr)->cmd);   \
    fsu_encode_ackn((ptr)->fsuhead, (ptr)->ackn);

#define fsu_decode_segment(buf,ptr) \
	fsu_decode_sign(buf, &(ptr)->sign); \
    fsu_decode_seq(buf, &(ptr)->seq);   \
	fsu_decode_ack(buf, &(ptr)->ack);   \
    fsu_decode_len(buf, &(ptr)->len);   \
    fsu_decode_wnd(buf, &(ptr)->wnd);   \
    fsu_decode_cmd(buf, &(ptr)->cmd);   \
	fsu_decode_ackn(buf, &(ptr)->ackn);
////////////////////////////////////////////////////////////////////////////////////////////

#define fsu_queue_offset(type,v) \
	(FSU_UINT)(&((type*)0)->v)
#define fsu_queue_entry(p,type,v) \
	(type*)((char*)p-fsu_queue_offset(type,v))

//#define FSU_CMD_ENCODE_CMD(cmd,v)   ((cmd)=(FSU_BYTE)(((cmd)&0XF0) & ((v)&0X0F)))
//#define FSU_CMD_ENCODE_ACKN(cmd,v)  ((cmd)=(FSU_BYTE)(((cmd)&0X0F) & ((v)<<4)))
//#define FSU_CMD_DECODE_CMD(cmd)     (FSU_BYTE)((cmd)&0X0F)
//#define FSU_CMD_DECODE_ACKN(cmd)    (FSU_BYTE)((cmd)>>4)

#define fsu_copy_buf2mq_define(fsu,temp_len) \
for (; temp_len > 0;) \
{ \
	FSU_USHORT size = min(FSU_MSS_LEN, temp_len); \
	if (!fsu_copy_buf2mq(fsu, size)) \
		return 0; \
	temp_len -= size; \
}

#define FSU_DEBUG  0

//#define fsu_cacle_add(queue,num) \
//	do{ \
//	    int i = 0; \
//		for (;i<num;++i){ \
//		    fsup * p = fsu_segment_malloc(FSU_INIT_MTU); \
//            fsu_queue_add(queue, &p->node); \
//		} \
//	} while (0)
	
//=====================================================================
// enum
//=====================================================================
enum FSU_CMD
{
	FSU_CMD_SEND = 1,
	FSU_CMD_ACK  = 2,
	//FSU_CMD_SENDWITHACK = 4,
};

enum FSU_CWMODE
{
	FSU_CWMODE_SS = 0,   //Slow Start
	FSU_CWMODE_CA = 1,   //Congestion Avoidance
	FSU_CWMODE_CS = 2,   //Congestion Status
	FSU_CWMODE_FR = 3,   //Fast Recovery
	FSU_CWMODE_FT = 4,   //Fast Retransmission
};

//=====================================================================
// static function
//=====================================================================
//static inline FSU_BYTE fsu_is_smallendian()
//{
//	static FSU_USHORT v = 0X1122;
//	if (*((FSU_BYTE*)&v) == 0X22) return 1;
//	else return 0;
//}

static inline int fsu_output(fsul *const fsu, const char *buf, int len)
{
	if (fsu && fsu->output)
		return fsu->output(buf, len, fsu->user);
	return 0;
}

static inline void fsu_writelog(fsul *const fsu, void *user, const char *log,...)
{
	if (fsu && fsu->writelog)
	{
		char buffer[1024];
		va_list argptr;
		va_start(argptr, log);
		vsprintf(buffer, log, argptr);
		va_end(argptr);
		fsu->writelog(buffer, user);
	}
}

static inline void fsu_srand(FSU_UINT seed /*= 0*/)
{
	FSU_UINT useed = seed;
	if (0 == seed)
	    fsu_online_time_get(useed);
	srand(useed);
}

static void* (*fsu_malloc_pool)(const FSU_UINT) = NULL;
static void(*fsu_free_pool)(void *) = NULL;

static inline fsup *fsu_segment_malloc(const FSU_UINT size)
{
	if (fsu_malloc_pool)
		return (fsup *)fsu_malloc_pool(size);
	return (fsup *)malloc(size);
}

static inline void fsu_segment_free(void *p)
{
	if (fsu_free_pool)
		fsu_free_pool(p);
	safe_free(p);
}

static inline fsu_list *fsu_queue_head(fsu_list *queue)
{
	if (queue == NULL || fsu_queue_isnull(queue))
		return NULL;

	return queue->tail;//tail is true head
}

static inline fsu_list *fsu_queue_pop(fsu_list *queue)
{
	fsu_list *node;

	if (queue == NULL || fsu_queue_isnull(queue))
		return NULL;

	node = queue->tail;  //tail is true head
	fsu_queue_del(node);
	return node;
}

static inline int fsu_exist_ack(fsu_list *mq, fsup *p)
{
	fsu_list *plist = mq->head;
	FSU_UINT seq = p->seq;

	for (; plist != mq; plist = plist->next)
	{
		fsup * ptr = fsu_queue_entry(plist, fsup, node);
		if (ptr->cmd & FSU_CMD_ACK)
		{
			if (seq >= ptr->ack && seq <= ptr->ack + ptr->ackn - 1)
			{
				printf("************addr=%d,seq=%d,ack=%d,ackn=%d*************************************\n",(int)ptr,seq,ptr->ack,ptr->ackn);//whb
				return 1;
			}
		}
	}

	return 0;
}

static inline int fsu_parse_withack(fsu_list *mq,fsup *p)
{
	fsu_list *plist = mq->head;

	for (; plist != mq;plist = plist->next)
	{
		fsup * ptr = fsu_queue_entry(plist, fsup, node);
#if FSU_DEBUG
		printf("***************************** addr=%d\n",(int)plist);//whb
#endif
		if (0 == ptr->rstimes)
		{
			if (0 == ptr->ackn)
			{
				ptr->ack = p->seq;
				ptr->ackn = 1;
				ptr->cmd |= FSU_CMD_ACK;
#if FSU_DEBUG
				printf("++++++++++++++++  with ack 111 addr=%d,p->seq=%d,p->ack=%d,p->cmd=%d,p->ackn=%d\n",(int)ptr,ptr->seq,ptr->ack,ptr->cmd,ptr->ackn);//whb
#endif
				return 1;
			}
			else if (ptr->ackn > 0 && ptr->ackn < 0XFF)
			{
				if (p->seq == (ptr->ack + ptr->ackn))
				{
					ptr->ackn += 1;
					ptr->cmd |= FSU_CMD_ACK;
#if FSU_DEBUG
					printf("++++++++++++++  with ack 222 addr=%d,p->seq=%d,p->ack=%d,p->cmd=%d,p->ackn=%d\n",(int)ptr,ptr->seq,ptr->ack,ptr->cmd,ptr->ackn);//whb
#endif
					return 1;
				}
			}
		}
	}
	return 0;
}

static inline int fsu_sendback_ack(fsul *fsu,fsup *p)
{
	if (fsu_exist_ack(&fsu->s_fq, p)) return 1;
	else if (fsu_exist_ack(&fsu->s_mq, p)) return 1;
	else if (fsu_parse_withack(&fsu->s_fq, p)) return 1;
	else if (fsu_parse_withack(&fsu->s_mq, p)) return 1;
	else
	{
		fsup *pback = fsu_segment_malloc(fsu->hsize);  //left some byte no useed
		if (NULL == pback)
		{
			assert(pback);
			return 1;  //return 0;
		}

		pback->sign = fsu->sign;
		pback->seq = fsu->seq;
		pback->ack = p->seq;
		pback->cmd = FSU_CMD_ACK;
		pback->len = 0;
		pback->tsv = 0;
		pback->rstimes = 0;
		pback->ackn = 1;
		fsu_queue_add(&fsu->s_fq, &pback->node);
		fsu->s_csize += FSU_PROTOCOL_LEN;
#if FSU_DEBUG
		printf("++++++++++++++++++  with ack 333 p->seq=%d,p->ack=%d,p->cmd=%d,p->ackn=%d\n",pback->seq,pback->ack,pback->cmd,pback->ackn);//whb
#endif
	}

	return 1;
}

static inline int fsu_exist_seq(fsu_list *mq, FSU_UINT seq)
{
	fsu_list *ptr = mq->head;
	for (; ptr != mq; ptr = ptr->next)
	{
		fsup * p = fsu_queue_entry(ptr, fsup, node);
		if (p->seq == seq)
			return 1;
	}
	return 0;
}

static inline int fsu_is_repeated_packet(fsul *fsu,FSU_UINT seq)
{
	int ret = 0;
	if (0 == (ret = fsu_exist_seq(&fsu->r_fq, seq)))
		ret = fsu_exist_seq(&fsu->r_mq, seq);
    return ret;
}

static inline int fsu_is_throw_packet(fsul *fsu, fsup *p)
{
	//error seq
	if (p->seq < fsu->oiseq)
	{
		fsu_writelog(fsu, fsu->user, "error seq packet.");
		return 1;
	}

	//repeated packet
	if (fsu_is_repeated_packet(fsu, p->seq))
	{
		fsu_writelog(fsu, fsu->user, "repeated packet.");  //whb
		return 1;
	}

	//handled packet 1.<  2.> (1<<32 == 0)
	if ((p->seq < fsu->oseq) || (p->seq > fsu->oseq && p->seq - fsu->oseq > 0xFFFFFFFF))
	{
		fsu_writelog(fsu, fsu->user, "handled packet or error seq.");  //whb
		fsu_sendback_ack(fsu,p); //maybe ack is lose
		return 1;
	}


	//if (p->seq > fsu->oseq && p->seq - fsu->oseq > FSU_MAX_DIFF_SEQ)
	//{
	//	fsu_writelog(fsu, fsu->user, "no serial seq packet too much.");
	//	return 1;
	//}

	return 0;
}

//algorithm addr : http ://tools.ietf.org/html/rfc6298 
static inline void fsu_set_rto(fsul *fsu, fsup *p)
{
	if (p->rstimes > 1) return;

	FSU_UINT now, rtt;
	fsu_online_time_get(now);
	if (now < p->tsv) return;
	fsu->rackn++;
	rtt = now - p->tsv;
 
	if (1 == fsu->rackn)
	{
		fsu->rttvar = rtt / 2;
		fsu->srtt = rtt;
		fsu->rto = fsu->srtt + max(FSU_RTO_VALUE, 4 * fsu->rttvar);
	}
	else
	{
		fsu->rttvar = fsu->rttvar * 3 / 4 + abs(fsu->srtt - rtt) * 3 / 4;
		fsu->srtt = fsu->srtt * 7 / 8 + rtt / 8;
		fsu->rto = fsu->srtt + max(FSU_RTO_VALUE, 4 * fsu->rttvar);
	}

	if (fsu->rto < 1000) fsu->rto = 1000;  //at least 1s
	printf("((((((((RTT = %d)))))))\n", rtt);  //whb
	printf("++++++++++rto = %d++++++++++\n",fsu->rto);//whb
}

static int fsu_copy_buf2mq(fsul *fsu, FSU_USHORT size)
{
	FSU_USHORT mod = fsu->stail % FSU_MAX_SENDBUF;
	FSU_USHORT right = min(size, FSU_MAX_SENDBUF - mod);
	fsup *ptr = fsu_segment_malloc(fsu->hsize + size);  //left some byte no useed
	if (ptr == NULL)
	{
		assert(ptr);
		return 0;
	}

	ptr->sign = fsu->sign;
	ptr->seq = fsu->seq++;
	ptr->ack = 0;
	ptr->cmd = FSU_CMD_SEND;
	ptr->len = size;
	ptr->tsv = 0;
	ptr->rstimes = 0;
	ptr->ackn = 0;

	memcpy(ptr->data, fsu->sbuf + mod, right);
	memcpy(ptr->data + right, fsu->sbuf, size - right);
	fsu_queue_add(&fsu->s_mq, &ptr->node);
	fsu->stail += size;
	return 1;
}

static int fsu_out_buf(fsul *fsu, fsu_list *mq,FSU_USHORT minlen)
{
	if (fsu->dmode)  //delay
	{
		FSU_UINT now;
		FSU_USHORT temp_len = fsu->shead - fsu->stail,len;
		if (0 == temp_len) return 0;
		len = temp_len;
		fsu_online_time_get(now);
		if (0 == fsu->stime)
			fsu->stime = now;

		if (now - fsu->stime >= fsu->delay)
		{
			fsu_copy_buf2mq_define(fsu, temp_len);
			fsu->stime = 0;
			return 1;
		}

		if (temp_len >= FSU_MSS_LEN)
		{
			if (!fsu_copy_buf2mq(fsu, FSU_MSS_LEN))
				return 0;
			temp_len -= FSU_MSS_LEN;
		}

		if (minlen > 0 && minlen > len - temp_len)
		{
			FSU_USHORT l = minlen - (len - temp_len);
			fsu_copy_buf2mq_define(fsu,l);
		}

		if (fsu->shead - fsu->stail == 0) 
			fsu->stime = 0;
	}
	else
	{
		FSU_USHORT temp_len = fsu->shead - fsu->stail;
		fsu_copy_buf2mq_define(fsu,temp_len)
	}

	return 1;
}

static int fsu_in_sbuf(fsul *fsu, const char *buf, FSU_USHORT len)
{
	FSU_USHORT remain, right, mod;
	FSU_UINT now;
	if (len > FSU_MAX_SENDBUF) return -1;

	remain = FSU_MAX_SENDBUF - fsu->shead + fsu->stail;
	if (len > remain)
	{
		if (!fsu_out_buf(fsu, &fsu->s_mq, len - remain))
			return 0;
	}

	fsu_online_time_get(now);
	if (0 == fsu->stime)
		fsu->stime = now;

	mod = fsu->shead%FSU_MAX_SENDBUF;
	right = min(FSU_MAX_SENDBUF - mod, len);
	memcpy(fsu->sbuf + mod, buf, right);
	memcpy(fsu->sbuf, buf + right, len - right);
	fsu->shead += len;

	return len;
}

static void fsu_set_cwnd(fsul *fsu,FSU_USHORT len)
{
	if (fsu->sendlen >= len)
	{
		fsu->sendlen -= len;
		if (0 == fsu->sendlen)
		{
			fsu->cansend = 1;
			switch (fsu->cwmode)
			{
			case FSU_CWMODE_SS:{
				if (fsu->cwnd + fsu->cwnd >= fsu->sshthresh){
					fsu->cwmode = FSU_CWMODE_CA;
					break;
				}
				fsu->cwnd *= 2;
				break;
			}
			case FSU_CWMODE_CA:{
				fsu->cwnd += FSU_MSS_LEN;
				if (fsu->cwnd >= fsu->sshthresh+FSU_PROTOCOL_LEN*8){
					fsu->sshthresh = fsu->cwnd;
				}
				break;
			}
			case FSU_CWMODE_CS:{break; }
			case FSU_CWMODE_FR:{break; }
			case FSU_CWMODE_FT:{break; }
			}
		}
	}
	else if (fsu->sendlen != 0 && fsu->sendlen < len)
	{
		fsu_writelog(fsu, fsu->user, "error sendlen.");
		fsu->cansend = 1;
	}

}

/*****************************************************************/
// create fsu
/*****************************************************************/
fsul *fsu_create(FSU_UINT sign, FSU_USHORT sendsize, FSU_USHORT recvsize, void *user)
{
	fsul *fsu = NULL;
	FSU_UINT size = (1 << 16);
	if (sendsize < FSU_INIT_MTU) sendsize = FSU_INIT_MTU;
	if (recvsize < FSU_INIT_MTU) recvsize = FSU_INIT_MTU;
	if (sendsize == size) sendsize -= 1;
	if (recvsize == size) recvsize -= 1;

	if (sendsize > size || recvsize > size)
		return NULL;

	fsu_srand(0);
	fsu = (fsul*)malloc(sizeof(struct FSULogic));
	assert(fsu != NULL);
	if (fsu == NULL) return NULL;

	fsu->sign = sign;
	fsu->oiseq = 0;
	fsu->oseq = 0;
	fsu->iseq = /*0; */rand() % 1000000 + 1;
	fsu->seq = fsu->iseq;
	//fsu->mdseq = FSU_MAX_DIFF_SEQ;
	fsu->ack = 0;
	fsu->mtu  = FSU_INIT_MTU;
	fsu->hsize= sizeof(struct FSUPro);
	fsu->mss = FSU_MSS_LEN;
	fsu->s_size = sendsize;
	fsu->r_size = recvsize;
	fsu->s_csize = 0;
	fsu->r_csize = 0;
	fsu->ownd = FSU_INIT_MTU;
	fsu_queue_init(&fsu->s_fq);
	fsu_queue_init(&fsu->r_fq);
	fsu_queue_init(&fsu->s_mq);
	fsu_queue_init(&fsu->r_mq);
	//fsu_queue_init(&fsu->cacle_q);
	fsu->output = NULL;
	fsu->writelog = NULL;
	fsu->user = user;
	fsu->shead = 0;
	fsu->stail = 0;
	fsu->dmode = 1;  //whb
	fsu->delay = FSU_DELAY_TIMELIMIT;
	fsu->stime = 0;
	fsu->self = fsu;
	fsu->replimit = FSU_MAX_REPCOUNT;
	fsu->repcount = 0;
	fsu->isserver = 0;
	fsu->utime = 0;
	fsu->stimes = 0;
	fsu->rtimes = 0;
	fsu->rackn = 0;
	fsu->rto = FSU_RTO_VALUE;  //should > 1s
	fsu->srtt = 0;
	fsu->rttvar = 0;
	fsu->cwnd = FSU_MSS_LEN;  //linux: cwnd = FSU_MSS_LEN <= 1095 ? 4 : (FSU_MSS_LEN > 2190 ? 2 : 3);  
	fsu->sshthresh = 65536;
	fsu->cwmode = FSU_CWMODE_SS;
	fsu->lrtime = 0;
	fsu->m2fqt = 0;
	fsu->cansend = 1;
	fsu->sendlen = 0;
	//fsu_cacle_add(&fsu->cacle_q, fsu->climit);
	//fsu->cacle_buf = malloc(sizeof(char*) * fsu->climit);
	//for (i = 0; i < fsu->climit; i++)
	//	fsu->cacle_buf[i] = NULL;

	return fsu;
}


/*****************************************************************/
//input data
/*****************************************************************/
int fsu_input(fsul *fsu, const char *buf, FSU_USHORT len/*,void *user*/)
{
	if (len < FSU_PROTOCOL_LEN || len > fsu->mtu)
	{
		fsu_writelog(fsu, fsu->user, "data too short or too long.");
		return -1;
	}

	if (fsu->r_csize < fsu->r_size && fsu->r_size - fsu->r_csize >= len)
	{
		fsup *ptr,ptemp;
		FSU_UINT retlen = 0;

		fsu_decode_segment(buf, &ptemp);

		printf("---------------- fsu_input,seq=%d,ack=%d,cmd=%d,len=%d,wnd=%d,ackn=%d\n", ptemp.seq, ptemp.ack, ptemp.cmd, ptemp.len,ptemp.wnd, ptemp.ackn);//whb

		if (ptemp.sign != fsu->sign)
		{
			fsu_writelog(fsu, fsu->user, "sign not equal."); //whb need delete
			return -2;
		}

		if (ptemp.len + FSU_PROTOCOL_LEN != len)
		{
			fsu_writelog(fsu, fsu->user, "datalen+head not equal len.");
			return -3;
		}

		if ((ptemp.cmd & FSU_CMD_ACK) == 0 && 0 == (ptemp.cmd & FSU_CMD_SEND))  //whb
		{
			fsu_writelog(fsu, fsu->user, "cmd not exist.");
			return -100;
		}

		if (0 == fsu->rtimes) 
		{
			if (0 == fsu->stimes) //server run
			{
				fsu->isserver = 1;
				fsu->oiseq = ptemp.seq;
				fsu->oseq = ptemp.seq;
			}
			else if (1 == fsu->stimes) //client run
			{
				fsu->oiseq = ptemp.seq;
				fsu->oseq = ptemp.seq;
				//if (0 == fsu->isserver)
				//	fsu->seq = ptemp.seq;
			}
		}

		if (FSU_CMD_ACK & ptemp.cmd)
		{
			fsu_list *plist,*next;
			FSU_UINT i;
			FSU_UINT end = ptemp.ack + ptemp.ackn;
#if FSU_DEBUG
			printf("---------------- FSU_CMD_ACK & ptemp.cmd\n");//whb
#endif
			for (i = ptemp.ack; i < end; ++i)
			{
				for (plist = fsu->s_fq.head; plist != &fsu->s_fq;)
				{
					fsup * p = fsu_queue_entry(plist, fsup, node);
					next = plist->next;
					if (p->seq == i)
					{
						fsu_set_cwnd(fsu, p->len);
						fsu_set_rto(fsu, p);
						fsu_queue_del(plist);
#if FSU_DEBUG
						printf("---------------- FSU_CMD_ACK & ptemp.cmd addr=%d\n", (int)(plist));//whb
#endif
						fsu_segment_free(p);
						break;
					}
					plist = next;
				}
			}

			retlen = 0;
		}

	    if (FSU_CMD_SEND & ptemp.cmd)
		{
			if (fsu_is_throw_packet(fsu, &ptemp))
				return -4;

			if (!fsu_sendback_ack(fsu, &ptemp))
				return 0;

			ptr = fsu_segment_malloc(len + fsu->hsize);  //some bytes no useed
			if (ptr == NULL)
			{
				assert(ptr);
				return 0;
			}

			*ptr = ptemp;

			memcpy(ptr->fsuhead, buf, len);
			fsu_queue_add(&fsu->r_mq, &ptr->node);
			retlen = len;
#if FSU_DEBUG
			printf("=============== recv ========== addr=%d\n", (int)(&ptr->node));//whb
#endif
		}

		fsu->ownd = ptemp.wnd;
		fsu->rtimes += 1;
		
		return retlen;
	}

	printf("[][][full][][][]\n"); //whb
	return 0;
}

/*****************************************************************/
//recv data
/*****************************************************************/
int fsu_recv(fsul *fsu, char *buf, FSU_USHORT len)
{
	FSU_USHORT ret_len;
	fsu_list *p,*next;

	//copy r_mq to r_fq
	for (p = fsu->r_mq.head; p != &fsu->r_mq;)
	{
		fsup * ptr = fsu_queue_entry(p, fsup, node);
		if (fsu->r_csize + ptr->len + FSU_PROTOCOL_LEN > fsu->r_size)
			break;

		next = p->next;
		fsu_queue_del(p);
		fsu->r_csize += ptr->len + FSU_PROTOCOL_LEN;
		fsu_queue_add(&fsu->r_fq, p);
		p = next;
	}

	//copy r_fq to out buffer
	for (p = fsu->r_fq.head; p != &fsu->r_fq;)
	{
		fsup * ptr = fsu_queue_entry(p, fsup, node);
        
		if (ptr->len > len)   //len of buf is too short
		{
			fsu_writelog(fsu, fsu->user, "len of buf is too short.");
			return -1;
		}

		if (ptr->seq == fsu->oseq)
		{
			fsu_queue_del(p);
			memcpy(buf, ptr->data, ptr->len);
			ret_len = ptr->len;
			fsu_segment_free(ptr);
			fsu->r_csize -= ret_len + FSU_PROTOCOL_LEN;
			fsu->oseq++;
			return ret_len;
		}

		p = p->next;
	}
	return 0;
}


/*****************************************************************/
//send data
/*****************************************************************/
int fsu_send(fsul *fsu, const char *buf, FSU_USHORT len)
{
	assert(FSU_MSS_LEN > 0);
	if(len <= 0 ) return 0;    
	if (len > FSU_MAX_SENDBUF) //too big
	{
		fsu_writelog(fsu, fsu->user, "buf longer than limit."); 
		return -1;  
	}

	return fsu_in_sbuf(fsu, buf, len);
}


//update data
int fsu_update(fsul *fsu)
{
	fsu_list *ptr,*next;
	FSU_UINT now;
	FSU_UINT len = 0,protlen = 0;

	fsu_online_time_get(now);
	//if (now - fsu->lrtime > fsu->srtt)
	//{
	//	fsu_set_cwnd(fsu,1);
	//	fsu->lrtime = now;
	//}

	//sbuf to mid queue
	fsu_out_buf(fsu, &fsu->s_mq, 0);

	if (fsu->s_csize < fsu->s_size)
	{
		//copy mid queue to final queue
		for (ptr = fsu->s_mq.head; ptr != &fsu->s_mq; ptr = next)
		{
			if (1 == fsu->m2fqt && 0 == fsu->rtimes) break; //client run
			fsup * p = fsu_queue_entry(ptr, fsup, node);
			if (fsu->s_csize + p->len + FSU_PROTOCOL_LEN > fsu->s_size)
				break;

			next = ptr->next;
			if (!(1 == fsu->cansend || (p->cmd & FSU_CMD_ACK)))
				continue;

			fsu_queue_del(ptr);
			fsu->s_csize += p->len + FSU_PROTOCOL_LEN;
			fsu_queue_add(&fsu->s_fq, ptr);
			fsu->m2fqt++;
		}
	}

	//copy final queue to cache
	for (ptr = fsu->s_fq.head; ptr != &fsu->s_fq; ptr = next)
	{
		fsup * p;

		if (1 == fsu->stimes && 0 == fsu->rtimes)  //client run
			break;

		p = fsu_queue_entry(ptr, fsup, node);
		next = ptr->next;

		if (!(1 == fsu->cansend || (p->cmd & FSU_CMD_ACK)))
			continue;

		if (1 == fsu->cansend)
		{
			if (len >= fsu->cwnd)
			{
				fsu->cansend = 0;
				fsu->sendlen = len - protlen;
				break;
			}
			len += p->len;
		}	
		
		if (p->len + FSU_PROTOCOL_LEN > fsu->ownd) //wnd full
			continue;
		
		if(0 == p->tsv || p->tsv > now)
			p->tsv = now;

		if ((0 == p->rstimes) || (p->rstimes < 3 && now - p->tsv >= fsu->rto) || (p->rstimes >= 3 && now - p->tsv >= fsu->rto*2))
		{
			p->tsv = now;
			fsu->stimes += 1;

			if (FSU_CMD_ACK == p->cmd)
			{
				protlen += FSU_PROTOCOL_LEN;
		        fsu_queue_del(ptr);
			}
			else if (FSU_CMD_SEND & p->cmd)
			{
		         p->rstimes += 1;
				 if (p->rstimes > 2)
				 {
					 fsu->sshthresh = fsu->cwnd / 2;
					 fsu->cwnd = FSU_MSS_LEN;
					 fsu->dmode = FSU_CWMODE_SS;

					 if (p->rstimes > fsu->replimit) //whb rstimes should bu set 0 when ok.
					 {
						 fsu_writelog(fsu, fsu->user, "repcount more than limit.");
						 return -1;
					 }
				 }
			}

			p->wnd = fsu->r_size - fsu->r_csize;

			//#if FSU_DEBUG whb
			do{
				static int sum = 0,lensum = 0;
				if (p->rstimes <= 1) { sum += p->ackn; lensum += p->len; }
				printf("-out-addr=%d,sq=%d,ac=%d,cm=%d,l=%d,w=%d,an=%d,s=%d,ls=%d\n", (int)(ptr), p->seq, p->ack, p->cmd, p->len + FSU_PROTOCOL_LEN, p->wnd, p->ackn, sum, lensum);//whb
			}while(0);
			//#endif
			
			fsu_encode_segment(p);
			fsu->s_csize -= p->len + FSU_PROTOCOL_LEN;
			fsu_output(fsu, p->fsuhead, p->len + FSU_PROTOCOL_LEN);
		}
	}

	return 0;
}

/*****************************************************************/
//fsu set delay mode
/*****************************************************************/
void fsu_setdelay(fsul *fsu,FSU_BYTE v)
{
	if (v > 0)
		fsu->dmode = 1;
	else
		fsu->dmode = 0;
}

/*****************************************************************/
//fsu release
/*****************************************************************/
void fsu_release(fsul *fsu)
{
	if (fsu)
	{
		fsu_list *head, *next;

		fsu_queue_free_all(head, fsu->s_mq, next);
		fsu_queue_free_all(head, fsu->r_mq, next);
		fsu_queue_free_all(head, fsu->s_fq, next);
		fsu_queue_free_all(head, fsu->r_fq, next);

		//for (i = 0; i < fsu->climit; i++)
		//{
		//	if (fsu->cacle_buf[i] != NULL)
		//		safe_free(fsu->cacle_buf[i])
		//}

		//safe_free(fsu->cacle_buf)
		safe_free(fsu)
	}
}