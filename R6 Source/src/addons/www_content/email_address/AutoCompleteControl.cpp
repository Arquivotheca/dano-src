// AutoCompleteControl.cpp -- BTextControl with auto-completion
// by Allen Brunson, based on TextListControl by Nathan Schrenk

#include <ctype.h>                 // Character classification
#include <AutoCompleteControl.h>   // My companion .h file
#include <Beep.h>                  // Temporary, for debugging only
#include <GraphicsDefs.h>          // rgb_color comparison operators
#include <ObjectList.h>            // from inc/support_p
#include <TextControl.h>           // Standard text control
#include <TextControlPrivate.h>    // _BTextInput_ hidden class
#include <Window.h>                // Main window class
#include <GHandler.h>

// #define DEBUG_OUTPUT


/****************************************************************************/
/*                                                                          */
/***  ASCII defines                                                       ***/
/*                                                                          */
/****************************************************************************/

enum ASCII
  {
    TAB   = 0x09,
    SPACE = 0x20,
  };
  

/****************************************************************************/
/*                                                                          */
/***  Message defines                                                     ***/
/*                                                                          */
/****************************************************************************/

const uint32  kAutoTextAcceptComplete = 'TAAC';    // User likes it
const uint32  kAutoTextAdded          = 'TADD';    // Text was added
const uint32  kAutoTextDeleted        = 'TDEL';    // Text was deleted
const uint32  kAutoTextRejectComplete = 'TRAC';    // User rejects complete
const uint32  kAutoTextTab            = 'TTAB';    // User pressed tab


/****************************************************************************/
/*                                                                          */
/***  Testing data                                                        ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING                            // If testing

PHRASE phraseListEmpty[] =
  {
    {-1, -1}
  };
  
PHRASE phraseList04[] =
  {
    {0, 4},
    {-1, -1}
  };

PHRASE phraseList05[] =
  {
    {0, 0},
    {2, 2},
    {-1, -1}
  };

PHRASE phraseList06[] =
  {
    {7, 9},
    {-1, -1}
  };

PHRASE phraseList08[] =
  {
    {2, 4},
    {-1, -1}
  };

PHRASE phraseList09[] =
  {
    {0, 1},
    {6, 12},
    {-1, -1}
  };

PHRASE phraseList10[] =
  {
    {0, 1},
    {4, 4},
    {6, 7},
    {10, 12},
    {-1, -1}
  };

PHRASE phraseList11[] =
  {
    {0, 1},
    {5, 6},
    {10, 12},
    {-1, -1}
  };

PHRASEDATA  phraseData[] =                         // Master phrase list
  {
    {"",                  " ",   phraseListEmpty}, // Test 00
    {" ",                 " ",   phraseListEmpty}, // Test 01
    {" ",                 ";",   phraseListEmpty}, // Test 02
    {";",                 ";",   phraseListEmpty}, // Test 03
    {"Apple ",            ";",   phraseList04},    // Test 04
    {"a;a",               ";",   phraseList05},    // Test 05
    {"  ; ;  abm",        ";",   phraseList06},    // Test 06
    {"  ; ;  abm;",       ";",   phraseList06},    // Test 07
    {"  arp",             ";",   phraseList08},    // Test 08
    {"ab  ; am  arg",     ";",   phraseList09},    // Test 09
    {"ab  ; am  arg",     " ",   phraseList10},    // Test 10
    {"ab , am ; arg",     ";,",  phraseList11},    // Test 11
  
    {NULL, 0, NULL}                                // End of the list
  };

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput                                               ***/
/*                                                                          */
/****************************************************************************/

class AutoCompleteTextInput: public _BTextInput_   // AutoCompleteTextInput
  {
    public:
    
                     AutoCompleteTextInput(BRect   // Constructor
                      rect, BRect trect,
                      ulong rMask, ulong flags);
						 
    virtual          ~AutoCompleteTextInput(void); // Destructor
	
    virtual void     KeyDown(const char* bytes,    // Key pressed
                      int32 numBytes);
                      
	virtual void     MakeFocus(bool state);        // Change in focus

    protected:

    virtual void     InsertText(const char* inText,// Text added 
                      int32 inLength,
                      int32 inOffset,
                      const text_run_array* inRuns);
								   
    virtual void     DeleteText(int32 fromOffset,  // Text removed
                      int32 toOffset);
                      
    private:
    
    int32            tabCount;                     // Consecutive tabs
    
    void             SendMessage(uint32 msg);      // Send message to parent
  };                                               // AutoCompleteTextInput


/****************************************************************************/
/*                                                                          */
/***  KeyedString                                                         ***/
/*                                                                          */
/****************************************************************************/

class KeyedString: public BString                  // Begin KeyedString
  {
    public:
    
                     KeyedString(int32 key,        // Minimal constructor
                      const char* string):
                      BString(string), key(key) {}

    virtual          ~KeyedString(void) {}         // Minimal destructor
	
    int32            key;                          // String's unique key
  };                                               // End KeyedString


/****************************************************************************/
/*                                                                          */
/***  KeyedStringObjectList                                               ***/
/*                                                                          */
/****************************************************************************/

class KeyedStringObjectList:                       // KeyedStringObjectList
 public BObjectList<KeyedString>
  {
    // Nothing to define
  };                                               // KeyedStringObjectList


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput::AutoCompleteTextInput()                      ***/
/*                                                                          */
/****************************************************************************

The control constructor.                                                    */

AutoCompleteTextInput::AutoCompleteTextInput(BRect // AutoCompleteTextInput()
 rect, BRect trect, ulong rMask, ulong flags):
 _BTextInput_(rect, trect, rMask, flags)
  {
    #ifdef DEBUG_OUTPUT
    printf("AutoCompleteTextInput constructor\n");
    #endif
    
    tabCount = 0;
  }                                                // AutoCompleteTextInput()


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput::~AutoCompleteTextInput()                     ***/
/*                                                                          */
/****************************************************************************

The control destructor.                                                     */

AutoCompleteTextInput::~AutoCompleteTextInput(void)// ~AutoCompleteTextInput()
  {
    // Nothing to do
  }                                                // ~AutoCompleteTextInput()


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput::DeleteText()                                 ***/
/*                                                                          */
/****************************************************************************

An override that gets called whenever the user deletes any text.            */

void AutoCompleteTextInput::DeleteText(int32       // Begin DeleteText()
 fromOffset, int32 toOffset)
  {
    #ifdef DEBUG_OUTPUT
    printf("AutoCompleteTextInput::DeleteText()\n");
    #endif
    
    _BTextInput_::DeleteText(fromOffset, toOffset);// Call base version
    SendMessage(kAutoTextDeleted);                 // Send message to parent
  }                                                // End DeleteText()


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput::InsertText()                                 ***/
/*                                                                          */
/****************************************************************************

An override that gets called whenever the user inserts any text.            */

void AutoCompleteTextInput::InsertText(const char* // Begin InsertText()
 inText, int32 inLength, int32 inOffset,
 const text_run_array* inRuns)
  {
    #ifdef DEBUG_OUTPUT
    printf("AutoCompleteTextInput::InsertText()\n");
    #endif
    
    _BTextInput_::InsertText(inText, inLength,     // Call base version
     inOffset, inRuns);
     
    SendMessage(kAutoTextAdded);                   // Send message to parent
  }                                                // End InsertText()


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput::KeyDown()                                    ***/
/*                                                                          */
/****************************************************************************

An override that gets called when the user presses keys.                    */

