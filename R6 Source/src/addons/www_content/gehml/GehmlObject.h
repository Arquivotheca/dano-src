
#ifndef _GEHMLOBJECT_H_
#define _GEHMLOBJECT_H_

#include <Binder.h>
#include <URL.h>
#include <SecurityManager.h>
#include <XMLExpressible.h>
#include "GehmlDefs.h"
#include "GehmlLayout.h"

class GehmlObject : public BinderNode
{
	public:
										GehmlObject();
										GehmlObject(BStringMap &attributes);
		virtual							~GehmlObject();

				void					Init();
		virtual	void					Acquired();

				int32					ID() const { return m_ID; };

		virtual	property				Parent();
		virtual	status_t				SetParent(const binder_node &parent);
				gehml_group_ref			LocalParent() const;

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

		virtual	void					GetConstraints(int32 axis, GehmlConstraint &constraint) const;
		inline	void					GetXYConstraints(GehmlConstraints &constraint) const;
		virtual	void					ConstraintsChanged();

		virtual	void					SetSize(BPoint size);
		virtual	void					SetPosition(BPoint location);
				void					SetFrame(BRect frame);

		virtual	int32					PropagateNeeds(int32 needFlags, const gehml_obj &from);
				void					NeedDraw() { PropagateNeeds(nfDraw,NULL); };
				void					NeedConstrain() { PropagateNeeds(nfConstrain,NULL); };
				void					NeedLayout() { PropagateNeeds(nfLayout,NULL); };
				int32					TendToNeeds();

				void					Trace(const char *msg, ...);

	protected:

				status_t				SetNamespace(binder_node ns);
				GehmlConstraints *		Constraints();

		bool							Lock() const;
		void							Unlock() const;

		binder_node						m_namespace;

	private:

		static int32					m_nextID;
		int32							m_needs;
		int32							m_ID;
		NestedGehnaphore				m_lock;
		GehmlConstraints *				m_constraints;
		gehml_group_ref					m_parent;

#if DEBUG_LAYOUT
	public:
		virtual void					DumpInfo(int32 indent);
		BString							DebugName() { return m_debugName; };

	private:
		BString							m_debugName;
#endif
};

inline void 
GehmlObject::GetXYConstraints(GehmlConstraints &constraints) const
{
	GetConstraints(HORIZONTAL,constraints.axis[HORIZONTAL]);
	GetConstraints(VERTICAL,constraints.axis[VERTICAL]);
}

#endif
