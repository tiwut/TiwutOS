#include "../api/types.h"

typedef unsigned short CHAR16;
typedef unsigned long long EFI_STATUS;
typedef void *EFI_HANDLE;

struct EFI_INPUT_KEY {
  unsigned short ScanCode;
  CHAR16 UnicodeChar;
};

struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
  void *Reset;
  EFI_STATUS (*ReadKeyStroke)(struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *,
                              EFI_INPUT_KEY *);
};

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  void *Reset;
  EFI_STATUS (*OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *,
                             CHAR16 *);
  void *TestString;
  void *QueryMode;
  void *SetMode;
  void *SetAttribute;
  EFI_STATUS (*ClearScreen)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *);
};

struct EFI_GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
};

struct EFI_PIXEL_BITMASK {
  uint32_t R;
  uint32_t G;
  uint32_t B;
  uint32_t Res;
};

struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
  uint32_t Version;
  uint32_t HorizontalResolution;
  uint32_t VerticalResolution;
  uint32_t PixelFormat;
  EFI_PIXEL_BITMASK PixelInformation;
  uint32_t PixelsPerScanLine;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
  uint32_t MaxMode;
  uint32_t Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  uint64_t SizeOfInfo;
  uint64_t FrameBufferBase;
  uint64_t FrameBufferSize;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
  void *QueryMode;
  void *SetMode;
  void *Blt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
};

struct EFI_TABLE_HEADER {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
};

struct EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER Hdr;
  void *RaiseTPL;
  void *RestoreTPL;
  void *AllocatePages;
  void *FreePages;
  EFI_STATUS (*GetMemoryMap)(uint64_t *, void *, uint64_t *, uint64_t *,
                             uint32_t *);
  EFI_STATUS (*AllocatePool)(int, uint64_t, void **);
  void *FreePool;
  void *CreateEvent;
  void *SetTimer;
  void *WaitForEvent;
  void *SignalEvent;
  void *CloseEvent;
  void *CheckEvent;
  void *InstallProtocolInterface;
  void *ReinstallProtocolInterface;
  void *UninstallProtocolInterface;
  void *HandleProtocol;
  void *Reserved;
  void *RegisterProtocolNotify;
  void *LocateHandle;
  void *LocateDevicePath;
  void *InstallConfigurationTable;
  void *LoadImage;
  void *StartImage;
  void *Exit;
  void *UnloadImage;
  EFI_STATUS (*ExitBootServices)(EFI_HANDLE, uint64_t);
  void *GetNextMonotonicCount;
  void *Stall;
  void *SetWatchdogTimer;
  void *ConnectController;
  void *DisconnectController;
  void *OpenProtocol;
  void *CloseProtocol;
  void *OpenProtocolInformation;
  void *ProtocolsPerHandle;
  void *LocateHandleBuffer;
  EFI_STATUS (*LocateProtocol)(EFI_GUID *, void *, void **);
};

struct EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER Hdr;
  CHAR16 *FirmwareVendor;
  unsigned int FirmwareRevision;
  void *ConsoleInHandle;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  void *ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
  void *StandardErrorHandle;
  void *StdErr;
  void *RuntimeServices;
  EFI_BOOT_SERVICES *BootServices;
};

extern "C" void cpp_main(multiboot_info *mb);

extern "C" EFI_STATUS efi_main(EFI_HANDLE ImageHandle,
                               EFI_SYSTEM_TABLE *SystemTable) {
  SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

  EFI_GUID gop_guid = {0x9042a9de,
                       0x23dc,
                       0x4a38,
                       {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = 0;
  SystemTable->BootServices->LocateProtocol(&gop_guid, 0, (void **)&gop);

  multiboot_info fake_mb;
  for (int i = 0; i < sizeof(multiboot_info); i++)
    ((char *)&fake_mb)[i] = 0;

  if (gop && gop->Mode && gop->Mode->Info) {
    fake_mb.fb_width = gop->Mode->Info->HorizontalResolution;
    fake_mb.fb_height = gop->Mode->Info->VerticalResolution;
    fake_mb.fb_pitch = gop->Mode->Info->PixelsPerScanLine * 4;
    fake_mb.fb_addr = gop->Mode->FrameBufferBase;
  } else {
    fake_mb.fb_addr = 0xA0000;
    fake_mb.fb_width = 320;
    fake_mb.fb_height = 200;
    fake_mb.fb_pitch = 320;
  }

  uint64_t map_size = 0, map_key = 0, desc_size = 0;
  uint32_t desc_version = 0;
  SystemTable->BootServices->GetMemoryMap(&map_size, 0, &map_key, &desc_size,
                                          &desc_version);

  map_size += 4096;
  void *mmap = 0;
  SystemTable->BootServices->AllocatePool(2, map_size, &mmap);

  SystemTable->BootServices->GetMemoryMap(&map_size, mmap, &map_key, &desc_size,
                                          &desc_version);
  EFI_STATUS status =
      SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);
  if (status != 0) {
    SystemTable->BootServices->GetMemoryMap(&map_size, mmap, &map_key,
                                            &desc_size, &desc_version);
    SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);
  }

  cpp_main(&fake_mb);

  while (1) {
  }
  return 0;
}

#include "../sys/core.cpp"
