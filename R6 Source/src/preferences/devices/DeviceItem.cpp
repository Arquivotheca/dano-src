#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "JumperedEditor.h"
#include "DeviceItem.h"
#include "dev_table.h"
#include "utils.h"

#define dprintf printf

/************************************************************/

void
DumpDeviceConfiguration(struct device_configuration *config)
{
#if 1
	printf("******\n");
	printf("flags: %x\n", config->flags);
	printf("resource count %i\n", config->num_resources);
	if (config->num_resources > 0) {
		char* str;
		int32 j;
		for (int32 i=0 ; i<(config->num_resources) ; i++) {
			switch (config->resources[i].type) {
				case B_IRQ_RESOURCE :
					j = mask_to_value(config->resources[i].d.m.mask);
					if ((j >= 0) && (j < 16)) {
						printf("irq bit %x %x %x\n", j,
							config->resources[i].d.m.flags,
							config->resources[i].d.m.cookie);
					} else
						printf("Invalid IRQ mask (%x)\n",config->resources[i].d.m.mask);
					break;
				case B_DMA_RESOURCE :
					j = mask_to_value(config->resources[i].d.m.mask);
					if ((j >= 0) && (j < 8)) {
						printf("dma bit %x %x %x\n", j,
							config->resources[i].d.m.flags,
							config->resources[i].d.m.cookie);
					} else
						printf("Invalid DMA mask (%x)\n",config->resources[i].d.m.mask);
					break;
				case B_IO_PORT_RESOURCE:
					printf("io port range %x %x %x %x %x\n",
						config->resources[i].d.r.minbase,
						config->resources[i].d.r.maxbase,
						config->resources[i].d.r.len,
						config->resources[i].d.r.flags,
						config->resources[i].d.r.cookie);
					break;
				case B_MEMORY_RESOURCE:
					printf("mem port range %x %x %x %x\n",
						config->resources[i].d.r.minbase,
						config->resources[i].d.r.len,
						config->resources[i].d.r.flags,
						config->resources[i].d.r.cookie);
					break;
				default :
					printf("Unhandled resource type (%x)\n", config->resources[i].type);
					break;
			}
		}
	}
	printf("******\n");
#endif
}

//*********************************************************************

TConflictItem::TConflictItem(resource_node* node, TDeviceItem* device)
	: BStringItem(""),
		fDevice(device), fNode(node)
{
	char str[128];
	
	if ( node->type == B_IRQ_RESOURCE
		 || node->type == B_DMA_RESOURCE) {
		if (node->type == B_IRQ_RESOURCE) {
			sprintf(str, "IRQ %i", node->bit);
		} else if (node->type == B_DMA_RESOURCE) {
			sprintf(str, "DMA %i", node->bit);
		}
	} else if (	node->type == B_IO_PORT_RESOURCE
				|| node->type == B_MEMORY_RESOURCE) {
//		uint32 end = node->start+node->length-1;
//		if (node->length == 0)
//			end = node->start;
	
		if (node->type == B_IO_PORT_RESOURCE) {
			sprintf(str, "IO Port range starting at 0x%x",
				node->start);		
		} else if (node->type == B_MEMORY_RESOURCE) {
			sprintf(str, "Memory Port range starting at 0x%x",
				node->start);		
		}
	}
	
	int32 cCount = node->conflictList->CountItems();
	char name[64];
	bool addtoend=false;
	TDeviceItem* cDevice;
	for (int32 i=0 ; i<cCount ; i++) {
		cDevice = (TDeviceItem*)node->conflictList->ItemAt(i);
		if (cDevice && (cDevice != device)) {
			cDevice->DeviceInfo()->LongDeviceName(name, 63);
			if (!addtoend) {
				sprintf(str, "%s conflicts with %s", str, name);
				addtoend = true;
			} else
				sprintf(str, "%s, %s", str, name);
		}
	}
		
	SetText(str);
	
	fWhoAmI = NULL;
}

TConflictItem::~TConflictItem()
{
	if (fWhoAmI)
		free(fWhoAmI);
}