void AutoCompleteTextInput::KeyDown(const char*    // Begin KeyDown()
 bytes, int32 numBytes)
  {
    uint32                 msg = 0;                // Send a message?
    BAutoCompleteControl*  parent;                 // Parent control pointer
    bool                   tailAdded;
    
    parent = dynamic_cast<BAutoCompleteControl*>   // Get parent pointer
     (Parent());
     
    if (!parent) goto doDefault;                   // If it didn't work
    
    if (!parent->IsEnabled()) goto doDefault;      // If it's disabled
    tailAdded = parent->TailAdded();               // Get tail state
	if (bytes[0] != B_TAB) tabCount = 0;           // Reset at first non-tab
	
	switch (bytes[0])                              // Decision on keypress
	  {
        case B_LEFT_ARROW:                         // Left arrow
          if (tailAdded)                           // If tail
            msg = kAutoTextRejectComplete;         // Get rid of it
          else                                     // No tail
            goto doDefault;                        // Handle as usual
          break;
          
        case B_RIGHT_ARROW:                        // Right arrow
          if (tailAdded)                           // If tail
            msg = kAutoTextAcceptComplete;         // Keep it
          else                                     // No tail  
            goto doDefault;                        // Handle as usual
          break;
        
        case B_RETURN:                             // Enter key
          ASSERT(fInitialText);                    // Better have this
			
          if (tailAdded)                           // If we've got tail
            {
              msg = kAutoTextAcceptComplete;
            }
          else if (strcmp(fInitialText, Text()))   // If text has changed
            {
              free(fInitialText);                  // Remove existing text
              fInitialText = strdup(Text());       // Replace with current
              
              parent->Invoke();                    // Invoke
              SelectAll();                         // Select all text
            }  
          break;
			
        case B_TAB:                                // Tab key
          tabCount++;                              // There's another tab
          if (tailAdded)                           // If tail added
            {
              msg = kAutoTextAcceptComplete;       // Send this message
            }  
          else if (parent->AutoCompletionMode() == // If passive completion
           BAutoCompleteControl::CompletionPassive)
            {
              if (tabCount >= 3)                   // Lots o' tabs
                {
                  tabCount = 0;                    // Clear count
                  goto doDefault;                  // Handle as usual
                }
              else
                {  
                  msg = kAutoTextTab;              // Tell my daddy
                }  
            }  
          else                                     // Otherwise
            {
              goto doDefault;                      // Handle as usual
            }  
          break;
          
        default:                                   // Any other key
        doDefault:                                 // Goto label
          _BTextInput_::KeyDown(bytes, numBytes);  // Pass it up the line
          break;
      }                                            // End key switch
      
    if (msg) SendMessage(msg);                     // Send msg if needed
  }                                                // End KeyDown()


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput::MakeFocus()                                  ***/
/*                                                                          */
/****************************************************************************

I need to capture this to do auto-completion when the user exits the
control.                                                                    */

void AutoCompleteTextInput::MakeFocus(bool state)  // Begin MakeFocus()
  {
    BAutoCompleteControl*  parent;                 // My daddy
    
    _BTextInput_::MakeFocus(state);                // Call base version
    if (state) return;                             // If activating
    
    parent = dynamic_cast<BAutoCompleteControl*>   // Get parent control
     (Parent());
     
    if (!parent) return;                           // If it failed
    if (!parent->TailAdded()) return;              // No auto-complete pending
    
    SendMessage(kAutoTextAcceptComplete);          // User sez "keep it!"
  }                                                // End MakeFocus()


/****************************************************************************/
/*                                                                          */
/***  AutoCompleteTextInput::SendMessage()                                ***/
/*                                                                          */
/****************************************************************************

This procedure sends a message up to the parent control.                    */

void AutoCompleteTextInput::SendMessage(uint32 msg)// Begin SendMessage()
  {
    BAutoCompleteControl*  parent;                 // My daddy
    BWindow*               window;                 // My window
    
    window = Window();                             // Get window pointer
    
    if (!window)                                   // If not there
      {
        #ifdef DEBUG_OUTPUT
        printf("Couldn't get window pointer!\n");
        #endif
        
        return;
      }
    
    parent = dynamic_cast<BAutoCompleteControl*>   // Get parent control
     (Parent());
     
    if (!parent)                                   // If it failed
      {
        #ifdef DEBUG_OUTPUT
        printf("Couldn't get parent control!\n");
        #endif
        
        return;
      }
    
    #ifdef DEBUG_OUTPUT
    printf("Sending message: %0lX\n", msg);        // For debugging
    #endif
    
    window->PostMessage(msg, parent);              // Send it
  }                                                // End SendMessage()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::BAutoCompleteControl()                        ***/
/*                                                                          */
/****************************************************************************

The control constructor.  NOTE: Isn't it wrong for this thing to become
B_NAVIGABLE?  I thought that was supposed to happen to the _BTextInput_
inside of us.                                                               */

BAutoCompleteControl::BAutoCompleteControl(BRect   // BAutoCompleteControl()
 frame, const char* name, const char* label,
 const char* initial_text, BMessage* message,
 BChoiceList* choiceList, bool ownChoiceList,
 uint32 resizeMask, uint32 flags):
 BTextControl(frame, name, label, initial_text,
 message, new AutoCompleteTextInput(BRect(0, 0,
 10, 10), BRect(0, 0, 10, 10), B_FOLLOW_ALL_SIDES,
 B_WILL_DRAW | B_FRAME_EVENTS), resizeMask, flags)
  {
    if ((flags & B_NAVIGABLE) != 0)                // Isn't this wrong?
      BView::SetFlags(Flags() | B_NAVIGABLE);
	
    fChoiceList = choiceList;                      // Save choice list
    fOwnsChoiceList = ownChoiceList;               // Save ownership bool
	
	InitData();                                    // Set up class data
	
	#if TEST_SCAFFOLDING                           // If testing
	testCompareStart();
	testPhraseStr();
	#endif
  }                                                // BAutoCompleteControl()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::~BAutoCompleteControl()                       ***/
/*                                                                          */
/****************************************************************************/

BAutoCompleteControl::~BAutoCompleteControl(void)  // ~BAutoCompleteControl()
  {
    if (fOwnsChoiceList)                           // If we own it
      {
        delete fChoiceList;                        // Delete it
        fChoiceList = NULL;                        // Invalidate the pointer
      }
  }                                                // ~BAutoCompleteControl()
  

/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::AutoCompletionMode()                          ***/
/*                                                                          */
/****************************************************************************/

BAutoCompleteControl::CompletionType               // Beg AutoCompletionMode()
 BAutoCompleteControl::AutoCompletionMode(void)
 const
  {
    return fCompletionMode;                        // Return current type
  }                                                // End AutoCompletionMode()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::ChangedHandler()                              ***/
/*                                                                          */
/****************************************************************************

If a GHandler has been registered to be notified of all control changes
then this procedure will call it.                                           */

void BAutoCompleteControl::ChangedHandler(void)    // Begin ChangedHandler()
  {
    if (!fChangedHandler) return;                  // If no handler registered
    fChangedHandler->PostMessage(BMessage(fChangedMessage)); // Do it
  }                                                // End ChangedHandler()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::ChoiceListChanged()                           ***/
