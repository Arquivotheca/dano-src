/*	_BMSystemTheme.cpp	*/

#include <Debug.h>
#include <list>
#include <algobase.h>
#include <assert.h>

#include <File.h>
#include <Bitmap.h>
#include <TextView.h>
#include <Slider.h>
#include <Button.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <OptionPopUp.h>
#include <TabView.h>
#include <Box.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <string.h>
#include <math.h>
#include <Screen.h>
#include <MediaRoster.h>

#include "ParameterWeb.h"
#include <ChannelSlider.h>
//#include "TunerControl.h"


#include "_BMSystemTheme.h"

#if !DEBUG
#define FPRINTF(x) (void)0
#else
#define FPRINTF(x) fprintf x
#endif


#define _B_MEDIA_VALUE_CHANGED '_MV_'


static rgb_color
scale(
	rgb_color value,
	uchar target_source,
	uchar target_destination)
{
	int red = value.red * target_destination / target_source;
	int green = value.green * target_destination / target_source;
	int blue = value.blue * target_destination / target_source;
	rgb_color ret = {
		red < 0 ? 0 : red > 255 ? 255 : red , 
		green < 0 ? 0 : green > 255 ? 255 : green , 
		blue < 0 ? 0 : blue > 255 ? 255 : blue ,
		255
	};
	return ret;
}


static rgb_color the_color = { 216, 216, 216, 255 };

	static float
	map_value(
		int32 value,
		BParameter * param,
		BControl * control)
	{
		BChannelControl * mcControl = dynamic_cast<BChannelControl *>(control);
		BSlider * slidControl = dynamic_cast<BSlider *>(control);
		if (slidControl == NULL && mcControl == NULL)
			return value;
		int32 limit_min = 0, limit_max = 11;
		if (mcControl != NULL)
			mcControl->GetLimitsFor(0, &limit_min, &limit_max);
		else if (slidControl != NULL)
			slidControl->GetLimits(&limit_min, &limit_max);
		if (abs((int)(limit_max - limit_min)) < 1)
			return 0.0;
		float fraction = ((float)value-limit_min)/((float)limit_max-limit_min);
		BContinuousParameter * cp = dynamic_cast<BContinuousParameter *>(param);
		assert(cp != NULL);
#if 0
		int response = 0;
		float factor;
		float offset;
		cp->GetResponse(&response, &factor, &offset);
#endif
		float ret = fraction * (cp->MaxValue()-cp->MinValue()) + cp->MinValue();
//		fprintf(stderr, "value %d  min %d  max %d  fraction %f  ret %f\n",
//			value, limit_min, limit_max, fraction, ret);
		return ret;
	}

	static int32
	unmap_value(
		BContinuousParameter * param,
		BChannelControl * mcControl,
		float value)
	{
		float diff = param->MaxValue() - param->MinValue();
		if (diff == 0.0) diff = 1.0;
		float fraction = (value - param->MinValue()) / diff;
		int32 minval = 0, maxval = 11;
		mcControl->GetLimitsFor(0, &minval, &maxval);
		return (int32)floor(fraction * (maxval - minval)) + minval;
	}

	static int32
	unmap_value(
		BContinuousParameter * param,
		BSlider * slider,
		float value)
	{
		float diff = param->MaxValue() - param->MinValue();
		if (diff == 0.0) diff = 1.0;
		float fraction = (value - param->MinValue()) / diff;
		int32 minval = 0, maxval = 11;
		slider->GetLimits(&minval, &maxval);
		int32 ret = (int32)floor(fraction * (maxval - minval)) + minval;
//		fprintf(stderr, "value %f  minval %f  maxval %f  fraction %f  param(%f - %f) return %f\n",
//			(float)value, (float)minval, (float)maxval, (float)fraction,
//			(float)param->MinValue(), (float)param->MaxValue(), (float)ret);
		return ret;
	}

	static void
	unmap_control(
		BParameter * param,
		BControl * control)
	{
		BChannelControl * mcControl = dynamic_cast<BChannelControl *>(control);
		bigtime_t time = 0;
		if (mcControl != NULL)
		{
			assert(param->Type() == BParameter::B_CONTINUOUS_PARAMETER);
			int channel_count = param->CountChannels();
			float * values = new float[channel_count];
			int32 * set_val = new int32[channel_count];
			size_t iosize = sizeof(float)*channel_count;
			param->GetValue(values, &iosize, &time);
			BContinuousParameter * cp = dynamic_cast<BContinuousParameter *>(param);
			assert(cp != NULL);
			for (int ix=0; ix<channel_count; ix++)
				set_val[ix] = unmap_value(cp, mcControl, values[ix]);
			mcControl->SetValue(0, channel_count, set_val);
			delete[] values;
			delete[] set_val;
		}
		else if (control != NULL)
		{
			int32 value = 0;
			if (param->Type() == BParameter::B_CONTINUOUS_PARAMETER)
			{
				BContinuousParameter * cp = dynamic_cast<BContinuousParameter *>(param);
				assert(cp != 0);
				float f;
				size_t iosize = sizeof(float);
				param->GetValue(&f, &iosize, &time);
				BSlider * slid = dynamic_cast<BSlider *>(control);
				if (slid != NULL)
					value = unmap_value(cp, slid, f);
				else
					value = (int32)f;
			}
			else if (param->Type() == BParameter::B_DISCRETE_PARAMETER)
			{
				size_t iosize = sizeof(int32);
				param->GetValue(&value, &iosize, &time);
			}
			else
			{
				//	ignore this kind of control
				FPRINTF((stderr, "SystemTheme: ignoring control type 0x%x in unmap_control()\n",
						param->Type()));
			}
			if (dynamic_cast<BOptionPopUp *>(control) != NULL)
			{
				FPRINTF((stderr, "This is a BOptionPopUp\n"));
			}
			control->SetValue(value);
		}
	}

