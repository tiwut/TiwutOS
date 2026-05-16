#pragma once
#include "../api/types.h"
#include "pci.cpp"

class HDAudio {
  struct HDA_REGS {
    uint16_t gcap;
    uint8_t vmin;
    uint8_t vmaj;
    uint16_t outpay;
    uint16_t inpay;
    uint32_t gctl;
    uint16_t wakeen;
    uint16_t statests;
    uint16_t gsts;
    uint16_t rsvd;
    uint16_t outstrmpay;
    uint16_t instrmpay;
    uint32_t rsvd2[4];
    uint32_t intctl;
    uint32_t intsts;
  };

public:
  static HDA_REGS *regs;

  static void Init(PCIe::Device &dev) {
    if (dev.class_id != 0x04 || dev.subclass_id != 0x03)
      return;

    PCIe::EnableBusMastering(dev);
    regs = (HDA_REGS *)(uint64_t)(dev.bar[0] & 0xFFFFFFF0);

    regs->gctl &= ~1;
    while ((regs->gctl & 1) != 0)
      ;

    regs->gctl |= 1;
    int timeout = 10000;
    while ((regs->gctl & 1) == 0 && timeout--)
      ;

    if (timeout <= 0)
      return;

    timeout = 10000;
    while (regs->statests == 0 && timeout--)
      ;

    regs->intctl |= (1 << 31) | (1 << 30);
  }
};

HDAudio::HDA_REGS *HDAudio::regs = nullptr;
