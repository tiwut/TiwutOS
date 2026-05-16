#pragma once
#include "../api/types.h"
#include "pci.cpp"

class AHCI {
  struct HBA_PORT {
    uint32_t clb, clbu, fb, fbu, is, ie, cmd, rsv0, tfd, sig, ssts, sctl, serr,
        sact, ci, sntf, fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
  };

  struct HBA_MEM {
    uint32_t cap, ghc, is, pi, vs, ccc_ctl, ccc_pts, em_loc, em_ctl, cap2, bohc;
    uint8_t rsv[0x74];
    uint8_t vendor[0x60];
    HBA_PORT ports[32];
  };

  struct HBA_CMD_HEADER {
    uint8_t cfl : 5, a : 1, w : 1, p : 1;
    uint8_t r : 1, b : 1, c : 1, rsv0 : 1, pmp : 4;
    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba, ctbau;
    uint32_t rsv1[4];
  };

  struct HBA_PRDT_ENTRY {
    uint32_t dba, dbau, rsv0;
    uint32_t dbc : 22, rsv1 : 9, i : 1;
  };

  struct HBA_CMD_TBL {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];
    HBA_PRDT_ENTRY prdt_entry[1];
  };

public:
  static HBA_MEM *abar;
  static HBA_PORT *active_port;

  static void Init(PCIe::Device &dev) {
    if (dev.class_id != 0x01 || dev.subclass_id != 0x06)
      return;

    PCIe::EnableBusMastering(dev);
    abar = (HBA_MEM *)(uint64_t)(dev.bar[5] & 0xFFFFFFF0);
    abar->ghc |= (1 << 31);
    abar->ghc |= (1 << 0);

    int timeout = 100000;
    while ((abar->ghc & 1) && timeout--)
      ;

    abar->ghc |= (1 << 31);

    uint32_t pi = abar->pi;
    for (int i = 0; i < 32; i++) {
      if (pi & (1 << i)) {
        uint32_t ssts = abar->ports[i].ssts;
        uint8_t ipm = (ssts >> 8) & 0x0F;
        uint8_t det = ssts & 0x0F;

        if (det == 3 && ipm == 1) {
          if (!active_port) {
            active_port = &abar->ports[i];
            ConfigurePort(active_port);
          }
        }
      }
    }
  }

  static bool ReadSector(uint64_t startlba, uint32_t count, uint8_t *buffer) {
    if (!active_port)
      return false;

    active_port->is = 0xFFFFFFFF;
    int slot = FindCmdSlot(active_port);
    if (slot == -1)
      return false;

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(uint64_t)active_port->clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(uint32_t) * 5 / sizeof(uint32_t);
    cmdheader->w = 0;
    cmdheader->prdtl = 1;

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)(uint64_t)cmdheader->ctba;
    Utils::fill32((uint8_t *)cmdtbl, 0, sizeof(HBA_CMD_TBL) / 4);

    cmdtbl->prdt_entry[0].dba = (uint32_t)(uint64_t)buffer;
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmdtbl->prdt_entry[0].i = 1;

    uint8_t *cmdfis = (uint8_t *)(&cmdtbl->cfis);
    cmdfis[0] = 0x27;
    cmdfis[1] = 0x80;
    cmdfis[2] = 0x24;
    cmdfis[4] = (uint8_t)(startlba & 0xFF);
    cmdfis[5] = (uint8_t)((startlba >> 8) & 0xFF);
    cmdfis[6] = (uint8_t)((startlba >> 16) & 0xFF);
    cmdfis[7] = 0x40;
    cmdfis[8] = (uint8_t)((startlba >> 24) & 0xFF);
    cmdfis[9] = (uint8_t)((startlba >> 32) & 0xFF);
    cmdfis[10] = (uint8_t)((startlba >> 40) & 0xFF);
    cmdfis[12] = (uint8_t)(count & 0xFF);
    cmdfis[13] = (uint8_t)((count >> 8) & 0xFF);

    while ((active_port->tfd & (0x80 | 0x08)))
      ;
    active_port->ci = 1 << slot;
    while (1) {
      if ((active_port->ci & (1 << slot)) == 0)
        break;
      if (active_port->is & (1 << 30))
        return false;
    }
    return true;
  }

private:
  static int FindCmdSlot(HBA_PORT *port) {
    uint32_t slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
      if ((slots & (1 << i)) == 0)
        return i;
    }
    return -1;
  }

  static void ConfigurePort(HBA_PORT *port) {
    port->cmd &= ~(1 << 0);
    port->cmd &= ~(1 << 4);
    while (port->cmd & (1 << 15))
      ;
    while (port->cmd & (1 << 14))
      ;

    port->clb = 0x400000;
    port->clbu = 0;
    port->fb = 0x410000;
    port->fbu = 0;

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)0x400000;
    for (int i = 0; i < 32; i++) {
      cmdheader[i].prdtl = 8;
      cmdheader[i].ctba = 0x420000 + (i * 256);
      cmdheader[i].ctbau = 0;
    }

    port->cmd |= (1 << 4);
    port->cmd |= (1 << 0);
  }
};

AHCI::HBA_MEM *AHCI::abar = nullptr;
AHCI::HBA_PORT *AHCI::active_port = nullptr;