class _BParamFilter :
		public BMessageFilter
{
public:
		_BParamFilter() :
			BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
			{
				m_web = 0;
			}
		filter_result Filter(
				BMessage * message,
				BHandler ** target)
			{
				if (message->what == _B_MEDIA_VALUE_CHANGED) {
					BControl * control = NULL;
					BParameter * param = NULL;
					int32 value = 0;
					if (message->FindPointer("source", (void **)&control))
						return B_DISPATCH_MESSAGE;
					BChannelControl * mcControl = dynamic_cast<BChannelControl *>(control);
					if (message->FindPointer("be:_parameter", (void **)&param))
						return B_DISPATCH_MESSAGE;
					if (message->FindInt32("be:value", &value))
						return B_DISPATCH_MESSAGE;
					if (param->Type() == BParameter::B_DISCRETE_PARAMETER)
					{
						FPRINTF((stderr, "SetValue(%d) (%s)\n", value, param->Name()));
						if (cache_new_value(param, &value, B_INT32_TYPE, 1)) {
							param->SetValue(&value, sizeof(value), -1LL);
						}
					}
					else if (param->Type() == BParameter::B_CONTINUOUS_PARAMETER)
					{
						int channel_count = param->CountChannels();
						float * ptr = new float[channel_count];
						for (int ix=0; ix<channel_count; ix++)
						{
							int32 c_value;
							if (!message->FindInt32("be:channel_value", ix, &c_value))
								ptr[ix] = map_value(c_value, param, control);
							else
								ptr[ix] = map_value(value, param, control);
						}
						FPRINTF((stderr, "SetValue(%f) (%s)\n", ptr[0], param->Name()));
						if (cache_new_value(param, ptr, B_FLOAT_TYPE, channel_count)) {
							param->SetValue(ptr, sizeof(float)*channel_count, -1LL);
						}
						delete[] ptr;
					}
					else
					{
						FPRINTF((stderr, "Unknown parameter kind %d in Filter()\n",
								param->Type()));
					}
					return B_SKIP_MESSAGE;
				}
				else if (message->what == B_MEDIA_NEW_PARAMETER_VALUE) {
					//	update the value in question being displayed
					int32 paramID = 0;
					const void * value = 0;
					ssize_t size = 0;
					if (message->FindData("be:node", B_RAW_TYPE, &value, &size))
						return B_DISPATCH_MESSAGE;
					if (*(media_node *)value != m_web->Node())
						return B_DISPATCH_MESSAGE;
					if (message->FindInt32("be:parameter", &paramID))
						return B_DISPATCH_MESSAGE;
					if (message->FindData("be:value", B_RAW_TYPE, &value, &size))
						return B_DISPATCH_MESSAGE;
					//	find the parameter
					BParameter * parameter;
					int cnt = m_web->CountParameters();
					for (int ix=0; ix<cnt; ix++) {
						parameter = m_web->ParameterAt(ix);
						if (parameter->ID() == paramID) {
							break;
						}
						parameter = 0;
					}
					if (!parameter) {
						return B_DISPATCH_MESSAGE;
					}
					//	find the control -- this is slow...
					BControl * c = control_walk(parameter);
					if (c == 0) {
						return B_DISPATCH_MESSAGE;
					}
					//	unmap parameter to control
					type_code type = parameter->ValueType();
					int chCount = parameter->CountChannels();
					ASSERT(sizeof(int32) == sizeof(float));
					if (chCount*sizeof(int32) > size)
						chCount = size/sizeof(int32);
					if (cache_new_value(parameter, value, type, chCount)) {
						unmap_control(parameter, c);
					}
					return B_SKIP_MESSAGE;
				}
				else if (message->what == B_MEDIA_PARAMETER_CHANGED) {
					//	we're lazy and let the app deal with it
					return B_DISPATCH_MESSAGE;
				}
				else if (message->what == B_MEDIA_WEB_CHANGED) {
					//	there ain't much we can do; app needs to worry about it
					return B_DISPATCH_MESSAGE;
				}
				return B_DISPATCH_MESSAGE;
			}
		BParameterWeb * m_web;
private:
		struct value_item {
			enum {
				MAX_CH = 8
			};
			int count;
			union {
				int32 ival[MAX_CH];
				float fval[MAX_CH];
			};
			bool operator==(const value_item & other) const
				{
					if (count != other.count) return false;
					int ix = min<int32>(count, MAX_CH);
					for (int i=0; i<ix; i++) {
						if (ival[i] != other.ival[i])
							return false;
					}
					return true;
				}
			bool operator!=(const value_item & other) const
				{
					return !operator==(other);
				}
		};
		map<int32, value_item> mValues;
		bool cache_new_value(BParameter * p, const void * data, type_code type, int ch_count)
			{
				if ((type != B_FLOAT_TYPE) && (type != B_INT32_TYPE)) {
					return true;
				}
				value_item i;
				i.count = min<int32>(ch_count, value_item::MAX_CH);
				for (int ix=0; ix<i.count; ix++) {
					switch (type) {
					case B_FLOAT_TYPE:
						i.fval[ix] = ((const float *)data)[ix];
						break;
					case B_INT32_TYPE:
						i.ival[ix] = ((const int32 *)data)[ix];
						break;
					}
				}
				map<int32, value_item>::iterator ptr(mValues.find(p->ID()));
				if ((ptr == mValues.end()) || ((*ptr).second != i)) {
					mValues[p->ID()] = i;
					return true;
				}
				return false;
			}
		BControl * control_walk(BParameter * p)
			{
				//	Recursively walk a window, looking for a control whose message
				//	includes the specified parameter.
				BWindow * w = dynamic_cast<BWindow *>(Looper());
				if (w == 0) return 0;
				BView * v;
				for (int ix=0; (v = w->ChildAt(ix)) != 0; ix++) {
					BControl * c = control_walk(v, p, 1);
					if (c != 0)
						return c;
				}
				return 0;
			}
		BControl * control_walk(BView * v, BParameter * p, int depth)
			{
				//	Recursively walk a view, looking for a control whose message
				//	includes the specified parameter.
				BControl * c = dynamic_cast<BControl *>(v);
				if (c != 0) {
					const BParameter * op;
					BMessage * m = c->Message();
					if ((m != 0) && !m->FindPointer("be:_parameter", (void **)&op) &&
							(op == p))
						return c;
				}
				BView * v2;
				for (int ix=0; (v2 = v->ChildAt(ix)) != 0; ix++) {
					c = control_walk(v2, p, depth+1);
					if (c != 0)
						return c;
				}
				return 0;
			}
};


