/*
 * Copyright (c) 2014 ACS, ICT
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Jiuyue Ma
 */

#include "arch/x86/regs/int.hh"
#include "arch/x86/isa_traits.hh"
#include "arch/vtophys.hh"
#include "base/loader/object_file.hh"
#include "base/loader/symtab.hh"
#include "base/trace.hh"
#include "cpu/thread_context.hh"
#include "debug/Loader.hh"
#include "hyper/arch/bios.hh"
#include "hyper/arch/system.hh"
#include "hyper/arch/guest/system.hh"
#include "hyper/arch/guest/linux/system.hh"
#include "hyper/arch/PARDg5VLDomain.hh"
#include "mem/port_proxy.hh"
#include "sim/byteswap.hh"
#include "sim/full_system.hh"

using namespace LittleEndianGuest;
using namespace X86ISA;

PARDg5VLDomain::PARDg5VLDomain(X86PARDg5VGuest *guest)
    : guest(guest), system(guest->system),
      physProxy(system->getSystemPort(), system->cacheLineSize(), -1),
      /* Kernel */
      kernelSymtab(guest->kernelSymtab), kernel(guest->kernel), 
      kernelStart(guest->kernelStart), kernelEnd(guest->kernelEnd),
      kernelEntry(guest->kernelEntry),
      loadAddrMask(guest->loadAddrMask), loadAddrOffset(guest->loadAddrOffset),
      /* BIOS */
      smbiosTable(guest->smbiosTable),
      mpFloatingPointer(guest->mpFloatingPointer),
      mpConfigTable(guest->mpConfigTable),
      rsdp(guest->rsdp)
{
    const LinuxX86PARDg5VGuestParams *params =
        static_cast<const LinuxX86PARDg5VGuestParams*>(guest->_params);

    /** Command Line */
    commandLine = params->boot_osflags;
    /** E820 */
    e820Table = params->e820_table;

    // Register to PARDg5VSystem
    domID = system->registerPARDg5VLDomain(this);
    physProxy.updateTag(domID);
}


#define assignISAInt(irq, apicPin)				\
    createIntelMPIOIntAssignment(				\
        /*     interrupt_type = */ Enums::INT, 			\
        /*           polarity = */ Enums::ConformPolarity,	\
        /*            trigger = */ Enums::ConformTrigger,	\
        /*      source_bus_id = */ 0,				\
        /*     source_bus_irq = */ irq,				\
        /*    dest_io_apic_id = */ io_apic_id,			\
        /* dest_io_apic_intin = */ apicPin)


