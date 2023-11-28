
#ifndef _INTERFACE2_VIEW_H_
#define _INTERFACE2_VIEW_H_

#include <InterfaceDefs.h>
#include <Binder.h>
#include <Layout.h>
#include <LayoutConstraints.h>

namespace B {
namespace Interface2 {

class BView : virtual public BBinder
{
	public:
										BView();
										BView(const BMessage &attributes);
		virtual							~BView();

				void					Init();
		virtual	void					Acquired();

				int32					ID() const { return m_ID; };

		virtual	property				Parent();
		virtual	status_t				SetParent(const binder &parent);
				viewgroup_ptr			LocalParent() const;

				property				Namespace();

		virtual	status_t				OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t				NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t				CloseProperties(void *cookie);

		virtual	put_status_t			WriteProperty(const char *name, const property &prop);
		virtual	get_status_t			ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		virtual	bool					Constrain();
		virtual	bool					Position(layoutbuilder_t layout, BRegion &outDirty);
		virtual	void					Clean(layout_t layout, BRegion &outWasDirty);
		virtual	void					Draw(layout_t layout, BDrawable &into, const BRegion &dirty);

		virtual	void					GetConstraints(int32 axis, BLayoutConstraint &constraint) const;
		inline	void					GetXYConstraints(BLayoutConstraints &constraint) const;
		virtual	void					ConstraintsChanged();

		virtual	void					SetSize(BPoint size);
		virtual	void					SetPosition(BPoint location);
				void					SetFrame(BRect frame);

		virtual	int32					PropagateNeeds(int32 needFlags, const view_ptr &from);
				void					NeedDraw() { PropagateNeeds(nfDraw,NULL); };
				void					NeedConstrain() { PropagateNeeds(nfConstrain,NULL); };
				void					NeedLayout() { PropagateNeeds(nfLayout,NULL); };
				int32					TendToNeeds();

				void					Trace(const char *msg, ...);

	protected:

				status_t				SetNamespace(binder ns);
				BLayoutConstraints *	Constraints();

		bool							Lock() const;
		void							Unlock() const;

		binder							m_namespace;

	private:

		static int32					m_nextID;
		int32							m_needs;
		int32							m_ID;
		mutable BNestedLocker			m_lock;
		BLayoutConstraints *			m_constraints;
		viewgroup_ref					m_parent;

#if DEBUG_LAYOUT
	public:
		virtual void					DumpInfo(int32 indent);
		BString							DebugName() { return m_debugName; };

	private:
		BString							m_debugName;
#endif
};

inline void 
BView::GetXYConstraints(BLayoutConstraints &constraints) const
{
	GetConstraints(B_HORIZONTAL,constraints.axis[B_HORIZONTAL]);
	GetConstraints(B_VERTICAL,constraints.axis[B_VERTICAL]);
}

} } // namespace B::Interface2

using namespace B::Interface2;

#endif	/* _INTERFACE2_VIEW_H_ */
