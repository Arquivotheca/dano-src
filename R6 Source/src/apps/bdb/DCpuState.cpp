// DCpuState - architecture-independant layer for CPU manipulations

#include "DCpuState.h"
#include "RemoteProtocol.h"

// Base class implementation
DCpuState *
DCpuState::LocalCpuState(const cpu_state& state)
{
#if __INTEL__
	return new Dx86CpuState(state);
#else
#error Unsupported bdb host architecture!
#endif
}

// The portable cpu-state representation used by bdb and bdbproxy
// is a 2-byte architecture identifier followed by the binary architecture-
// specific cpu_state structure.

DCpuState *
DCpuState::Unflatten(const void* buffer)
{
	DCpuState* state = NULL;
	uint16 ident = *((uint16*) buffer);
	switch (ident)
	{
	case FLAT_INTEL_CPUSTATE:
		{
			x86_cpu_state s;
			memcpy(&s, ((char*) buffer) + 2, sizeof(s));
			state = new Dx86CpuState(s);
		}
		break;

	case FLAT_ARM_CPUSTATE:
		break;

	default:
		THROW(("Attempt to unflatten unknown CPU state: 0x%04x", ident));
		break;
	}
	return state;
}

// Intel variant
Dx86CpuState::Dx86CpuState(const x86_cpu_state& cpu)
	: mCpu(cpu)
{
}

Dx86CpuState::Dx86CpuState(const Dx86CpuState& other)
	: DCpuState()
{
	mCpu = other.mCpu;
}

ptr_t 
Dx86CpuState::GetPC() const
{
	return mCpu.eip;
}

ptr_t 
Dx86CpuState::GetFP() const
{
	return mCpu.ebp;
}

ptr_t 
Dx86CpuState::GetSP() const
{
	return mCpu.uesp;
}

void 
Dx86CpuState::SetPC(ptr_t pc)
{
	mCpu.eip = pc;
}

void 
Dx86CpuState::SetFP(ptr_t fp)
{
	mCpu.ebp = fp;
}

void 
Dx86CpuState::SetSP(ptr_t sp)
{
	mCpu.uesp = sp;
}

void 
Dx86CpuState::Assign(const DCpuState& other)
{
	const Dx86CpuState* optr = dynamic_cast<const Dx86CpuState*>(&other);
	if (optr)
	{
		mCpu = optr->mCpu;
	}
	else THROW(("Passed a non-x86 DCpuState to x86 Assign()!"));
}

DCpuState *
Dx86CpuState::Clone() const
{
	return new Dx86CpuState(*this);
}

ssize_t 
Dx86CpuState::Flatten(void *buffer, size_t bufferSize) const
{
	if (bufferSize < sizeof(uint16) + sizeof(x86_cpu_state)) return B_ERROR;

	// !!! Does this have endianness issues
	*((uint16*)buffer) = FLAT_INTEL_CPUSTATE;
	memcpy(((char*)buffer)+sizeof(uint16), &mCpu, sizeof(x86_cpu_state));
	return sizeof(uint16) + sizeof(x86_cpu_state);
}

ssize_t 
Dx86CpuState::FlattenedSize() const
{
	return sizeof(uint16) + sizeof(x86_cpu_state);
}

uint16 
Dx86CpuState::Architecture() const
{
	return FLAT_INTEL_CPUSTATE;
}

void *
Dx86CpuState::RawState() const
{
	return (void*) &mCpu;
}

Dx86CpuState::Dx86CpuState()
{
	// Private; should never be invoked
}
