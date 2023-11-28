
#if !defined(Gizmos_h)
#define Gizmos_h

#include <View.h>

class Gizmo
{
public:
		Gizmo(const BRect & area);
		virtual ~Gizmo();

		virtual void	Draw(BView * parent);
		bool			Contains(BPoint where) { return m_frame.Contains(where); }
		BRect			Frame() { return m_frame; } 
		virtual void	Click(BView * parent, BPoint where);

private:

		BRect			m_frame;
};

class PlayGizmo
	: public Gizmo
{
public:
		PlayGizmo(const BRect & area);
		~PlayGizmo();

		void			Draw(BView * parent);
		void			Click(BView * parent, BPoint where);

		enum
		{
			kOff,
			kOn,
			kClicked
		};
		int32			State() { return m_state; }
		void			SetState(int32 state) { m_state = state; }
private:
		int32			m_state;
		BBitmap *		m_graphics;
};

class PositionGizmo
	: public Gizmo
{
public:
		PositionGizmo(const BRect & area);

		void			Draw(BView * parent);

		void			SetPosition(float position) { m_position = position; }
private:
		float			m_position;
};


#endif	//	Gizmos_h

