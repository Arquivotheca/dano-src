// EmailAddressContent.cpp -- E-mail address auto-completion control
// by Allen Brunson  January 25, 2001

#include <stdio.h>
#include <ctype.h>
#include <AutoCompleteControl.h>
#include <Autolock.h>
#include <ObjectList.h>
#include <ContentView.h>
#include <util.h>            // From headers/www (I hope)
#include <Window.h>
#include "EmailAddressContent.h"

// #define DEBUG_OUTPUT   // un-comment for printf() output

static const char*   kContentSignature = "application/x-vnd.Be.EmailAddress";
static const uint32  kChangedMessage = 'cHaN';

class EmailAddressContentFactory: public ContentFactory
  {
    public:
    
    void GetIdentifiers(BMessage* into);
	
    Content* CreateContent(void* handle, const char* mime,
     const char* extension);
  };                                        // End EmailAddressContentFactory
  
extern "C" _EXPORT ContentFactory* make_nth_content(int32 n,
 image_id you, uint32 flags, ...)
  {
    (void)you;
    (void)flags;
    
    if (n == 0) return new EmailAddressContentFactory;
    return 0;
  }

EmailAddressContent::EmailAddressContent(void* handle,
 const char* mime): Content(handle), fMimeType(mime)
  {

  }

EmailAddressContent::~EmailAddressContent(void)
  {

  }

size_t EmailAddressContent::GetMemoryUsage(void)
  {
    return 0;
  }

ssize_t EmailAddressContent::Feed(const void* buffer, ssize_t count,
 bool done)
  {
    (void)buffer;
    (void)count;
    (void)done;
    
    return 0;
  }

status_t EmailAddressContent::
 CreateInstance(ContentInstance** outInstance,
 GHandler* handler, const BMessage& attributes)
  {
    *outInstance = new EmailAddressContentInstance(this, handler,
     fMimeType, attributes);
     
    return B_OK;
  }

const int kPropertyHasContents = 0;                // These are used as
const int kPropertyValueInput  = 1;                //  indexes into the
const int kPropertyValueMail   = 2;                //  array list below

EmailAddressContentInstance::PROPERTY
 EmailAddressContentInstance::propertyList[] =
  {
    {"hasContents",                                // 0: Anything in there?
     &EmailAddressContentInstance::pfr_hasContents,
     NULL},
     
    {"valueInput",                                 // 1: Simple value
     &EmailAddressContentInstance::pfr_valueInput,
     &EmailAddressContentInstance::pfw_valueInput},
     
    {"valueMail",                                  // 2: Translated value
     &EmailAddressContentInstance::pfr_valueMail,
     NULL},
    
    {NULL, NULL, NULL}                             // End of the list
  };
  
Content* EmailAddressContentFactory::
 CreateContent(void* handle, const char* mime,
 const char* extension)
  {
    (void)extension;
    
    return new EmailAddressContent(handle, mime);
  }

void EmailAddressContentFactory::GetIdentifiers(BMessage* into)
  {
    static char desc[] = "E-mail Address Auto-Complete Control";
    
    into->AddString(S_CONTENT_MIME_TYPES, kContentSignature);
	into->AddString(S_CONTENT_PLUGIN_IDS, desc);
	into->AddString(S_CONTENT_PLUGIN_DESCRIPTION, desc);
  }
	
