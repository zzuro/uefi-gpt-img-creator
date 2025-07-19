#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    uefi_call_wrapper((void *)(uintptr_t)SystemTable->ConOut->ClearScreen, 1, SystemTable->ConOut);    
    // Print something
    Print(L"Hello from UEFI!\n");

    // Wait for key press
    WaitForSingleEvent(SystemTable->ConIn->WaitForKey, 0);
    return EFI_SUCCESS;
}
