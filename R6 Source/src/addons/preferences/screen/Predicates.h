#if !defined(_PREDICATES_H_)
#define _PREDICATES_H

#include <Accelerant.h>
#include <utility>

#include "screen_utils.h"

class Predicate {
public:
	virtual bool	operator()(const display_mode *dm) = 0;
	virtual			~Predicate() {};
};

class DisplayShapePredicate : public Predicate {
public:
					DisplayShapePredicate(uint16 width, uint16 height)
							: width(width), height(height) {};
					DisplayShapePredicate(const display_mode *dm)
							: width(dm->timing.h_display), height(dm->timing.v_display) {};

	virtual bool	operator()(const display_mode *dm);
private:
	uint16			width;
	uint16			height;
};

class VirtualShapePredicate : public Predicate {
public:
					VirtualShapePredicate(uint16 width, uint16 height)
							: width(width), height(height) {};
					VirtualShapePredicate(const display_mode *dm)
							: width(dm->virtual_width), height(dm->virtual_height) {};
	virtual bool	operator()(const display_mode *dm);
private:
	uint16			width;
	uint16			height;
};

class RefreshRatePredicate : public Predicate {
public:
					RefreshRatePredicate(double rate, double epsilon = PREDICATE_EPSILON)
						: rate(rate), epsilon(epsilon) {};
					RefreshRatePredicate(const display_mode *dm, double epsilon = PREDICATE_EPSILON)
						: rate(rate_from_display_mode(dm)), epsilon(epsilon) {};
	virtual bool	operator()(const display_mode *dm);
private:
	double			rate;
	double			epsilon;
};

class PixelConfigPredicate : public Predicate {
public:
					PixelConfigPredicate(uint32 space) : space(space) {};
					PixelConfigPredicate(const display_mode *dm) : space(dm->space) {};
	virtual bool	operator()(const display_mode *dm);
private:
	uint32			space;
};

class OtherParamsPredicate : public Predicate {
public:
					OtherParamsPredicate(const display_mode *dm) : mode(*dm) {};
	virtual bool	operator()(const display_mode *dm);
private:
	display_mode	mode;
};

#if 0
class UniquePredicate : public Predicate {
public:
					UniquePredicate(void) : shape_count(0), seen_shapes(0) {};
	virtual			~UniquePredicate();
	virtual bool	operator()(const display_mode *dm);
protected:
	virtual bool	compare(void *candidate, const display_mode *dm);
	virtual void	assign(void *candidate, const display_mode *dm);
	virtual size_t	size_of_seen(void);
private:
	size_t			unique_count;
	void			*unique_shapes;
};

class DisplayShapeUnique : public UniqePredicate {
protected:
	virtual bool	compare(void *candidate, const display_mode *dm);
	virtual void	assign(void *candidate, const display_mode *dm);
	virtual size_t	size_of_seen(void);
};
#endif

class DisplayShapeUnique : public Predicate {
public:
					DisplayShapeUnique(void) : shape_count(0), seen_shapes(0) {};
	virtual			~DisplayShapeUnique();
	virtual bool	operator()(const display_mode *dm);
private:
	size_t			shape_count;
	struct ss {
		uint16		width;
		uint16		height;
	}				*seen_shapes;
};

class VirtualShapeUnique : public Predicate {
public:
					VirtualShapeUnique(void) : shape_count(0), seen_shapes(0) {};
	virtual			~VirtualShapeUnique();
	virtual bool	operator()(const display_mode *dm);
private:
	size_t			shape_count;
	struct ss {
		uint16		width;
		uint16		height;
	}				*seen_shapes;
};

class RefreshRateUnique : public Predicate {
public:
					RefreshRateUnique(double epsilon = PREDICATE_EPSILON) : rate_count(0), seen_rates(0), epsilon(epsilon) {};
	virtual			~RefreshRateUnique();
	virtual bool	operator()(const display_mode *dm);
private:
	size_t			rate_count;
	double			*seen_rates;
	double			epsilon;
};

class PixelConfigUnique : public Predicate {
public:
					PixelConfigUnique() : config_count(0), seen_configs(0) {};
	virtual			~PixelConfigUnique();
	virtual bool	operator()(const display_mode *dm);
private:
	size_t			config_count;
	uint32			*seen_configs;
};

class OtherParamsUnique : public Predicate {
public:
					OtherParamsUnique() : mode_count(0), seen_modes(0) {};
	virtual			~OtherParamsUnique();
	virtual bool	operator()(const display_mode *dm);
private:
	size_t			mode_count;
	display_mode	*seen_modes;
};

class DisplayShapeClosest : public Predicate {
public:
					DisplayShapeClosest(uint16 width, uint16 height)
							: width(width), height(height), first(true) {};
					DisplayShapeClosest(const display_mode *dm)
							: width(dm->timing.h_display), height(dm->timing.v_display), first(true) {};

	virtual bool	operator()(const display_mode *dm);
private:
	uint16			width;
	uint16			height;
	uint16			bestwidth;
	uint16			bestheight;
	bool			first;
};

class VirtualShapeClosest : public Predicate {
public:
					VirtualShapeClosest(uint16 width, uint16 height)
							: width(width), height(height), first(true) {};
					VirtualShapeClosest(const display_mode *dm)
							: width(dm->virtual_width), height(dm->virtual_height), first(true) {};
	virtual bool	operator()(const display_mode *dm);
private:
	uint16			width;
	uint16			height;
	uint16			bestwidth;
	uint16			bestheight;
	bool			first;
};

class RefreshRateClosest : public Predicate {
public:
					RefreshRateClosest(double rate)
						: rate(rate), first(true) {};
					RefreshRateClosest(const display_mode *dm)
						: rate(rate_from_display_mode(dm)), first(true) {};
	virtual bool	operator()(const display_mode *dm);
private:
	double			rate;
	double			bestrate;
	bool			first;
};

class PixelConfigClosest : public Predicate {
public:
					PixelConfigClosest(uint32 space) : space(space), first(true) {};
					PixelConfigClosest(const display_mode *dm) : space(dm->space), first(true) {};
	virtual bool	operator()(const display_mode *dm);
private:
	uint32			space;
	uint32			bestspace;
	bool			first;
};

class OtherParamsClosest : public Predicate {
public:
					OtherParamsClosest(const display_mode *dm) : mode(*dm) {};
	virtual bool	operator()(const display_mode *dm);
private:
	display_mode	mode;
};

#endif