/*                                                                          */
/****************************************************************************

This procedure is supposed to be called whenever our choice list changes
so we can update class state appropriately.                                 */

void BAutoCompleteControl::ChoiceListChanged(void) // Beg ChoiceListChanged()
  {
    SetConfirmed();
  }                                                // End ChoiceListChanged()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::CompareStart()                                ***/
/*                                                                          */
/****************************************************************************

This procedure returns the offset into a string where you should start
comparing for the purposes of doing an auto-completion.  If you shouldn't
do auto-completion at all on this string it will return a negative number.  */

int32 BAutoCompleteControl::CompareStart(const     // Begin CompareStart()
 char* str, const BString* separator)
  {
    int32  i, iStart, iStop;                       // Loop counters
    int32  len;                                    // String length
    bool   rval;                                   // Return value
    int32  start = -1, stop = -1;                  // Offset values
    
    if (!str || !str[0]) return -1;                // If no string
    len = strlen(str);                             // Get its length
    
    for (i = 0; ; i++)                             // Loop to get last phrase
      {
        rval = PhraseStr(str, separator, i,        // Get next phrase
         &iStart, &iStop);
         
        if (!rval) break;                          // If not found 
        
        start = iStart;                            // Save it
        stop  = iStop;                             // Ditto
      }
      
    if (start < 0 || stop < 0) return -1;          // If no phrases
    
    for (i = stop + 1; i < len; i++)               // Loop for tail chars
      if (SeparatorChar(separator, str[i]))        // If separator found
        return -1;                                 // Don't do it
    
    return start;                                  // That's your phrase
  }                                                // End CompareStart()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::ConfirmColor()                                ***/
/*                                                                          */
/****************************************************************************

This procedure gets the color for confirmed auto-completed strings.         */

rgb_color BAutoCompleteControl::ConfirmColor(void) // ConfirmColor()
 const
  {
    return fConfirmColor;                          // There you go
  }                                                // ConfirmColor()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::CurrentSelection()                            ***/
/*                                                                          */
/****************************************************************************

This procedure returns the index of the last selection we auto-completed
with.                                                                       */

int32 BAutoCompleteControl::CurrentSelection(void) // Begin CurrentSelection()
 const
  {
    return fSelected;                              // Dicey
  }                                                // End CurrentSelection()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::InitData()                                    ***/
/*                                                                          */
/****************************************************************************

This is the procedure used by constructors to set up our class data.        */

void BAutoCompleteControl::InitData(void)          // Begin InitData()
  {
    BFont  font;                                   // Temp font
    
    fConfirmColor.red = 0;                         // Set confirm color
    fConfirmColor.green = 0;
    fConfirmColor.blue = 255;
    fConfirmColor.alpha = 255;
    
    TextView()->GetFontAndColor(0, &font,          // Get normal color
     &fNormalColor);
    
    TextView()->SetStylable(true);                 // Allow highlighting
    
    fChangedHandler = NULL;                        // Clear handler
    fChangedMessage = 0;                           // Clear message to send
    fCompletionMode = CompletionDefault;           // Set default mode
    fMatchIndex = -1;                              // None matched yet
    fIgnoreNextDelete = false;
    fIgnoreNextInsert = false;
    fSelected = -1;                                // Nothing yet selected
    fSeparator.SetTo(NULL);                        // No separator chars
    fTailAdded = false;                            // No tail yet
    fUnique = false;
  }                                                // End InitData()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::InsertSeparator()                             ***/
/*                                                                          */
/****************************************************************************

If necessary this procedure will insert a separator character between the
previous auto-completed thing and the next one.                             */

void BAutoCompleteControl::InsertSeparator(void)   // Begin InsertSeparator()
  {
    char             buf[4];                       // Insertion buffer
    char             ch;                           // Character entered
    int32            confirmed = 0;                // Last confirmed offset
    int32            i;                            // Loop counter
    int32            len;                          // Text length
    text_run_array*  run = NULL;                   // Text runs
    int32            selStart, selStop;            // Selection indexes
    char             sep;                          // First separator char
    BTextView*       text;                         // Control pointer
    const char*      textPtr;                      // Text pointer
    
    sep = fSeparator.ByteAt(0);                    // Get first char
    if (!sep || isspace(sep)) goto end;            // If no real separator
    
    text = TextView();                             // Get view pointer
    if (!text) goto end;                           // If it didn't work
    
    text->GetSelection(&selStart, &selStop);       // Get selection
    if (selStart != selStop) goto end;             // If there's a selection
    
    len = text->TextLength();                      // Get current length
    if (selStart < len) goto end;                  // If not at the end
    
    textPtr = text->Text();                        // Get text pointer
    ch = textPtr[len - 1];                         // Get last character
    
    if (ch == 0) goto end;                         // Ignore if null
    if (isspace(ch)) goto end;                     // Ignore if whitespace
    if (ch == sep) goto end;                       // Ignore if separator
    
    run = text->RunArray(0, len);                  // Get all runs
    if (!run) goto end;                            // If it didn't work
    
    for (i = 0; i < run->count; i++)               // Loop the list
      {
        if (run->runs[i].color != fConfirmColor)   // If not confirmed
          continue;                                // Ignore it
          
        if ((i + 1) < run->count)                  // If one past this
          confirmed = run->runs[i+1].offset - 1;   // Go to that one
        else                                       // None after this
          confirmed = len - 1;                     // Use max length
      }
      
    if (confirmed <= 0) goto end;                  // If none confirmed
    if (confirmed >= (len - 1)) goto end;          // If off the end
    
    for (i = confirmed + 1; i < len; i++)          // Loop to find char
      if (SeparatorChar(&fSeparator, textPtr[i]))  // If separator found
        goto end;                                  // Quit now
      
    buf[0] = sep;                                  // Separator buffer
    buf[1] = 0;
    
    text->Select(confirmed + 1, confirmed + 1);    // Force caret position
    text->Insert(buf);                             // Stick it in
    
    len = text->TextLength();                      // Get new length
    text->Select(len, len);                        // Force caret to the end
    
    end:                                           // Jump here to exit
    free(run);                                     // Done with it
    run = NULL;                                    // Invalidate pointer
  }                                                // End InsertSeparator()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::KeepTail()                                    ***/
/*                                                                          */
/****************************************************************************

Okay.  We've added text onto what the user typed, and it is highlighted.
We know we added it because fTailAdded is set.  The user has made some
gesture indicating they want to keep it so now it is time to make it
permanent.                                                                  */

