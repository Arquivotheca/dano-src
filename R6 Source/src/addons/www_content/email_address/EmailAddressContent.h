// EmailAddressContent.h -- E-mail address auto-completion control
// by Allen Brunson  January 25, 2001

#ifndef _EmailAddressContent_h
#define _EmailAddressContent_h

#include <AutoCompleteControl.h>
#include <Binder.h>
#include <ObjectList.h>
#include <Content.h>
#include <Locker.h>

using namespace Wagner;

class EmailAddressContent: public Content
  {
    public:
    
                     EmailAddressContent(void* handle, const char* mime);
    virtual          ~EmailAddressContent(void);

    virtual size_t   GetMemoryUsage(void);
    virtual ssize_t  Feed(const void* buffer, ssize_t bufferLen,
                      bool done = false);

    private:
    
    virtual status_t CreateInstance(ContentInstance** outInstance,
                      GHandler* handler, const BMessage& attributes);
                      
    BString  fMimeType;
  };

class EmailAddressContentInstance: public ContentInstance, public BinderNode
  {
    private:
    
    enum ASCII
      {
        TAB    = 0x09,
        SPACE  = 0x20,
      };

    typedef get_status_t
     (EmailAddressContentInstance::*PROPERTYREADFUNC)
     (property& outProperty,
     const property_list& inArgs);
     
    typedef put_status_t
     (EmailAddressContentInstance::*PROPERTYWRITEFUNC)
     (const property& inProperty);
     
    struct PROPERTY
      {
        const char*        name;
        PROPERTYREADFUNC   readFunc;
        PROPERTYWRITEFUNC  writeFunc;
      };
      
    struct ADDRESS
      {
        BString  fullname;
        BString  email;
      };
      
    class AddressList: public BObjectList<ADDRESS> {};
    
    static PROPERTY  propertyList[];  
       
    public:
    
                     EmailAddressContentInstance(EmailAddressContent* parent,
                      GHandler* h, const BString& mime,
                      const BMessage& attributes);
                     
    virtual          ~EmailAddressContentInstance(void);

    status_t         OpenProperties(void** cookie, void* copyCookie);
    status_t         NextProperty(void* cookie, char* nameBuffer,
                      int32* length);
    status_t         CloseProperties(void* cookie);

    get_status_t     ReadProperty(const char* name, property& outProperty,
                      const property_list& inArgs);
                      
    put_status_t     WriteProperty(const char* name,
                      const property& inProperty);

    virtual status_t AttachedToView(BView* view, uint32* contentFlags);
    virtual status_t DetachedFromView(void);
    virtual status_t GetSize(int32* x, int32* y, uint32* outResizeFlags);
    virtual status_t FrameChanged(BRect newFrame, int32 fullWidth,
                      int32 fullHeight);
    virtual status_t HandleMessage(BMessage* msg);
    virtual void     Cleanup(void);
    void             MarkDirty(void);
    
    // Property functions: for internal use only
    
    private:

    get_status_t     pfr_hasContents(property& outProperty,
                      const property_list& inArgs);
                      
    get_status_t     pfr_valueInput(property& outProperty,
                      const property_list& inArgs);
                      
    get_status_t     pfr_valueMail(property& outProperty,
                      const property_list& inArgs);
                      
    put_status_t     pfw_valueInput(const property& inProperty);                  
                      
    private:
		
    AddressList*                addrList;
    BinderNode::property        addressRoot;
    rgb_color                   backgroundColor;
    bool                        hasContents;
    BAutoCompleteControl*       input;
    BStringChoiceList*          choiceList;
    
    bool                        completeEmail;
    bool                        completeFullname;
    
    ADDRESS*         AddressFind(const char* str);
    void             AddressFormat(const ADDRESS* addr,
                      BString* out);
    
    void             CompleteAgainst(const char* str);
    void             ReadAddresses(void);
    static void      strTrim(BString* str);
    static bool      strWord(BString* str, uint32 num,
                      char* word, uint32 wordSize);
    void             testProperty(void);
    void             UpdateHasContents(void);
    void             ValueFixup(BString* out);
  };                                       // End EmailAddressContentInstance
  
#endif                                          // End _EmailAddressContent_h