PARDg5VLDomain::PARDg5VLDomain(X86PARDg5VSystem *_sys, int num_cpus, Addr memsize)
    : guest(NULL), system(_sys),
      physProxy(system->getSystemPort(), system->cacheLineSize(), -1),
      loadAddrMask(0xFFFFFFFFFF), loadAddrOffset(0)
{
    // Kernel
    std::string kernelName = "/opt/stack/pard/pardg5-v/system/binaries/"
                             "x86_64-vmlinux-2.6.28.4.smp";

    // Command Line
    commandLine = "earlyprintk=ttyS0 console=ttyS0 lpj=7999923 "
                  "root=/dev/sda1 no_timer_check";

    // E820
    X86ISA::E820Entry * e820_entries[] = {
        createE820Entry(0x00000000, 0x9fc00, 1),
        createE820Entry(0x0009fc00, 0x60400, 2),
        createE820Entry(0x00100000, memsize-0x100000, 1),
        createE820Entry(0xFFFF0000, 0x10000, 2)
    };
    e820Table = createE820Table(e820_entries,
                                 sizeof(e820_entries)/sizeof(e820_entries[0]));

    // Bios information structure.
    X86ISA::SMBios::SMBiosStructure *structures[] = {
        createSMBiosBiosInformation()
    };
    smbiosTable = createSMBiosTable(structures,
                                    sizeof(structures)/sizeof(structures[0]));

    // IntelMP table
    int io_apic_id = 1;
    X86ISA::IntelMP::BaseConfigEntry *base_entries[] = {
        createIntelMPProcessor(0, 0x14, true, true),
        createIntelMPIOAPIC(1, 0x11, true, 0xfec00000),
        createIntelMPBus("ISA", 0),
        createIntelMPBus("PCI", 1),
        createIntelMPIOIntAssignment(
            /*     interrupt_type = */ Enums::INT, 
            /*           polarity = */ Enums::ConformPolarity,
            /*            trigger = */ Enums::ConformTrigger,
            /*      source_bus_id = */ 1,
            /*     source_bus_irq = */ 0 + (4 << 2), 
            /*    dest_io_apic_id = */ io_apic_id,
            /* dest_io_apic_intin = */ 16), 
        assignISAInt( 0,  2), assignISAInt( 1,  1), assignISAInt( 3,  3),
        assignISAInt( 4,  4), assignISAInt( 5,  5), assignISAInt( 6,  6),
        assignISAInt( 7,  7), assignISAInt( 8,  8), assignISAInt( 9,  9),
        assignISAInt(10, 10), assignISAInt(11, 11), assignISAInt(12, 12),
        assignISAInt(13, 13), assignISAInt(14, 14), assignISAInt(15, 15),
    };
    X86ISA::IntelMP::ExtConfigEntry * ext_entries[] = {
        createIntelMPBusHierarchy(0, true, 1)
    };
    mpConfigTable = createIntelMPConfigTable(
        base_entries, sizeof(base_entries)/sizeof(base_entries[0]),
         ext_entries, sizeof( ext_entries)/sizeof( ext_entries[0]));
    mpFloatingPointer = createIntelMPFloatingPointer();

    // ACPI
    rsdp = createACPIRSDP();

    // Kernel
    if (FullSystem) {
        kernelSymtab = new SymbolTable;
        if (!debugSymbolTable)
            debugSymbolTable = new SymbolTable;
    }

    if (FullSystem) {
        // Get the kernel code
        kernel = createObjectFile(kernelName);
        inform("kernel located at: %s", kernelName.c_str());

        if (kernel == NULL)
            fatal("Could not load kernel file %s", kernelName.c_str());

        // setup entry points
        kernelStart = kernel->textBase();
        kernelEnd = kernel->bssBase() + kernel->bssSize();
        kernelEntry = kernel->entryPoint();

        // load symbols
        if (!kernel->loadGlobalSymbols(kernelSymtab))
            fatal("could not load kernel symbols\n");

        if (!kernel->loadLocalSymbols(kernelSymtab))
            fatal("could not load kernel local symbols\n");

        if (!kernel->loadGlobalSymbols(debugSymbolTable))
            fatal("could not load kernel symbols\n");

        if (!kernel->loadLocalSymbols(debugSymbolTable))
            fatal("could not load kernel local symbols\n");

        // Loading only needs to happen once and after memory system is
        // connected so it will happen in initState()
    }

    // Register to PARDg5VSystem
    domID = system->registerPARDg5VLDomain(this);
    physProxy.updateTag(domID);
}

PARDg5VLDomain::~PARDg5VLDomain()
{
    if (guest) {
        // If duplicate from X86PARDg5VGuest, nothing to cleanup
        return;
    }
    delete e820Table;
    delete smbiosTable;
    delete mpConfigTable;
    delete mpFloatingPointer;
    delete rsdp;
}


static void
installSegDesc(ThreadContext *tc, SegmentRegIndex seg,
        SegDescriptor desc, bool longmode)
{
    uint64_t base = desc.baseLow + (desc.baseHigh << 24);
    bool honorBase = !longmode || seg == SEGMENT_REG_FS ||
                                  seg == SEGMENT_REG_GS ||
                                  seg == SEGMENT_REG_TSL ||
                                  seg == SYS_SEGMENT_REG_TR;
    uint64_t limit = desc.limitLow | (desc.limitHigh << 16);

    SegAttr attr = 0;

    attr.dpl = desc.dpl;
    attr.unusable = 0;
    attr.defaultSize = desc.d;
    attr.longMode = desc.l;
    attr.avl = desc.avl;
    attr.granularity = desc.g;
    attr.present = desc.p;
    attr.system = desc.s;
    attr.type = desc.type;
    if (desc.s) {
        if (desc.type.codeOrData) {
            // Code segment
            attr.expandDown = 0;
            attr.readable = desc.type.r;
            attr.writable = 0;
        } else {
            // Data segment
            attr.expandDown = desc.type.e;
            attr.readable = 1;
            attr.writable = desc.type.w;
        }
    } else {
        attr.readable = 1;
        attr.writable = 1;
        attr.expandDown = 0;
    }

    tc->setMiscReg(MISCREG_SEG_BASE(seg), base);
    tc->setMiscReg(MISCREG_SEG_EFF_BASE(seg), honorBase ? base : 0);
    tc->setMiscReg(MISCREG_SEG_LIMIT(seg), limit);
    tc->setMiscReg(MISCREG_SEG_ATTR(seg), (MiscReg)attr);
}