void BAutoCompleteControl::KeepTail(void)          // Begin KeepTail()
  {
    int32           len;                           // String length
    int32           offset;                        // Where to start replacing
    text_run_array  run;                           // For highlighting
    const char*     str;                           // Matched string
    BTextView*      text;                          // Text control
    const char*     textPtr;                       // Text pointer
    
    if (!fChoiceList) return;                      // If no choice list
    if (!fTailAdded) return;                       // If no tail
    fTailAdded = false;                            // Tail gone soon
    
    str = fChoiceList->GetChoiceAt(fMatchIndex);   // Get match string
    if (!str) return;                              // If not found
    
    text = TextView();                             // Get control pointer
    if (!text) return;                             // Exit if not there
    textPtr = text->Text();                        // Get text pointer
    
    len = text->TextLength();                      // Get text length
    offset = CompareStart(textPtr, &fSeparator);   // Get start point
    if (offset < 0) return;                        // If nothing to check
    
    text->Select(offset, len);                     // Stuff to be replaced
    text->Clear();                                 // Remove it
    text->Insert(str);                             // Add the new stuff
    
    len = text->TextLength();                      // Get new length
    text->Select(len, len);                        // Force caret to the end
    
    fSelected = fMatchIndex;                       // Copy that index
    fMatchIndex = -1;                              // Then throw it away
    
    fIgnoreNextDelete = true;
    fIgnoreNextInsert = true;
  }                                                // End KeepTail()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::MessageReceived()                             ***/
/*                                                                          */
/****************************************************************************

This thing is a fucking mess.                                               */

void BAutoCompleteControl::MessageReceived(        // Begin MessageReceived()
 BMessage* msg)
  {
    #ifdef DEBUG_OUTPUT
    bool  handled = true;
    #endif
    
    switch (msg->what)                             // Decision on message type
      {
        case kAutoTextAcceptComplete:              // User keeps the tail
          KeepTail();                              // Handle it
          SetConfirmed();
          break;
          
        case kAutoTextAdded:                       // Text was added
          if (fIgnoreNextInsert)
            {
              fIgnoreNextInsert = false;
              break;
            }
          RemoveTail();                            // Kill tail if any
          InsertSeparator();                       // Possibly insert sep
		  if (fCompletionMode == CompletionActive) // If aggressive completion
            TryAutoComplete();                     // Do it now
          SetConfirmed();                          // Update confirm color
          ChangedHandler();                        // Call the handler
          break;
		
		case kAutoTextDeleted:                     // Text taken out
		  if (fIgnoreNextDelete)
		    {
		      fIgnoreNextDelete = false;
		      break;
		    }
          RemoveTail();                            // Kill tail, if any
          SetConfirmed();                          // Update confirm color
          ChangedHandler();                        // Call the handler
		  break;
		  
        case kAutoTextRejectComplete:              // User doesn't like it
          RemoveTail();                            // Kill tail, if any
          break;
          
        case kAutoTextTab:                         // Tab pressed
          if (fCompletionMode == CompletionPassive)// If passive completion
            TryAutoComplete();                     // Do it now
          break;
        
        default:                                   // Any other type
          BTextControl::MessageReceived(msg);      // Shuffle it off
          #ifdef DEBUG_OUTPUT
          handled = false;                         // It wasn't mine
          #endif
          break;                                   // Get outta here
      }                                            // End message type switch
      
    #ifdef DEBUG_OUTPUT
    if (handled) printf("Got message: %0lX\n", msg->what);    
    #endif
  }                                                // End MessageReceived()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::NextPhrase()                                  ***/
/*                                                                          */
/****************************************************************************

This procedure uses PhraseStr() to get the Nth phrase out of the control.   */

bool BAutoCompleteControl::NextPhrase(int32 num,   // Begin NextPhrase()
 BString* out) const
  {
    int32        len;                              // String length
    int32        start, stop;                      // String offsets
    BTextView*   text;                             // Control pointer
    const char*  textPtr;                          // Text in the control
    
    out->SetTo(NULL);                              // In case of errors
    
    text = TextView();                             // Get control pointer
    if (!text) return false;                       // Exit on failure
    textPtr = text->Text();                        // Get text pointer
    
    if (!PhraseStr(textPtr, &fSeparator, num,      // Get next phrase
     &start, &stop))
      return false;                                // Exit if not there
      
    len = stop - start + 1;                        // Set final length  
    out->SetTo(&textPtr[start], len);              // Get phrase string
    
    return true;                                   // Found it
  }                                                // End NextPhrase()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::PhraseStr()                                   ***/
/*                                                                          */
/****************************************************************************

The text in the control can be broken up into one or more "phrases"
separated by the separator characters (or whitespace if none is defined).
This procedure gets the start and stop offsets of the Nth phrase out of
a string.  It returns true if it collected the phrase, false if there are
no more phrases to get.                                                     */

bool BAutoCompleteControl::PhraseStr(const char*   // Begin PhraseStr()
 str, const BString* separator, int32 num,
 int32* start, int32* stop)
  {
    int32  i;                                      // Loops through chars
    
    if (!str || !str[0]) goto fail;                // If no string
    if (num < 0) num = 0;                          // Don't go negative
    
    for (i = 0; str[i]; )                          // Loop the string
      {
        for ( ; str[i]; i++)                       // Past non-phrase stuff
          {
            if (isspace(str[i])) continue;         // Go past whitespace
            
            if (SeparatorChar(separator, str[i]))  // If separator
              continue;                            // Go past it
              
            break;                                 // Stop at phrase char
          }
          
        if (!str[i]) goto fail;                    // If string end found
        
        *start = i;                                // Phrase starts here
        
        for ( ; str[i]; i++)                       // Go through phrase
          if (SeparatorChar(separator, str[i]))    // If separator
            break;                                 // Stop
          
        *stop = i - 1;                             // Stop before separator
        
        for ( ; *stop >= 0; (*stop)--)             // Remove final whites
          if (!isspace(str[*stop])) break;         // Stop at last non-white
          
        num--;                                     // Did another one
        if (num < 0) return true;                  // If it was that one  
      }
    
    fail:                                          // Jump here to exit
    *start = -1;                                   // No start offset
    *stop = -1;                                    // No stop offset
    return false;                                  // Didn't find it
  }                                                // End PhraseStr()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::RemoveTail()                                  ***/
/*                                                                          */
/****************************************************************************

If the user didn't like the auto-complete tail we added (signified by
continuing to type past it) then this procedure will remove it.             */

void BAutoCompleteControl::RemoveTail(void)        // Begin RemoveTail()
  {
    int32        len;                              // Text length
    int32        selStart, selEnd;                 // Selection bounds
    BTextView*   text;                             // Text input control
    
    if (!fTailAdded) return;                       // If no tail
    fTailAdded = false;                            // Soon to be removed
    
    text = TextView();                             // Get control pointer
    if (!text) return;                             // If not there
    
    text->GetSelection(&selStart, &selEnd);        // Get selection
    if (selStart == selEnd) return;                // If nothing selected
    
    text->Delete(selStart, selEnd);                // Get rid of tail
 
    len = text->TextLength();                      // Get total length
    text->Select(len, len);                        // Force caret to end
    
    fIgnoreNextDelete = true;
  }                                                // End RemoveTail()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SelectByIndex()                               ***/
/*                                                                          */
/****************************************************************************

This procedure removes whatever is in the control and stuffs in the text
for the given selection.                                                    */

void BAutoCompleteControl::SelectByIndex(int32     // Begin SelectByIndex()
 index, bool changeTextSelection)
  {
    (void)index;
    (void)changeTextSelection;
  }                                                // End SelectByIndex()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SelectByKey()                                 ***/
/*                                                                          */
/****************************************************************************

Works the same as SelectByIndex() but takes a unique key number instead of
an index.                                                                   */

void BAutoCompleteControl::SelectByKey(int32 key,  // Begin SelectByKey()
 bool changeTextSelection)
  { 
    (void)key;
    (void)changeTextSelection;
  }                                                // End SelectByKey()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::Separator()                                   ***/
