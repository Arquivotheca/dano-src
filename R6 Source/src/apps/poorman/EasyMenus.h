//
// EasyMenu header
//
// Quick-and-easy way to build menus.
//

struct MenuItemTemplate {
	char	*label;			// Item label (and default name)
							// (is a BMenu * for submenus)
	char	shortcut;		// Shortcut key
	uint32	modifiers;		// Modifier keys
	uint32	messageType;	// Mesage to send when chosen
	BMenuItem *itemPtr;		// Pointer to menu item once created
};
typedef struct MenuItemTemplate MenuItemTemplate, *MenuItemTemplatePtr;


struct MenuTemplate {
	char		*label;		// Menu label (and default name)
	menu_layout	layout;		// Menu layout
	MenuItemTemplate **itemList;	// Pointer to item list
};
typedef struct MenuTemplate MenuTemplate;


struct MenuBarTemplate {
	char		*name;		// Menu bar name
	uint32		resizingMode;
	menu_layout	layout;
	bool		resizeToFit;
	MenuTemplate **menuList;	// List of menus
};
typedef struct MenuBarTemplate MenuBarTemplate;


BMenuBar *EZCreateMenuBar(MenuBarTemplate *barTemplate, BRect frame);

extern MenuItemTemplate			ezMenuSeparator;
#define EZMenuSeparator			&ezMenuSeparator