class _BFancyTitle :
		public BView
{
public:
		_BFancyTitle(
				int fanciness,
				const BRect & frame, 
				const char * name, 
				const char * label,
				uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW) :
			BView(frame, name, resize, flags)
			{
				m_fanciness = fanciness;
				if (!label)
					m_str = (char *)calloc(1, 1);
				m_str = strdup(label);
				m_separation_x = 1.0;
				m_separation_y = 1.0;
			}
		~_BFancyTitle()
			{
				free(m_str);
			}
		void Draw(
				BRect area)
			{
				BFont f;
				GetFont(&f);
				font_height fh;
				f.GetHeight(&fh);
				float base = fh.ascent+fh.leading;
				rgb_color hi = HighColor();
				rgb_color lo = LowColor();
				SetDrawingMode(B_OP_COPY);
				SetLowColor(ViewColor());
				if (m_fanciness > 0)
				{
					SetHighColor(lo);
					DrawString(m_str, BPoint(m_separation_x, base+m_separation_y));
					SetHighColor(hi);
					SetDrawingMode(B_OP_OVER);
					DrawString(m_str, BPoint(0.0, base));
				}
				else
					DrawString(m_str, BPoint(1.0, base));
				SetDrawingMode(B_OP_COPY);
				SetLowColor(lo);
			}
		void GetPreferredSize(
				float * width,
				float * height)
			{
				BFont f;
				GetFont(&f);
				font_height fh;
				f.GetHeight(&fh);
				*height = fh.ascent+fh.descent+fh.leading;
				*width = f.StringWidth(m_str);
				if (m_fanciness > 0)
				{
					*width += m_separation_x;
					*height += m_separation_y;
				}
				else
				{
					*width += 1;
				}
			}
		void ResizeToPreferred()
			{
				float width = 100, height = 20;
				GetPreferredSize(&width, &height);
				ResizeTo(width, height);
			}
private:
		char * m_str;
		float m_separation_x;
		float m_separation_y;
		int m_fanciness;
};