/*                                                                          */
/****************************************************************************

Returns a pointer to the list of separator characters.                      */

const char* BAutoCompleteControl::Separator(void)  // Begin Separator()
 const
  {
    if (fSeparator.ByteAt(0))                      // If we have these
      return fSeparator.String();                  // Use 'em
    else                                           // Don't have 'em
      return " ";                                  // It's just a space
  }                                                // End Separator()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SeparatorChar()                               ***/
/*                                                                          */
/****************************************************************************

Since the class allows more than one separator char this procedure takes
a char as input and tells you if it's one of them.                          */

bool BAutoCompleteControl::SeparatorChar(const     // Begin SeparatorChar()
 BString* separator, char ch)
  {
    uint32  i;                                     // Loop counter
    char    test;                                  // Next char from list
    
    for (i = 0; ; i++)                             // Loop for chars
      {
        test = separator->ByteAt(i);               // Get next char
        
        if (test == 0)                             // If no more chars
          {
            if (i == 0)                            // If string is empty
              return isspace(ch);                  // Spaces are separators
            else                                   // String is not empty
              return false;                        // No match
          }    
        
        if (ch == test) return true;               // If it's a match
      }
    
    return false;                                  // Not in the list
  }                                                // End SeparatorChar()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SetAutoCompletionMode()                       ***/
/*                                                                          */
/****************************************************************************

This procedure sets the auto-completion mode for the control.               */

void BAutoCompleteControl::SetAutoCompletionMode(  // SetAutoCompletionMode()
 CompletionType newCompletionType)
  {
    fCompletionMode = newCompletionType;           // Set current mode
    
    if (fCompletionMode < CompletionMin)           // If too small
      fCompletionMode = CompletionMin;             // Move it up
      
    if (fCompletionMode > CompletionMax)           // If too big
     fCompletionMode = CompletionMax;              // Rein it in
  }                                                // SetAutoCompletionMode()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SetAutoCompletionMode()                       ***/
/*                                                                          */
/****************************************************************************

This procedure sets the auto-completion mode for the control with a text
string: "None", "Passive" or "Active".                                      */

void BAutoCompleteControl::SetAutoCompletionMode(  // SetAutoCompletionMode()
 const char* string)
  {
    struct INDEX                                   // Indexed string
      {
        const char*     str;                       // String description
        CompletionType  type;                      // Type
      };
      
    static INDEX list[] =                          // String-to-type list
      {
        {"none",    CompletionNone},               // No completion
        {"passive", CompletionPassive},            // Passive completion
        {"active",  CompletionActive},             // Active completion
        {NULL,      CompletionUnknown}             // End of the list
      };
      
    uint32  i;                                     // Loop counter
    
    if (!string || !string[0]) return;             // If no string given
    
    for (i = 0; list[i].str; i++)                  // Loop the list
      if (!strcasecmp(list[i].str, string))        // If it's a match
        {
          SetAutoCompletionMode(list[i].type);     // Set this type
          break;                                   // Stop the loop
        }
  }                                                // SetAutoCompletionMode()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SetChangedHandler()                           ***/
/*                                                                          */
/****************************************************************************

This procedure is called to set a GHandler to call whenever the contents
of the control change in any way.                                           */

void BAutoCompleteControl::SetChangedHandler(      // Beg SetChangedHandler()
 GHandler* handler, uint32 msg)
  {
    fChangedHandler = handler;                     // Save the handler
    fChangedMessage = msg;                         // Save the message
  }                                                // End SetChangedHandler()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SetConfirmColor()                             ***/
/*                                                                          */
/****************************************************************************

This procedure sets the color for confirmed auto-completed strings.         */

void BAutoCompleteControl::SetConfirmColor(        // SetConfirmColor()
 rgb_color color)
  {
    fConfirmColor = color;                         // Save it
  }                                                // SetConfirmColor()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SetConfirmed()                                ***/
/*                                                                          */
/****************************************************************************

This procedure clears all text to the "normal" color then looks through
the string for confirm matches and sets all the colors properly.            */

void BAutoCompleteControl::SetConfirmed(void)      // Begin SetConfirmed()
  { 
    int32           begin, end;                    // Phrase range
    status_t        found;                         // Found a match?
    int32           i;                             // Loop counter
    int32           len;                           // Text length
    BString         match;                         // Final match
    text_run_array  run;                           // Run to set
    bool            rval;                          // Function return
    int32           tailStart, tailStop;           // Tail offsets
    BString         test;                          // Test match string
    BTextView*      text;                          // Control pointer
    const char*     textPtr;                       // Text pointer
    
    
    //*
    //***  Do initial setup
    //*
    
    text = TextView();                             // Get control pointer
    if (!text) return;                             // If it didn't work
    
    textPtr = text->Text();                        // Get text pointer
    
    memset(&run, 0, sizeof (run));                 // Wipe all bytes
    
    run.count = 1;                                 // Just one item
    run.runs[0].color = fNormalColor;              // Set normal color
    run.runs[0].offset = 0;                        // Set starting offset
    GetFont(&run.runs[0].font);                    // Get current font
    
    
    //*
    //***  Clear all text to the "normal" color
    //*
    
    len = text->TextLength();                      // Get current length
    text->SetRunArray(0, len, &run);               // Clear to normal color
    
    
    //*
    //***  Get tail offsets
    //*
    
    if (len <= 0) return;                          // If nothing left to do
    if (!fChoiceList) return;                      // If no choice list
    
    if (fTailAdded)                                // If we have tail
      {
        text->GetSelection(&tailStart, &tailStop); // Get the tail
      }  
    else                                           // No tail
      {
        tailStart = len + 1;                       // Impossibly far away
        tailStop = len + 1;                        // Ditto
      }
    
    
    //*
    //***  Loop to set confirm color for each confirmed string
    //*
    
    run.runs[0].color = fConfirmColor;             // Set confirm color
    
    for (i = 0; ; i++)                             // Loop for phrases
      {
        rval = PhraseStr(textPtr, &fSeparator, i,  // Get next phrase
         &begin, &end);
         
        if (!rval) break;                          // If not found
        if (end >= tailStart) continue;            // If in the tail
        
        len = end - begin + 1;                     // Set length to get
        test.SetTo(&textPtr[begin], len);          // Set test text
      
        found = fChoiceList->Find(test.String(),   // See if it's there
         &match);
        
        if (found == B_OK)                         // If found
          text->SetRunArray(begin, end + 1, &run); // Set confirm color
      }
  }                                                // End SetConfirmed()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SetSeparator()                                ***/
/*                                                                          */
/****************************************************************************

This procedure sets the separator to be used between multiple completions.  */

void BAutoCompleteControl::SetSeparator(const      // SetSeparator()
 char* charList)
  {
    uint32  i;                                     // Loop counter
    
    if (!charList || !charList[0]) return;         // If no string given
    fSeparator.SetTo(NULL);                        // Clear existing
    
    for (i = 0; charList[i]; i++)                  // Loop for chars
      {
        if (isspace(charList[i])) continue;        // Don't add whitespace
        fSeparator.Append(charList[i], 1);         // Add it
      }
  }                                                // SetSeparator()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::SetUnique()                                   ***/