void 
TConflictItem::DrawItem(BView *owner, BRect bounds, bool complete)
{	
	if (!Text())
		return;

	BPoint where = bounds.LeftTop();
	where.x += 4;
	where.y += (bounds.Height()/2 + FontHeight(owner, true)/2)-2;

	owner->PushState();
	
	rgb_color low;
	if (IsSelected()) {
		low.red = low.green = low.blue = 152;
		owner->SetHighColor(low);
	} else {
		low = owner->ViewColor();
		owner->SetHighColor(owner->ViewColor());
	}
		
	owner->SetLowColor(low);
	owner->FillRect(bounds);
	
	if (!IsEnabled()) {
		owner->SetHighColor(owner->ViewColor());
	}

	owner->SetDrawingMode(B_OP_COPY);
	owner->SetHighColor(0,0,0,255);
	owner->MovePenTo(where);

	owner->DrawString(Text());
	
	owner->PopState();
}

//*********************************************************************

TDeviceItem::TDeviceItem(struct isa_device_info* info,
						struct 	device_configuration* current,
						struct 	possible_device_configurations* possible)
	: BStringItem("", 1)
{
	fWindowIsShowing = false;
	fWindow = NULL;
	fDevice = new TDevice(info, current, possible);
	fDevice->SetParent(this);
	fIsValid = true;	//only for jumpered
}

TDeviceItem::TDeviceItem(struct pci_info* pciInfo, struct device_info* deviceInfo,
						struct 	device_configuration* current,
						struct 	possible_device_configurations* possible)
	: BStringItem("", 1)
{
	fWindowIsShowing = false;
	fWindow = NULL;
	fDevice = new TDevice(pciInfo, deviceInfo, current, possible);
	fDevice->SetParent(this);
	fIsValid = true;	//only for jumpered
}

TDeviceItem::~TDeviceItem()
{
	delete fDevice;
}

void 
TDeviceItem::DrawItem(BView *owner, BRect bounds, bool complete)
{
	if (!Text())
		return;

	BPoint where = bounds.LeftTop();
	where.x += 4;
	where.y += (bounds.Height()/2 + FontHeight(owner, true)/2)-2;

	BPoint configLoc = where;
	configLoc.x = bounds.Width() - owner->StringWidth("disabled by sytem");
	
	owner->PushState();
	
	rgb_color low;
	if (IsSelected()) {
		low.red = low.green = low.blue = 152;
		owner->SetHighColor(low);
	} else {
		low = owner->ViewColor();
		owner->SetHighColor(owner->ViewColor());
	}
		
	owner->SetLowColor(low);
	owner->FillRect(bounds);
	
	if (!IsEnabled()) {
		owner->SetHighColor(owner->ViewColor());
	}

	owner->SetDrawingMode(B_OP_COPY);
	if (fDevice->IsEnabled())
		owner->SetHighColor(0,0,0,255);
	else
		owner->SetHighColor(255, 20, 20, 255);
	owner->MovePenTo(where);

	char str[128];
	fDevice->LongDeviceName(str, 128);
	owner->DrawString(str);

	owner->MovePenTo(configLoc);
	
	if (fDevice->DeviceInfo()->config_status == B_OK)
		owner->DrawString("enabled");
	else if (fDevice->DeviceInfo()->config_status == B_DEV_DISABLED_BY_USER)
		owner->DrawString("disabled by user");
	else if (fDevice->DeviceInfo()->config_status == B_DEV_RESOURCE_CONFLICT)
		owner->DrawString("disabled by system");
	else if (fDevice->DeviceInfo()->config_status == B_DEV_CONFIGURATION_ERROR)
		owner->DrawString("configuration error");
	else {
		sprintf(str, "disabled for unknown reason (%x)",
			fDevice->DeviceInfo()->config_status);
		owner->DrawString(str);
	}

	owner->PopState();
}

TDevice*
TDeviceItem::DeviceInfo()
{
	return fDevice;
}

bool
TDeviceItem::WindowIsShowing()
{
	return fWindowIsShowing;
}

void
TDeviceItem::ShowDeviceInfoWindow(TDeviceListWindow* mw, int32 windowCount)
{
	if (fWindowIsShowing)
		fWindow->Activate();
	else {
		fWindow = new TDeviceInfoWindow(mw, this, windowCount);
		if (fWindow) {
			fWindowIsShowing = true;
			fWindow->Show();
			mw->InfoWindowShowing();		
		}
	}
}

