// AutoCompleteControl.h -- BTextControl with auto-completion
// by Allen Brunson, based on TextListControl by Nathan Schrenk

#ifndef AUTO_COMPLETE_CONTROL_H                    // If file not defined
#define AUTO_COMPLETE_CONTROL_H                    // Start it now

#include <String.h>
#include <TextControl.h>

class BChoiceList;                                 // Forward declare needed
class BStringChoiceList;                           // Forward declare needed
class KeyedStringObjectList;                       // Forward declare needed


/****************************************************************************/
/*                                                                          */
/***  Test data                                                           ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING                            // If testing

struct PHRASE                                      // Result for one phrase
  {
    int32  start;                                  // Start value
    int32  stop;                                   // Stop value
  };
  
struct PHRASEDATA
  {
    const char*  str;                              // String to test
    const char*  separator;                        // Phrase separators
    PHRASE*      offset;                           // Phrase offsets
  };

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  BAutoCompleteControl                                                ***/
/*                                                                          */
/****************************************************************************/

class BAutoCompleteControl: public BTextControl    // BAutoCompleteControl
  {
    public:
    
    enum CompletionType                            // Auto-completion modes
      {
        CompletionUnknown = 0,                     // Don't use this one
        CompletionNone    = 1,                     // No auto-completion
        CompletionPassive = 2,                     // Press tab to complete
        CompletionActive  = 3,                     // Control sticks in text
        
        CompletionMin     = CompletionNone,        // Min legal value
        CompletionMax     = CompletionActive,      // Max legal value
        CompletionDefault = CompletionActive,      // Default value
      };
    
                     BAutoCompleteControl(BRect frame,
                      const char* name,
                      const char* label,
                      const char* initial_text,
                      BMessage* message,
                      BChoiceList* choiceList = NULL,
                      bool ownChoiceList = true,
                      uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
                      uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS |
                      B_NAVIGABLE); 
							
    virtual          ~BAutoCompleteControl(void);
    
    // Returns the current auto-completion mode.
    
    CompletionType   AutoCompletionMode(void) const;
    
	// ChoiceListChanged should be called whenever an item in the choice
	// list changes so that BAutoCompleteControl can perform the proper
	// updating.  BStringChoiceList calls this method on its owner for
	// all add/remove operations
	
    virtual void     ChoiceListChanged(void);
    
    // Gets current confirmation color
    
    rgb_color        ConfirmColor(void) const;
    
	// Returns the key of the current selection, or a negative value
	
    int32            CurrentSelection(void) const;
	
    virtual void     MessageReceived(BMessage* msg);
    
    // Gets the next "phrase" out of the control, i.e., the stuff
    // that occurs between delimiters, whatever that happens to be.
    
    bool             NextPhrase(int32 num, BString* out) const;
	
    // Select changes the list selection to the specified index, and if the
    // changeTextSelection flag is true, changes the text in the TextView to
    // the value at index and selects all the text in the TextView.  If
    // index is negative, the current selection will be deselected.
    
    virtual void     SelectByIndex(int32 index,
                      bool changeTextSelection = false);
                      
	// Same as SelectByIndex, but takes a key instead of an index
	
    virtual void     SelectByKey(int32 key,
                      bool changeTextSelection = false);
                      
    // Set the auto-completion mode
    
    void             SetAutoCompletionMode(CompletionType newCompletionType);
    void             SetAutoCompletionMode(const char* string);
    
    // Sets the handler that gets notified on control change
    
    void             SetChangedHandler(GHandler* handler, uint32 msg);
    
    // Sets color for confirmed auto-completed entries
    
    void             SetConfirmColor(rgb_color color);
    
    // Sets one or more characters that separate completions.  If no
    // separator characters are assigned then whitespace is used.
    
    const char*      Separator(void) const;
    void             SetSeparator(const char* charList);
    
    // For use by the text input control to see if we've got an
    // auto-completion string currently "under review."
    
    bool             TailAdded(void) const;
    
    // Sets the "greedy" state of the control: either it auto-completes
    // with the first match it finds (false) or waits until the user
    // has typed enough text that an unambiguous match can be found (true).
    
    void             SetUnique(bool unique);
    
    // Forces control to re-calculate highlights and re-draw itself.
    
    void             Update(void);
	
    private:
    
    GHandler*        fChangedHandler;              // Who's notified on change
    uint32           fChangedMessage;              // What to send
    BChoiceList*     fChoiceList;                  // List of choices
    rgb_color        fConfirmColor;                // Auto-completed color
    bool             fIgnoreNextDelete;            // Gets past sticky problem
    bool             fIgnoreNextInsert;            // Ditto
    int32            fMatchIndex;                  // Index that matched
    rgb_color        fNormalColor;                 // Normal color
    bool             fOwnsChoiceList;              // Delete on destruction?
    int32            fSelected;                    // Last index selected
    BString          fSeparator;                   // Separator chars
    bool             fTailAdded;                   // Added a tail?
    bool             fUnique;                      // Unique completions?
    CompletionType   fCompletionMode;              // Passive, agressive, none
    
    void             ChangedHandler(void);         // Time to call handler
    
    static int32     CompareStart(const char* str, // Where to start compare?
                      const BString* separator);
                      
    static bool      PhraseStr(const char* str,    // Gets Nth phrase in
                      const BString* separator,    //  text string
                      int32 num, int32* start,
                      int32* stop);
    
    #ifdef           TEST_SCAFFOLDING              // If testing
    void             testCompareStart(void);       // For testing only
    void             testPhraseStr(void);          // For testing only
    #endif                                         // End test stuff
    
    void             InitData(void);               // Constructor init
    void             InsertSeparator(void);        // Time to do this?
    void             KeepTail(void);               // Auto-complete permanent
    void             RemoveTail(void);             // Remove tail if needed
    static bool      SeparatorChar(const BString*  // Is this a separator?
                      separator, char ch);
    void             SetConfirmed(void);           // Sets confirm colors
    void             TryAutoComplete(void);        // Stick on auto-complete
  };

  