class _BMSTStorage
{
	friend class _BMSystemTheme;
public:
		_BMSTStorage()
			{
				m_web = NULL;
				m_filter = NULL;
			}
		~_BMSTStorage()
			{
				if ((m_filter != NULL) && (m_filter->Looper() != NULL))
					m_filter->Looper()->RemoveFilter(m_filter);
				delete m_filter;
				delete m_web;
			}
protected:
		BParameterWeb * m_web;
		_BParamFilter * m_filter;
};

class _BFrameView :
		public BView,
		public _BMSTStorage
{
	friend class _BMSystemTheme;
public:
		_BFrameView(
				const BRect & frame,
				const char * name,
				uint32 resize,
				uint32 flags) :
			BView(frame, name, resize, flags)
			{
				m_topCoord = 0;
			}
		void Draw(
				BRect area)
			{
				rgb_color hi = scale(ViewColor(), 216, 255);
				rgb_color lo = scale(ViewColor(), 216, 120);
				rgb_color h = HighColor();
#if 0
				float y1 = m_topCoord;
				float y2 = Bounds().bottom;
				//	this calculation of y2 should be done only once...
				for (int ix=0; ix<CountChildren(); ix++)
				{
					BView * v = ChildAt(ix);
					if ((dynamic_cast<_BFrameView *>(v) == NULL) &&
							(dynamic_cast<_BFancyTitle *>(v) == NULL))
					{
						if (y2 > v->Frame().top-5.0)
							y2 = v->Frame().top-5.0;
					}
				}
#endif
				for (int ix=0; ix<CountChildren()-1; ix++)
				{
					_BFrameView * v1 = dynamic_cast<_BFrameView *>(ChildAt(ix));
					_BFrameView * v2 = dynamic_cast<_BFrameView *>(ChildAt(ix+1));
					if (v2 != NULL && v1 != NULL && v2->Frame().left > v1->Frame().right)
					{
						float x = floor((v1->Frame().right+v2->Frame().left)/2);
						float y1 = min(v1->Frame().top, v2->Frame().top);
						float y2 = max(v1->Frame().bottom, v2->Frame().bottom);
						SetHighColor(lo);
						StrokeLine(BPoint(x,y1), BPoint(x,y2));
						SetHighColor(hi);
						StrokeLine(BPoint(x+1,y1), BPoint(x+1,y2));
					}
				}
				SetHighColor(h);
			}
		void SetTop(
				float top)
			{
				m_topCoord = top;
			}
		void AllAttached()
			{
				BView::AllAttached();
				if (m_filter != NULL) {
					Window()->AddFilter(m_filter);
					BMessenger msgr(Window());
					status_t err = BMediaRoster::CurrentRoster()->StartWatching(msgr, m_web->Node(), B_MEDIA_WILDCARD);
				}
			}
		void AllDetached()
			{
				BView::AllDetached();
				if (m_filter != NULL) {
					BMessenger msgr(Window());
					status_t err = BMediaRoster::CurrentRoster()->StopWatching(msgr, m_web->Node(), B_MEDIA_WILDCARD);
					Window()->RemoveFilter(m_filter);
				}
			}
private:
		float m_topCoord;
};

