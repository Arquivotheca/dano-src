
#include <OS.h>
#include <support2/StdIO.h>
#include <render2/RenderState.h>


namespace B {
namespace Render2 {

#define DEBUG_STATE_STACK	1

#if DEBUG_STATE_STACK
	static inline void STATE_STACK_ERROR(const char *s) {	debugger(s); }
#else
	static inline void STATE_STACK_ERROR(const char *s) {	bser << s; }
#endif


// --------------------------------------------------------------------------
// #pragma mark -

BRenderState::BRenderState()
	: m_prev(NULL), m_next(NULL)
{
	Revert();
}

BRenderState::BRenderState(const BRenderState& copy)
	: m_prev(NULL), m_next(NULL)
{
	// Set the last state on the stack
	const BRenderState *s = &copy;
	while (s->m_next)
		s = s->m_next;
	*this = *s;
}

BRenderState& BRenderState::operator = (const BRenderState& copy)
{
	m_transform = copy.m_transform;
	m_prev = NULL;
	m_next = NULL;
	
	// ---- dumb ass implementation ----
	memcpy(&path, &copy.path, sizeof(path));
	cntpt = copy.cntpt;
	closed = copy.closed;
	string = copy.string;
	// ---------------------------------

	return *this;
}

void BRenderState::Push(BRenderState **stack, BRenderState *object)
{
	if (*stack == NULL) {
		STATE_STACK_ERROR("Push() : No current state set!");
		return;
	}
	(*stack)->m_next = object;
	(*stack)->m_next->m_prev = (*stack);
	*stack = (*stack)->m_next;
}

void BRenderState::Pop(BRenderState **stack)
{
	BRenderState *prev = (*stack)->m_prev;
	if (prev == NULL) {
		STATE_STACK_ERROR("Pop() count doesn't match Push() count!");
		return;
	}
	prev->m_next = NULL;
	delete (*stack);
	(*stack) = prev;
}

void BRenderState::Revert()
{
	m_transform = B2dTransform::MakeIdentity();
	// ---- dumb ass implementation ----
	cntpt = 0;
	closed = false;
	string = NULL;
	// ---------------------------------
}

const B2dTransform& BRenderState::Transform()
{
	return m_transform;
}

void BRenderState::ApplyTransform(const B2dTransform& transform)
{
	m_transform = transform * m_transform;
}


} }	// namespace B::Render2

