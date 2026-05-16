#pragma once
#include "../api/types.h"
#include "pci.cpp"

class XHCI {
  struct XHCI_CAP_REGS {
    uint8_t caplength;
    uint8_t rsvd;
    uint16_t hciversion;
    uint32_t hcsparams1;
    uint32_t hcsparams2;
    uint32_t hcsparams3;
    uint32_t hccparams1;
    uint32_t dboff;
    uint32_t rtsoff;
    uint32_t hccparams2;
  };

  struct XHCI_OP_REGS {
    uint32_t usbcmd;
    uint32_t usbsts;
    uint32_t pagesize;
    uint8_t rsvd1[8];
    uint32_t dnctrl;
    uint64_t crcr;
    uint8_t rsvd2[16];
    uint64_t dcbaap;
    uint32_t config;
  };

public:
  static XHCI_CAP_REGS *cap_regs;
  static XHCI_OP_REGS *op_regs;

  static void Init(PCIe::Device &dev) {
    if (dev.class_id != 0x0C || dev.subclass_id != 0x03 || dev.prog_if != 0x30)
      return;

    PCIe::EnableBusMastering(dev);

    uint64_t base_addr = dev.bar[0] & 0xFFFFFFF0;
    if ((dev.bar[0] & 0x06) == 0x04) {
      base_addr |= ((uint64_t)dev.bar[1] << 32);
    }

    cap_regs = (XHCI_CAP_REGS *)base_addr;
    op_regs = (XHCI_OP_REGS *)(base_addr + cap_regs->caplength);

    op_regs->usbcmd &= ~1;
    while ((op_regs->usbsts & 1) == 0)
      ;

    op_regs->usbcmd |= 2;
    while ((op_regs->usbcmd & 2) != 0)
      ;
    while ((op_regs->usbsts & (1 << 29)) != 0)
      ;

    op_regs->dcbaap = 0x600000;

    uint32_t max_slots = cap_regs->hcsparams1 & 0xFF;
    op_regs->config = max_slots;

    op_regs->crcr = 0x610000 | 1;

    op_regs->usbcmd |= 1;
  }
};

XHCI::XHCI_CAP_REGS *XHCI::cap_regs = nullptr;
XHCI::XHCI_OP_REGS *XHCI::op_regs = nullptr;
