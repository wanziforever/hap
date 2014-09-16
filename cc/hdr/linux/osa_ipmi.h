/*
 * File:	osa_ipmi.h
 *
 * Description:	Header file used to interface with the OSA IPMI driver via the
 *		libraw.so library interface.
 *
 *		Copyright (c) 2006 Lucent Technologies
 *
 * Reference:	IPMI (Intelligent Platform Management Interface) Specification
 *		v1.5, Document Revision 1.1, February 20, 2002
 *
 * Owner:	Rick Lane
 *
 * Revision History:
 *
 *     Date       Developer                       Comment
 *  ----------	--------------	---------------------------------------------
 *  03/07/2006	Rick Lane	First version created.
 */
#ifndef __OSA_IPMI_H
#define __OSA_IPMI_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *****************************************************************************
 *
 *  OSA IPMI Type Definitions
 *
 *****************************************************************************
 */
/* Handle returned by RawOpen that is used in subsequent API calls */
typedef void*	RawAPIHandle_t;

/* Predicate to test for Raw API handle validity */
#define IS_VALID_RAW_API_HANDLE(h) ((h) != NULL)

/* NetFn/Lun type, i.e. Table 5-1 in the IPMI v1.5 spec */
typedef unsigned char	NetFnLun_t;

/* Command type, i.e. Appendix G of the IPMI v1.5 spec */
typedef unsigned char	Command_t;

/* Completion Code type, i.e. Table 5-2 in the IPMI v1.5 spec */
typedef unsigned char	CompletionCode_t;

/* Request or Response length type */
typedef unsigned short	Length_t;

/* Destination Owner address type */
typedef unsigned char	DestinationAddr_t;

/* Error code type for OS dependent and defined error codes
 * It is OS dependent: use OS error number */
typedef int		Error_t;

/* Raw Library version code */
typedef unsigned int	Version_t;

/*
 *****************************************************************************
 *
 *  OSA IPMI Command/Code Definitions Layouts
 *
 *****************************************************************************
 */
/* IPMI Network Function Codes */
#define IPMI_NETFN_APP				0x18

/*
 *  IPMI Command Definitions
 */
#define IPMI_CMD_RESET_WATCHDOG_TIMER		0x22
#define IPMI_CMD_SET_WATCHDOG_TIMER		0x24
#define IPMI_CMD_GET_WATCHDOG_TIMER		0x25

/*
 *  IPMI Command Completion Codes
 *  (see IPMI v1.5 spec Table 5-2 for full list)
 */
#define IPMI_CMPLCODE_SUCCESS			0x00
#define IPMI_CMPLCODE_BUSY			0xC0
#define IPMI_CMPLCODE_INVALID_CMD		0xC1
#define IPMI_CMPLCODE_TIMEOUT			0xC3
#define IPMI_CMPLCODE_UNSPECIFIED		0xff

/*
 *  IPMI Watchdog Set/Get Command Structure
 */
typedef struct
{
	unsigned char	wdTimerUse;
	unsigned char	wdTimerAction;
	unsigned char	wdPreTimeoutInterval;
	unsigned char	wdExpFlags;
	unsigned char	wdInitialCount_lsb;
	unsigned char	wdInitialCount_msb;

}  IpmiWatchdog_t;

	/* Definition of IpmiWatchdog_t.wdTimerUse byte			*/
				/* [7] - don't log			*/
#define IPMI_WDT_DONT_LOG	0x80
				/* [6] - watchdog timer enable[d]	*/
#define IPMI_WDT_ENABLE		0x40
				/* [5:3] - reserved			*/
				/* [2:0] - timer use			*/
#define IPMI_WDT_USE		0x07
#define IPMI_WDT_USE_BIOS_FRB2	0x01
#define IPMI_WDT_USE_BIOS_POST	0x02
#define IPMI_WDT_USE_OS_LOAD	0x03
#define IPMI_WDT_USE_SMS_OS	0x04
#define IPMI_WDT_USE_OEM	0x05

	/* Definition of IpmiWatchdog_t.wdTimerAction byte		*/
				/* [7] - reserved			*/
				/* [6:4] - pre-timeout interrupt	*/