EmailAddressContentInstance::EmailAddressContentInstance(
 EmailAddressContent* parent, GHandler* h, const BString& mime,
 const BMessage& attributes): ContentInstance(parent, h) 
  { 
    BFont        font = be_bold_font;
    BRect        frame;
    status_t     rval;
    const char*  str;
    
    (void)mime;
    
    #ifdef DEBUG_OUTPUT
    printf("EmailAddressContentInstance constructor\n");
    #endif
    
    addressRoot = BinderNode::Root()
     ["user"] ["~"] ["addressbook"];
     
    backgroundColor = B_TRANSPARENT_COLOR;
    
    completeEmail    = true;
    completeFullname = true;
    
    hasContents      = false;
    
    addrList = new AddressList();
    choiceList = new BStringChoiceList();
    
    frame.left   =   0;
    frame.top    =   0;
    frame.right  = 400;
    frame.bottom =  35;
    
    rval = attributes.FindString("value", &str);
    
    if (rval != B_OK || !str || !strcasecmp(str, "undefined") ||
     !strcasecmp(str, "0"))
      str = NULL;
    
    #ifdef DEBUG_OUTPUT
    printf("Loading text: %s\n", str);
    #endif
    
    if (str && str[0]) hasContents = true;
    
    input = new BAutoCompleteControl(frame, "email_auto_complete",
     NULL, str, NULL, choiceList, false,
     B_FOLLOW_ALL);
     
    input->SetDivider(-3);
    input->SetSeparator(",");
    input->TextView()->DisallowChar(B_INSERT);
    input->SetChangedHandler(this, kChangedMessage);
    
    
    rval = attributes.FindString("font", &str);
    
    if (rval == B_OK && str)
      {
        rval = decode_font(str, &font);
        
        if (rval == B_OK)
          {
            #ifdef DEBUG_OUTPUT
            printf("Setting control font %s\n", str);
            #endif
    
            input->SetFont(&font);
            input->TextView()->SetFontAndColor(&font);
          }  
      }
      
    rval = attributes.FindString("mode", &str);
    
    if (rval == B_OK && str)
      input->SetAutoCompletionMode(str);
      
    rval = attributes.FindString("backgroundcolor", &str);
    
    if (rval == B_OK && str)
      {
        backgroundColor = decode_color(str);
        
        #ifdef DEBUG_OUTPUT
        printf("Found background color: %u %u %u\n",
         backgroundColor.red, backgroundColor.green,
         backgroundColor.blue);
        #endif
      }  
    
    rval = attributes.FindString("separator", &str);
    
    if (rval == B_OK && str)
      input->SetSeparator(str);
      
    rval = attributes.FindString("completeagainst", &str);
    
    if (rval == B_OK && str)
      CompleteAgainst(str);
    
    ReadAddresses();
  }

EmailAddressContentInstance::~EmailAddressContentInstance(void)
  {
    ADDRESS*  addr;
    
    #ifdef DEBUG_OUTPUT
    printf("EmailAddressContentInstance destructor\n");
    #endif
    
    if (addrList)
      {
        while (true)
          {
            addr = addrList->RemoveItemAt(0);
            if (!addr) break;
            delete addr;
          }
          
        delete addrList;
        addrList = NULL;
      }
      
    delete input;
    input = NULL;
    
    delete choiceList;
    choiceList = NULL;
  }

// This procedure finds an entry from the address book with any of
// the possible strings as input: e-mail addy, full name, etc.

EmailAddressContentInstance::ADDRESS*              // Begin AddressFind()
 EmailAddressContentInstance::AddressFind(const 
 char* str)
  {
    ADDRESS*  addr;                                // Item from the list
    int32     i;                                   // Loop counter
    int32     total;                               // Total items
    
    if (!addrList) return NULL;                    // If no list
    total = addrList->CountItems();                // Get total count
    
    for (i = 0; i < total; i++)                    // Loop the list
      {
        addr = addrList->ItemAt(i);                // Get next item
        if (!addr) {ASSERT(false); continue;}      // It should work
        
        if (!addr->fullname.ICompare(str))         // If it matches full name
          return addr;                             // There you go
          
        if (!addr->email.ICompare(str))            // If it matches e-mail
          return addr;                             // You got it    
      }
  
    return NULL;                                   // Didn't find it
  }                                                // End AddressFind()

// Formats an e-mail address entry all nice and pretty in
// the form "Firstname Lastname" <addr@domain.com>
//
// NOTE: An address entry might be used as a "group list," an
// ugly hack really but necessary.  In that case it would have a bunch
// of e-mail addresses in the e-mail field, not just one.  The thinking
// in that case is that the addresses should be in the form
//
// addr1@this.com, addr2@that.com
//
// with no first name or last name.

