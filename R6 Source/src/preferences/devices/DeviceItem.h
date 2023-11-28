#ifndef DEVICE_ITEM
#define DEVICE_ITEM

#include <drivers/module.h>
#include <config_manager.h>
#include <config_manager_p.h>
#include <config_driver.h>
#include <isapnp.h>
#include <PCI.h>

#include <ListItem.h>
#include <ListView.h>

#include "utils.h"
#include "cm_wrapper.h"

#include "DeviceInfo.h"
#include "DeviceListWindow.h"
#include "ResourceList.h"

//	for logical device id and compatible ids
void Vendor(uint32 id, uchar* code);
void ProductNumber(uint32 id, char* prodNum);
void Revision(uint32 id, int32 *revision);
void EISAInfo(uint32 id, uchar* vendor, char* prodNum, int32* revision);

enum which_device {
	kISAType = 0,
	kPCIType,
	kJumperedType
};

class TConflictItem : public BStringItem {
public:
								TConflictItem(resource_node* n, TDeviceItem* d);
/*								TConflictItem(const char* thisdevice,
											  const char* conflictdevice,
											  resource_type type,
											  uint32 start, uint32 length,
											  TDeviceItem* d);*/
								~TConflictItem();
		
		void					DrawItem(BView *owner, BRect frame,
									bool complete = false);
									
		TDeviceItem*			Device() { return fDevice; }
		resource_node*			Node() { return fNode; }
									
private:
		char*					fWhoAmI;
		TDeviceItem*			fDevice;
		resource_node*			fNode;
};

class TDevice {
public:
								TDevice(struct isa_device_info* info,
									struct 	device_configuration* current,
									struct 	possible_device_configurations* possible);
								TDevice(struct pci_info* pciInfo,
									struct device_info* deviceInfo,
									struct 	device_configuration* current,
									struct 	possible_device_configurations* possible);
		void					InitObject(struct 	device_configuration* current,
									struct 	possible_device_configurations* possible);
								~TDevice();
								
		void					SetDeviceConfiguration(struct device_configuration* config);

		const char*				DeviceLabel();
		
		bool					IsPCI() { return fDeviceType == kPCIType; }
		bool					IsISA() { return fDeviceType == kISAType; }
		bool					Jumpered();
		void					SetJumpered(bool state);
		
struct 	device_info*			DeviceInfo();
struct 	isa_device_info*		ISAInfo();
struct 	pci_info*				PCIInfo();
	
		uint32					VendorID();
		uint32					CardID();
		const char*				LogicalDeviceName();
		uint32					LogicalDeviceID();

		int32					CountCompatibleIDs();
		uint32					CompatibleID(int32 index);
		
		const char*				CardName();
		void					SetCardName(const char* name);
		bus_type				BusType();
		const char*				DeviceName();
		void		 			LongDeviceName(char* name, int32 max);
		
		uchar					Base();
		void					SetBase(uchar);
		uchar 					SubType();
		void					SetSubType(uchar);
		uchar					Interface();
		void					SetInterface(uchar);
		
		void					SetEnabled(bool);
		bool					IsEnabled();
		bool					IsConfigured();
		bool					CanConfigure();
		bool					CanDisable();
		
		//	current config info
struct 	device_configuration*	CurrentConfiguration();
		void					SetCurrentConfiguration(struct device_configuration* config);
		void					SetOverrideConfig(struct device_configuration* newConfig);
		bool					NeedToOverrideConfig() { return fNeedToOverrideConfig; }

		void					SetDefaultConfiguration();
		
		//	possible config info
		int32							CountPossibleConfigs();
struct 	possible_device_configurations*	PossibleConfigInfo();

		//	resource info
		bool					ResourceIsSet(resource_type type, int32 max,
									int32 which);
		bool 					GetNextRange(resource_type type, int32* index,
									int32* start, int32* length);
		bool					IRQIsShareable(bus_type bus, resource_type type,
									int32 which);
											
		void					AddConflict(TConflictItem*);
		void 					RemoveConflict(resource_node*);
		void					ClearConflictList();
		BList*					ConflictList() { return fConflictList; }
		
		void					SetParent(TDeviceItem*);
		TDeviceItem*			Parent();
		
		void					PrintToStream();
		
private:
		void							MakeDeviceName();
		
		which_device					fDeviceType;		
struct 	isa_device_info*				fISAInfo;			// isapnp.h
struct 	pci_info*						fPCIInfo;			// PCI.h
struct 	device_info*					fDeviceInfo;
		bool							fIsJumpered;
		bool							fEnabled;
		bool							fPossibleResourceConflict;

struct 	device_configuration*			fCurrentConfig;
struct 	possible_device_configurations*	fPossibleConfig;
		
		char							fDeviceName[128];

		bool							fNeedToOverrideConfig;
struct 	device_configuration*			fOverrideConfig;

		BList*							fConflictList;
		TDeviceItem*					fParent;
};

class TDeviceItem : public BStringItem {
public:
								TDeviceItem(struct isa_device_info* info,
									struct 	device_configuration* current,
									struct 	possible_device_configurations* possible);
								TDeviceItem(struct pci_info* pciInfo,
									struct device_info* deviceInfo,
									struct 	device_configuration* current,
									struct 	possible_device_configurations* possible);
								~TDeviceItem();
		
		void					DrawItem(BView *owner, BRect frame,
									bool complete = false);
									
		TDevice*				DeviceInfo();
		
		bool					WindowIsShowing();
		void					ShowDeviceInfoWindow(TDeviceListWindow*, int32 wc);
		void					CloseDeviceInfoWindow(bool dontSave);
		TDeviceInfoWindow*		Window() { return fWindow; }
		void					WindowClosing();

		void					ValidDevice(bool state) {fIsValid=state;}
		bool					IsValidDevice() { return fIsValid; }
									
private:
		TDevice*				fDevice;
		bool					fWindowIsShowing;
		TDeviceInfoWindow*		fWindow;
		bool					fIsValid;
};

class THeaderItem : public BStringItem {
public:
								THeaderItem(const char* text);
								~THeaderItem();
		
		void					DrawItem(BView *owner, BRect frame,
									bool complete = false);
									
private:
};

void DumpDeviceConfiguration(struct device_configuration *config);

#endif
