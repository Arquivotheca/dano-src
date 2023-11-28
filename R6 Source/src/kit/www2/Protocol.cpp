#include <support2/Autolock.h>
#include <support2/Locker.h>
#include <support2/String.h>
#include <www2/Protocol.h>
#include <www2/ContentManager.h>

using namespace B::Support2;
using namespace B::WWW2;

//Protocol::Protocol(void* handle)
//	: m_handle(reinterpret_cast<ContentHandle*>(handle))
//{
//}

Protocol::Protocol(void* handle)
{}


Protocol::~Protocol()
{
	//	m_handle->Close();
}


bool Protocol::GetRedirectURL(BUrl&, bigtime_t*)
{
	return false;
}

status_t Protocol::Open(const BUrl &, const BUrl &, BMessage *, uint32 )
{
	return B_ERROR;
}

ssize_t Protocol::GetContentLength()
{
	return -1;
}

void Protocol::GetContentType(char *type, int )
{
	if (type != NULL)
		strcpy(type, "");
}

CachePolicy Protocol::GetCachePolicy()
{
	return CC_CACHE;
}

ssize_t Protocol::ReadV(const struct iovec *vector, ssize_t count)
{
	return B_ERROR;
}

off_t Protocol::Seek(off_t , uint32 )
{
	return 0;
}

off_t Protocol::Position() const
{
	return 0;
}

ssize_t Protocol::WriteV(const struct iovec *vector, ssize_t count)
{
	return B_ERROR;
}

status_t Protocol::End()
{
	return B_ERROR;
}

status_t Protocol::Sync()
{
	return B_ERROR;
}

status_t Protocol::SetMetaCallback(void*, MetaCallback)
{
	return B_ERROR;
}

status_t Protocol::SetSize(off_t )
{
	return B_ERROR;
}

Protocol* Protocol::InstantiateProtocol(const char *scheme)
{
	//	return ContentManager::Default().InstantiateProtocol(scheme);
	return NULL;
}

void Protocol::Abort()
{}


void Protocol::_ReservedProtocol2()
{}

void Protocol::_ReservedProtocol3()
{}

void Protocol::_ReservedProtocol4()
{}

void Protocol::_ReservedProtocol5()
{}

void Protocol::_ReservedProtocol6()
{}

void Protocol::_ReservedProtocol7()
{}

void Protocol::_ReservedProtocol8()
{}

void Protocol::_ReservedProtocol9()
{}

void Protocol::_ReservedProtocol10()
{}

void Protocol::_ReservedProtocol11()
{}

void Protocol::_ReservedProtocol12()
{}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------


ProtocolFactory::ProtocolFactory()
{}


ProtocolFactory::~ProtocolFactory()
{}


bool ProtocolFactory::KeepLoaded() const
{
	return false;
}

size_t ProtocolFactory::GetMemoryUsage() const
{
	return 0;
}

void ProtocolFactory::_ReservedProtocolFactory1()
{}

void ProtocolFactory::_ReservedProtocolFactory2()
{}

void ProtocolFactory::_ReservedProtocolFactory3()
{}

void ProtocolFactory::_ReservedProtocolFactory4()
{}

void ProtocolFactory::_ReservedProtocolFactory5()
{}

void ProtocolFactory::_ReservedProtocolFactory6()
{}

void ProtocolFactory::_ReservedProtocolFactory7()
{}

void ProtocolFactory::_ReservedProtocolFactory8()
{}

void ProtocolFactory::_ReservedProtocolFactory9()
{}

void ProtocolFactory::_ReservedProtocolFactory10()
{}

void ProtocolFactory::_ReservedProtocolFactory11()
{}

void ProtocolFactory::_ReservedProtocolFactory12()
{}