class _BMTabView :
		public BTabView,
		public _BMSTStorage
{
public:
		_BMTabView(
				BRect frame,
				const char *name,
				button_width width=B_WIDTH_AS_USUAL,
				uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_FULL_UPDATE_ON_RESIZE |
						B_WILL_DRAW | B_NAVIGABLE_JUMP |
						B_FRAME_EVENTS | B_NAVIGABLE) :
			BTabView(frame, name, width, resizingMode, flags)
			{
			}
		void AllAttached()
			{
				BTabView::AllAttached();
				if (m_filter != NULL) {
					Window()->AddFilter(m_filter);
					BMessenger msgr(Window());
					status_t err = BMediaRoster::CurrentRoster()->StartWatching(msgr, m_web->Node(), B_MEDIA_WILDCARD);
				}
			}
		void AllDetached()
			{
				BTabView::AllDetached();
				if (m_filter != NULL) {
					BMessenger msgr(Window());
					status_t err = BMediaRoster::CurrentRoster()->StopWatching(msgr, m_web->Node(), B_MEDIA_WILDCARD);
					Window()->RemoveFilter(m_filter);
				}
			}
		virtual void Select(int32 tabNum) 
			{
				BTabView::Select(tabNum);
				BView * v = ViewForTab(tabNum);
				if (v) {
					BRect r(ContainerView()->Bounds());
					r.InsetBy(1,1);
					v->MoveTo(r.LeftTop());
					v->ResizeTo(r.Width(), r.Height());
#if DEBUG
					v->Frame().PrintToStream();
#endif
				}
			}

		virtual void AddTab(
				BView* tabContents,
				BTab* tab=NULL)
			{
				BRect r(ContainerView()->Bounds());
				r.InsetBy(1,1);
				tabContents->MoveTo(r.LeftTop());
				tabContents->ResizeTo(r.Width(), r.Height());
#if DEBUG
				tabContents->Frame().PrintToStream();
#endif
				BTabView::AddTab(tabContents, tab);
			}
};


_BMSystemTheme::_BMSystemTheme() :
	BMediaTheme("BeOS Theme", "BeOS version 4.1 built-in Theme")
{
}


_BMSystemTheme::~_BMSystemTheme()
{
}


BControl *
_BMSystemTheme::MakeControlFor(
	BParameter * control)
{
	BControl * c = MakeFallbackViewFor(control);
	FPRINTF((stderr, "MakeControlFor(%s) returns 0x%x (%s)\n",
			control->Name(), c, c ? typeid(*c).name() : NULL));
	if (c != NULL)
	{
		BMessage * msg = new BMessage(_B_MEDIA_VALUE_CHANGED);
		msg->AddPointer("be:_parameter", control);
		c->SetMessage(msg);
		BSlider * slid = dynamic_cast<BSlider *>(c);
		if (slid != NULL)
			slid->SetModificationMessage(new BMessage(*msg));
		c->SetViewColor(the_color);
		unmap_control(control, c);
	}
	return c;
}


