/*
 * HiSLIP.h
 *
 *  Created on: Jun 3, 2024
 *      Author: grzegorz
 */

#ifndef HISLIP_INC_HISLIP_H_
#define HISLIP_INC_HISLIP_H_



#define HISLIP_INITIALIZE									(unsigned char )0
#define HISLIP_INITIALIZERESPONSE							(unsigned char )1
#define	HISLIP_FATALERROR									(unsigned char )2
#define	HISLIP_ERROR										(unsigned char )3
#define HISLIP_ASYNCLOCK									(unsigned char )4
#define HISLIP_ASYNCLOCKRESPONSE							(unsigned char )5
#define HISLIP_DATA											(unsigned char )6
#define HISLIP_DATAEND										(unsigned char )7
#define HISLIP_DEVICECLEARCOMPLETE							(unsigned char )8
#define HISLIP_DEVICECLEARACKNOWLEDGE						(unsigned char )9
#define HISLIP_ASYNCREMOTELOCALCONTROL						(unsigned char )10
#define HISLIP_ASYNCREMOTELOCALRESPONSE						(unsigned char )11
#define HISLIP_TRIGGER										(unsigned char )12
#define HISLIP_INTERRUPTED									(unsigned char )13
#define HISLIP_ASYNCINTERRUPTED								(unsigned char )14
#define HISLIP_ASYNCMAXIMUMMESSAGESIZE						(unsigned char )15
#define HISLIP_ASYNCMAXIMUMMESSAGESIZERESPONSE				(unsigned char )16
#define HISLIP_ASYNCINITIALIZE								(unsigned char )17
#define HISLIP_ASYNCINITIALIZERESPONSE						(unsigned char )18
#define HISLIP_ASYNCDEVICECLEAR								(unsigned char )19
#define HISLIP_ASYNCSERVICEREQUEST							(unsigned char )20
#define HISLIP_ASYNCSTATUSQUERY								(unsigned char )21
#define HISLIP_ASYNCSTATUSRESPONSE							(unsigned char )22
#define HISLIP_ASYNCDEVICECLEARACKNOWLEDGE					(unsigned char )23
#define HISLIP_ASYNCLOCKINFO								(unsigned char )24
#define HISLIP_ASYNCLOCKINFORESPONSE						(unsigned char )25
#define HISLIP_GETDESCRIPTORS								(unsigned char )26
#define HISLIP_GETDESCRIPTORSRESPONSE						(unsigned char )27
#define HISLIP_STARTTLS										(unsigned char )28
#define HISLIP_ASYNCSTARTTLS								(unsigned char )29
#define HISLIP_ASYNCSTARTTLSRESPONSE						(unsigned char )30
#define HISLIP_ENDTLS										(unsigned char )31
#define HISLIP_ASYNCENDTLS									(unsigned char )32
#define HISLIP_ASYNCENDTLSRESPONSE							(unsigned char )33
#define HISLIP_GETSASLMECHANISMLIST							(unsigned char )34
#define HISLIP_GETSASLMECHANISMLISTRESPONSE					(unsigned char )35
#define HISLIP_AUTHENTICATIONSTART							(unsigned char )36
#define HISLIP_AUTHENTICATIONEXCHANGE						(unsigned char )37
#define HISLIP_AUTHENTICATIONRESULT							(unsigned char )38

typedef enum
{
	Initialize = HISLIP_INITIALIZE,
	InitializeResponse = HISLIP_INITIALIZERESPONSE,
	FatalError = HISLIP_FATALERROR,
	Error = HISLIP_ERROR,
	AsyncLock = HISLIP_ASYNCLOCK,
	AsyncLockResponse = HISLIP_ASYNCLOCKRESPONSE,
	Data = HISLIP_DATA,
	DataEnd = HISLIP_DATAEND,
	DeviceClearComplete = HISLIP_DEVICECLEARCOMPLETE,
	DeviceClearAcknowledge = HISLIP_DEVICECLEARACKNOWLEDGE,
	AsyncRemoteLocalControl = HISLIP_ASYNCREMOTELOCALCONTROL,
	AsyncRemoteLocalResponse = HISLIP_ASYNCREMOTELOCALRESPONSE,
	Trigger = HISLIP_TRIGGER,
	Interrupted = HISLIP_INTERRUPTED,
	AsyncInterrupted = HISLIP_ASYNCINTERRUPTED,
	AsyncMaximumMessageSize = HISLIP_ASYNCMAXIMUMMESSAGESIZE,
	AsyncMaximumMessageSizeResponse = HISLIP_ASYNCMAXIMUMMESSAGESIZERESPONSE,
	AsyncInitialize = HISLIP_ASYNCINITIALIZE,
	AsyncInitializeResponse = HISLIP_ASYNCINITIALIZERESPONSE,
	AsyncDeviceClear = HISLIP_ASYNCDEVICECLEAR,
	AsyncServiceRequest = HISLIP_ASYNCSERVICEREQUEST,
	AsyncStatusQuery = HISLIP_ASYNCSTATUSQUERY,
	AsyncStatusResponse = HISLIP_ASYNCSTATUSRESPONSE,
	AsyncDeviceClearAcknowledge = HISLIP_ASYNCDEVICECLEARACKNOWLEDGE,
	AsyncLockInfo = HISLIP_ASYNCLOCKINFO,
	AsyncLockInfoResponse = HISLIP_ASYNCLOCKINFORESPONSE,
	GetDescriptors = HISLIP_GETDESCRIPTORS,
	GetDescriptorsResponse = HISLIP_GETDESCRIPTORSRESPONSE,
	StartTLS = HISLIP_STARTTLS,
	AsyncStartTLS = HISLIP_ASYNCSTARTTLS,
	AsyncStartTLSResponse = HISLIP_ASYNCSTARTTLSRESPONSE,
	EndTLS = HISLIP_ENDTLS,
	AsyncEndTLS = HISLIP_ASYNCENDTLS,
	AsyncEndTLSResponse = HISLIP_ASYNCENDTLSRESPONSE,
	GetSaslMechanismList = HISLIP_GETSASLMECHANISMLIST,
	GetSaslMechanismListResponse = HISLIP_GETSASLMECHANISMLISTRESPONSE,
	AuthenticationStart = HISLIP_AUTHENTICATIONSTART,
	AuthenticationExchange = HISLIP_AUTHENTICATIONEXCHANGE,
	AuthenticationResult = HISLIP_AUTHENTICATIONRESULT,
	HISLIP_CONN_ERR = 256

}hislip_msg_type_t;

#define HISLIP_PROLOGUE									0x4853

// TBD
#define HISLIP_VENDOR_ID								0x1111

#define HISLIP_MAX_DATA_SIZE							1024

#pragma pack(push, 1)

typedef struct
{
	unsigned int hi;
	unsigned int lo;
}payload_len_t;

typedef struct
{
	unsigned short prologue;
	unsigned char  msg_type;
	unsigned char  control_code;
	unsigned int msg_param;
	payload_len_t payload_len;
}hislip_msg_t;

#pragma pack(pop)


void hislip_CreateTask(void);

#endif /* HISLIP_INC_HISLIP_H_ */