void EmailAddressContentInstance::AddressFormat(   // Begin AddressFormat()
 const ADDRESS* addr, BString* out)
  {
    char         ch;                               // Next char from string
    BString      email;                            // Email stuff
    uint32       i;                                // Loop counter
    bool         single = true;                    // Single/mult e-mails?
    const char*  str;                              // String pointer
    
    out->SetTo(NULL);                              // Clear existing stuff
    
    str = addr->email.String();                    // Pointer to bytes
    if (!str || !str[0]) goto end;                 // Exit if nothing there
    
    for (i = 0; str[i]; i++)                       // Loop the bytes
      {
        ch = str[i];                               // Get next char
        
        if (ch == ';' || ch == ',' ||              // If separator
         ch == ' ' || ch == '\t')                  // Or whitespace
           {
             single = false;                       // Multiple addresses
             break;                                // Stop the loop
           }
      }
    
    end:                                           // Jump here to finish
    
    if (single)                                    // If single address
      {
        out->Append('"', 1);                       // Open quote
        out->Append(addr->fullname);               // First and last name
        out->Append("\" <");                       // Close quote, open <
        out->Append(addr->email);                  // E-mail address
        out->Append(">");                          // Close >
      }
    else                                           // Multiple addresses
      {
        out->Append(addr->email);                  // Add them as is
      }  
  }                                                // End AddressFormat()

status_t EmailAddressContentInstance::AttachedToView(
 BView* inView, uint32* contentFlags)
  {
    BRect     rect   = FrameInParent();
    BWindow*  window = inView->Window();
    
    (void)contentFlags;
    
    if (!input) return B_OK;
    window->Lock();
    
    inView->AddChild(input);
    input->MoveTo(rect.LeftTop());
    input->ResizeTo(rect.Width(), rect.Height());
    input->SetViewColor(backgroundColor);
 
    input->Update();
    
    window->Unlock();
    return B_OK;
  }

void EmailAddressContentInstance::Cleanup(void)
{
	BinderNode::Cleanup();
	ContentInstance::Cleanup();
}

status_t EmailAddressContentInstance::CloseProperties(void* cookie)
{
	int32*  index = *(int32**)cookie;
	
	delete index;
	index = NULL;
	
	return B_OK;
}

// In the constructor we look for a property string in the form
//
// completeagainst = "fullname email"
//
// to set bools accordingly.  This is the procedure that sets those
// bools ...

void EmailAddressContentInstance::CompleteAgainst( // Begin CompleteAgainst
 const char* str)
  {
    uint32   i;                                    // Loop counter
    BString  orig;                                 // Original string
    char     word[55];                             // Next word
    
    completeEmail    = false;
    completeFullname = false;
    
    orig.SetTo(str);                               // Set contents
    
    for (i = 0; ; i++)                             // Loop to get all words
      {
        if (!strWord(&orig, i, word, sizeof(word)))// Get next word
          break;                                   // Stop if not found
          
        if (!strcasecmp(word, "fullname"))
          completeFullname = true;
        else if (!strcasecmp(word, "email"))
          completeEmail = true;
      }
  }                                                // End CompleteAgainst

status_t EmailAddressContentInstance::DetachedFromView(void)
  {
    if (!input) return B_OK;
    
    input->RemoveSelf();
    delete input;
    input = NULL;
	
    return B_OK;
  }

status_t EmailAddressContentInstance::
 FrameChanged(BRect newFrame, int32 fullWidth,
 int32 fullHeight)
  {
    float     height, width;
    bool      locked = false;
    BPoint    newLoc;
    BView*    parent;
    BWindow*  window;
    
    // NOTE: You can't resize the BAutoCompleteControl
    // in here if it is not attached to a window.  If you
    // do the resize message(s) are not propagated to the
    // BTextView inside the control so the two will not
    // be in sync.
    
    window = input->Window();
    if (!window) goto end;
    locked = window->Lock();
      
    height = newFrame.Height();
    width  = newFrame.Width();
    
    if (height <= 0.00) height = 22;
    if (width <= 0.00) width = 300;
    
    input->ResizeTo(width, height);
	
    newLoc = newFrame.LeftTop();
    parent = input->Parent();
    
    if (parent != NULL)
      newLoc -= parent->Bounds().LeftTop();
					
    newLoc.y += 2;  // Fudge it down a bit
    
    input->MoveTo(newLoc);
    
    if (locked) input->Window()->Unlock();

    end:
    
    return ContentInstance::FrameChanged(newFrame,
     fullWidth, fullHeight);
  }

status_t EmailAddressContentInstance::GetSize(int32* x, int32* y,
 uint32* outResizeFlags)
{
	*x = 600; 	// Doesn't really matter
	*y = 350;	// Doesn't really matter	
	
	*outResizeFlags = STRETCH_VERTICAL | STRETCH_HORIZONTAL;
	return B_OK;
} 