/*                                                                          */
/****************************************************************************

This procedure sets greedy vs. non-greedy completion.                       */

void BAutoCompleteControl::SetUnique(bool unique)  // SetUnique()
  {
    fUnique = unique;                              // Save it
  }                                                // SetUnique()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::TailAdded()                                   ***/
/*                                                                          */
/****************************************************************************

This procedure's purpose is to let the input control know if there is a
tail added (but not yet accepted) to what the caller typed or not.  The
control must modify its behavior accordingly.                               */

bool BAutoCompleteControl::TailAdded(void) const   // Begin TailAdded()
  {
    return fTailAdded;                             // If tail added
  }                                                // End TailAdded()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::testCompareStart()                            ***/
/*                                                                          */
/****************************************************************************

Figuring out the offset into the control's text where we need to start
comparing for the purposes of auto-completion turned out to be fairly
difficult, so I wrote this test function.                                   */

#ifdef TEST_SCAFFOLDING
void BAutoCompleteControl::testCompareStart(void)  // Begin testCompareStart()
  {
    int32    offset;                               // Final offset
    BString  sep;                                  // Separator chars
    
    sep.SetTo(" ");                                // A single space
    
    offset = CompareStart(NULL, &sep);             // No string
    ASSERT(offset < 0);
    
    offset = CompareStart("", &sep);               // Empty string
    ASSERT(offset < 0);
    
    offset = CompareStart(" ", &sep);              // Single space
    ASSERT(offset < 0);
    
    offset = CompareStart("  ", &sep);             // Two spaces
    ASSERT(offset < 0);
    
    sep.SetTo(";");                                // Semicolon for awhile
    
    offset = CompareStart("  ", &sep);             // Semicolon separator
    ASSERT(offset < 0);
    
    offset = CompareStart("a", &sep);              // Simple
    ASSERT(offset == 0);
    
    offset = CompareStart("  a", &sep);            // Should be simple
    ASSERT(offset == 2);
    
    offset = CompareStart(" ; a", &sep);           // Should be simple
    ASSERT(offset == 3);
    
    offset = CompareStart("a;a", &sep);            // Harder
    ASSERT(offset == 2);
    
    offset = CompareStart("a ; a", &sep);          // Harder
    ASSERT(offset == 4);
    
    sep.SetTo(" ");                                // Now whitespace
    
    offset = CompareStart("booga a", &sep);        // Whitespace separator
    ASSERT(offset == 6);
    
    sep.SetTo(";");                                // Back to semicolon
    
    offset = CompareStart("booga a", &sep);        // Semicolon separator
    ASSERT(offset == 0);
    
    offset = CompareStart("booga a;", &sep);       // Shouldn't complete here
    ASSERT(offset < 0);
    
    sep.SetTo(",");                                // Comma
    
    offset = CompareStart(",b ", &sep);            // Comma separator
    ASSERT(offset == 1);
    
    offset = CompareStart(", b ", &sep);           // Harder
    ASSERT(offset == 2);
    
    sep.SetTo(";");                                // Semicolon again
    
    offset = CompareStart(" a;Apple; a", &sep);    // Harder
    ASSERT(offset == 10);
    
    offset = CompareStart(";", &sep);              // Tough!
    ASSERT(offset < 0);
    
    offset = CompareStart("Apple a", &sep);        // Used to be hard
    ASSERT(offset == 0);
  }                                                // End testCompareStart()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::testPhraseStr()                               ***/
/*                                                                          */
/****************************************************************************

This procedure tests the function that breaks a string into phrases.        */

#ifdef TEST_SCAFFOLDING                            // If testing
void BAutoCompleteControl::testPhraseStr(void)     // Begin testPhraseStr()
  {
    uint32       i, j;                             // Loop counters
    PHRASE*      offset;                           // Offset list
    PHRASEDATA*  phrase;                           // Current test set
    bool         rval;                             // Return value
    BString      sep;                              // Separator chars
    int32        start, stop;                      // Phrase offsets
    
    sep.SetTo(";");                                // Semicolon
    
    rval = PhraseStr(NULL, &sep, 0, &start, &stop);// No string
    ASSERT(!rval && start == -1 && stop == -1);    // That doesn't work
    
    for (i = 0; phraseData[i].str; i++)            // Loop the list
      {
        phrase = &phraseData[i];                   // Get this pointer
        
        for (j = 0; ; j++)                         // Loop for each phrase
          {
            offset = &phrase->offset[j];           // Get next offset list
            start = 85000;                         // Munge this up
            stop = 85000;                          // Ditto
            
            sep.SetTo(phrase->separator);          // Set new separator
            
            rval = PhraseStr(phrase->str,          // Do next phrase
             &sep, j, &start, &stop);
             
            ASSERT(offset->start == start);        // This must match
            ASSERT(offset->stop == stop);          // And this
            
            if (!rval)                             // If last one
              {
                ASSERT(start == -1);               // Must be negative
                ASSERT(stop == -1);                // Ditto
                break;                             // Stop the loop
              }
          }                                        // End this phrase
      }                                            // End phrase loop
  }                                                // End testPhraseStr()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::TryAutoComplete()                             ***/
/*                                                                          */
/****************************************************************************

This all-important procedure gets called when it is time to try to add
text to the end of what the user is typing.                                 */

void BAutoCompleteControl::TryAutoComplete(void)   // Begin TryAutoComplete()
  {
    BString      match;                            // Match text
    int32        offset;                           // Where to start looking
    int32        prefixLen;                        // Prefix length
    int32        selFrom, selTo;                   // Selection indexes
    status_t     status;                           // Return value
    const char*  str;                              // Match text pointer
    BTextView*   text;                             // Pointer to control
    int32        textLen;                          // Total text length
    const char*  textPtr;                          // Pointer to text
	
    if (!fChoiceList) return;                      // If no choice list
    if (TailAdded()) return;                       // If already pending
    
    text = TextView();                             // Get control pointer
    textPtr = text->Text();                        // Get text pointer
    textLen = text->TextLength();                  // Get total length
    text->GetSelection(&selFrom, &selTo);          // Get selection indexes
    
    if ((selFrom == selTo) && (selFrom < textLen)) // Not at the end
      return;                                      // Don't auto-complete

    offset = CompareStart(textPtr, &fSeparator);   // Set comparison start
    if (offset < 0) return;                        // If no place to go
    
    prefixLen = strlen(&textPtr[offset]);          // Get prefix length
    if (prefixLen <= 0) return;                    // If nothing there
    
    status = fChoiceList->FindMatch(               // Unique search
     &textPtr[offset], &match,
     &fMatchIndex, NULL, 0, false, fUnique);
     
    if (status != B_OK) return;                    // If no match
    if (prefixLen >= match.Length()) return;       // If already completed
    
    str = &(match.String())[prefixLen];            // Get tail part
    if (strlen(str) <= 0) return;                  // If nothing left
    text->Insert(str);                             // Stick it in
    
    selFrom = textLen;                             // Set start point
    selTo   = text->TextLength();                  // Set end point
    
    text->Select(selFrom, selTo);                  // Select our insertion
    fTailAdded = true;                             // We've got a tail now
    fIgnoreNextInsert = true;
    
    #ifdef DEBUG_OUTPUT
    printf("Added auto-complete tail\n");
    #endif
  }                                                // End TryAutoComplete()


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl::Update()                                      ***/
/*                                                                          */
/****************************************************************************

Forces the control to update its highlighting and re-draw itself.           */