void
TDeviceItem::CloseDeviceInfoWindow(bool saveTemplate)
{
	if (fWindowIsShowing){
		fWindow->SetSaveIfNecessary(saveTemplate);
		fWindow->PostMessage(B_QUIT_REQUESTED);
	}
}

void
TDeviceItem::WindowClosing()
{
	fWindowIsShowing = false;
	fWindow = NULL;
}

//*********************************************************************

THeaderItem::THeaderItem(const char* text)
	: BStringItem(text)
{
}

THeaderItem::~THeaderItem()
{
}

void 
THeaderItem::DrawItem(BView *owner, BRect bounds, bool complete)
{	
	if (!Text())
		return;

	BPoint where = bounds.LeftTop();
	where.x += 4;
	where.y += (bounds.Height()/2 + FontHeight(owner, true)/2)-2;

	owner->PushState();
	
	rgb_color low;
	if (IsSelected()) {
		low.red = low.green = low.blue = 152;
		owner->SetHighColor(low);
	} else {
		low = owner->ViewColor();
		owner->SetHighColor(owner->ViewColor());
	}
		
	owner->SetLowColor(low);
	owner->FillRect(bounds);
	
	if (!IsEnabled()) {
		owner->SetHighColor(owner->ViewColor());
	}

	owner->SetDrawingMode(B_OP_COPY);
	owner->SetHighColor(0,0,0,255);
	owner->MovePenTo(where);

	owner->DrawString(Text());
	
	owner->PopState();
}

//*********************************************************************

TDevice::TDevice(struct isa_device_info *info,
	struct device_configuration* current, struct possible_device_configurations* possible)
{
	fISAInfo = info;
	fPCIInfo = NULL;
	fDeviceInfo = &(info->info);
	fDeviceType = kISAType;
	
	InitObject(current, possible);
}

TDevice::TDevice(struct pci_info* pciInfo, struct device_info* deviceInfo,
	struct device_configuration* current, struct possible_device_configurations* possible)
{
	fISAInfo = NULL;
	fPCIInfo = pciInfo;
	fDeviceInfo = deviceInfo;
	fDeviceType = kPCIType;

	InitObject(current, possible);
}

void
TDevice::InitObject(struct 	device_configuration* current,
	struct 	possible_device_configurations*	possible)
{
	fCurrentConfig = current;
	fPossibleConfig = possible;
	
	fIsJumpered = false;
	fEnabled = !(fDeviceInfo->config_status == B_DEV_DISABLED_BY_USER);
	fPossibleResourceConflict = false;
	MakeDeviceName();
	
	fNeedToOverrideConfig = false;
	fOverrideConfig = NULL;
	
	fConflictList = new BList();
	fParent = NULL;
}

TDevice::~TDevice()
{		
	if (fConflictList) {
		int32 count = fConflictList->CountItems()-1;
		TConflictItem* item;
		for (int32 i=count ; i>=0 ; i--) {
			item = (TConflictItem*)fConflictList->RemoveItem(i);
			if (item)
				delete item;
		}
	}
		
}

void
TDevice::SetDeviceConfiguration(struct device_configuration* config)
{
	if (fCurrentConfig)
		free(fCurrentConfig);
	fCurrentConfig = config;
}

const char*
TDevice::DeviceLabel()
{
	if (CardName() && strlen(CardName()) > 0)
		return CardName();
	else if (LogicalDeviceName() && strlen(LogicalDeviceName()) > 0)
		return LogicalDeviceName();
	else
		return DeviceName();
}

bool
TDevice::Jumpered()
{
	return fIsJumpered;
}

void
TDevice::SetJumpered(bool state)
{
	fIsJumpered = state;
}

struct device_info*
TDevice::DeviceInfo()
{
	return fDeviceInfo;
}

struct isa_device_info*
TDevice::ISAInfo()
{
	if (IsISA())
		return fISAInfo;
	else
		return NULL;
}

struct pci_info*
TDevice::PCIInfo()
{
	if (IsISA())
		return NULL;
	else
		return fPCIInfo;
}

//	device info accessors

uint32
TDevice::VendorID()
{
	if (IsISA()) {
		return fISAInfo->vendor_id;
	} else {
		return (uint32)fPCIInfo->vendor_id;
	}
}

uint32
TDevice::CardID()
{
	if (IsISA())
		return fISAInfo->card_id;
	else
		return fPCIInfo->device_id;
}

