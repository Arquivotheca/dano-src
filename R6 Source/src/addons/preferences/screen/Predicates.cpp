#include "Predicates.h"
#include <stdlib.h>
#include <stdio.h>

bool 
DisplayShapePredicate::operator()(const display_mode *dm)
{
	return (dm->timing.h_display == width) && (dm->timing.v_display == height);
}

bool 
VirtualShapePredicate::operator()(const display_mode *dm)
{
	return (dm->virtual_width == width) && (dm->virtual_height == height);
}

bool 
RefreshRatePredicate::operator()(const display_mode *dm)
{
	double dmRate = rate_from_display_mode(dm);
	return fabs(dmRate - rate) <= epsilon;
}

bool 
PixelConfigPredicate::operator()(const display_mode *dm)
{
	return dm->space == space;
}

bool 
OtherParamsPredicate::operator()(const display_mode *dm)
{
	return modes_match(dm, &mode);
}


DisplayShapeUnique::~DisplayShapeUnique()
{
	if (seen_shapes) free(seen_shapes);
}

bool 
DisplayShapeUnique::operator()(const display_mode *dm)
{
	bool notseen = true;
	uint16 width = dm->timing.h_display;
	uint16 height = dm->timing.v_display;

	for (size_t i = 0; i < shape_count; i++) {
		// check for match
		if ((seen_shapes[i].width == width) &&
			(seen_shapes[i].height == height)) {
			notseen = false;
			break;
		}
	}
	if (notseen) {
		struct ss *new_shapes = (struct ss *)
			realloc(seen_shapes, (shape_count+1)* sizeof (*seen_shapes));
		if (new_shapes) {
			seen_shapes = new_shapes;
			seen_shapes[shape_count].width = width;
			seen_shapes[shape_count++].height = height;
		}
	}
	return notseen;
}


VirtualShapeUnique::~VirtualShapeUnique()
{
	if (seen_shapes) free(seen_shapes);
}

bool 
VirtualShapeUnique::operator()(const display_mode *dm)
{
	bool notseen = true;
	uint16 width = dm->virtual_width;
	uint16 height = dm->virtual_height;

	for (size_t i = 0; i < shape_count; i++) {
		// check for match
		if ((seen_shapes[i].width == width) &&
			(seen_shapes[i].height == height)) {
			notseen = false;
			break;
		}
	}
	if (notseen) {
		struct ss *new_shapes = (struct ss *)
			realloc(seen_shapes, (shape_count+1)* sizeof (*seen_shapes));
		if (new_shapes) {
			seen_shapes = new_shapes;
			seen_shapes[shape_count].width = width;
			seen_shapes[shape_count++].height = height;
		}
	}
	return notseen;
}


RefreshRateUnique::~RefreshRateUnique()
{
	if (seen_rates) free(seen_rates);
}

bool 
RefreshRateUnique::operator()(const display_mode *dm)
{
	bool notseen = true;
	double dmRate = rate_from_display_mode(dm);

	for (size_t i = 0; i < rate_count; i++) {
		// check for match
		if (fabs(dmRate - seen_rates[i]) <= epsilon) {
			notseen = false;
			break;
		}
	}
	if (notseen) {
		double *new_rates = (double *)
			realloc(seen_rates, (rate_count+1)* sizeof (*seen_rates));
		if (new_rates) {
			seen_rates = new_rates;
			seen_rates[rate_count++] = dmRate;
		}
	}
	return notseen;
}


PixelConfigUnique::~PixelConfigUnique()
{
	if (seen_configs) free(seen_configs);
}

bool 
PixelConfigUnique::operator()(const display_mode *dm)
{
	bool notseen = true;
	uint32 config = dm->space;

	for (size_t i = 0; i < config_count; i++) {
		// check for match
		if (seen_configs[i] == config) {
			notseen = false;
			break;
		}
	}
	if (notseen) {
		uint32 *new_configs = (uint32 *)
			realloc(seen_configs, (config_count+1)* sizeof (*seen_configs));
		if (new_configs) {
			seen_configs = new_configs;
			seen_configs[config_count++] = config;
		}
	}
	return notseen;
}

bool 
OtherParamsUnique::operator()(const display_mode *dm)
{
	bool notseen = true;

	for (size_t i = 0; i < mode_count; i++) {
		// check for match
		if (modes_match(dm, seen_modes + i)) {
			notseen = false;
			break;
		}
	}
	if (notseen) {
		display_mode *new_modes = (display_mode *)
			realloc(seen_modes, (mode_count+1)* sizeof (*seen_modes));
		if (new_modes) {
			seen_modes = new_modes;
			new_modes[mode_count++] = *dm;
		}
	}
	return notseen;
}

OtherParamsUnique::~OtherParamsUnique()
{
	if (seen_modes) free(seen_modes);
}


bool 
DisplayShapeClosest::operator()(const display_mode *dm)
{
	bool	best = false;

	if(first)
	{
		best = true;
		first = false;
	}
	else
	{
		uint32	newdist = (uint32)(dm->timing.h_display - width) * (uint32)(dm->timing.h_display - width) +
			(uint32)(dm->timing.v_display - height) * (uint32)(dm->timing.v_display - height);
		uint32	olddist = (uint32)(bestwidth - width) * (uint32)(bestwidth - width) +
			(uint32)(bestheight - height) * (uint32)(bestheight - height);
		best = newdist < olddist;
	}

	if(best)
	{
		bestwidth = dm->timing.h_display;
		bestheight = dm->timing.v_display;
	}

	return best;
}

bool 
VirtualShapeClosest::operator()(const display_mode *dm)
{
	bool	best = false;

	if(first)
	{
		best = true;
		first = false;
	}
	else
	{
		uint32	newdist = (uint32)(dm->virtual_width - width) * (uint32)(dm->virtual_width - width) +
			(uint32)(dm->virtual_height - height) * (uint32)(dm->virtual_height - height);
		uint32	olddist = (uint32)(bestwidth - width) * (uint32)(bestwidth - width) +
			(uint32)(bestheight - height) * (uint32)(bestheight - height);
		best = newdist < olddist;
	}

	if(best)
	{
		bestwidth = dm->virtual_width;
		bestheight = dm->virtual_height;
	}

	return best;
}

bool 
RefreshRateClosest::operator()(const display_mode *dm)
{
	bool	best = false;
	double	dmRate = rate_from_display_mode(dm);

	if(first)
	{
		best = true;
		first = false;
	}
	else
		// check if current is better than the best
		best = fabs(dmRate - rate) < fabs(bestrate - rate);

	if(best)
		bestrate = dmRate;

//printf("original: %f, current: %f, best: %f (isbest: %d)\n", rate, dmRate, bestrate, best);

	return best;
}

bool 
PixelConfigClosest::operator()(const display_mode *dm)
{
	return dm->space == space;
}

bool 
OtherParamsClosest::operator()(const display_mode *dm)
{
	return modes_match(dm, &mode);
}
