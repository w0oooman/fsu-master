//=====================================================================
//fsu:fast and safe UDP system
//=====================================================================

#ifndef __FSU_H__
#define __FSU_H__

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#if defined(_WIN32) ||  defined(WIN32) || defined(_WIN64) || defined(WIN64)
#define __FSU_WINDOWS_SYSTEM__
#include <windows.h>
#endif

//=====================================================================
// 32BIT INTEGER DEFINITION 
//=====================================================================
#ifndef __INTEGER_32_BITS__
#define __INTEGER_32_BITS__
#if defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_IA64) || \
	defined(_M_AMD64)
typedef unsigned int ISTDUINT32;
typedef int ISTDINT32;
#elif defined(_WIN32) || defined(WIN32) || defined(__i386__) || \
	defined(__i386) || defined(_M_X86)
typedef unsigned long ISTDUINT32;
typedef long ISTDINT32;
#elif defined(__MACOS__)
typedef UInt32 ISTDUINT32;
typedef SInt32 ISTDINT32;
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/types.h>
typedef u_int32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#elif defined(__BEOS__)
#include <sys/inttypes.h>
typedef u_int32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#elif (defined(_MSC_VER) || defined(__BORLANDC__)) && (!defined(__MSDOS__))
typedef unsigned __int32 ISTDUINT32;
typedef __int32 ISTDINT32;
#elif defined(__GNUC__)
#include <stdint.h>
typedef uint32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#else 
typedef unsigned long ISTDUINT32;
typedef long ISTDINT32;
#endif
#endif

//=====================================================================
// Integer Definition
//=====================================================================
#ifndef __IINT8_DEFINED
#define __IINT8_DEFINED
typedef char FSU_CHAR;
#endif

#ifndef __IUINT8_DEFINED
#define __IUINT8_DEFINED
typedef unsigned char FSU_BYTE;
#endif

#ifndef __IUINT16_DEFINED
#define __IUINT16_DEFINED
typedef unsigned short FSU_USHORT;
#endif

#ifndef __IINT16_DEFINED
#define __IINT16_DEFINED
typedef short FSU_SHORT;
#endif

#ifndef __IINT32_DEFINED
#define __IINT32_DEFINED
typedef ISTDINT32 FSU_INT;
#endif

#ifndef __IUINT32_DEFINED
#define __IUINT32_DEFINED
typedef ISTDUINT32 FSU_UINT;
#endif

#ifndef __IINT64_DEFINED
#define __IINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 FSU_INT64;
#else
typedef long long FSU_LL64;
#endif
#endif

#ifndef __IUINT64_DEFINED
#define __IUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 FSU_UINT64;
#else
typedef unsigned long long FSU_ULL64;
#endif
#endif

#ifndef INLINE
#if defined(__GNUC__)

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define INLINE         __inline__ __attribute__((always_inline))
#else
#define INLINE         __inline__
#endif

#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE 
#endif
#endif

#if (!defined(__cplusplus)) && (!defined(inline))
#define inline INLINE
#endif

#define FSU_MAX_SENDBUF       4096
#define FSU_INIT_MTU          1400   //1500
#define FSU_PROTOCOL_LEN      18
#define FSU_MSS_LEN  (FSU_INIT_MTU-FSU_PROTOCOL_LEN)
//#define FSU_MAX_DIFF_SEQ      20
#define FSU_RTO_VALUE         1500    //ms (should be set > 1s)
#define FSU_MAX_REPCOUNT      15
#define FSU_DELAY_TIMELIMIT   20

struct FSU_LIST
{
	union{
		struct FSU_LIST *prev;
		struct FSU_LIST *tail;
	};
	union{
		struct FSU_LIST *next;
		struct FSU_LIST *head;
	};
	//struct FSU_LIST *prev,*next 
	//struct FSUPro   *data;
};
typedef struct FSU_LIST fsu_list;


//fsu protocal
struct FSUPro         
{
	fsu_list    node;     //place here,fsu_send need only once malloc!!

	FSU_UINT    sign;     //every connect have a sign  (header)
	FSU_UINT    seq;      //sequence number  (header)
	FSU_UINT    ack;      //acknowledgement  (header)
	FSU_UINT    tsv;      //TimeStamp value
	FSU_USHORT  len;      //total msg len  (header)
	FSU_USHORT  wnd;      //window size  (header)
	FSU_BYTE    cmd;      //command  (header)
	FSU_BYTE    rstimes;  //resend times
	FSU_BYTE    ackn;     //with ack num (header)