const char*
TDevice::LogicalDeviceName()
{
	if (IsISA())
		return fISAInfo->logical_device_name;
	else
		return NULL;
}

static void
unpack_eisa_id(EISA_PRODUCT_ID pid, unsigned char* str)
{
	str[0] = ((pid.b[0] >> 2) & 0x1F) + 'A' - 1; 
	str[1] = ((pid.b[1] & 0xE0)>>5) | ((pid.b[0] & 0x3) << 3) + 'A' - 1;
	str[2] = (pid.b[1] & 0x1F) + 'A' - 1;
	str[3] = '\0';
}

void
Vendor(uint32 id, uchar* code)
{
	EISA_PRODUCT_ID eisa;
	eisa.id = id;
	unpack_eisa_id(eisa, code);
}

void
ProductNumber(uint32 id, char* prodNum)
{
	EISA_PRODUCT_ID eisa;
	eisa.id = id;
	sprintf(prodNum, "%x%x", eisa.b[2], eisa.b[3]>>4);
}

void
Revision(uint32 id, int32* revision)
{
	EISA_PRODUCT_ID eisa;		// in isapnp.h
	eisa.id = id;
	*revision = eisa.b[3] & 0xf;
}

void
EISAInfo(uint32 id, uchar* vendor, char* prodNum, int32* revision)
{
	Vendor(id, vendor);
	ProductNumber(id, prodNum);
	Revision(id, revision);
}

uint32
TDevice::LogicalDeviceID()
{
	//	look in config_mgr.c
	if (IsISA())
		return fISAInfo->logical_device_id;
	else
		return 0;
}

int32
TDevice::CountCompatibleIDs()
{
	if (IsISA())
		return fISAInfo->num_compatible_ids;
	else
		return 0;
}

uint32
TDevice::CompatibleID(int32 index)
{
	if (IsISA()){
		if (index > CountCompatibleIDs()) {
			return 0;
		}
		
		return fISAInfo->compatible_ids[index];
	} else
		return 0;
}

const char*
TDevice::CardName()
{
	if (IsISA())
		return fISAInfo->card_name;
	else
		return NULL;
}

void
TDevice::SetCardName(const char* name)
{
	strncpy(fISAInfo->card_name, name, B_OS_NAME_LENGTH-1);
}

bus_type
TDevice::BusType()	// ISA / PCI
{
	if (IsISA())
		return B_ISA_BUS;
	else
		return B_PCI_BUS;
}
			
const char*
TDevice::DeviceName()
{
	return fDeviceName;
}

void
TDevice::LongDeviceName(char* name, int32 max)
{
	if (!name)
		return;
	
	const char* dl = DeviceLabel();
	if (dl)
		strncpy(name, dl, max);
	else
		sprintf(name, "Unknown");
		
	const char* ldn = LogicalDeviceName();
	if (ldn && strlen(ldn) > 0) {
		if (strlen(ldn) + strlen(name) < max)
			sprintf(name, "%s (%s)", name, ldn);
		else {
			char temp[128];
			strncpy(temp, ldn, (max-strlen(name)-1));
			sprintf(name, "%s (%s)", name, temp);
		}
	}
}

uchar
TDevice::Base()
{
	return fDeviceInfo->devtype.base;
}

void
TDevice::SetBase(uchar b)
{
	fDeviceInfo->devtype.base = b;
}

uchar 
TDevice::SubType()
{
	return fDeviceInfo->devtype.subtype;
}

void
TDevice::SetSubType(uchar s)
{
	fDeviceInfo->devtype.subtype = s;
}

uchar
TDevice::Interface()
{
	return fDeviceInfo->devtype.interface;
}

void
TDevice::SetInterface(uchar i)
{
	fDeviceInfo->devtype.interface = i;
}

void
TDevice::SetEnabled(bool state)
{
	fEnabled = state;
}

bool
TDevice::IsEnabled()
{
	return fEnabled;
}

bool
TDevice::IsConfigured()
{
	return fDeviceInfo->flags & B_DEVICE_INFO_CONFIGURED;
}

bool
TDevice::CanConfigure()
{
	return fDeviceInfo->flags & B_DEVICE_INFO_CAN_BE_DISABLED;
}

