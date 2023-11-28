/*	$Id: DCpuState.h,v 1.2 1998/11/17 12:16:31 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/10/98 12:11:54
*/

#ifndef DCPUSTATE_H
#define DCPUSTATE_H

/*
	A cpu independant interface to common cpu registers.
		
		- PC is the program counter
		- SP is the stack pointer
		- FP is the stack frame pointer
*/

#include "bdb.h"
class DRegsItem;

// DCpuState - abstract base class for CPU state manipulation; not CPU specific.
class DCpuState
{
public:
	virtual ptr_t GetPC() const = 0;			// program counter
	virtual ptr_t GetFP() const = 0;				// frame pointer
	virtual ptr_t GetSP() const = 0;			// stack pointer

	virtual void SetPC(ptr_t pc) = 0;
	virtual void SetFP(ptr_t fp) = 0;
	virtual void SetSP(ptr_t sp) = 0;

	virtual void Assign(const DCpuState& other) = 0;
	virtual DCpuState* Clone() const = 0;
	virtual ssize_t Flatten(void* buffer, size_t bufferSize) const = 0;	// returns bytes written or error
	virtual ssize_t FlattenedSize() const = 0;

	// What architecture does this struct describe?  Returns a FLAT_* code as defined
	// in the "RemoteProtocol.h" header.
	virtual uint16 Architecture() const = 0;

	static DCpuState* LocalCpuState(const cpu_state& state);
	static DCpuState* Unflatten(const void* buffer);

protected:
	// the Register Window support code needs to be able to manipulate the underlying
	// CPU state structures, so we let only it get a pointer directly to the state structure
	// through this protected virtual method, combined with a friendship declaration.
	virtual void* RawState() const = 0;
	friend class DRegsItem;
};

// Target-specific concrete DCpuState subclasses
class Dx86CpuState : public DCpuState
{
public:
	Dx86CpuState(const x86_cpu_state& cpu);
	Dx86CpuState(const Dx86CpuState& other);

	ptr_t GetPC() const;
	ptr_t GetFP() const;
	ptr_t GetSP() const;
	void SetPC(ptr_t pc);
	void SetFP(ptr_t fp);
	void SetSP(ptr_t sp);

	void Assign(const DCpuState& other);
	DCpuState* Clone() const;
	ssize_t Flatten(void* buffer, size_t bufferSize) const;
	ssize_t FlattenedSize() const;
	uint16 Architecture() const;

	x86_cpu_state GetNativeState() const { return mCpu; }

protected:
	void* RawState() const;

private:
	Dx86CpuState();
	x86_cpu_state mCpu;
};

class DARMCpuState : public DCpuState
{
public:
};

/*
#if __INTEL__

inline ptr_t GetPC(const cpu_state& cpu)		{ return cpu.eip; };
inline ptr_t GetFP(const cpu_state& cpu)		{ return cpu.ebp; };
inline ptr_t GetSP(const cpu_state& cpu)		{ return cpu.uesp; };

inline void SetPC(cpu_state& cpu, ptr_t pc)		{ cpu.eip = pc; };
inline void SetFP(cpu_state& cpu, ptr_t fp)		{ cpu.ebp = fp; };
inline void SetSP(cpu_state& cpu, ptr_t sp)		{ cpu.uesp = sp; };

#else
#	error "Have to implement for this CPU"
#endif
*/

#endif
