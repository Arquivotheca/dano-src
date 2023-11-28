#ifndef _PR_SERVER_H_
#define _PR_SERVER_H_

// Needed by GraphicsEnv.cpp for run_add_printer_panel()
#define	msg_add_printer		'addp'

// Needed by Printers, print_server, GraphicsEnv.cpp, BPintJob
#define PSRV_PRINT_PREFS_SIG 		"application/x-vnd.Be-PRNT"
#define PSRV_PRINT_SERVER_SIG		"application/x-vnd.Be-PSRV"
#define PSRV_PRINTER_MIMETYPE		"application/x-vnd.Be.printer"
#define	PSRV_PRINTER_SPOOL_MIMETYPE	"application/x-vnd.Be.printer-spool"

// Values for the print jobs status attribute (should be a pretty name for the moment)
#define PSRV_JOB_STATUS_PROCESSING	"Processing"
#define PSRV_JOB_STATUS_COMPLETED	"Completed"
#define PSRV_JOB_STATUS_CANCELLED	"Cancelled"
#define PSRV_JOB_STATUS_CANCELLING	"Cancelling"
#define PSRV_JOB_STATUS_FAILED		"Failed"
#define PSRV_JOB_STATUS_WAITING		"Waiting"

// spool Attributes 
#define PSRV_SPOOL_ATTR_MIMETYPE	"_spool/MimeType"
#define PSRV_SPOOL_ATTR_PAGECOUNT	"_spool/Page Count"
#define PSRV_SPOOL_ATTR_DESCRIPTION	"_spool/Description"
#define PSRV_SPOOL_ATTR_PRINTER		"_spool/Printer"
#define PSRV_SPOOL_ATTR_STATUS		"_spool/Status"
#define PSRV_SPOOL_ATTR_ERRCODE		"_spool/_errorcode"
#define PSRV_SPOOL_ATTR_JOB_NAME	"_spool/jobname"
#define PSRV_SPOOL_ATTR_PREVIEW		"_spool/_preview"

// printer attributes
#define PSRV_PRINTER_ATTR_DRV_NAME			"Driver Name"
#define PSRV_PRINTER_ATTR_PRT_NAME			"Printer Name"
#define PSRV_PRINTER_ATTR_COMMENTS			"Comments"
#define PSRV_PRINTER_ATTR_STATE				"state"
#define PSRV_PRINTER_ATTR_TRANSPORT			"transport"
#define PSRV_PRINTER_ATTR_TRANSPORT_ADDR	"transport_address"
#define PSRV_PRINTER_ATTR_CNX				"connection"
#define PSRV_PRINTER_ATTR_PNP				"_PNP"
#define PSRV_PRINTER_ATTR_MDL				"_MDL"
#define PSRV_PRINTER_ATTR_SETTINGS			"_settings"

#endif