void
PARDg5VLDomain::initBSPState(ThreadContext *tcBSP)
{
    ThreadContext *tc = tcBSP;

    /*
     * Set up QR0
     */
    tc->setMiscReg(MISCREG_QR0, (MiscReg)domID);

    // Load kernel to guest's memory
    kernel->loadSections(physProxy, loadAddrMask, loadAddrOffset);
    DPRINTF(Loader, "Kernel start = %#x\n", kernelStart);
    DPRINTF(Loader, "Kernel end   = %#x\n", kernelEnd);
    DPRINTF(Loader, "Kernel entry = %#x\n", kernelEntry);
    DPRINTF(Loader, "Kernel loaded...\n");

    if (!kernel)
        fatal("No kernel to load.\n");

    if (kernel->getArch() == ObjectFile::I386)
        fatal("Loading a 32 bit x86 kernel is not supported.\n");
    
    // This is the boot strap processor (BSP). Initialize it to look like
    // the boot loader has just turned control over to the 64 bit OS. We
    // won't actually set up real mode or legacy protected mode descriptor
    // tables because we aren't executing any code that would require
    // them. We do, however toggle the control bits in the correct order
    // while allowing consistency checks and the underlying mechansims
    // just to be safe.

    const int NumPDTs = 4;

    const Addr PageMapLevel4 = 0x70000;
    const Addr PageDirPtrTable = 0x71000;
    const Addr PageDirTable[NumPDTs] =
        {0x72000, 0x73000, 0x74000, 0x75000};
    const Addr GDTBase = 0x76000;

    const int PML4Bits = 9;
    const int PDPTBits = 9;
    const int PDTBits = 9;

    /*
     * Set up the gdt.
     */
    uint8_t numGDTEntries = 0;
    // Place holder at selector 0
    uint64_t nullDescriptor = 0;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&nullDescriptor), 8);
    numGDTEntries++;

    //64 bit code segment
    SegDescriptor csDesc = 0;
    csDesc.type.codeOrData = 1;
    csDesc.type.c = 0; // Not conforming
    csDesc.type.r = 1; // Readable
    csDesc.dpl = 0; // Privelege level 0
    csDesc.p = 1; // Present
    csDesc.l = 1; // 64 bit
    csDesc.d = 0; // default operand size
    csDesc.g = 1; // Page granularity
    csDesc.s = 1; // Not a system segment
    csDesc.limitHigh = 0xF;
    csDesc.limitLow = 0xFFFF;
    //Because we're dealing with a pointer and I don't think it's
    //guaranteed that there isn't anything in a nonvirtual class between
    //it's beginning in memory and it's actual data, we'll use an
    //intermediary.
    uint64_t csDescVal = csDesc;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&csDescVal), 8);

    numGDTEntries++;

    SegSelector cs = 0;
    cs.si = numGDTEntries - 1;

    tc->setMiscReg(MISCREG_CS, (MiscReg)cs);

    //32 bit data segment
    SegDescriptor dsDesc = 0;
    dsDesc.type.codeOrData = 0;
    dsDesc.type.e = 0; // Not expand down
    dsDesc.type.w = 1; // Writable
    dsDesc.dpl = 0; // Privelege level 0
    dsDesc.p = 1; // Present
    dsDesc.d = 1; // default operand size
    dsDesc.g = 1; // Page granularity
    dsDesc.s = 1; // Not a system segment
    dsDesc.limitHigh = 0xF;
    dsDesc.limitLow = 0xFFFF;
    uint64_t dsDescVal = dsDesc;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&dsDescVal), 8);

    numGDTEntries++;

    SegSelector ds = 0;
    ds.si = numGDTEntries - 1;

    tc->setMiscReg(MISCREG_DS, (MiscReg)ds);
    tc->setMiscReg(MISCREG_ES, (MiscReg)ds);
    tc->setMiscReg(MISCREG_FS, (MiscReg)ds);
    tc->setMiscReg(MISCREG_GS, (MiscReg)ds);
    tc->setMiscReg(MISCREG_SS, (MiscReg)ds);

    tc->setMiscReg(MISCREG_TSL, 0);
    tc->setMiscReg(MISCREG_TSG_BASE, GDTBase);
    tc->setMiscReg(MISCREG_TSG_LIMIT, 8 * numGDTEntries - 1);

    SegDescriptor tssDesc = 0;
    tssDesc.type = 0xB;
    tssDesc.dpl = 0; // Privelege level 0
    tssDesc.p = 1; // Present
    tssDesc.d = 1; // default operand size
    tssDesc.g = 1; // Page granularity
    tssDesc.s = 0;
    tssDesc.limitHigh = 0xF;
    tssDesc.limitLow = 0xFFFF;
    uint64_t tssDescVal = tssDesc;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&tssDescVal), 8);

    numGDTEntries++;

    SegSelector tss = 0;
    tss.si = numGDTEntries - 1;

    tc->setMiscReg(MISCREG_TR, (MiscReg)tss);
    installSegDesc(tc, SYS_SEGMENT_REG_TR, tssDesc, true);

    /*
     * Identity map the first 4GB of memory. In order to map this region
     * of memory in long mode, there needs to be one actual page map level
     * 4 entry which points to one page directory pointer table which
     * points to 4 different page directory tables which are full of two
     * megabyte pages. All of the other entries in valid tables are set
     * to indicate that they don't pertain to anything valid and will
     * cause a fault if used.
     */

    // Put valid values in all of the various table entries which indicate
    // that those entries don't point to further tables or pages. Then
    // set the values of those entries which are needed.

    // Page Map Level 4

    // read/write, user, not present
    uint64_t pml4e = X86ISA::htog(0x6);
    for (int offset = 0; offset < (1 << PML4Bits) * 8; offset += 8) {
        physProxy.writeBlob(PageMapLevel4 + offset, (uint8_t *)(&pml4e), 8);
    }
    // Point to the only PDPT
    pml4e = X86ISA::htog(0x7 | PageDirPtrTable);
    physProxy.writeBlob(PageMapLevel4, (uint8_t *)(&pml4e), 8);

    // Page Directory Pointer Table

    // read/write, user, not present
    uint64_t pdpe = X86ISA::htog(0x6);
    for (int offset = 0; offset < (1 << PDPTBits) * 8; offset += 8) {
        physProxy.writeBlob(PageDirPtrTable + offset,
                            (uint8_t *)(&pdpe), 8);
    }
    // Point to the PDTs
    for (int table = 0; table < NumPDTs; table++) {
        pdpe = X86ISA::htog(0x7 | PageDirTable[table]);
        physProxy.writeBlob(PageDirPtrTable + table * 8,
                            (uint8_t *)(&pdpe), 8);
    }

    // Page Directory Tables

    Addr base = 0;
    const Addr pageSize = 2 << 20;
    for (int table = 0; table < NumPDTs; table++) {
        for (int offset = 0; offset < (1 << PDTBits) * 8; offset += 8) {
            // read/write, user, present, 4MB
            uint64_t pdte = X86ISA::htog(0x87 | base);
            physProxy.writeBlob(PageDirTable[table] + offset,
                                (uint8_t *)(&pdte), 8);
            base += pageSize;
        }
    }

    /*
     * Transition from real mode all the way up to Long mode
     */
    CR0 cr0 = tc->readMiscRegNoEffect(MISCREG_CR0);
    //Turn off paging.
    cr0.pg = 0;
    tc->setMiscReg(MISCREG_CR0, cr0);
    //Turn on protected mode.
    cr0.pe = 1;
    tc->setMiscReg(MISCREG_CR0, cr0);

    CR4 cr4 = tc->readMiscRegNoEffect(MISCREG_CR4);
    //Turn on pae.
    cr4.pae = 1;
    tc->setMiscReg(MISCREG_CR4, cr4);

    //Point to the page tables.
    tc->setMiscReg(MISCREG_CR3, PageMapLevel4);

    Efer efer = tc->readMiscRegNoEffect(MISCREG_EFER);
    //Enable long mode.
    efer.lme = 1;
    tc->setMiscReg(MISCREG_EFER, efer);

    //Start using longmode segments.
    installSegDesc(tc, SEGMENT_REG_CS, csDesc, true);
    installSegDesc(tc, SEGMENT_REG_DS, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_ES, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_FS, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_GS, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_SS, dsDesc, true);

    //Activate long mode.
    cr0.pg = 1;
    tc->setMiscReg(MISCREG_CR0, cr0);

    tc->pcState(kernelEntry);

    // We should now be in long mode. Yay!

    Addr ebdaPos = 0xF0000;
    Addr fixed, table;

    //Write out the SMBios/DMI table
    writeOutSMBiosTable(ebdaPos, fixed, table);
    ebdaPos += (fixed + table);
    ebdaPos = roundUp(ebdaPos, 16);

    //Write out the Intel MP Specification configuration table
    writeOutMPTable(ebdaPos, fixed, table);
    ebdaPos += (fixed + table);

    // Set MTRR Register
    tc->setMiscReg(MISCREG_MTRR_PHYS_BASE_0, 0x0000000000000006);	// Base: 0x0, Type: WriteBack
    tc->setMiscReg(MISCREG_MTRR_PHYS_MASK_0, 0xFFFFFFFF80000800);	// Size: 2048MB, VALID
    tc->setMiscReg(MISCREG_MTRR_PHYS_BASE_1, 0x0000000080000006);	// Base: 2048MB, Type: WriteBack
    tc->setMiscReg(MISCREG_MTRR_PHYS_MASK_1, 0xFFFFFFFFC0000800);	// Size: 1024MB, VALID

    // Prepare Linux Environment
    prepareLinuxEnv(tcBSP);
}

