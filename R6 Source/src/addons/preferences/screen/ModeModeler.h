#if !defined(_MODEMODELER_H)
#define _MODEMODELER_H

#include <Accelerant.h>
#include <utility>

#include "Predicates.h"

// the various column types
typedef enum {
	DisplayShape,
	VirtualShape,
	PixelConfig,
	RefreshRate,
	OtherParams,
	BadColumnType,
	BadColumnIndex
} ColumnType;

typedef pair<size_t, size_t *> MatchList;

#if 0
class ColumnItem {
					ColumnItem(ColumnType ct);
					~ColumnItem();

	MatchList		ModesMatching(void);	// the list of modes matching it's predicate

private:
	ColumnItem		*left;			// the ColumnItem to our left
	ColumnItem		*right;			// the ColumnItem to our right
	Predicate		*match_predicate;	// predicate used to generate ModesMatching
	Predicate		*list_predicate;	// predicate used to generate menu items
	ColumnType		column_type;
	const char		*header;
	MatchList		items;
};



class Column {
public:
					Column(ColumnType t, Column *left);
					~Column();
const MatchList		*MenuItems(void);
const MatchList		*MatchingItems(void);

private:
	ColumnType		type;			// what kind of column is this
	MatchList		menu_items;		// generated by the Unique predicate
	MatchList		matching_items;	// generated by the standard predicate	
};
#endif

enum {
	MM_COLUMN_COUNT		= 5,
	MM_ITEM_SELECTED	= 'mmIS'
};

class ModeModeler {
private:
	typedef struct {
		ColumnType	type;			// type of column
		size_t		selected;		// which element is selected
		MatchList	menu_items;		// items which make up the menu selection
		MatchList	matching_items;	// items which match the current selection
	} Column;
	Column			columns[MM_COLUMN_COUNT];
	size_t			mode_count;
	display_mode	*mode_list;		// array of display_modes
	MatchList		identity_list;
	MatchList		IdentityList(void);
	MatchList		ModesMatching(const MatchList &source, Predicate *p);
	status_t		UpdateColumn(size_t column);


public:
					ModeModeler();
					~ModeModeler();

	void			RepopulateModeList(void);
	display_mode	*AddEntry();
	const display_mode
					*ModeList(void) { return mode_list; };
	size_t			ModeCount(void) { return mode_count; };

	ColumnType		TypeOf(uint32 index);
	uint32			ColumnOf(ColumnType t);

	status_t		Select(ColumnType t, size_t item);
	status_t		Select(size_t column, size_t item);
	size_t			Selected(size_t column);
	size_t			ItemsInColumn(size_t column);
const char			*HeaderForColumn(size_t column);
	char			*LabelForColumnItem(size_t column, size_t item);

const MatchList		*MenuItems(size_t column) { if (column >= MM_COLUMN_COUNT) return 0; return &columns[column].menu_items; };
const MatchList		*MatchingItems(size_t column) { if (column >= MM_COLUMN_COUNT) return 0; return &columns[column].matching_items; };
};

#endif