BView *
_BMSystemTheme::MakeViewFor(
	BParameterWeb * web,
	const BRect * hintRect)
{
	list<BView *> group_views;
	//	the last tab/group added is remembered (so we can return it if there's only one)
	BView * group = NULL;
	bool has_label = false;
	for (int ix=0; ix<web->CountGroups(); ix++)
	{
		group = MakeViewFor(web->GroupAt(ix), NULL);
		if (group != NULL)
		{
			group_views.push_back(group);
			if (group->Name() && group->Name()[0])
				has_label = true;
		}
	}

	//	figure out whether to use a tab view
	BView * ret = NULL;
	if (group_views.size() > 0)
	{
		if (!has_label || group_views.size() == 1)
		{	//	just use the thing
			ret = *group_views.begin();
		}
		else
		{	//	we use a tab view if there are more than one top-level group
			BRect frame(0,0,0,0);
			for (list<BView *>::iterator ptr(group_views.begin());
					ptr != group_views.end(); ptr++)
				frame = frame | (*ptr)->Frame();
			frame.right += 5.0+6.0;
			frame.bottom += 5.0+24.0;	//	fake TabHeight() because we don't know yet
			//	find the name
			media_node node = web->Node();
			live_node_info live;
			BMediaRoster::Roster()->GetLiveNodeInfo(node, &live);
			_BMTabView * tab_view = new _BMTabView(frame, live.name,
					B_WIDTH_AS_USUAL, B_FOLLOW_LEFT | B_FOLLOW_TOP);
			tab_view->SetViewColor(the_color);
			for (list<BView *>::iterator ptr(group_views.begin());
					ptr != group_views.end(); ptr++)
				tab_view->AddTab(*ptr);
			//	make sure the view is big enough to show all the tabs
			BRect r(0,0,0,0);
			for (int ix=0; ix<tab_view->CountTabs(); ix++)
				r = r | tab_view->TabFrame(ix);
//			r.PrintToStream();
			if (r.right > tab_view->Bounds().right)
			{
//				tab_view->Bounds().PrintToStream();
				tab_view->ResizeBy(r.right-tab_view->Bounds().right, 0);
				BRect r = tab_view->ContainerView()->Bounds();
				r.InsetBy(1, 1);
				for (int ix=0; ix<tab_view->CountTabs(); ix++)
				{
					tab_view->ViewForTab(ix)->MoveTo(r.LeftTop());
					tab_view->ViewForTab(ix)->ResizeTo(r.Width(), r.Height());
				}
			}
			ret = tab_view;
		}
	}

	if (ret != NULL)
	{
		ret->SetResizingMode(B_FOLLOW_ALL);
		_BMSTStorage * stor = dynamic_cast<_BMSTStorage *>(ret);
		if (stor != NULL)
		{
			stor->m_web = web;
			stor->m_filter = new _BParamFilter;
			stor->m_filter->m_web = web;
		}
	}

	FPRINTF((stderr, "MakeViewFor(BParameterWeb *) for %d groups returns 0x%x\n",
			group_views.size(), ret));
	FPRINTF((stderr, "Frame is (%g,%g,%g,%g)\n", ret->Frame().left, ret->Frame().top,
			ret->Frame().right, ret->Frame().bottom));

	//	return what we've got
	return ret;
}