status_t EmailAddressContentInstance::HandleMessage(BMessage* msg)
  {
    status_t  result = B_OK;
	
    switch (msg->what)
      {
        case kChangedMessage:
          UpdateHasContents();
          break;
          
        default:
		  result = BinderNode::HandleMessage(msg);
		  break;
      }
      
    return result;
  }

void EmailAddressContentInstance::MarkDirty(void)
  {
  }

status_t EmailAddressContentInstance::NextProperty(void* cookie,
 char* nameBuffer, int32* length)
  {
    int32        count;
    int32*       index = *(int32**)cookie;
    const char*  item;
    
    for (count = 0; propertyList[count].name; count++) ;
    if (*index >= count) return ENOENT;
	
    item = propertyList[*index].name;
	(*index)++;
	
	strncpy(nameBuffer, item, *length);
	*length = strlen(item);
	
    return B_OK;
  }

status_t EmailAddressContentInstance::OpenProperties(void** cookie,
 void* copyCookie)
  {
    int32* index = new int32;

    *index = 0;

    if (copyCookie) *index = *((int32*)copyCookie);
    *cookie = index;

    return B_OK;
  }

get_status_t EmailAddressContentInstance::         // Begin pfr_hasContents()
 pfr_hasContents(property& outProperty,
 const property_list& inArgs)
  {
    const char*  str;                              // Result string
    
    (void)inArgs;                                  // Not used
    
    if (!hasContents)
      str = "false";
    else
      str = "true";
      
    outProperty = property(str);                   // Give value to caller
        
    #ifdef DEBUG_OUTPUT
    printf("hasContents: %s\n", str);
    #endif
    
    return B_OK;                                   // That's all folks
  }                                                // End pfr_hasContents()

get_status_t EmailAddressContentInstance::         // Begin pfr_valueInput()
 pfr_valueInput(property& outProperty,
 const property_list& inArgs)
  {
    const char*  str = NULL;                       // Contents string
        
    (void)inArgs;                                  // Not used
    
    if (input) str = input->Text();                // Get control text
    outProperty = property(str);                   // Save it for caller
    
    #ifdef DEBUG_OUTPUT
    printf("valueInput: %s\n", str);
    #endif
    
    return B_OK;                                   // That's all folks
  }                                                // End pfr_valueInput()

get_status_t EmailAddressContentInstance::         // Begin pfr_valueMail()
 pfr_valueMail(property& outProperty,
 const property_list& inArgs)
  {
    BString  out;                                  // Output string
        
    (void)inArgs;                                  // Not used
    ValueFixup(&out);                              // Fix it up
        
    outProperty = property(out.String());          // Save it for caller
    
    #ifdef DEBUG_OUTPUT
    printf("valueMail: %s\n", out.String());
    #endif
    
    return B_OK;                                   // That's all folks
  }                                                // End pfr_valueMail()

put_status_t EmailAddressContentInstance::         // Begin pfw_valueInput()
 pfw_valueInput(const property& inProperty)
  {
    uint32       len;
    const char*  str = inProperty.String().String();
    
    if (!input) return B_OK;
    if (!input->Window()->Lock()) return B_OK;
    
    input->TextView()->SelectAll();
    input->TextView()->Clear();
    if (str) input->TextView()->Insert(str);
    
    #ifdef DEBUG_OUTPUT
    printf("EmailAddressContent loading: %s\n", str);
    #endif
    
    len = input->TextView()->TextLength();
    input->TextView()->Select(len, len);
    
    input->Window()->Unlock();
    return B_OK;                                   // That's all folks
  }                                                // End pfw_valueInput()