void BAutoCompleteControl::Update(void)            // Begin Update()
  {
    SetConfirmed();
    Invalidate();
  }                                                // End Update()


/****************************************************************************/
/*                                                                          */
/***  BChoiceList::BChoiceList()                                          ***/
/*                                                                          */
/****************************************************************************

This is the class constructor.                                              */

BChoiceList::BChoiceList(void)                     // Begin BChoiceList()
  {
    fOwner = NULL;                                 // No owner yet
  }                                                // End BChoiceList()


/****************************************************************************/
/*                                                                          */
/***  BChoiceList::~BChoiceList()                                         ***/
/*                                                                          */
/****************************************************************************

This is the class destructor.                                               */

BChoiceList::~BChoiceList(void)                    // Begin ~BChoiceList()
  {
    // Nothing to do
  }                                                // End ~BChoiceList()


/****************************************************************************/
/*                                                                          */
/***  BChoiceList::Owner()                                                ***/
/*                                                                          */
/****************************************************************************

This procedure returns the BAutoCompleteControl that the class belongs to.  */

BAutoCompleteControl* BChoiceList::Owner(void)     // Begin Owner()
 const
  {
    return fOwner;                                 // There you go
  }                                                // End Owner()


/****************************************************************************/
/*                                                                          */
/***  BChoiceList::SetOwner()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure can re-assign the class to a different control, or to none.  */

void BChoiceList::SetOwner(BAutoCompleteControl*   // Begin SetOwner()
 owner)
  {
    fOwner = owner;                                // Save it
  }                                                // End Set Owner()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::BStringChoiceList()                              ***/
/*                                                                          */
/****************************************************************************

This is the constructor for the string choice list.                         */

BStringChoiceList::BStringChoiceList(void)         // BStringChoiceList()
  {
    fList = new KeyedStringObjectList();           // Create string list
    fAutoKey = 1;                                  // Set first key value
  }                                                // BStringChoiceList()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::~BStringChoiceList()                             ***/
/*                                                                          */
/****************************************************************************

This is the destructor for the string choice list.                          */

BStringChoiceList::~BStringChoiceList(void)        // Beg ~BStringChoiceList()
  {
    KeyedString*  str;                             // Next item from list
  
    if (!fList) return;                            // If no list
    
    while (true)                                   // Loop until gone
      {
        str = fList->RemoveItemAt(0);              // Pull out first item
        if (!str) break;                           // Stop at list end
        delete str;                                // Delete it
      }
  
    delete fList;                                  // Delete the list
    fList = NULL;                                  // Invalidate pointer
  }                                                // End ~BStringChoiceList()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::AddChoice()                                      ***/
/*                                                                          */
/****************************************************************************

This procedure adds a new choice with a string and a unique key.            */

status_t BStringChoiceList::AddChoice(const char*  // Begin AddChoice()
 string, int32 key)
  {
    return AddChoice(string, key, -1);             // Call the other one
  }                                                // End AddChoice()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::AddChoice()                                      ***/
/*                                                                          */
/****************************************************************************

This procedure adds a new choice with a string, a unique key, *and* a
desired index, which allows sticking in strings at places other than the
end.  Passing a negative value for the key will cause a new one to be
generated.                                                                  */

status_t BStringChoiceList::AddChoice(const char*  // Begin AddChoice()
 string, int32 key, int32 index)
  {
    bool          rval;                            // Did it work?
    KeyedString*  str;                             // New string to add
    
    if (!fList) return B_ERROR;                    // If no list
    if (!string || !string[0]) return B_ERROR;     // If nothing given
    
    if (index < 0) index = fList->CountItems();    // If no index given
    if (key < 0) key = fAutoKey++;                 // Generate key if needed
	
    str = new KeyedString(key, string);            // Create new keyed string
    if (!str) return B_ERROR;                      // If no memory
    
    rval = fList->AddItem(str, index);             // Add it
    
    if (Owner()) Owner()->ChoiceListChanged();     // Notify my daddy
    return (rval) ? B_OK : B_ERROR;                // It worked, or it didn't
  }                                                // End AddChoice()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::CountChoices()                                   ***/
/*                                                                          */
/****************************************************************************

This procedure returns the total number of items in its list.               */

int32 BStringChoiceList::CountChoices() const      // Begin CountChoices()
  {
    if (!fList) return 0;                          // If no list
    return fList->CountItems();                    // Count 'em
  }                                                // End CountChoices()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::Find()                                           ***/
/*                                                                          */
/****************************************************************************

This procedure searches for an exact match in the string list.              */

status_t BStringChoiceList::Find(const char* str,  // Begin Find()
 BString* match, int32* matchIndex,
 int32* matchKey, int32 startIndex) const
  {
	int32         choices;                         // Total choices
	int32         i;                               // Loop counter
    KeyedString*  kStr;                            // Next item from list
	
    
    //*
    //***  Do initial setup
    //*
    
    if (!fList) return B_ERROR;                    // If no list
    if (!str || !str[0]) return B_ERROR;           // If no search string
    if (startIndex < 0) startIndex = 0;            // Don't go below zero
    
    match->SetTo(NULL);                            // No match text yet
    if (matchIndex) *matchIndex = -1;              // In case of errors
    if (matchKey) *matchKey = -1;                  // Ditto
    
    choices = fList->CountItems();                 // Get total choices
	
    
    //*
    //***  Loop to find first match
    //*
    
    for (i = startIndex; i < choices; i++)         // Loop the list
      {
        kStr = fList->ItemAt(i);                   // Get next item
        if (!kStr) {ASSERT(false); continue;}      // Should be found
        
        if (!kStr->Compare(str))                   // If it's a match
          {
            match->SetTo(kStr->String());          // Save the string
            if (matchIndex) *matchIndex = i;       // Save the index
            if (matchKey) *matchKey = kStr->key;   // Save the key
            return B_OK;                           // Found it
          }
      }                                            // End list loop
    
    return B_ERROR;                                // Didn't find it
  }                                                // End Find()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::FindMatch()                                      ***/
/*                                                                          */
/****************************************************************************

This is the all-important procedure that the whole rest of the class exists
to support.                                                                 */