void
PARDg5VLDomain::writeOutSMBiosTable(Addr header,
                                    Addr &headerSize, Addr &structSize, Addr table)
{
    // If the table location isn't specified, just put it after the header.
    // The header size as of the 2.5 SMBios specification is 0x1F bytes
    if (!table)
        table = header + 0x1F;
    smbiosTable->setTableAddr(table);

    smbiosTable->writeOut(physProxy, header, headerSize, structSize);

    // Do some bounds checking to make sure we at least didn't step on
    // ourselves.
    assert(header > table || header + headerSize <= table);
    assert(table > header || table + structSize <= header);
}

void
PARDg5VLDomain::writeOutMPTable(Addr fp,
        Addr &fpSize, Addr &tableSize, Addr table)
{
    // If the table location isn't specified and it exists, just put
    // it after the floating pointer. The fp size as of the 1.4 Intel MP
    // specification is 0x10 bytes.
    if (mpConfigTable) {
        if (!table)
            table = fp + 0x10;
        mpFloatingPointer->setTableAddr(table);
    }

    fpSize = mpFloatingPointer->writeOut(physProxy, fp);
    if (mpConfigTable)
        tableSize = mpConfigTable->writeOut(physProxy, table);
    else
        tableSize = 0;

    // Do some bounds checking to make sure we at least didn't step on
    // ourselves and the fp structure was the size we thought it was.
    assert(fp > table || fp + fpSize <= table);
    assert(table > fp || table + tableSize <= fp);
    assert(fpSize == 0x10);
}


