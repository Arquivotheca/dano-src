SRC_DIR:=src/kit/www

TARGET_NAME := libwww.so

SRCS :=  \
	ContentManager.cpp util.cpp \
	Resource.cpp ViewTransaction.cpp Content.cpp \
	History.cpp ResourceCache.cpp ContentView.cpp CookieManager.cpp \
	ThreadPool.cpp Timer.cpp DNSCache.cpp Protocol.cpp URL.cpp WagnerDebug.cpp \
	PasswordManager.cpp WindowManager.cpp Settings.cpp SourceWindow.cpp \
	SecurityManager.cpp StringBuffer.cpp \
	SocketConnection.cpp BufferedConnection.cpp SSLConnection.cpp \
	ConnectionPool.cpp SocketConnectionPool.cpp BufferedConnectionPool.cpp \
	SSLConnectionPool.cpp Dialer.cpp

MY_CFLAGS := -DDO_AUTHENTICATION=1 -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

ifneq ($(call MFLAG_IS_SET, WWW_DEBUGGING_LOG_CACHE),)
	MY_CFLAGS += -DDEBUG_CACHE=1
endif

ifneq ($(call MFLAG_IS_SET, WWW_LEAK_CHECKING),)
	MY_CFLAGS += -DDEBUG_RESOURCE_LEAKS=1
endif

ifneq ($(call MFLAG_IS_SET, WWW_LOG_CONNECTION),)
        MY_DFLAGS += -DENABLE_CONNECTION_TRACE=1
endif

ifneq ($(call MFLAG_IS_SET, WWW_CONNECTION_STATISTICS),)
        MY_DFLAGS += -DENABLE_CONNECTION_STATISTICS=1
endif

MY_INCLUDES := \
		-I$(BUILDHOME)/headers/www -I$(BUILDHOME)/headers/www/mail \
		-I$(BUILDHOME)/src/drivers/history -I$(BUILDHOME)/src/inc/www_p \
		-I$(BUILDHOME)/src/inc/support_p -I$(BUILDHOME)/src/inc/app_p \
		-I$(BUILDHOME)/src/inc/os_p
		
ifneq ($(call MFLAG_IS_SET,LIBWWW_CONTAINS_MAIL),)
MY_INCLUDES+= -Imail

SRCS+= \
	mail/Base64ToRawAdapter.cpp \
	mail/BufferedFileAdapter.cpp \
	mail/ConcatenateAdapter.cpp \
	mail/HTMLMailAdapter.cpp \
	mail/HTMLMailToUtf8Adapter.cpp \
	mail/MailServer.cpp \
	mail/IMAP.cpp \
	mail/POP3.cpp \
	mail/PopHash.cpp \
	mail/POP3Parse.cpp \
	mail/KeyedMD5.cpp \
	mail/MailBinderSupport.cpp \
	mail/MailDebug.cpp \
	mail/MailStatusCodes.cpp \
	mail/MallocIOAdapter.cpp \
	mail/md5.c \
	mail/MailCacheProxy.cpp \
	mail/MimeMessage.cpp \
	mail/MimePartHeaderAdapter.cpp \
	mail/PartContainer.cpp \
	mail/PlainToQPAdapter.cpp \
	mail/PostOffice.cpp \
	mail/QPToPlainAdapter.cpp \
	mail/RawToBase64Adapter.cpp \
	mail/Rfc822MessageAdapter.cpp \
	mail/SendMailProxy.cpp \
	mail/SendMessageContainer.cpp \
	mail/SMTP.cpp \
	mail/StreamToFileAdapter.cpp \
	mail/StyledTextToHtmlAdapter.cpp \
	mail/SummaryContainer.cpp \
	mail/TextToHtmlAdapter.cpp \
	mail/TextToRfc822BodyAdapter.cpp \
	mail/TextToUtf8Adapter.cpp \
	mail/UserContainer.cpp \
	mail/UuencodeToRawAdapter.cpp \

	
endif

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so libsocket.so libbind.so libnetconfig.so libsettings.so libtextencoding.so libxml.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