bool
TDevice::CanDisable()
{
	return fDeviceInfo->flags & B_DEVICE_INFO_CAN_BE_DISABLED;
}

//	current config info
struct device_configuration*
TDevice::CurrentConfiguration()
{
	//	override will be modified in the current/possible config window
	//	
	if (fOverrideConfig && fNeedToOverrideConfig) {
		return fOverrideConfig;
	} else {
		return fCurrentConfig;
	}
}

void
TDevice::SetCurrentConfiguration(struct device_configuration* config)
{
	if (fCurrentConfig)
		free(fCurrentConfig);
	
	fCurrentConfig = config;
}

void
TDevice::SetOverrideConfig(struct device_configuration* newConfig)
{
	if (!newConfig)
		return;
		
	if (fOverrideConfig)
		free(fOverrideConfig);
		
	fOverrideConfig = newConfig;
	fNeedToOverrideConfig = true;
}

void
TDevice::SetDefaultConfiguration()
{
	if (fNeedToOverrideConfig && fOverrideConfig) {
		fNeedToOverrideConfig = false;
		free(fOverrideConfig);
		fOverrideConfig = NULL;
	}
}

//	possible config info
int32
TDevice::CountPossibleConfigs()
{
	if (!Jumpered())
		return fPossibleConfig->num_possible;
	else
		return 0;
}

struct 	possible_device_configurations*
TDevice::PossibleConfigInfo()
{
	if (!Jumpered())
		return fPossibleConfig;
	else
		return NULL;
}

//	resource info
bool
TDevice::IRQIsShareable(bus_type bus, resource_type type, int32 which)
{
	if (type != B_IRQ_RESOURCE)
		return false;
		
	//	always return not shareable for ISA, as per vyt	
	if (bus == B_ISA_BUS)
		return false;
		
	struct device_configuration* config = CurrentConfiguration();
	if (!config)
		return false;
		
	status_t cnt = CountResourceDescriptors(config, type);
		
	resource_descriptor desc;
	for (int32 resCnt=0 ; resCnt<cnt ; resCnt++) {
		GetNthResourceDescriptor(config, resCnt, type,
			&desc, sizeof(resource_descriptor));
	
		if (type == B_IRQ_RESOURCE) {
			int32 bit = mask_to_value(desc.d.m.mask);
			if ((bit == which)) {
				if (bus == B_ISA_BUS)
					return desc.d.m.flags & B_IRQ_ISA_SHAREABLE;
				else if (bus == B_PCI_BUS)
					return desc.d.m.flags & B_IRQ_PCI_SHAREABLE;
				else
					return false;
			}
		}
	}
	
	return false;
}

bool
TDevice::ResourceIsSet(resource_type type, int32 val1, int32 val2)
{
	struct device_configuration* config = CurrentConfiguration();
	if (!config)
		return false;
		
//printf("resourceisset %s %i %i %i\n", DeviceLabel(), type, val1, val2);
	status_t cnt = CountResourceDescriptors(config, type);
		
	resource_descriptor desc;
	for (int32 resCnt=0 ; resCnt<cnt ; resCnt++) {
		GetNthResourceDescriptor(config, resCnt, type,
			&desc, sizeof(resource_descriptor));
	
		if (type == B_IRQ_RESOURCE || type == B_DMA_RESOURCE) {
			int32 bit = mask_to_value(desc.d.m.mask);
			if ((bit >= 0) && (bit < val1)) {
				if (bit == val2) {
//printf("device %s has resource %i\n", DeviceLabel(), bit);
					return true;
				}
			}
		} else {
			// see if the start intersects with any range
			if (val1 >= desc.d.r.minbase &&
				val1 <= (desc.d.r.minbase + desc.d.r.len - 1)) {
				return true;
			}			
		}
	}
	
	return false;
}

bool
TDevice::GetNextRange(resource_type type, int32* index, int32* start, int32* length)
{
	struct device_configuration* config = CurrentConfiguration();
	if (!config)
		return false;

	status_t cnt = CountResourceDescriptors(config, type);
	if (*index >= cnt)
		return false;
		
	resource_descriptor desc;
	GetNthResourceDescriptor(config, *index, type, &desc, sizeof(resource_descriptor));
	*start = desc.d.r.minbase;
	*length = desc.d.r.len;	
	(*index)++;
	
	return true;
}