void EmailAddressContentInstance::                 // Begin ReadAddresses()
 ReadAddresses(void)
  {
    ADDRESS*     addr;                             // Address to add
    const char*  addrStr;                          // Address string
    BString      cookie;                           // Address cookie
    BString      propStr;                          // Property string
    const char*  str;                              // String from node
    BString      value;                            // Next item value
    
    if (!choiceList) return;                       // Must have this
    if (!addrList) return;                         // And this
    
    // Should clear addrList here ...
    choiceList->RemoveAll();                       // Clear the list
    
    BinderNode::iterator address =                 // Gee whiz I shure do
     addressRoot->Properties();                    //  LOOOVE the binder!
     
    while (true)                                   // Loop for address entries
      {
        cookie = address.Next();                   // Get next one
        addrStr = cookie.String();                 // Get string pointer
        if (!addrStr[0]) break;                    // Stop at the end
        
        addr = new ADDRESS();                      // Create address
        if (!addr) break;                          // Stop on failure
        
        addrList->AddItem(addr);                   // In you go
        
        BinderNode::property firstname =           // First name part
         addressRoot [addrStr] ["firstName"];
         
        value = firstname.String();
        str = value.String();
        
        addr->fullname.SetTo(str);                 // Start with first name
        addr->fullname.Append(" ");                // Add a space
        
        BinderNode::property lastname =            // Last name part
         addressRoot [addrStr] ["lastName"];
         
        value = lastname.String();
        str = value.String(); 
         
        addr->fullname.Append(str);                // That's the whole thing
        strTrim(&addr->fullname);                  // Trim whitespace
        
        if (completeFullname)                      // If using for completion
          choiceList->AddChoice(                   // Into the list
           addr->fullname.String());
        
        BinderNode::property email =               // E-mail part
         addressRoot [addrStr] ["email"];
         
        value = email.String();
        str = value.String();
        
        addr->email.SetTo(str);                    // Save e-mail string
        strTrim(&addr->email);                     // Trim whitespace
        
        if (completeEmail)                         // If completing this
          choiceList->AddChoice(str);              // Add the string
      }                                            // End address loop
  }                                                // End ReadAddresses()

get_status_t EmailAddressContentInstance::         // Begin ReadProperty()
 ReadProperty(const char* name,
 property& outProperty,
 const property_list& inArgs)
  {
    PROPERTYREADFUNC  func;                        // Function pointer
    uint32            i;                           // Loop counter
    
    for (i = 0; propertyList[i].name; i++)         // Loop property list
      if (!strcasecmp(name, propertyList[i].name)) // If it's a match
        {
          func = propertyList[i].readFunc;         // Save that pointer
          if (!func) break;                        // If no read function
          
          return (this->*func)(outProperty,inArgs);// Call it
        }  
    
    printf("EmailAddressContentInstance "          // Might help with
     "unrecognized read property: %s\n", name);    //  debugging
    
    return BinderNode::ReadProperty(name,          // Unrecognized name
     outProperty, inArgs);
  }                                                // End ReadProperty()


/****************************************************************************/
/*                                                                          */
/***  EmailAddressContentInstance::strTrim()                              ***/
/*                                                                          */
/****************************************************************************

This procedure removes leading and trailing whitespace from a BString.      */

void EmailAddressContentInstance::strTrim(BString* // Begin strTrim()
 str)
  {
    char*   buf;                                   // The string buffer
    int32   i, j;                                  // Loop counters
    int32   len;                                   // Total length
    
    
    //*
    //***  Do initial setup
    //*
    
    
    len = str->Length();                           // Get current length
    buf = str->LockBuffer(len + 1);                // We will change it
    
    
    //*
    //***  Remove leading whitespace
    //*
    
    for (j = 0; buf[j] && isspace(buf[j]); j++);   // Go to first non-space

    if (j)                                         // If leading whitespace
      {
        for (i = 0; buf[j]; i++, j++)              // Loop through string
          buf[i] = buf[j];                         // Move chars backward
          
        buf[i] = 0;                                // Terminate string
      }    

    
    //*
    //***  Remove trailing whitespace
    //*
    
    i = strlen(buf);                               // Get current length
    
    if (i >= 1)                                    // If any remaining
      {
        for (i--; i >= 0 && isspace(buf[i]); i--)  // While in whitespace
          buf[i] = 0;                              // Lop off space/tab
      }
    
    
    //*
    //***  Clean up and exit
    //*
    
    str->UnlockBuffer();                           // Finished  
  }                                                // End strTrim()
  

/****************************************************************************/
/*                                                                          */
/***  EmailAddressContentInstance::strWord()                              ***/
/*                                                                          */
/****************************************************************************

This procedure gets the Nth "word" out of a string, where a "word" is any
number of characters delimited with whitespace.  It returns true if it
found the word, false if not.                                               */

