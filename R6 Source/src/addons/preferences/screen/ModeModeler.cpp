#include "ModeModeler.h"
#include <math.h>
#include <stdarg.h>
#include <InterfaceDefs.h>
#include <Screen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ModeModeler::ModeModeler()
{
	mode_list = 0;
	mode_count = 0;
	for (size_t i = 0; i < MM_COLUMN_COUNT; i++) {
		columns[i].type = (ColumnType)i;
		columns[i].selected = 0;
		columns[i].menu_items = MatchList::pair(0,0);
		columns[i].matching_items = MatchList::pair(0,0);
	}
	identity_list = MatchList::pair(0,0);
}


ModeModeler::~ModeModeler()
{
	if (mode_list) free(mode_list);
	for (size_t i = 0; i < MM_COLUMN_COUNT; i++) {
		if (columns[i].menu_items.second) free(columns[i].menu_items.second);
		if (columns[i].matching_items.second) free(columns[i].matching_items.second);
	}
	if (identity_list.second) free(identity_list.second);
}

ColumnType 
ModeModeler::TypeOf(size_t index)
{
	if (index < MM_COLUMN_COUNT) return columns[index].type;
	return BadColumnIndex;
}

size_t 
ModeModeler::ColumnOf(ColumnType t)
{
	for (size_t i = 0; i < MM_COLUMN_COUNT; i++)
		if (columns[i].type == t) return i;
	return (size_t)BadColumnType;
}

status_t 
ModeModeler::Select(size_t column, size_t item)
{
	// make the item'th entry of the column selected, propagating the change to the columns to the right
	// preserve the current selection of the next column if possible
	
	// validate column index
	if (column >= MM_COLUMN_COUNT) return B_OK;
	
	// validate item index
	if (item >= columns[column].menu_items.first) return B_ERROR;

	// remember the new selected item
	columns[column].selected = item;

	// update our matching list
	Predicate *p;
	display_mode *dm = mode_list + columns[column].menu_items.second[item];
	switch (columns[column].type) {
	case DisplayShape:
		p = new DisplayShapePredicate(dm);
		break;
	case VirtualShape:
		p = new VirtualShapePredicate(dm);
		break;
	case RefreshRate:
		p = new RefreshRatePredicate(dm);
		break;
	case PixelConfig:
		p = new PixelConfigPredicate(dm);
		break;
	case OtherParams:
		p = new OtherParamsPredicate(dm);
		break;
	default:
		// yikes
		return B_ERROR;
		break;
	}
	MatchList source;
	if (column == 0) source = identity_list;
	else source = columns[column-1].matching_items;

	// free any previous list
	if (columns[column].matching_items.second)
		free(columns[column].matching_items.second);

	// get our new list of matching items
	columns[column].matching_items = ModesMatching(source, p);

	// propagate the change to the downstream columns
	return UpdateColumn(column+1);
}

status_t 
ModeModeler::Select(ColumnType t, size_t item)
{
	return Select(ColumnOf(t), item);
}


status_t 
ModeModeler::UpdateColumn(size_t column)
{
	// get a new list of menu items and matching items for this column

	// validate column index
	if (column >= MM_COLUMN_COUNT) return B_OK;

	MatchList source;
	if (column == 0) source = identity_list;
	else source = columns[column-1].matching_items;

	// free up any previous lists
	if (columns[column].menu_items.second) free(columns[column].menu_items.second);
	if (columns[column].matching_items.second) free(columns[column].matching_items.second);

	// update our matching list
	Predicate *p;
	display_mode *dm;

// = mode_list + columns[column].menu_items.second[item];

	switch (columns[column].type) {
	case DisplayShape:
		p = new DisplayShapeUnique();
		columns[column].menu_items = ModesMatching(source, p);
		delete p;
		dm = mode_list + columns[column].menu_items.second[0];
		p = new DisplayShapePredicate(dm);
		columns[column].matching_items = ModesMatching(source, p);
		break;
	case VirtualShape:
		p = new VirtualShapeUnique();
		columns[column].menu_items = ModesMatching(source, p);
		delete p;
		dm = mode_list + columns[column].menu_items.second[0];
		p = new VirtualShapePredicate(dm);
		columns[column].matching_items = ModesMatching(source, p);
		break;
	case RefreshRate:
		p = new RefreshRateUnique();
		columns[column].menu_items = ModesMatching(source, p);
		delete p;
		dm = mode_list + columns[column].menu_items.second[0];
		p = new RefreshRatePredicate(dm);
		columns[column].matching_items = ModesMatching(source, p);
		break;
	case PixelConfig:
		p = new PixelConfigUnique();
		columns[column].menu_items = ModesMatching(source, p);
		delete p;
		dm = mode_list + columns[column].menu_items.second[0];
		p = new PixelConfigPredicate(dm);
		columns[column].matching_items = ModesMatching(source, p);
		break;
	case OtherParams:
		p = new OtherParamsUnique();
		columns[column].menu_items = ModesMatching(source, p);
		delete p;
		dm = mode_list + columns[column].menu_items.second[0];
		p = new OtherParamsPredicate(dm);
		columns[column].matching_items = ModesMatching(source, p);
		break;
	default:
		// yikes
		return B_ERROR;
		break;
	}
	// delete the final predicate
	delete p;

	// update the downstream columns
	UpdateColumn(column+1);
	return B_OK;
}