	char        fsuhead[FSU_PROTOCOL_LEN];  //FSU protocol header

	char        data[1];  //data begin
};

//fsu logic
struct FSULogic
{
	FSU_UINT    sign;     //every connect have a sign

	FSU_UINT    oiseq;    //opposite init seq
	FSU_UINT    oseq;     //opposite now sequence number
	FSU_UINT    iseq;     //init seq
	FSU_UINT    seq;      //now sequence number
	//FSU_USHORT  mdseq;    //max diff seq

	FSU_UINT    ack;      //acknowledgement
	FSU_UINT    mtu;      //max trans unit
	FSU_UINT    hsize;    //fsu head size:FSUPro size
	FSU_UINT    mss;      //max msg segment
	FSU_USHORT  s_size;   //send buf max size
	FSU_USHORT  r_size;   //recv buf max size
	FSU_USHORT  s_csize;  //send buf current size
	FSU_USHORT  r_csize;  //recv buf current size
	FSU_USHORT  ownd;     //opposite window size
	fsu_list    s_fq;     //send final queue
	fsu_list    r_fq;     //recv final queue
	fsu_list    s_mq;     //send middle queue
	fsu_list    r_mq;     //recv middle queue
	//fsu_list    cacle_q;  //cacle queue
	//char      **cacle_buf;//cacle buffer
	void        *user;    //user
	char        sbuf[FSU_MAX_SENDBUF];    //send buffer
	FSU_USHORT  shead;   //send buffer head
	FSU_USHORT  stail;   //send buffer tail
	FSU_BYTE    dmode;   //delay mode 0:nodelay 1:delay
	FSU_UINT    delay;   //when dmode == 1,delay is used. (ms)
	FSU_UINT    stime;   //send time
	struct FSULogic *self;//point to self
	FSU_BYTE    replimit; //more than resend count limit,then release resource and close the socket
	FSU_BYTE    repcount; //resend current count
	FSU_BYTE    isserver; //server or client
	FSU_UINT    utime;    //last update time
	FSU_UINT    stimes;   //send times
	FSU_UINT    rtimes;   //recv times
	FSU_UINT    rackn;    //recv ack num
	FSU_UINT    rto;      //Retransmission TimeOut(ms)
	FSU_UINT    srtt;     //smoothed round-trip time
	FSU_UINT    rttvar;
	FSU_UINT    cwnd;     //Congestion Window
	FSU_UINT    sshthresh;//slow start threshold
	FSU_BYTE    cwmode;   //cwnd mode
	FSU_UINT    lrtime;   //last rtt time value
	FSU_UINT    m2fqt;    //mq to fq times
	FSU_BYTE    cansend;  //is can send data
	FSU_UINT    sendlen;  //send data length

	int(*output)(/*struct FSULogic *fsu, */const char *buf, int len, void *user);
	void(*writelog)(/*struct FSULogic *fsu, */const char *log, void *user);
};

typedef struct FSUPro   fsup;
typedef struct FSULogic fsul;

#ifdef __cplusplus
extern "C"
{
#endif
	/*****************************************************************/
	// create fsu
	// sendsize:fsu send buf size,different setsockopt to set size
	// recvsize:fsu recv buf size,different setsockopt to set size
	/*****************************************************************/
	fsul *fsu_create(FSU_UINT sign, FSU_USHORT sendsize, FSU_USHORT recvsize, void *user);


	/*****************************************************************/
	//input data
	//callstack:recvfrom->fsu_input->fsu_recv
	//buf:recvfrom receive data
	//len:recvfrom receive data len
	//user:pointer for a user's addr or index
	/*****************************************************************/
	int fsu_input(fsul *fsu, const char *buf, FSU_USHORT len);


	/*****************************************************************/
	//recv data
	//callstack:recvfrom->fsu_input->fsu_recv
	//buf:after fsu hancle,out data to buf
	//len:after fsu hancle,the max length of out data
	/*****************************************************************/
	int fsu_recv(fsul *fsu, char *buf, FSU_USHORT len);


	/*****************************************************************/
	//send data
	//callstack:fsu_send->fsu_update->sendto
	/*****************************************************************/
	int fsu_send(fsul *fsu, const char *buf, FSU_USHORT len);


	//update data
	int fsu_update(fsul *fsu);


	void fsu_setdelay(fsul *fsu, FSU_BYTE v);


	//fsu release
	void fsu_release(fsul *fsu);

#ifdef __cplusplus
}
#endif
#endif