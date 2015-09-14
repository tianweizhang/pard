#ifndef __ARCH_X86_PREDECODER_HH__
#define __ARCH_X86_PREDECODER_HH__

#include <cassert>
#include <vector>

#include "arch/x86/regs/misc.hh"
#include "arch/x86/decoder.hh"
#include "arch/x86/types.hh"
#include "base/bitfield.hh"
#include "base/misc.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "cpu/static_inst.hh"
#include "debug/X86Predecoder.hh"

namespace X86ISA
{

class X86Predecoder {

  protected:
    //These are defined in decode.hh
    typedef Decoder::State State;
    static const uint8_t * Prefixes;
    static const uint8_t (* UsesModRM)[256];
    static const uint8_t (* ImmediateType)[256];
    static const uint8_t (* SizeTypeToSize)[10];

  protected:

    //The size of the displacement value
    int displacementSize;
    //The size of the immediate value
    int immediateSize;
    //This is how much of any immediate value we've gotten. This is used
    //for both the actual immediate and the displacement.
    int immediateCollected;

    State state;
    bool instDone;
    int offset;
    ExtMachInst emi;
    MachInst inst;

    uint8_t altOp;
    uint8_t defOp;
    uint8_t altAddr;
    uint8_t defAddr;
    uint8_t stack;

  public:
    X86Predecoder(MachInst _inst, Decoder *decoder)
      : state(Decoder::ResetState), instDone(false), offset(0), inst(_inst) {
        altOp = decoder->altOp;
        defOp = decoder->defOp;
        altAddr = decoder->altAddr;
        defAddr = decoder->defAddr;
        stack = decoder->stack;
        process();
    }

    ExtMachInst& getExtMachInst() { assert(instDone); return emi; }
    int getInstSize() { assert(instDone); return offset; }

  protected:
    void consumeByte() {
        offset ++;
        assert(offset<sizeof(MachInst));
    }
    void consumeBytes(int numBytes) {
        offset += numBytes;
        assert(offset<sizeof(MachInst));
    }
    uint8_t getNextByte() {
        assert(offset<sizeof(MachInst));
        return ((uint8_t *)&inst)[offset];
    }

    void getImmediate(int &collected, uint64_t &current, int size)
    {
        //Figure out how many bytes we still need to get for the
        //immediate.
        int toGet = size - collected;
        //Figure out how many bytes are left in our "buffer"
        int remaining = sizeof(MachInst) - offset;
        //Get as much as we need, up to the amount available.
        toGet = toGet > remaining ? remaining : toGet;

        //Shift the bytes we want to be all the way to the right
        uint64_t partialImm = inst >> (offset * 8);
        //Mask off what we don't want
        partialImm &= mask(toGet * 8);
        //Shift it over to overlay with our displacement.
        partialImm <<= (immediateCollected * 8);
        //Put it into our displacement
        current |= partialImm;
        //Update how many bytes we've collected.
        collected += toGet;
        consumeBytes(toGet);
    }


  protected:
    void process();

    //Functions to handle each of the states
    State doResetState();
    State doPrefixState(uint8_t);
    State doOpcodeState(uint8_t);
    State doModRMState(uint8_t);
    State doSIBState(uint8_t);
    State doDisplacementState();
    State doImmediateState();
};

} // namespace X86ISA

#endif // __ARCH_X86_PREDECODER_HH__
