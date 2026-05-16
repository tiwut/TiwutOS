#pragma once
#include "../api/types.h"
#include "pci.cpp"

class NVMe {
  struct NVMe_BAR {
    uint64_t cap;
    uint32_t vs;
    uint32_t intms;
    uint32_t intmc;
    uint32_t cc;
    uint32_t csts;
    uint32_t nssr;
    uint32_t aqa;
    uint64_t asq;
    uint64_t acq;
  };

public:
  static NVMe_BAR *regs;

  static void Init(PCIe::Device &dev) {
    if (dev.class_id != 0x01 || dev.subclass_id != 0x08 || dev.prog_if != 0x02)
      return;

    PCIe::EnableBusMastering(dev);
    regs = (NVMe_BAR *)(uint64_t)(dev.bar[0] & 0xFFFFFFF0);

    regs->cc &= ~1;
    while ((regs->csts & 1) == 1)
      ;

    regs->aqa = (63 << 16) | 63;
    regs->asq = 0x500000;
    regs->acq = 0x510000;

    uint32_t cc = regs->cc;
    cc |= (0 << 4);
    cc |= (0 << 7);
    cc |= (6 << 16);
    cc |= (4 << 20);
    cc |= 1;
    regs->cc = cc;

    while ((regs->csts & 1) == 0)
      ;
  }
};

NVMe::NVMe_BAR *NVMe::regs = nullptr;
