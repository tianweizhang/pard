#include "arch/x86/predecoder.hh"
#include "arch/x86/regs/misc.hh"
#include "base/misc.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "debug/X86Predecoder.hh"

namespace X86ISA
{

const uint8_t * X86Predecoder::Prefixes = Decoder::Prefixes;
const uint8_t (* X86Predecoder::UsesModRM)[256] = Decoder::UsesModRM;
const uint8_t (* X86Predecoder::ImmediateType)[256] = Decoder::ImmediateType;
const uint8_t (* X86Predecoder::SizeTypeToSize)[10] = Decoder::SizeTypeToSize;

X86Predecoder::State
X86Predecoder::doResetState()
{
    emi.rex = 0;
    emi.legacy = 0;
    emi.opcode.num = 0;
    emi.opcode.op = 0;
    emi.opcode.prefixA = emi.opcode.prefixB = 0;

    immediateCollected = 0;
    emi.immediate = 0;
    emi.displacement = 0;
    emi.dispSize = 0;

    emi.modRM = 0;
    emi.sib = 0;

    return Decoder::PrefixState;
}

void
X86Predecoder::process()
{
    state = doResetState();

    while (!instDone) {
        uint8_t nextByte = getNextByte();
        switch (state) {
          case Decoder::PrefixState:
            state = doPrefixState(nextByte);
            break;
          case Decoder::OpcodeState:
            state = doOpcodeState(nextByte);
            break;
          case Decoder::ModRMState:
            state = doModRMState(nextByte);
            break;
          case Decoder::SIBState:
            state = doSIBState(nextByte);
            break;
          case Decoder::DisplacementState:
            state = doDisplacementState();
            break;
          case Decoder::ImmediateState:
            state = doImmediateState();
            break;
          case Decoder::ErrorState:
            panic("Went to the error state in the decoder.\n");
          default:
            panic("Unrecognized state! %d\n", state);
        }
    }
}

//Either get a prefix and record it in the ExtMachInst, or send the
//state machine on to get the opcode(s).
X86Predecoder::State
X86Predecoder::doPrefixState(uint8_t nextByte)
{
    uint8_t prefix = Prefixes[nextByte];
    State nextState = Decoder::PrefixState;
    // REX prefixes are only recognized in 64 bit mode.
    if (prefix == RexPrefix && emi.mode.submode != SixtyFourBitMode)
        prefix = 0;
    if (prefix)
        consumeByte();
    switch(prefix)
    {
        //Operand size override prefixes
      case OperandSizeOverride:
        DPRINTF(X86Predecoder, "Found operand size override prefix.\n");
        emi.legacy.op = true;
        break;
      case AddressSizeOverride:
        DPRINTF(X86Predecoder, "Found address size override prefix.\n");
        emi.legacy.addr = true;
        break;
        //Segment override prefixes
      case CSOverride:
      case DSOverride:
      case ESOverride:
      case FSOverride:
      case GSOverride:
      case SSOverride:
        DPRINTF(X86Predecoder, "Found segment override.\n");
        emi.legacy.seg = prefix;
        break;
      case Lock:
        DPRINTF(X86Predecoder, "Found lock prefix.\n");
        emi.legacy.lock = true;
        break;
      case Rep:
        DPRINTF(X86Predecoder, "Found rep prefix.\n");
        emi.legacy.rep = true;
        break;
      case Repne:
        DPRINTF(X86Predecoder, "Found repne prefix.\n");
        emi.legacy.repne = true;
        break;
      case RexPrefix:
        DPRINTF(X86Predecoder, "Found Rex prefix %#x.\n", nextByte);
        emi.rex = nextByte;
        break;
      case 0:
        nextState = Decoder::OpcodeState;
        break;
      default:
        panic("Unrecognized prefix %#x\n", nextByte);
    }
    return nextState;
}

//Load all the opcodes (currently up to 2) and then figure out
//what immediate and/or ModRM is needed.
X86Predecoder::State
X86Predecoder::doOpcodeState(uint8_t nextByte)
{
    State nextState = Decoder::ErrorState;
    emi.opcode.num++;
    //We can't handle 3+ byte opcodes right now
    assert(emi.opcode.num < 4);
    consumeByte();
    if(emi.opcode.num == 1 && nextByte == 0x0f)
    {
        nextState = Decoder::OpcodeState;
        DPRINTF(X86Predecoder, "Found two byte opcode.\n");
        emi.opcode.prefixA = nextByte;
    }
    else if(emi.opcode.num == 2 && (nextByte == 0x38 || nextByte == 0x3A))
    {
        nextState = Decoder::OpcodeState;
        DPRINTF(X86Predecoder, "Found three byte opcode.\n");
        emi.opcode.prefixB = nextByte;
    }
    else
    {
        DPRINTF(X86Predecoder, "Found opcode %#x.\n", nextByte);
        emi.opcode.op = nextByte;

        //Figure out the effective operand size. This can be overriden to
        //a fixed value at the decoder level.
        int logOpSize;
        if (emi.rex.w)
            logOpSize = 3; // 64 bit operand size
        else if (emi.legacy.op)
            logOpSize = altOp;
        else
            logOpSize = defOp;

        //Set the actual op size
        emi.opSize = 1 << logOpSize;

        //Figure out the effective address size. This can be overriden to
        //a fixed value at the decoder level.
        int logAddrSize;
        if(emi.legacy.addr)
            logAddrSize = altAddr;
        else
            logAddrSize = defAddr;

        //Set the actual address size
        emi.addrSize = 1 << logAddrSize;

        //Figure out the effective stack width. This can be overriden to
        //a fixed value at the decoder level.
        emi.stackSize = 1 << stack;

        //Figure out how big of an immediate we'll retreive based
        //on the opcode.
        int immType = ImmediateType[emi.opcode.num - 1][nextByte];
        if (emi.opcode.num == 1 && nextByte >= 0xA0 && nextByte <= 0xA3)
            immediateSize = SizeTypeToSize[logAddrSize - 1][immType];
        else
            immediateSize = SizeTypeToSize[logOpSize - 1][immType];

        //Determine what to expect next
        if (UsesModRM[emi.opcode.num - 1][nextByte]) {
            nextState = Decoder::ModRMState;
        } else {
            if(immediateSize) {
                nextState = Decoder::ImmediateState;
            } else {
                instDone = true;
                nextState = Decoder::ResetState;
            }
        }
    }
    return nextState;
}

//Get the ModRM byte and determine what displacement, if any, there is.
//Also determine whether or not to get the SIB byte, displacement, or
//immediate next.
X86Predecoder::State
X86Predecoder::doModRMState(uint8_t nextByte)
{
    State nextState = Decoder::ErrorState;
    ModRM modRM;
    modRM = nextByte;
    DPRINTF(X86Predecoder, "Found modrm byte %#x.\n", nextByte);
    if (defOp == 1) {
        //figure out 16 bit displacement size
        if ((modRM.mod == 0 && modRM.rm == 6) || modRM.mod == 2)
            displacementSize = 2;
        else if (modRM.mod == 1)
            displacementSize = 1;
        else
            displacementSize = 0;
    } else {
        //figure out 32/64 bit displacement size
        if ((modRM.mod == 0 && modRM.rm == 5) || modRM.mod == 2)
            displacementSize = 4;
        else if (modRM.mod == 1)
            displacementSize = 1;
        else
            displacementSize = 0;
    }

    // The "test" instruction in group 3 needs an immediate, even though
    // the other instructions with the same actual opcode don't.
    if (emi.opcode.num == 1 && (modRM.reg & 0x6) == 0) {
       if (emi.opcode.op == 0xF6)
           immediateSize = 1;
       else if (emi.opcode.op == 0xF7)
           immediateSize = (emi.opSize == 8) ? 4 : emi.opSize;
    }

    //If there's an SIB, get that next.
    //There is no SIB in 16 bit mode.
    if (modRM.rm == 4 && modRM.mod != 3) {
            // && in 32/64 bit mode)
        nextState = Decoder::SIBState;
    } else if(displacementSize) {
        nextState = Decoder::DisplacementState;
    } else if(immediateSize) {
        nextState = Decoder::ImmediateState;
    } else {
        instDone = true;
        nextState = Decoder::ResetState;
    }
    //The ModRM byte is consumed no matter what
    consumeByte();
    emi.modRM = modRM;
    return nextState;
}

//Get the SIB byte. We don't do anything with it at this point, other
//than storing it in the ExtMachInst. Determine if we need to get a
//displacement or immediate next.
X86Predecoder::State
X86Predecoder::doSIBState(uint8_t nextByte)
{
    State nextState = Decoder::ErrorState;
    emi.sib = nextByte;
    DPRINTF(X86Predecoder, "Found SIB byte %#x.\n", nextByte);
    consumeByte();
    if (emi.modRM.mod == 0 && emi.sib.base == 5)
        displacementSize = 4;
    if (displacementSize) {
        nextState = Decoder::DisplacementState;
    } else if(immediateSize) {
        nextState = Decoder::ImmediateState;
    } else {
        instDone = true;
        nextState = Decoder::ResetState;
    }
    return nextState;
}

//Gather up the displacement, or at least as much of it
//as we can get.
X86Predecoder::State
X86Predecoder::doDisplacementState()
{
    State nextState = Decoder::ErrorState;

    getImmediate(immediateCollected,
            emi.displacement,
            displacementSize);

    DPRINTF(X86Predecoder, "Collecting %d byte displacement, got %d bytes.\n",
            displacementSize, immediateCollected);

    if(displacementSize == immediateCollected) {
        //Reset this for other immediates.
        immediateCollected = 0;
        //Sign extend the displacement
        switch(displacementSize)
        {
          case 1:
            emi.displacement = sext<8>(emi.displacement);
            break;
          case 2:
            emi.displacement = sext<16>(emi.displacement);
            break;
          case 4:
            emi.displacement = sext<32>(emi.displacement);
            break;
          default:
            panic("Undefined displacement size!\n");
        }
        DPRINTF(X86Predecoder, "Collected displacement %#x.\n",
                emi.displacement);
        if(immediateSize) {
            nextState = Decoder::ImmediateState;
        } else {
            instDone = true;
            nextState = Decoder::ResetState;
        }

        emi.dispSize = displacementSize;
    }
    else
        nextState = Decoder::DisplacementState;
    return nextState;
}

//Gather up the immediate, or at least as much of it
//as we can get
X86Predecoder::State
X86Predecoder::doImmediateState()
{
    State nextState = Decoder::ErrorState;

    getImmediate(immediateCollected,
            emi.immediate,
            immediateSize);

    DPRINTF(X86Predecoder, "Collecting %d byte immediate, got %d bytes.\n",
            immediateSize, immediateCollected);

    if(immediateSize == immediateCollected)
    {
        //Reset this for other immediates.
        immediateCollected = 0;

        //XXX Warning! The following is an observed pattern and might
        //not always be true!

        //Instructions which use 64 bit operands but 32 bit immediates
        //need to have the immediate sign extended to 64 bits.
        //Instructions which use true 64 bit immediates won't be
        //affected, and instructions that use true 32 bit immediates
        //won't notice.
        switch(immediateSize)
        {
          case 4:
            emi.immediate = sext<32>(emi.immediate);
            break;
          case 1:
            emi.immediate = sext<8>(emi.immediate);
        }

        DPRINTF(X86Predecoder, "Collected immediate %#x.\n",
                emi.immediate);
        instDone = true;
        nextState = Decoder::ResetState;
    }
    else
        nextState = Decoder::ImmediateState;
    return nextState;
}

}