#define IPMI_WDT_INTR		0x70
#define IPMI_WDT_INTR_NONE	0x00
#define IPMI_WDT_INTR_SMI	0x10
#define IPMI_WDT_INTR_NMI	0x20
#define IPMI_WDT_INTR_MSG	0x30
				/* [3] - reserved			*/
				/* [2:0] - timeout action		*/
#define IPMI_WDT_ACTION		0x07
#define IPMI_WDT_ACTION_NONE	    0x00
#define IPMI_WDT_ACTION_HARD_RESET  0x01
#define IPMI_WDT_ACTION_POWER_DOWN  0x02
#define IPMI_WDT_ACTION_POWER_CYCLE 0x03

	/* Definition of IpmiWatchdog_t.wdExpFlags byte			*/
				/* [7:6] - reserved			*/
				/* [5] - OEM				*/
#define IPMI_WDT_EXP_OEM	0x20
				/* [4] - SMS/OS				*/
#define IPMI_WDT_EXP_SMS_OS	0x10
				/* [3] - OS Load			*/
#define IPMI_WDT_EXP_OS_LOAD	0x08
				/* [2] - BIOS/POST			*/
#define IPMI_WDT_EXP_BIOS_POST	0x04
				/* [1] - BIOS/FRB2			*/
#define IPMI_WDT_EXP_BIOS_FRB2	0x02
				/* [0] - reserved			*/

/*
 *****************************************************************************
 *
 *  OSA IPMI libraw.so Library Prototypes
 *
 *****************************************************************************
 */
RawAPIHandle_t	RawOpen ();

void		RawClose (
			/* Handle (in) - handle returned by RawOpen */
			RawAPIHandle_t		Handle
		);

Version_t	RawVersion (); 

Error_t		RawSMS_ATN (
			/* (in) the handle returned by open */
			RawAPIHandle_t		Handle, 

			/* (out) current BMC value of the SMS_ATN flag */
			unsigned char*		SMS_ATNFlag
		); 

Error_t		RawRequest (
			/* (in) the handle returned by open */
			RawAPIHandle_t		Handle,

			/* (in) Network Function/Lun */
			NetFnLun_t		NetFnLun, 

			/* (in) a valid command encoding from Appendix G 
			        of the IPMI Spec v1.5 */
			Command_t		RequestCommand, 

			/* (in) data (if any) that is associated with
			        the request */ 
			const unsigned char*	RequestData, 

			/* (in) the length in bytes of the request's data */
			Length_t		RequestDataLength, 

			/* (out) completion code */
			CompletionCode_t*	CompletionCode,

			/* (out) the corresponding IPMI response */
			unsigned char*		Response, 

			/* (in/out) the length in bytes of the IPMI response */
    			Length_t*		ResponseLength
		); 

Error_t		RawIPMBRequest (
			/* (in) the handle returned by open */
			RawAPIHandle_t		Handle,

			/* (in) slave address of the owner of a sensor
			        on the IPMB */
			DestinationAddr_t 	rsSa, 

			/* (in) Network Function/Lun */
			NetFnLun_t		NetFnLun, 

			/* (in) a valid command encoding from Appendix G 
			        of the IPMI Spec v1.5 */
			Command_t		RequestCommand, 

			/* (in) data (if any) that is associated with
			        the request */ 
			const unsigned char*	RequestData, 

			/* (in) the length in bytes of the request's data */
			Length_t		RequestDataLength, 

			/* (out) completion code */
			CompletionCode_t*	CompletionCode,

			/* (out) the corresponding IPMI response */
			unsigned char*		Response, 

			/* (in/out) the length in bytes of the IPMI response */
			Length_t*		ResponseLength
		); 

#ifdef __cplusplus
}
#endif

#endif /* __OSA_IPMI_H */
