#include "hyper/arch/bios.hh"

using namespace X86ISA;

/**
 * IntelMP
 **/
X86ISA::IntelMP::ConfigTable *
createIntelMPConfigTable(
    X86ISA::IntelMP::BaseConfigEntry * base_entries[], int num_base_entries,
    X86ISA::IntelMP::ExtConfigEntry  *  ext_entries[], int num_ext_entries)
{
    X86IntelMPConfigTableParams *params = new X86IntelMPConfigTableParams();
    params->name = "dynamic_created_X86ISA::IntelMP::ConfigTable";
    params->pyobj = NULL;

    params->spec_rev = 4;
    params->oem_id = "";
    params->product_id = "";
    params->oem_table_addr = 0;
    params->oem_table_size = 0;
    params->local_apic = 0xFEE00000;

    for (int i=0; i<num_base_entries; i++)
        params->base_entries.push_back(base_entries[i]);
    for (int i=0; i<num_ext_entries; i++)
        params->ext_entries.push_back(ext_entries[i]);

    return params->create();
}

X86ISA::IntelMP::FloatingPointer *
createIntelMPFloatingPointer()
{
    X86IntelMPFloatingPointerParams *params = new X86IntelMPFloatingPointerParams();
    params->name = "dynamic_created_X86ISA::IntelMP::FloatingPointer";
    params->pyobj = NULL;

    params->spec_rev = 4;
    params->default_config = 0;
    params->imcr_present = true;

    return params->create();
}
 
X86ISA::IntelMP::Processor *
createIntelMPProcessor(
    uint8_t local_apic_id, uint8_t local_apic_version, bool enable, bool bootstrap)
{
    X86IntelMPProcessorParams *params = new X86IntelMPProcessorParams();
    params->name = "dynamic_created_X86ISA::IntelMP::Processor";
    params->pyobj = NULL;

    params->local_apic_id = local_apic_id;
    params->local_apic_version = local_apic_version;
    params->enable = enable;
    params->bootstrap = bootstrap;
    params->family = 0;
    params->feature_flags = 0;
    params->model = 0;
    params->stepping = 0; 

    return params->create();
}

X86ISA::IntelMP::IOAPIC *
createIntelMPIOAPIC(uint8_t id, uint8_t version, bool enable, uint32_t address)
{
    X86IntelMPIOAPICParams *params = new X86IntelMPIOAPICParams();
    params->name = "dynamic_created_X86ISA::IntelMP::IOAPIC";
    params->pyobj = NULL;

    params->id = id;
    params->version = version;
    params->enable = enable;
    params->address = address;

    return params->create();
}

X86ISA::IntelMP::Bus *
createIntelMPBus(std::string bus_type, uint8_t bus_id)
{
    X86IntelMPBusParams *params = new X86IntelMPBusParams();
    params->name = "dynamic_created_X86ISA::IntelMP::Bus";
    params->pyobj = NULL;

    params->bus_type = bus_type;
    params->bus_id = bus_id;

    return params->create();
}

X86ISA::IntelMP::BusHierarchy *
createIntelMPBusHierarchy(uint8_t bus_id, bool subtractive_decode, uint8_t parent_bus)
{
    X86IntelMPBusHierarchyParams *params = new X86IntelMPBusHierarchyParams();
    params->name = "dynamic_created_X86ISA::IntelMP::BusHierarchy";
    params->pyobj = NULL;

    params->bus_id = bus_id;
    params->subtractive_decode = subtractive_decode;
    params->parent_bus = parent_bus;

    return params->create();
}

