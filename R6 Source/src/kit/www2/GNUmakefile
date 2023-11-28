SRC_DIR:=src/kit/www2

TARGET_NAME := libwww2.so

SRCS := \
		BufferedConnection.cpp \
		BufferedConnectionPool.cpp \
		ConnectionPool.cpp \
 		Content.cpp \
 		ContentManager.cpp \
		DNSCache.cpp \
		History.cpp \
		PasswordManager.cpp \
		Protocol.cpp \
		SecurityManager.cpp \
		Settings.cpp \
		SocketConnection.cpp \
		SocketConnectionPool.cpp \
		SSLConnection.cpp \
		SSLConnectionPool.cpp \
		ThreadPool.cpp \
		Timer.cpp \
 		util.cpp \
#
#		ContentView.cpp \
# 		CookieManager.cpp \
# 		Resource.cpp \
# 		ResourceCache.cpp \
#		SourceWindow.cpp \
#		ViewTransaction.cpp \
#		WindowManager.cpp \
#		WagnerDebug.cpp \

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
		-I$(BUILDHOME)/headers/content2 \
		-I$(BUILDHOME)/headers/render2 \
		-I$(BUILDHOME)/headers/storage2 \
		-I$(BUILDHOME)/headers/support2 \
		-I$(BUILDHOME)/headers/www2 \
		-I$(BUILDHOME)/headers/www2/mail \
		-I$(BUILDHOME)/src/drivers/history \
		-I$(BUILDHOME)/src/inc/www2_p \
		-I$(BUILDHOME)/src/inc/support2_p \
		-I$(BUILDHOME)/src/inc/app_p \
		-I$(BUILDHOME)/src/inc/os_p \
		
# ifneq ($(call MFLAG_IS_SET,LIBWWW_CONTAINS_MAIL),)
# MY_INCLUDES+= -Imail
# 
# SRCS+= \
# 	mail/Base64ToRawAdapter.cpp \
# 	mail/BufferedFileAdapter.cpp \
# 	mail/ConcatenateAdapter.cpp \
# 	mail/HTMLMailAdapter.cpp \
# 	mail/HTMLMailToUtf8Adapter.cpp \
# 	mail/IMAP.cpp \
# 	mail/KeyedMD5.cpp \
# 	mail/MailBinderSupport.cpp \
# 	mail/MailDebug.cpp \
# 	mail/MailStatusCodes.cpp \
# 	mail/MallocIOAdapter.cpp \
# 	mail/md5.c \
# 	mail/MailCacheProxy.cpp \
# 	mail/MimeMessage.cpp \
# 	mail/MimePartHeaderAdapter.cpp \
# 	mail/PartContainer.cpp \
# 	mail/PlainToQPAdapter.cpp \
# 	mail/PostOffice.cpp \
# 	mail/QPToPlainAdapter.cpp \
# 	mail/RawToBase64Adapter.cpp \
# 	mail/Rfc822MessageAdapter.cpp \
# 	mail/SendMailProxy.cpp \
# 	mail/SendMessageContainer.cpp \
# 	mail/SMTP.cpp \
# 	mail/StreamToFileAdapter.cpp \
# 	mail/StyledTextToHtmlAdapter.cpp \
# 	mail/SummaryContainer.cpp \
# 	mail/TextToHtmlAdapter.cpp \
# 	mail/TextToRfc822BodyAdapter.cpp \
# 	mail/TextToUtf8Adapter.cpp \
# 	mail/UserContainer.cpp \
# 	mail/UuencodeToRawAdapter.cpp \
# 	
# endif

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS :=  libroot.so \
				libbe2.so \
				libsocket.so \
				libbind.so \
				libnetconfig.so \
				libtextencoding.so \
#
#				libsettings.so \
#				libxml.so \

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