bool EmailAddressContentInstance::strWord(BString* // Begin strWord()
 str, uint32 num, char* word, uint32 wordSize)
  {
    uint32       n, s, w;                          // Loop counters
    const char*  src = str->String();              // String buffer
    
    for (s = 0, n = 0; ; n++)                      // Loop to find Nth word
      {
        word[0] = 0;  w = 0;                       // Clear current word
        while (src[s] && isspace(src[s])) s++;     // Past leading whites
        
        for ( ; src[s] && !isspace(src[s]); s++)   // Loop for word chars
          if (w < (wordSize - 1))                  // If this char fits
            word[w++] = src[s];                    // Put it in
            
        word[w] = 0;                               // Terminate word
        if (word[0] && n == num) return true;      // If word found
        if (!src[s]) {word[0] = 0; return false;}  // If string finished
      }
  }                                                // End strWord()
  

#ifdef STUPID_BUSINESS_PRACTICES

void EmailAddressContentInstance::                 // Begin testProperty()
 testProperty(void)
  { 
    void*     cookie = NULL;                       // State cookie
    int32     copyCookie = 0;                      // Cookie to copy from
    int32     i;                                   // Loop counter
    int32     len;                                 // Buffer length
    char      name[120];                           // Copied property name
    status_t  status;                              // Status return
    
    debugger("honk honk");                         // Hey there, sailor
    
    OpenProperties(&cookie, &copyCookie);          // Open property list
    
    for (i = 0; ; i++)                             // Loop for all properties
      {
        len = sizeof (name);                       // Set buffer size
        status = NextProperty(&cookie, name, &len);// Get next property
        if (status != B_OK) break;                 // Exit if done
        ASSERT(len == strlen(name));               // Should be set right
      }

    CloseProperties(&cookie);                      // Close properties
  }                                                // End testProperty()
  
#endif

void EmailAddressContentInstance::
 UpdateHasContents(void)
{
  bool         state;
  const char*  str;
  
  str = input->Text();
  
  if (str && str[0])
    state = true;
  else
    state = false;
    
  if (state == hasContents) return;
  
  hasContents = state;
    
  NotifyListeners(B_PROPERTY_CHANGED,
   propertyList[kPropertyHasContents].name);
   
  #ifdef DEBUG_OUTPUT
  printf("Property %s changed: %d\n", 
   propertyList[kPropertyHasContents].name, hasContents);
  #endif 
}

void EmailAddressContentInstance::ValueFixup(      // Begin ValueFixup()
 BString* out)
  {
    ADDRESS*     addr;                             // Address from the list
    BString      final;                            // Final value
    uint32       i;                                // Loop counter
    BString      phrase;                           // Next phrase
    const char*  ptr;                              // Pointer to separators
    char         sep[8];                           // Separator string
    
    out->SetTo(NULL);                              // In case of errors
    if (!input) return;                            // If no input control
    
    ptr = input->Separator();                      // Get separators
    
    sep[0] = ptr[0];                               // Set first separator
    sep[1] = 0;                                    // Terminate it
    
    if (!isspace(sep[0]))                          // If it's not a space
      {
        sep[1] = ' ';                              // Add a space after
        sep[2] = 0;                                // Terminate it
      }
    
    for (i = 0; ; i++)                             // Loop for phrases
      {
        if (!input->NextPhrase(i, &phrase)) break; // Get next phrase
        
        if (i >= 1) out->Append(sep);              // If second or beyond
        addr = AddressFind(phrase.String());       // Try to find it
        
        if (addr)                                  // If found
          {
            AddressFormat(addr, &final);           // Format it
            out->Append(final);                    // In you go
          }
        else                                       // No match found
          {
            out->Append(phrase);                   // Stick it in raw
          }  
      }
  }                                                // End ValueFixup()

put_status_t EmailAddressContentInstance::
 WriteProperty(const char* name, 
 const property& inProperty)
  {
    PROPERTYWRITEFUNC  func;                       // Function pointer
    uint32             i;                          // Loop counter
    
    for (i = 0; propertyList[i].name; i++)         // Loop property list
      if (!strcasecmp(name, propertyList[i].name)) // If it's a match
        {
          func = propertyList[i].writeFunc;        // Save that pointer
          if (!func) break;                        // If no write function
          
          return (this->*func)(inProperty);        // Call it
        }
        
    printf("EmailAddressContentInstance "          // Might help with
     "unrecognized write property: %s\n", name);   //  debugging
    
    return BinderNode::WriteProperty(name,         // Unrecognized name
     inProperty);
  }