void 
ModeModeler::RepopulateModeList(void)
{
	BScreen s;
	
	if (mode_list) {
		free(mode_list);
		mode_list = 0;
	}
	mode_count = 0;

	display_mode	*p;
	s.GetModeList(&mode_list, &mode_count);

	// add the current modes to the mode list, if it's not in there
	if((p = (display_mode *)realloc(mode_list, sizeof(display_mode) * (mode_count + 32))) != 0)
	{
		mode_list = p;

		int32 count = count_workspaces();
		for(int32 j = 0; j < count; j++)
		{
			display_mode	current;
			display_mode	*dm = mode_list;
			size_t			i;

			s.GetMode(j, &current);
			for(i = 0; i < mode_count; i++, dm++)
				if(modes_match(dm, &current))
					break;

			if(i == mode_count)
				mode_list[mode_count++] = current;
		}
	}

	// free up the old space
	if (identity_list.second) delete [] identity_list.second;

	// get some space for the list of matches
	identity_list.second = new size_t[mode_count];

	// test every mode in the list
	for (size_t i = 0; i < mode_count; i++)
		identity_list.second[i] = i;

	identity_list.first = mode_count;

	UpdateColumn(0);
}

display_mode *ModeModeler::AddEntry()
{
	display_mode	*result = 0;
	display_mode	*p;

	if((p = (display_mode *)realloc(mode_list, sizeof(display_mode) * (mode_count + 1))) != 0)
	{
		mode_list = p;
		result = &mode_list[mode_count++];
	}

	return result;
}

MatchList
ModeModeler::ModesMatching(const MatchList &source, Predicate *p)
{
	size_t	nth = 0;
	MatchList result;

	// get some space for the list of matches
	result.second = new size_t[source.first];

	// test every mode in the list
	for (size_t i = 0; i < source.first; i++) {
		nth = source.second[i];
		if ((*p)(mode_list + nth))
			result.second[result.first++] = nth;
	}
	return result;
}

const char *
ModeModeler::HeaderForColumn(size_t column)
{
	switch (columns[column].type) {
	case DisplayShape:
		return "Resolution:";
	case VirtualShape:
		return "Desktop:";
	case RefreshRate:
		return "Refresh Rate:";
	case PixelConfig:
		return "Colors:";
	case OtherParams:
		return "Other:";
	default:
		return "Unknown:";
	}
}


char *
ModeModeler::LabelForColumnItem(size_t column, size_t item)
{
	display_mode *dm = mode_list + columns[column].menu_items.second[item];
	char buffer[32];

	switch (columns[column].type) {
	case DisplayShape:
		sprintf(buffer, "%d x %d", dm->timing.h_display, dm->timing.v_display);
		break;
	case VirtualShape:
		sprintf(buffer, "%d x %d", dm->virtual_width, dm->virtual_height);
		break;
	case RefreshRate:
		sprintf(buffer, "%.0f Hz", rate_from_display_mode(dm));
		break;
	case PixelConfig:
		sprintf(buffer, "%s", spaceToString(dm->space));
		break;
	case OtherParams:
		sprintf(buffer, "Configuration %ld", item + 1);
		break;
	default:
		*buffer = '\0';
		break;
	}
	return strdup(buffer);
}

size_t 
ModeModeler::ItemsInColumn(size_t column)
{
	// validate column index
	if (column >= MM_COLUMN_COUNT) return 0;
	return columns[column].menu_items.first;
}

size_t 
ModeModeler::Selected(size_t column)
{
	// validate column index
	if (column >= MM_COLUMN_COUNT) return 0;
	return columns[column].selected;
}


#if 0
MatchList
ModeModeler::ModesMatching(MatchList source, Predicate *p, ...)
{
	const size_t MAX_PREDICATES = 8;
	va_list	ap;
	Predicate *pp;
	Predicate *plist[MAX_PREDICATES+1];
	size_t	predicate_count = 0;
	size_t	*match_list;
	size_t	match_count = 0;
	MatchList result;

	// remember the first one
	plist[0] = p;
	// are there more predicates
	if (p) {
		// count the first one only if it's not null
		predicate_count++;
		// get all of the rest of the predicates
		va_start(ap, p);
		for (pp = va_arg(ap, Predicate *); pp && (predicate_count < MAX_PREDICATES); pp = va_arg(ap, Predicate *)) {
			plist[predicate_count] = pp;
			predicate_count++;
		}
		va_end(ap);
	}
	// null terminate the list
	plist[predicate_count] = 0;

	// get some space for the list of matches
	match_list = new size_t[mode_count];

	// test every mode in the list
	for (size_t i = 0; i < mode_count; i++) {
		display_mode *dm;
		bool match = true;	// everything matches a NULL predicate
		size_t	predicate = 0;
		dm = mode_list + i;
		
		// test the predicates, short-circuting if a test fails
		while (match && (predicate < predicate_count))
			match = match && (*(plist[predicate++]))(dm);
		
		if (match) match_list[match_count++] = i;
	}
	result.first = match_count;
	result.second = match_list;
	return result;
}
#endif

