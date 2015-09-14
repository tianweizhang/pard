#ifndef __HYPER_ARCH_BIOS_HH__
#define __HYPER_ARCH_BIOS_HH__

/**
 * IntelMP
 **/
#include "arch/x86/bios/intelmp.hh"
#include "params/X86IntelMPBus.hh"
#include "params/X86IntelMPBusHierarchy.hh"
#include "params/X86IntelMPConfigTable.hh"
#include "params/X86IntelMPFloatingPointer.hh"
#include "params/X86IntelMPIOAPIC.hh"
#include "params/X86IntelMPIOIntAssignment.hh"
#include "params/X86IntelMPProcessor.hh"

X86ISA::IntelMP::ConfigTable *
createIntelMPConfigTable(
    X86ISA::IntelMP::BaseConfigEntry * base_entries[], int num_base_entries,
    X86ISA::IntelMP::ExtConfigEntry  *  ext_entries[], int num_ext_entries);

X86ISA::IntelMP::FloatingPointer *
createIntelMPFloatingPointer();

X86ISA::IntelMP::Processor *
createIntelMPProcessor(
    uint8_t local_apic_id, uint8_t local_apic_version, bool enable, bool bootstrap);

X86ISA::IntelMP::IOAPIC *
createIntelMPIOAPIC(uint8_t id, uint8_t version, bool enable, uint32_t address);

X86ISA::IntelMP::Bus *
createIntelMPBus(std::string bus_type, uint8_t bus_id);

X86ISA::IntelMP::BusHierarchy *
createIntelMPBusHierarchy(uint8_t bus_id, bool subtractive_decode, uint8_t parent_bus);

X86ISA::IntelMP::IOIntAssignment *
createIntelMPIOIntAssignment(
    Enums::X86IntelMPInterruptType interrupt_type,
    Enums::X86IntelMPPolarity polarity,
    Enums::X86IntelMPTriggerMode trigger,
    uint8_t source_bus_id, uint8_t source_bus_irq,
    uint8_t dest_io_apic_id, uint8_t dest_io_apic_intin);


/**
 * E820
 **/
#include "arch/x86/bios/e820.hh"
#include "params/X86E820Entry.hh"
#include "params/X86E820Table.hh"

X86ISA::E820Entry *
createE820Entry(Addr addr, Addr size, uint32_t type);

X86ISA::E820Table *
createE820Table(X86ISA::E820Entry * entries[], int entries_count);

/**
 * SMBios
 **/
#include "arch/x86/bios/smbios.hh"
#include "params/X86SMBiosBiosInformation.hh"
#include "params/X86SMBiosSMBiosStructure.hh"
#include "params/X86SMBiosSMBiosTable.hh"

X86ISA::SMBios::SMBiosTable *
createSMBiosTable(X86ISA::SMBios::SMBiosStructure * structures[],
                  int structures_count);

X86ISA::SMBios::BiosInformation *
createSMBiosBiosInformation();


/**
 * ACPI
 **/

#include "arch/x86/bios/acpi.hh"
#include "params/X86ACPIRSDP.hh"
#include "params/X86ACPIXSDT.hh"
#include "params/X86ACPIRSDT.hh"

X86ISA::ACPI::RSDP * createACPIRSDP();
X86ISA::ACPI::XSDT * createACPIXSDT();
X86ISA::ACPI::RSDT * createACPIRSDT();

#endif	// __HYPER_ARCH_BIOS_HH__