/****************************************************************************/
/*                                                                          */
/***  BChoiceList                                                         ***/
/*                                                                          */
/****************************************************************************

Abstract class provides an interface for BTextListControl to access
possible choices.  Choices are used for auto-completion and for showing
the pop-up list.                                                            */

class BChoiceList                                  // Begin BChoiceList
  {
    public:
	
                     BChoiceList(void);            // Constructor
    virtual          ~BChoiceList(void);           // Destructor
	
	// Returns the text for the specified choice, and sets *key to the
	// choice's key, or does nothing and returns NULL if index is out
	// of range.
	
	virtual const char*	GetChoiceAt(int32 index, int32 *key = NULL) const = 0;
	
	// Returns the text for the specified choice, and sets *index to the
	// choice's index, or does nothing and returns NULL if key is not found.
	
	virtual const char*	GetChoiceByKey(int32 key, 
		int32 *index = NULL) const = 0;
	
    // Simple "find the string" function.  Searches starting at the given
    // index for the first exact case-sensitive match.
    
    virtual status_t  Find(const char* str, BString* match,
                       int32* matchIndex = NULL,
                       int32* matchKey = NULL,
                       int32 startIndex = 0) const = 0;
    
    // Looks for a match at or after startIndex which contains a choice
    // that starts with prefix.  Case-sensitive search or not, based on the
    // bool.  If 'unique' is true it will return a match ONLY if the prefix
    // is unambiguous and matches only one choice.  If a match is found it
    // returns B_OK, matchIndex is set to the list index of the found item,
    // and match is set to the full matching text from the found item.
    // If no match is found it returns B_ERROR.
	
    virtual status_t FindMatch(const char* prefix,
                      BString* match,
                      int32* matchIndex = NULL,
                      int32* matchKey = NULL,
                      int32 startIndex = 0,
                      bool caseSensitive = false,
                      bool unique = true)
                      const = 0;
									
	// Returns the number of choices
	
    virtual int32    CountChoices(void) const = 0;

    void             SetOwner(BAutoCompleteControl* owner);
    BAutoCompleteControl*  Owner(void) const;

    private:

    BAutoCompleteControl*  fOwner;     // My parent control
  };


/****************************************************************************/
/*                                                                          */
/***  BStringChoiceList                                                   ***/
/*                                                                          */
/****************************************************************************

A BChoiceList that stores BStrings.  It keeps copies of each choice added
and frees the memory when the choices are removed.                          */

class BStringChoiceList: public BChoiceList        // Begin BStringChoiceList
  {
	public:
	
                     BStringChoiceList(void);      // Constructor
   virtual           ~BStringChoiceList(void);     // Destructor

    virtual const char* GetChoiceAt(int32 index,   // Choice by index
                         int32* key = NULL) const;
                         
    virtual const char* GetChoiceByKey(int32 key,  // Choice by key
                         int32* index = NULL) const;
	
    virtual status_t  Find(const char* str, BString* match,
                       int32* matchIndex = NULL,
                       int32* matchKey = NULL,
                       int32 startIndex = 0) const;
    
    virtual status_t FindMatch(const char* prefix, // The lone finder
                      BString* match,
                      int32* matchIndex = NULL,
                      int32* matchKey = NULL,
                      int32 startIndex = 0,
                      bool caseSensitive = false,
                      bool unique = true) const;
									
	virtual int32	CountChoices(void) const;

	// If you call AddChoice with a negative key value, a positive key value
	// will be generated and assigned.  You must have BStringChoiceList
	// assign either all values, or assign them all yourself.  Otherwise,
	// you risk non-unique keys.
	
	status_t		AddChoice(const char* string, int32 key = -1);
	status_t		AddChoice(const char* string, int32 key, int32 index);

	status_t        RemoveAll(void);
	status_t		RemoveChoice(const char* string);
	status_t		RemoveChoiceAt(int32 index);
	status_t		RemoveChoiceByKey(int32 key);

	// Note: Sorting functionality should be added to the API.

    private:

    KeyedStringObjectList*  fList;
    int32                   fAutoKey;
  };                                               // End BStringChoiceList


/****************************************************************************/
/*                                                                          */
/***  BAutoComplete control                                               ***/
/*                                                                          */
/****************************************************************************


Maintenance notes
-----------------

When the control adds a possible auto-completion to the end of what the
user types I call that "adding a tail," which will be highlighted.  The
state is kept in the bool fTailAdded.  If the user rejects the completion
(left arrow, keeps typing) then the tail is removed and fTailAdded is set
to false.  This can happen either automatically by code inside the BTextView
or via code inside BAutoCompleteControl, in either case you can tell if the
tail is still there by checking for a selection.  If the user opts to keep
the completion (right arrow, enter, exits the control, etc.) then everything
is removed, including stuff the user typed, and replaced with stuff from
our BChoiceList so that the capitalization can be made proper, and fTailAdded
is again set to false.

All the auto-completion magic takes place only at the end of the control and
only while the user is inserting text (as opposed to deleting).  This stuff
is already complicated and fragile, it would take a better programmer than
me to auto-complete at any position.


Stuff to do
-----------

Perhaps a mouse click should trigger an Invoke() call?  That's something I
didn't take from Nathan's original control.

SelectByKey() and SelectByIndex() don't work and I'm wondering if they even
make sense in a control that might do multiple auto-completes.

*/

#endif                                             // AUTO_COMPLETE_CONTROL_H