BView *
_BMSystemTheme::MakeViewFor(
	BParameterGroup* group,
	const BRect * hintRect)
{
	list<BView *> sub_groups;
	list<BView *> sub_controls;

	float horiz = -1.0;
	float vert = -1.0;
	BView * title = NULL;
	//	get a title, but only if we aren't the only top-level group
#if 0
	if (group->Name() && group->Name()[0] &&
			!((group->Web()->CountGroups() == 1) && (group->Web()->GroupAt(0) == group)))
	{
		font_height fh;
		be_plain_font->GetHeight(&fh);
		float height = fh.ascent+fh.descent+fh.leading;
		float width = be_plain_font->StringWidth(group->Name());
		BStringView * sv = new BStringView(BRect(5.0, 5.0, 5.0+width, 5.0+height),
				"_label", group->Name());
		sv->SetViewColor(the_color);
		sv->SetFont(be_plain_font);
		title = sv;
//		horiz = sv->Frame().right;
		vert = sv->Frame().bottom;
	}
#else
	BParameter * titleParam = NULL;
	if (!((group->Web()->CountGroups() == 1) && (group->Web()->GroupAt(0) == group)))
	{
		for (int ix=0; ix<group->CountParameters(); ix++)
		{
			if ((group->ParameterAt(ix)->Type() == BParameter::B_NULL_PARAMETER) &&
					(group->ParameterAt(ix)->Name() != NULL) &&
					(group->ParameterAt(ix)->Name()[0] != 0))
			{
				titleParam = group->ParameterAt(ix);
				break;
			}
		}
	}
	if (titleParam != NULL)
	{
		font_height fh;
		be_plain_font->GetHeight(&fh);
		float height = fh.ascent+fh.descent+fh.leading;
		float width = be_plain_font->StringWidth(titleParam->Name());
		_BFancyTitle * sv = new _BFancyTitle(1, BRect(5.0, 5.0, 5.0+width, 5.0+height),
				"_label", titleParam->Name());
		sv->SetViewColor(the_color);
		sv->SetHighColor(128, 48, 48);
		sv->SetFont(be_plain_font);
		sv->ResizeToPreferred();
		title = sv;
//		horiz = sv->Frame().right;
		vert = sv->Frame().bottom;
	}
#endif

	//	put all groups across the top
	float group_v = vert+6.0;
	for (int ix=0; ix<group->CountGroups(); ix++)
	{
		BParameterGroup * grp = group->GroupAt(ix);
		BView * v = MakeViewFor(grp, NULL);
		if (v != NULL)
		{
			v->MoveTo(horiz+6.0, group_v);
			horiz = v->Frame().right;
			if (v->Frame().bottom > vert)
				vert = v->Frame().bottom;
			sub_groups.push_back(v);
		}
	}

	//	put all controls vertically below any groups
	bool has_slider = false;
	for (int ix=0; ix<group->CountParameters(); ix++)
	{
		BParameter * param = group->ParameterAt(ix);
		if (param != titleParam)
		{
			BView * p = NULL;
			if (param->Type() == BParameter::B_NULL_PARAMETER)
			{
				//	we don't make labels for things connected to muxes
				bool deny = false;
				for (int ix=0; ix<param->CountOutputs(); ix++)
					if (!strcmp(param->OutputAt(ix)->Kind(), B_INPUT_MUX))
					{
						deny = true;
						break;
					}
				for (int ix=0; ix<param->CountInputs(); ix++)
					if (!strcmp(param->InputAt(ix)->Kind(), B_OUTPUT_MUX))
					{
						deny = true;
						break;
					}
				if (!deny)
				{
					p = new _BFancyTitle(0, BRect(0,0,30,15), "_info", param->Name());
					p->SetViewColor(the_color);
					p->ResizeToPreferred();
				}
			}
			else
				p = MakeControlFor(param);
			if (p != NULL)
			{
				p->MoveTo(5.0, vert+6.0);
				vert = p->Frame().bottom;
				if (p->Frame().right > horiz)
					horiz = p->Frame().right;
				sub_controls.push_back(p);
				if (dynamic_cast<BChannelSlider *>(p) != NULL)
					has_slider = true;
			}
		}
	}

	//	center controls if there is a slider
	if (title != NULL)
	{
		if (horiz < title->Frame().right)
			horiz = title->Frame().right;
	}
	if (has_slider)
	{
		if (title != NULL)
			title->MoveBy(floor((horiz - 4 - title->Frame().Width()) / 2), 0);

		for (list<BView *>::iterator ptr(sub_controls.begin());
				ptr != sub_controls.end(); ptr++)
			(*ptr)->MoveBy(floor((horiz - 4 - (*ptr)->Frame().Width()) / 2), 0);
	}

	//	create the view
	BView * ret = NULL;
	if ((sub_controls.size() > 0) || (sub_groups.size() > 0) || (title != NULL))
	{
		_BFrameView * v = new _BFrameView(BRect(0,0,horiz+5.0,vert+5.0), group->Name(),
				B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		v->SetViewColor(the_color);
		if (title != NULL)
		{
			v->AddChild(title);
			v->SetTop(title->Frame().bottom+5.0);
		}
		for (list<BView *>::iterator ptr(sub_groups.begin());
				ptr != sub_groups.end(); ptr++)
			v->AddChild(*ptr);
		for (list<BView *>::iterator ptr(sub_controls.begin());
				ptr != sub_controls.end(); ptr++)
			v->AddChild(*ptr);
		ret = v;
	}
	else
	{
		FPRINTF((stderr, "Returning NULL view from MakeViewFor(BParameterGroup*) '%s'\n",
				group->Name()));
	}

	//	return it
	return ret;
}