status_t BStringChoiceList::FindMatch(const char*  // Begin FindMatch()
 prefix, BString* match, int32* matchIndex,
 int32* matchKey, int32 startIndex,
 bool caseSensitive, bool unique) const
  {
	int32         choices;                         // Total choices
	KeyedString*  found = NULL;                    // Found from list
	int32         foundIndex = -1;                 // Found index
	int32         i;                               // Loop counter
	int32         len;                             // Prefix length
	bool          matched;                         // Found a match?
    KeyedString*  str;                             // Next item from list
	
    
    //*
    //***  Do initial setup
    //*
    
    if (!fList) return B_ERROR;                    // If no list
    if (!prefix || !prefix[0]) return B_ERROR;     // If no search string
    if (startIndex < 0) startIndex = 0;            // Don't go below zero
    
    match->SetTo(NULL);                            // No match text yet
    if (matchIndex) *matchIndex = -1;              // In case of errors
    if (matchKey) *matchKey = -1;                  // Ditto
    
    choices = fList->CountItems();                 // Get total choices
    len = strlen(prefix);                          // Get prefix length
	
    
    //*
    //***  Loop to find first (and maybe second) match
    //*
    
    for (i = startIndex; i < choices; i++)         // Loop the list
      {
		str = fList->ItemAt(i);                    // Get next item
		if (!str) {ASSERT(false); continue;}       // Should be found
		
        if (caseSensitive)                         // Case-sensitive search
          matched = !str->Compare(prefix, len);    // Do this
        else                                       // Case-insensitive
          matched = !str->ICompare(prefix, len);   // Do it like this  
        
        if (matched)                               // If it's a match
          {
            if (!unique)                           // Non-unique search
              {
                if (found)                         // If already found one
                  {
                    if (found->Length() >          // If older is longer
                     str->Length())
                      {
                        found = str;               // Save the shorter one
                        foundIndex = i;            // And its index
                      }
                  }
                else                               // Haven't yet found one
                  {
                    found = str;                   // Save that pointer
                    foundIndex = i;                // Save that index
                  }
              }
            else                                   // We need a unique match
              {
                if (!found)                        // If none so far
                  {
                    found = str;                   // Save that pointer
                    foundIndex = i;                // Save that index
                    continue;                      // And keep looking
                  }
                else                               // It's the second match
                  {
                    found = NULL;                  // Clear it
                    foundIndex = -1;               // And clear index
                    break;                         // Ambiguous, no match
                  }  
              }
          }                                        // End match found
      }                                            // End list loop
      
    
    //*
    //***  Return final result
    //*
    
    if (found)                                     // If item was found
      {
        if (matchIndex) *matchIndex = foundIndex;  // Save index
        if (matchKey) *matchKey = found->key;      // Save key
        match->SetTo(found->String());             // Save string
        return B_OK;                               // Success
      }
    else                                           // No match found
      {  
        return B_ERROR;                            // Didn't find it
      }  
  }                                                // End FindMatch()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::GetChoiceAt()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure returns the index and the string at a given index.           */

const char* BStringChoiceList::GetChoiceAt(int32   // Begin GetChoiceAt()
 index, int32 *key) const
  {
    KeyedString*  str;                             // Item at given index
    
	if (key) *key = -1;                            // In case of errors
	if (!fList) return NULL;                       // If no item list
	
	str = fList->ItemAt(index);                    // Get item at index
	if (!str) return NULL;                         // If it's not there
	
    if (key) *key = str->key;                      // Give key to caller
    return str->String();                          // Return the string
  }                                                // End GetChoiceAt()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::GetChoiceByKey()                                 ***/
/*                                                                          */
/****************************************************************************

Given a particular unique key, this procedure will return the string and
the index associated with it.                                               */

const char* BStringChoiceList::GetChoiceByKey(     // Begin GetChoiceByKey()
 int32 key, int32* index) const
  {
    int32         choices;                         // Total choices
    int32         i;                               // Loop counter
    KeyedString*  str;                             // Next item from list
	
    if (index) *index = -1;                        // In case of errors
    if (!fList) return NULL;                       // If no list
    
    choices = fList->CountItems();                 // Get item count
	
    for (i = 0; i < choices; i++)                  // Loop for all items
      {
		str = fList->ItemAt(i);                    // Get next item
		if (!str) {ASSERT(false); continue;}       // Should find it
		
		if (str->key == key)                       // If we've got a match
		  {
            if (index) *index = i;                 // Save index for caller
            return str->String();                  // Return the string
          }
      }                                            // End item loop
	
    return NULL;                                   // Didn't find it
  }                                                // End Get ChoiceByKey()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::RemoveAll()                                      ***/
/*                                                                          */
/****************************************************************************

This procedure removes all items from the list.                             */

status_t BStringChoiceList::RemoveAll(void)        // Begin RemoveAll()
  {
    KeyedString*  str;                             // Next item from list
  
    if (!fList) return B_ERROR;                    // If no list
    if (fList->CountItems() <= 0) return B_OK;     // If nothing in there
    
    while (true)                                   // Loop until gone
      {
        str = fList->RemoveItemAt(0);              // Pull out first item
        if (!str) break;                           // Stop at list end
        delete str;                                // Delete it
      }
  
    if (Owner()) Owner()->ChoiceListChanged();     // Notify my daddy
    return B_OK;                                   // Success
  }                                                // End RemoveAll()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::RemoveChoice()                                   ***/
/*                                                                          */
/****************************************************************************

This procedure removes a particular choice with a target string.            */

status_t BStringChoiceList::RemoveChoice(const     // Begin RemoveChoice()
 char* string)
  {
    int32     i;                                   // Loop counter
	int32     choices;                             // Total choices
    BString*  str;                                 // String from the list
    
    if (!fList) return B_ERROR;                    // If no list
    if (!string || !string[0]) return B_ERROR;     // If no string given
    
    choices = fList->CountItems();                 // Get total items
	
    for (i = 0; i < choices; i++)                  // Loop the list
      {
		str = fList->ItemAt(i);                    // Get next item
		if (!str) {ASSERT(false); continue;}       // Should be found
		
        if (!str->Compare(string))                 // If an exact match
          {
            fList->RemoveItemAt(i);                // Take it out
            
            if (Owner())                           // If we have an owner
              Owner()->ChoiceListChanged();        // I'm tellin' mom
            
            return B_OK;                           // Success
          }		
      }                                            // End item loop
	
    return B_ERROR;		                           // Not found
  }                                                // End RemoveChoice()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::RemoveChoiceAt()                                 ***/
/*                                                                          */
/****************************************************************************

This procedure removes a particular choice by index number.                 */

status_t BStringChoiceList::RemoveChoiceAt(int32   // Begin RemoveChoiceAt()
 index)
  {
    KeyedString*  str;                             // Item from the list
	
	if (!fList) return B_ERROR;                    // If no list
	
	str = fList->RemoveItemAt(index);              // Pull item out
	if (!str) return B_ERROR;                      // If not found
	
    delete str;                                    // Delete it
    str = NULL;                                    // Toss the pointer
        
    if (Owner()) Owner()->ChoiceListChanged();     // Notify my daddy
    return B_OK;                                   // Success
  }                                                // End RemoveChoiceAt()


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList::RemoveChoiceByKey()                              ***/
/*                                                                          */
/****************************************************************************

This procedure removes a particular choice by unique key number.            */

status_t BStringChoiceList::RemoveChoiceByKey(     // Beg RemoveChoiceByKey()
 int32 key)
  {
    int32         choices;                         // Total item count
    int32         i;                               // Loop counter
    KeyedString*  str;                             // String from the list
    
    if (!fList) return B_ERROR;                    // If no list
    choices = fList->CountItems();                 // Get total item count
	
    for (i = 0; i < choices; i++)                  // Loop the list
      {
		str = fList->ItemAt(i);                    // Get next item
		if (!str) {ASSERT(false); continue;}       // It should work
		
        if (key == str->key)                       // If we found it
          {
			fList->RemoveItemAt(i);                // Take it out
			
			if (Owner())                           // If owned
			  Owner()->ChoiceListChanged();        // Notify landlord
				
            return B_OK;                           // Success
          }
      }                                            // End item loop
	
    return B_ERROR;                                // Didn't find it
  }                                                // End RemoveChoiceByKey()