void
PARDg5VLDomain::prepareLinuxEnv(ThreadContext *tcBSP)
{
    // The location of the real mode data structure.
    const Addr realModeData = 0x90200;

    /*
     * Deal with the command line stuff.
     */

    // A buffer to store the command line.
    const Addr commandLineBuff = 0x90000;
    // A pointer to the commandLineBuff stored in the real mode data.
    const Addr commandLinePointer = realModeData + 0x228;

    if (commandLine.length() + 1 > realModeData - commandLineBuff)
        panic("Command line \"%s\" is longer than %d characters.\n",
                commandLine, realModeData - commandLineBuff - 1);
    physProxy.writeBlob(commandLineBuff, (uint8_t *)commandLine.c_str(),
                        commandLine.length() + 1);

    // Generate a pointer of the right size and endianness to put into
    // commandLinePointer.
    uint32_t guestCommandLineBuff =
        X86ISA::htog((uint32_t)commandLineBuff);
    physProxy.writeBlob(commandLinePointer, (uint8_t *)&guestCommandLineBuff,
                        sizeof(guestCommandLineBuff));

    /*
     * Screen Info.
     */

    // We'll skip on this for now because it's only needed for framebuffers,
    // something we don't support at the moment.

    /*
     * EDID info
     */

    // Skipping for now.

    /*
     * Saved video mode
     */

    // Skipping for now.

    /*
     * Loader type.
     */

    // Skipping for now.

    /*
     * E820 memory map
     */

    // A pointer to the number of E820 entries there are.
    const Addr e820MapNrPointer = realModeData + 0x1e8;

    // A pointer to the buffer for E820 entries.
    const Addr e820MapPointer = realModeData + 0x2d0;

    e820Table->writeTo(physProxy, e820MapNrPointer, e820MapPointer);

    /*
     * Pass the location of the real mode data structure to the kernel
     * using register %esi. We'll use %rsi which should be equivalent.
     */
    tcBSP->setIntReg(INTREG_RSI, realModeData);
}
