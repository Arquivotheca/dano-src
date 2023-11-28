
#include <interface2/InterfaceDefs.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <interface2/InterfaceUtils.h>
#include <interface2/LayoutConstraints.h>

namespace B {
namespace Interface2 {

dimth dimth::undefined;
device device::screen = { 72.0 };

void 
dimth::PrintToStream()
{
	if (units == nil)
		printf("undefined");
	else {
		printf("%f%s",value,
			((units==unset)?"(unset)":
			((units==pixels)?"px":
			((units==points)?"pt":
			((units==centimeters)?"cm":
			((units==percent)?"%":
			((units==norms)?"nm":
			"(null)")))))));
	}
}

void BLayoutConstraint::PrintToStream() {
	printf("min(");
	min.PrintToStream();
	printf("), ");

	printf("max(");
	max.PrintToStream();
	printf("), ");

	printf("pref(");
	pref.PrintToStream();
	printf(")");
}

int32 count_dimths(const char *str)
{
	int32 count = 0;
	const char *p = str;
	if (!*p) count++;
	while (*p) { if (*p++ == ',') count++; };
	return count;
}

int32 parse_dimths(const char *str, dimth *values, int32 numValues, dimth::unit defUnits)
{
	dimth *lastUnitSet,*d;
	lastUnitSet = d = values;
	const char *end,*p = str;
	bool setLastValue=true;

	while (*p && ((*p == ' ') || (*p == '\n') || (*p == '\t'))) p++;

	while (*p) {
		if (*p == '*') {
			p++;
		} else if (*p == ',') {
			*d++ = dimth::undefined;
			if ((d-values) == numValues) break;
			setLastValue = true;
			p++;
		} else {
			double val = strtod(p,const_cast<char**>(&end));
			p = end;
			if (*p == '/') {
				val /= strtod(p+1,const_cast<char**>(&end));
				p = end;
			}
			d->units = dimth::unset;
			d->value = val;
			if (!strncmp(p,"px",2)) {
				d->units = dimth::pixels;
				p+=2;
			} else if (!strncmp(p,"pt",2)) {
				d->units = dimth::points;
				p+=2;
			} else if (!strncmp(p,"cm",2)) {
				d->units = dimth::centimeters;
				p+=2;
			} else if (!strncmp(p,"%",2)) {
				d->units = dimth::percent;
				p+=1;
			} else if (!strncmp(p,"nm",2)) {
				d->units = dimth::norms;
				p+=2;
			}
			
			while (*p && ((*p == ' ') || (*p == '\n') || (*p == '\t'))) p++;
			if (*p == ',') p++;
			else setLastValue = false;

			if (d->units != dimth::unset) {
				while (lastUnitSet <= d) {
					if (lastUnitSet->units == dimth::unset)
						lastUnitSet->units = d->units;
					lastUnitSet++;
				}
			}
			d++;
			if ((d-values) == numValues) break;
		}

		while (*p && ((*p == ' ') || (*p == '\n') || (*p == '\t'))) p++;
	}

	if (setLastValue) *d++ = dimth::undefined;

	while (lastUnitSet < d) {
		if (lastUnitSet->units == dimth::unset)
			lastUnitSet->units = defUnits;
		lastUnitSet++;
	}
	
	return d-values;
}

dimth 
parse_dimth(const char *str, dimth:: unit defUnits)
{
	dimth d;
	parse_dimths(str,&d,1,defUnits);
	return d;
}

} } // namespace B::Interface2