X86ISA::IntelMP::IOIntAssignment *
createIntelMPIOIntAssignment(
    Enums::X86IntelMPInterruptType interrupt_type,
    Enums::X86IntelMPPolarity polarity,
    Enums::X86IntelMPTriggerMode trigger,
    uint8_t source_bus_id, uint8_t source_bus_irq,
    uint8_t dest_io_apic_id, uint8_t dest_io_apic_intin)
{
    X86IntelMPIOIntAssignmentParams *params = new X86IntelMPIOIntAssignmentParams();
    params->name = "dynamic_created_X86ISA::IntelMP::IOIntAssignment";
    params->pyobj = NULL;

    params->interrupt_type     = interrupt_type;
    params->polarity           = polarity;
    params->trigger            = trigger;
    params->source_bus_id      = source_bus_id;
    params->source_bus_irq     = source_bus_irq;
    params->dest_io_apic_id    = dest_io_apic_id;
    params->dest_io_apic_intin = dest_io_apic_intin;

    return params->create();
}


/**
 * E820
 **/

E820Entry *
createE820Entry(Addr addr, Addr size, uint32_t type)
{
    X86E820EntryParams *params = new X86E820EntryParams();
    params->name = "dynamic_created_e820_entry";
    params->pyobj = NULL;
    params->range_type = type;
    params->addr = addr;
    params->size = size;
    return params->create();
}

E820Table *
createE820Table(E820Entry * entries[], int entries_count)
{
    X86E820TableParams *params = new X86E820TableParams();
    params->name = "dynamic_created_e820_table";
    params->pyobj = NULL;
    for (int i=0; i<entries_count; i++)
        params->entries.push_back(entries[i]);
    return params->create();
}


/**
 * SMBios
 **/

X86ISA::SMBios::SMBiosTable *
createSMBiosTable(X86ISA::SMBios::SMBiosStructure * structures[],
                  int structures_count)
{
    X86SMBiosSMBiosTableParams *params = new X86SMBiosSMBiosTableParams();
    params->name = "dynamic_created_smbios_table";
    params->pyobj = NULL;
    params->major_version = 2;
    params->minor_version = 5;
    for (int i=0; i<structures_count; i++)
        params->structures.push_back(structures[i]);
    return params->create();
}

X86ISA::SMBios::BiosInformation *
createSMBiosBiosInformation()
{
    X86SMBiosBiosInformationParams *params = new X86SMBiosBiosInformationParams();
    params->name = "dynamic_created_smbios_biosinformation";
    params->pyobj = NULL;
    params->vendor = "";
    params->version = "";
    params->starting_addr_segment = 0;
    params->release_date = "06/08/2008";
    params->rom_size = 0;
    //params->characteristics []
    //params->characteristic_ext_bytes []
    params->major = 0;
    params->minor = 0;
    params->emb_cont_firmware_major = 0;
    params->emb_cont_firmware_minor = 0;

    return params->create();
}


/**
 * ACPI
 **/
X86ISA::ACPI::RSDP * createACPIRSDP()
{
    X86ACPIRSDPParams *params = new X86ACPIRSDPParams();
    params->name = "dynamic_created_X86ISA::ACPI::RSDP";
    params->pyobj = NULL;

    params->oem_id = "";
    params->revision = 2;
    params->rsdt = NULL;
    params->xsdt = createACPIXSDT();

    return params->create();
}

X86ISA::ACPI::XSDT * createACPIXSDT()
{
    X86ACPIXSDTParams *params = new X86ACPIXSDTParams();
    params->name = "dynamic_created_X86ISA::ACPI::XSDT";
    params->pyobj = NULL;

    params->oem_id = "";
    params->oem_table_id = "";
    params->oem_revision = 0;
    params->creator_id = "";
    params->creator_revision = 0;
    // params->entries []

    return params->create();
}

X86ISA::ACPI::RSDT * createACPIRSDT()
{
    X86ACPIRSDTParams *params = new X86ACPIRSDTParams();
    params->name = "dynamic_created_X86ISA::ACPI::RSDT";
    params->pyobj = NULL;

    params->oem_id = "";
    params->oem_table_id = "";
    params->oem_revision = 0;
    params->creator_id = "";
    params->creator_revision = 0;
    // params->entries []

    return params->create();
}
