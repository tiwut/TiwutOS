#pragma once
#include "../api/types.h"
#include "../components/utils.cpp"

class PCIe {
public:
  struct Device {
    uint8_t bus, slot, func;
    uint16_t vendor_id, device_id;
    uint8_t class_id, subclass_id, prog_if;
    uint32_t bar[6];
    bool is_pcie;
  };

  static uint32_t Read(uint8_t bus, uint8_t slot, uint8_t func,
                       uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                                  (offset & 0xFC) | 0x80000000);
    Port::out32(0xCF8, address);
    return Port::in32(0xCFC);
  }

  static void Write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                    uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                                  (offset & 0xFC) | 0x80000000);
    Port::out32(0xCF8, address);
    Port::out32(0xCFC, value);
  }

  static Device GetDevice(uint8_t bus, uint8_t slot, uint8_t func) {
    Device dev;
    dev.bus = bus;
    dev.slot = slot;
    dev.func = func;
    uint32_t id_reg = Read(bus, slot, func, 0x00);
    dev.vendor_id = id_reg & 0xFFFF;
    dev.device_id = (id_reg >> 16) & 0xFFFF;

    if (dev.vendor_id == 0xFFFF)
      return dev;

    uint32_t class_reg = Read(bus, slot, func, 0x08);
    dev.prog_if = (class_reg >> 8) & 0xFF;
    dev.subclass_id = (class_reg >> 16) & 0xFF;
    dev.class_id = (class_reg >> 24) & 0xFF;

    for (int i = 0; i < 6; i++) {
      dev.bar[i] = Read(bus, slot, func, 0x10 + (i * 4));
    }

    dev.is_pcie = (Read(bus, slot, func, 0x34) & 0xFF) != 0;
    return dev;
  }

  static void EnableBusMastering(Device &dev) {
    uint32_t cmd_status = Read(dev.bus, dev.slot, dev.func, 0x04);
    cmd_status |= (1 << 2);
    cmd_status |= (1 << 1);
    Write(dev.bus, dev.slot, dev.func, 0x04, cmd_status);
  }
};
