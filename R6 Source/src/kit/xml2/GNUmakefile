SRC_DIR:=src/kit/xml2

TARGET_NAME := libxml2.so

# PARSER_SRCS
SRCS :=  \
	BufferSource.cpp						\
	DataIOSource.cpp						\
	DataSource.cpp							\
	DocumentCreationContext.cpp				\
	DocumentParseContext.cpp				\
	EntityParseContext.cpp					\
	EntityStore.cpp							\
	ObjectFactory.cpp						\
	ParseContext.cpp						\
	ParseXML.cpp							\
	ValidateChildren.cpp					\
	ValidatingContext.cpp					\

# CONTAINER_SRCS
SRCS +=  \
	Element.cpp								\
	Namespace.cpp							\
	Named.cpp								\
	NamedSet.cpp							\
	Document.cpp							\
	Comment.cpp								\
	Content.cpp								\
	Attribute.cpp							\
	CData.cpp								\
	ProcessingInstruction.cpp				\
	Text.cpp								\
	Valued.cpp								\
	XMLObject.cpp							\

# DTD_SRCS
SRCS +=  \
	AttributeDecl.cpp						\
	DocumentType.cpp						\
	DocumentTypeDefinition.cpp				\
	ElementDecl.cpp							\
	EntityDecl.cpp							\

# TOOLS_SRCS
SRCS +=  \
	Writer.cpp								\
	StringUtils.cpp							\

# Binder
SRCS +=  \
	XMLOStr.cpp								\
	XMLParser.cpp    						\
	XMLWriter.cpp    						\
	XMLRootSplay.cpp   						\
#	XMLBinderNode.cpp						\

MY_DFLAGS := -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 

MY_CFLAGS += $(F_NO_EXCEPTIONS) -O0 -g

MY_INCLUDES := -I$(BUILDHOME)/src/inc/xml2_p

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe2.so

LINKS := $(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

