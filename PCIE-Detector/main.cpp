#pragma warning(disable : 4996)
#include <ntddk.h>

typedef struct _PCI_COMMON_CONFIG_CUSTOM {
    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)
    USHORT  Command;                    // Device control
    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)
    UCHAR   ProgIf;                     // (ro)
    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test
    union {
        struct _PCI_HEADER_TYPE_0 {
            ULONG   BaseAddresses[6];
            ULONG   CIS;
            USHORT  SubVendorID;
            USHORT  SubSystemID;
            ULONG   ROMBaseAddress;
            ULONG   Reserved2[2];
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            UCHAR   MinimumGrant;
            UCHAR   MaximumLatency;
        } type0;
    } u;
    UCHAR   DeviceSpecific[192];
} PCI_COMMON_CONFIG_CUSTOM, * PPCI_COMMON_CONFIG_CUSTOM;

#define VENDOR_ID_TO_DETECT 0x10EE
#define DEVICE_ID_TO_DETECT 0x0666

NTSTATUS ReadPCIConfigSpace(
    IN ULONG BusNumber,
    IN ULONG DeviceNumber,
    IN ULONG FunctionNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG Length
)
{
    PCI_SLOT_NUMBER slot;
    ULONG bytesRead = 0;

    // Initialize PCI slot structure
    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = DeviceNumber;
    slot.u.bits.FunctionNumber = FunctionNumber;

    // Read the PCI config space
    bytesRead = HalGetBusDataByOffset(
        PCIConfiguration,
        BusNumber,
        slot.u.AsULONG,
        Buffer,
        Offset,
        Length
    );

    if (bytesRead == Length) {
        return STATUS_SUCCESS;
    }
    else {
        return STATUS_UNSUCCESSFUL;
    }
}

BOOLEAN IsMatchingDevice(PPCI_COMMON_CONFIG_CUSTOM PciConfig)
{
    return (PciConfig->VendorID == VENDOR_ID_TO_DETECT) && (PciConfig->DeviceID == DEVICE_ID_TO_DETECT);
}

VOID PrintPCICommonConfig(PPCI_COMMON_CONFIG_CUSTOM PciConfig)
{
    KdPrint(("Vendor ID: %04x\n", PciConfig->VendorID));
    KdPrint(("Device ID: %04x\n", PciConfig->DeviceID));
    KdPrint(("Command: %04x\n", PciConfig->Command));
    KdPrint(("Status: %04x\n", PciConfig->Status));
    KdPrint(("Revision ID: %02x\n", PciConfig->RevisionID));
    KdPrint(("Prog IF: %02x\n", PciConfig->ProgIf));
    KdPrint(("Sub Class: %02x\n", PciConfig->SubClass));
    KdPrint(("Base Class: %02x\n", PciConfig->BaseClass));
    KdPrint(("Cache Line Size: %02x\n", PciConfig->CacheLineSize));
    KdPrint(("Latency Timer: %02x\n", PciConfig->LatencyTimer));
    KdPrint(("Header Type: %02x\n", PciConfig->HeaderType));
    KdPrint(("BIST: %02x\n", PciConfig->BIST));

    for (int i = 0; i < 6; i++) {
        KdPrint(("Base Address[%d]: %08x\n", i, PciConfig->u.type0.BaseAddresses[i]));
    }

    KdPrint(("Sub Vendor ID: %04x\n", PciConfig->u.type0.SubVendorID));
    KdPrint(("Sub System ID: %04x\n", PciConfig->u.type0.SubSystemID));
    KdPrint(("ROM Base Address: %08x\n", PciConfig->u.type0.ROMBaseAddress));
    KdPrint(("Interrupt Line: %02x\n", PciConfig->u.type0.InterruptLine));
    KdPrint(("Interrupt Pin: %02x\n", PciConfig->u.type0.InterruptPin));
    KdPrint(("Minimum Grant: %02x\n", PciConfig->u.type0.MinimumGrant));
    KdPrint(("Maximum Latency: %02x\n", PciConfig->u.type0.MaximumLatency));
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    KdPrint(("Driver Unload\n"));
}

extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = DriverUnload;

    KdPrint(("Driver Loaded\n"));

    BOOLEAN found = FALSE;

    for (ULONG bus = 0; bus < 256; bus++) {
        for (ULONG device = 0; device < 32; device++) {
            for (ULONG function = 0; function < 8; function++) {
                KdPrint(("Checking Bus %lu, Device %lu, Function %lu\n", bus, device, function));

                PCI_COMMON_CONFIG_CUSTOM pciConfig;
                NTSTATUS status = ReadPCIConfigSpace(bus, device, function, 0, &pciConfig, sizeof(pciConfig));

                if (NT_SUCCESS(status) && pciConfig.VendorID != 0xFFFF) { // Check if the device is present
                    if (IsMatchingDevice(&pciConfig)) {
                        KdPrint(("Matching: Found fucking DMA device on your machine!\n"));
                        PrintPCICommonConfig(&pciConfig);
                        found = TRUE;
                    }
                }
            }
        }
    }

    if (!found) {
        KdPrint(("No Matching Device Found\n"));
    }

    return STATUS_SUCCESS;
}
