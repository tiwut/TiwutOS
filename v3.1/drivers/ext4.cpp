#pragma once
#include "../api/types.h"
#include "../components/utils.cpp"
#include "ahci.cpp"

class Ext4 {
public:
  struct Superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count_lo;
    uint32_t s_r_blocks_count_lo;
    uint32_t s_free_blocks_count_lo;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_cluster_size;
    uint32_t s_blocks_per_group;
    uint32_t s_clusters_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];
  } __attribute__((packed));

  static Superblock sb;
  static bool is_mounted;
  static uint32_t block_size;
  static char mount_info[1024];

  static void Mount() {
    is_mounted = false;
    Utils::strcpy(mount_info, "Not Mounted");

    uint8_t buffer[1024];
    if (!AHCI::ReadSector(2, 2, buffer)) {
      Utils::strcpy(mount_info, "AHCI Read Failed. No disk found.");
      return;
    }

    for (int i = 0; i < sizeof(Superblock); i++)
      ((uint8_t *)&sb)[i] = buffer[i];

    if (sb.s_magic != 0xEF53) {
      Utils::strcpy(mount_info, "Invalid Magic Number. Not an Ext4 partition.");
      return;
    }

    block_size = 1024 << sb.s_log_block_size;
    is_mounted = true;

    int len = 0;
    Utils::strcpy(mount_info, "Ext4 Volume Mounted Successfully!\n");
    len = Utils::strlen(mount_info);
    Utils::strcpy(mount_info + len, "Volume Name: ");
    len = Utils::strlen(mount_info);
    int vn_len = 0;
    while (sb.s_volume_name[vn_len] && vn_len < 16) {
      mount_info[len++] = sb.s_volume_name[vn_len++];
    }
    mount_info[len++] = '\n';
    mount_info[len] = 0;

    Utils::strcpy(mount_info + len, "Total Inodes: ");
    len = Utils::strlen(mount_info);
    Utils::itoa(sb.s_inodes_count, mount_info + len);
    len = Utils::strlen(mount_info);
    Utils::strcpy(mount_info + len, "\nTotal Blocks: ");
    len = Utils::strlen(mount_info);
    Utils::itoa(sb.s_blocks_count_lo, mount_info + len);
    len = Utils::strlen(mount_info);
    Utils::strcpy(mount_info + len, "\nBlock Size: ");
    len = Utils::strlen(mount_info);
    Utils::itoa(block_size, mount_info + len);
    len = Utils::strlen(mount_info);
    Utils::strcpy(mount_info + len, " Bytes\nFree Blocks: ");
    len = Utils::strlen(mount_info);
    Utils::itoa(sb.s_free_blocks_count_lo, mount_info + len);
    len = Utils::strlen(mount_info);

    Utils::strcpy(mount_info + len, "\nFeatures: ");
    len = Utils::strlen(mount_info);
    if (sb.s_feature_incompat & 0x40) {
      Utils::strcpy(mount_info + len, "EXTENTS ");
      len = Utils::strlen(mount_info);
    }
    if (sb.s_feature_incompat & 0x80) {
      Utils::strcpy(mount_info + len, "64BIT ");
      len = Utils::strlen(mount_info);
    }
    if (sb.s_feature_incompat & 0x10000) {
      Utils::strcpy(mount_info + len, "FLEX_BG ");
      len = Utils::strlen(mount_info);
    }
    if (sb.s_feature_ro_compat & 0x01) {
      Utils::strcpy(mount_info + len, "SPARSE_SUPER ");
      len = Utils::strlen(mount_info);
    }
    if (sb.s_feature_ro_compat & 0x02) {
      Utils::strcpy(mount_info + len, "LARGE_FILE ");
      len = Utils::strlen(mount_info);
    }
    if (sb.s_feature_ro_compat & 0x04) {
      Utils::strcpy(mount_info + len, "BTREE_DIR ");
      len = Utils::strlen(mount_info);
    }
  }
};

Ext4::Superblock Ext4::sb;
bool Ext4::is_mounted = false;
uint32_t Ext4::block_size = 0;
char Ext4::mount_info[1024];