void
TDevice::AddConflict(TConflictItem* item)
{
	if (fConflictList && item) {
		fConflictList->AddItem(item);
		if (fParent->WindowIsShowing()) {
			fParent->Window()->PostMessage(msg_update_conflict_list);
		}
	}
}

void
TDevice::RemoveConflict(resource_node* node)
{
	int32 count = fConflictList->CountItems();
	TConflictItem* currItem;
	for (int32 i=0 ; i<count ; i++) {
		currItem = (TConflictItem*)fConflictList->ItemAt(i);
		if(currItem) {
			if (currItem->Node() == node) {
				fConflictList->RemoveItem(i);
			}
		}
	}
	if (fParent->WindowIsShowing()) {
		fParent->Window()->PostMessage(msg_update_conflict_list);
	}
}

void
TDevice::ClearConflictList()
{
	if (fConflictList && fConflictList->CountItems() > 0) {
		int32 count = fConflictList->CountItems() - 1;
		if (count >= 0) {
			for (int32 i=count ; i>=0 ; i--) {
				TConflictItem* currItem = (TConflictItem*)fConflictList->RemoveItem(i);
			}
			if (fParent->WindowIsShowing()) {
				fParent->Window()->PostMessage(msg_update_conflict_list);
			}
		}
	}
}

void
TDevice::SetParent(TDeviceItem* p)
{
	fParent = p;
}

TDeviceItem*
TDevice::Parent()
{
	return fParent;
}

void
TDevice::PrintToStream()
{
#if DEBUG
	printf("***************************\n");
	
	printf("Device label: %s\n", DeviceLabel());
	printf("Logical Device Name: %s\n", LogicalDeviceName());
	printf("Card Name: %s\n", CardName());
	printf("Device Name: %s\n", DeviceName());

	switch(BusType()) {
		case B_ISA_BUS:
			printf("ISA bus\n");
			break;
		
		case B_PCI_BUS:
			printf("PCI bus\n");
			break;
			
		case B_PCMCIA_BUS:
			printf("PCMCIA bus\n");
			break;
			
		case B_UNKNOWN_BUS:
		default:
			printf("Unknown bus\n");
			break;
	}
	
	printf("Device ID: %x %x %x %x\n",
		DeviceInfo()->id[0], DeviceInfo()->id[1],
		DeviceInfo()->id[2], DeviceInfo()->id[3]);
	
	printf("Base: %x, Subtype: %x, Interface: %x\n", Base(), SubType(), Interface());

	printf("Device State: %s\n", (IsEnabled() ? "enabled" : "disabled"));
	printf("Configured State: %s\n", (IsConfigured() ? "configured" : "not configured"));
	printf("Can configure: %s\n", (CanConfigure() ? "yes" : "no"));
	printf("Can disable: %s\n", (CanDisable() ? "yes" : "no"));
	printf("Config status: %i (%i %i %i)\n",
		DeviceInfo()->config_status,
		B_DEV_RESOURCE_CONFLICT, B_DEV_CONFIGURATION_ERROR, B_DEV_DISABLED_BY_USER);	
	
	uchar vendor[4];
	char prodNum[8];
	int32 revision;
	uint32 id = LogicalDeviceID();
	if (id > 0) {
		Vendor(id, vendor);
		ProductNumber(id, prodNum);
		Revision(id,&revision);
		printf("Logical Device ID: Vendor %s, Product# %s, Revision %x\n",
			vendor, prodNum, revision);
	}
		
	int32 count = CountCompatibleIDs();
	for (int32 i=0 ; i<count ; i++) {
		id = CompatibleID(i);
		if (id > 0) {
		Vendor(id, vendor);
		ProductNumber(id, prodNum);
		Revision(id,&revision);
			printf("Compatible ID # %i: Vendor %s, Product# %s, Revision %x\n",
				id, vendor, prodNum, revision);
		}
	}

	printf("***************************\n\n");
#endif
}

void
TDevice::MakeDeviceName()
{
	char* str = DeviceType(fDeviceInfo->devtype.base, 
		fDeviceInfo->devtype.subtype, fDeviceInfo->devtype.interface);

	if (str)
		strcpy(fDeviceName, str);
	else
		strcpy(fDeviceName, BaseType(fDeviceInfo->devtype.base));
}
