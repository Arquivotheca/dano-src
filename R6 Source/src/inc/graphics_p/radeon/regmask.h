#if !defined (_rage6_MASK_HEADER)
#define _rage6_MASK_HEADER
/*
*        rage6_mask.h
*
*	 (c) 2000 ATI Technologies Inc.  (unpublished)
*
*	 All rights reserved.  This notice is intended as a precaution against
*	 inadvertent publication and does not imply publication or any waiver
*	 of confidentiality.  The year included in the foregoing notice is the
*	 year of creation of the work.
*
*/

// GENENB
#define GENENB__BLK_IO_BASE_MASK                           0x000000ff

// MM_INDEX
#define MM_INDEX__MM_ADDR_MASK                             0x7fffffff
#define MM_INDEX__MM_APER_MASK                             0x80000000
#define MM_INDEX__MM_APER                                  0x80000000

// MM_DATA
#define MM_DATA__MM_DATA_MASK                              0xffffffff

// BUS_CNTL
#define BUS_CNTL__BUS_DBL_RESYNC_MASK                      0x00000001
#define BUS_CNTL__BUS_DBL_RESYNC                           0x00000001
#define BUS_CNTL__BUS_MSTR_RESET_MASK                      0x00000002
#define BUS_CNTL__BUS_MSTR_RESET                           0x00000002
#define BUS_CNTL__BUS_FLUSH_BUF_MASK                       0x00000004
#define BUS_CNTL__BUS_FLUSH_BUF                            0x00000004
#define BUS_CNTL__BUS_STOP_REQ_DIS_MASK                    0x00000008
#define BUS_CNTL__BUS_STOP_REQ_DIS                         0x00000008
#define BUS_CNTL__BUS_READ_COMBINE_EN_MASK                 0x00000010
#define BUS_CNTL__BUS_READ_COMBINE_EN                      0x00000010
#define BUS_CNTL__BUS_WRT_COMBINE_EN_MASK                  0x00000020
#define BUS_CNTL__BUS_WRT_COMBINE_EN                       0x00000020
#define BUS_CNTL__BUS_MASTER_DIS_MASK                      0x00000040
#define BUS_CNTL__BUS_MASTER_DIS                           0x00000040
#define BUS_CNTL__BIOS_ROM_WRT_EN_MASK                     0x00000080
#define BUS_CNTL__BIOS_ROM_WRT_EN                          0x00000080
#define BUS_CNTL__BUS_PREFETCH_MODE_MASK                   0x00000300
#define BUS_CNTL__BUS_VGA_PREFETCH_EN_MASK                 0x00000400
#define BUS_CNTL__BUS_VGA_PREFETCH_EN                      0x00000400
#define BUS_CNTL__BUS_SGL_READ_DISABLE_MASK                0x00000800
#define BUS_CNTL__BUS_SGL_READ_DISABLE                     0x00000800
#define BUS_CNTL__BIOS_DIS_ROM_MASK                        0x00001000
#define BUS_CNTL__BIOS_DIS_ROM                             0x00001000
#define BUS_CNTL__BUS_PCI_READ_RETRY_EN_MASK               0x00002000
#define BUS_CNTL__BUS_PCI_READ_RETRY_EN                    0x00002000
#define BUS_CNTL__BUS_AGP_AD_STEPPING_EN_MASK              0x00004000
#define BUS_CNTL__BUS_AGP_AD_STEPPING_EN                   0x00004000
#define BUS_CNTL__BUS_PCI_WRT_RETRY_EN_MASK                0x00008000
#define BUS_CNTL__BUS_PCI_WRT_RETRY_EN                     0x00008000
#define BUS_CNTL__BUS_RETRY_WS_MASK                        0x000f0000
#define BUS_CNTL__BUS_MSTR_RD_MULT_MASK                    0x00100000
#define BUS_CNTL__BUS_MSTR_RD_MULT                         0x00100000
#define BUS_CNTL__BUS_MSTR_RD_LINE_MASK                    0x00200000
#define BUS_CNTL__BUS_MSTR_RD_LINE                         0x00200000
#define BUS_CNTL__BUS_SUSPEND_MASK                         0x00400000
#define BUS_CNTL__BUS_SUSPEND                              0x00400000
#define BUS_CNTL__LAT_16X_MASK                             0x00800000
#define BUS_CNTL__LAT_16X                                  0x00800000
#define BUS_CNTL__BUS_RD_DISCARD_EN_MASK                   0x01000000
#define BUS_CNTL__BUS_RD_DISCARD_EN                        0x01000000
#define BUS_CNTL__ENFRCWRDY_MASK                           0x02000000
#define BUS_CNTL__ENFRCWRDY                                0x02000000
#define BUS_CNTL__BUS_MSTR_WS_MASK                         0x04000000
#define BUS_CNTL__BUS_MSTR_WS                              0x04000000
#define BUS_CNTL__BUS_PARKING_DIS_MASK                     0x08000000
#define BUS_CNTL__BUS_PARKING_DIS                          0x08000000
#define BUS_CNTL__BUS_MSTR_DISCONNECT_EN_MASK              0x10000000
#define BUS_CNTL__BUS_MSTR_DISCONNECT_EN                   0x10000000
#define BUS_CNTL__SERR_EN_MASK                             0x20000000
#define BUS_CNTL__SERR_EN                                  0x20000000
#define BUS_CNTL__BUS_READ_BURST_MASK                      0x40000000
#define BUS_CNTL__BUS_READ_BURST                           0x40000000
#define BUS_CNTL__BUS_RDY_READ_DLY_MASK                    0x80000000
#define BUS_CNTL__BUS_RDY_READ_DLY                         0x80000000

// HI_STAT
#define HI_STAT__HI_STAT_MASK                              0x00000007
#define HI_STAT__AGP_BUSY_MASK                             0x00000008
#define HI_STAT__AGP_BUSY                                  0x00000008

// BUS_CNTL1
#define BUS_CNTL1__PMI_IO_DISABLE_MASK                     0x00000001
#define BUS_CNTL1__PMI_IO_DISABLE                          0x00000001
#define BUS_CNTL1__PMI_MEM_DISABLE_MASK                    0x00000002
#define BUS_CNTL1__PMI_MEM_DISABLE                         0x00000002
#define BUS_CNTL1__PMI_BM_DISABLE_MASK                     0x00000004
#define BUS_CNTL1__PMI_BM_DISABLE                          0x00000004
#define BUS_CNTL1__PMI_INT_DISABLE_MASK                    0x00000008
#define BUS_CNTL1__PMI_INT_DISABLE                         0x00000008
#define BUS_CNTL1__BUS2_STALE_DATA_TIMER_MASK              0x000000f0
#define BUS_CNTL1__BUS2_VGA_REG_COHERENCY_DIS_MASK         0x00000100
#define BUS_CNTL1__BUS2_VGA_REG_COHERENCY_DIS              0x00000100
#define BUS_CNTL1__BUS2_VGA_MEM_COHERENCY_DIS_MASK         0x00000200
#define BUS_CNTL1__BUS2_VGA_MEM_COHERENCY_DIS              0x00000200
#define BUS_CNTL1__BUS2_HDP_REG_COHERENCY_DIS_MASK         0x00000400
#define BUS_CNTL1__BUS2_HDP_REG_COHERENCY_DIS              0x00000400
#define BUS_CNTL1__BUS2_GUI_INITIATOR_COHERENCY_DIS_MASK   0x00000800
#define BUS_CNTL1__BUS2_GUI_INITIATOR_COHERENCY_DIS        0x00000800

// CONFIG_CNTL
#define CONFIG_CNTL__APER_REG_ENDIAN_MASK                  0x00000030
#define CONFIG_CNTL__CFG_VGA_RAM_EN_MASK                   0x00000100
#define CONFIG_CNTL__CFG_VGA_RAM_EN                        0x00000100
#define CONFIG_CNTL__CFG_VGA_IO_DIS_MASK                   0x00000200
#define CONFIG_CNTL__CFG_VGA_IO_DIS                        0x00000200
#define CONFIG_CNTL__CFG_ATI_REV_ID_MASK                   0x000f0000

// CONFIG_MEMSIZE
#define CONFIG_MEMSIZE__CONFIG_MEMSIZE_MASK                0x1f000000

// CONFIG_APER_0_BASE
#define CONFIG_APER_0_BASE__APER_0_BASE_MASK               0xfe000000

// CONFIG_APER_1_BASE
#define CONFIG_APER_1_BASE__APER_1_BASE_MASK               0xff000000

// CONFIG_APER_SIZE
#define CONFIG_APER_SIZE__APER_SIZE_MASK                   0x0f000000

// CONFIG_REG_1_BASE
#define CONFIG_REG_1_BASE__REG_APER_1_SELECT_MASK          0x00040000
#define CONFIG_REG_1_BASE__REG_APER_1_SELECT               0x00040000
#define CONFIG_REG_1_BASE__REG_1_BASE_MASK                 0xfff80000

// CONFIG_REG_APER_SIZE
#define CONFIG_REG_APER_SIZE__REG_APER_SIZE_MASK           0x0007ffff

// PAD_AGPINPUT_DELAY
#define PAD_AGPINPUT_DELAY__PAD_AGPINPUT_DELAY_MASK        0xffffffff

// PAD_CTLR_STRENGTH
#define PAD_CTLR_STRENGTH__PAD_N_STRENGTH_READ_BACK_MASK   0x0000000f
#define PAD_CTLR_STRENGTH__PAD_P_STRENGTH_READ_BACK_MASK   0x000000f0
#define PAD_CTLR_STRENGTH__PAD_N_MANUAL_STRENGTH_MASK      0x00000f00
#define PAD_CTLR_STRENGTH__PAD_P_MANUAL_STRENGTH_MASK      0x0000f000
#define PAD_CTLR_STRENGTH__PAD_MANUAL_OVERRIDE_MASK        0x00010000
#define PAD_CTLR_STRENGTH__PAD_MANUAL_OVERRIDE             0x00010000
#define PAD_CTLR_STRENGTH__PAD_TEST_OUT_MASK               0x00020000
#define PAD_CTLR_STRENGTH__PAD_TEST_OUT                    0x00020000
#define PAD_CTLR_STRENGTH__PAD_DUMMY_OUT_MASK              0x00040000
#define PAD_CTLR_STRENGTH__PAD_DUMMY_OUT                   0x00040000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DFR_MASK              0x00080000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DFR                   0x00080000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SCHMEN_MASK           0x00100000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SCHMEN                0x00100000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DREN_MASK             0x00200000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DREN                  0x00200000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SLEW_MASK             0x00400000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SLEW                  0x00400000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_VDIFF_MASK            0x00800000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_VDIFF                 0x00800000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DFR_OVERRIDE_MASK     0x01000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DFR_OVERRIDE          0x01000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SCHMEN_OVERRIDE_MASK  0x02000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SCHMEN_OVERRIDE       0x02000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DREN_OVERRIDE_MASK    0x04000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_DREN_OVERRIDE         0x04000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SLEW_OVERRIDE_MASK    0x08000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_SLEW_OVERRIDE         0x08000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_VDIFF_OVERRIDE_MASK   0x10000000
#define PAD_CTLR_STRENGTH__PAD_HI_IO_VDIFF_OVERRIDE        0x10000000

// PAD_CTLR_UPDATE
#define PAD_CTLR_UPDATE__PAD_UPDATE_RATE_MASK              0x0000001f
#define PAD_CTLR_UPDATE__PAD_SAMPLE_DELAY_MASK             0x00001f00
#define PAD_CTLR_UPDATE__PAD_INC_THRESHOLD_MASK            0x001f0000
#define PAD_CTLR_UPDATE__PAD_DEC_THRESHOLD_MASK            0x1f000000

// AGP_CNTL
#define AGP_CNTL__MAX_IDLE_CLK_MASK                        0x000000ff
#define AGP_CNTL__HOLD_RD_FIFO_MASK                        0x00000100
#define AGP_CNTL__HOLD_RD_FIFO                             0x00000100
#define AGP_CNTL__HOLD_RQ_FIFO_MASK                        0x00000200
#define AGP_CNTL__HOLD_RQ_FIFO                             0x00000200
#define AGP_CNTL__EN_2X_STBB_MASK                          0x00000400
#define AGP_CNTL__EN_2X_STBB                               0x00000400
#define AGP_CNTL__FORCE_FULL_SBA_MASK                      0x00000800
#define AGP_CNTL__FORCE_FULL_SBA                           0x00000800
#define AGP_CNTL__SBA_DIS_MASK                             0x00001000
#define AGP_CNTL__SBA_DIS                                  0x00001000
#define AGP_CNTL__AGP_REV_ID_MASK                          0x00002000
#define AGP_CNTL__AGP_REV_ID                               0x00002000
#define AGP_CNTL__REG_CRIPPLE_AGP4X_MASK                   0x00004000
#define AGP_CNTL__REG_CRIPPLE_AGP4X                        0x00004000
#define AGP_CNTL__REG_CRIPPLE_AGP2X4X_MASK                 0x00008000
#define AGP_CNTL__REG_CRIPPLE_AGP2X4X                      0x00008000
#define AGP_CNTL__FORCE_INT_VREF_MASK                      0x00010000
#define AGP_CNTL__FORCE_INT_VREF                           0x00010000
#define AGP_CNTL__PENDING_SLOTS_VAL_MASK                   0x00060000
#define AGP_CNTL__PENDING_SLOTS_SEL_MASK                   0x00080000
#define AGP_CNTL__PENDING_SLOTS_SEL                        0x00080000
#define AGP_CNTL__EN_EXTENDED_AD_STB_2X_MASK               0x00100000
#define AGP_CNTL__EN_EXTENDED_AD_STB_2X                    0x00100000
#define AGP_CNTL__DIS_QUEUED_GNT_FIX_MASK                  0x00200000
#define AGP_CNTL__DIS_QUEUED_GNT_FIX                       0x00200000
#define AGP_CNTL__EN_RDATA2X4X_MULTIRESET_MASK             0x00400000
#define AGP_CNTL__EN_RDATA2X4X_MULTIRESET                  0x00400000
#define AGP_CNTL__EN_RBFCALM_MASK                          0x00800000
#define AGP_CNTL__EN_RBFCALM                               0x00800000
#define AGP_CNTL__FORCE_EXT_VREF_MASK                      0x01000000
#define AGP_CNTL__FORCE_EXT_VREF                           0x01000000
#define AGP_CNTL__DIS_RBF_MASK                             0x02000000
#define AGP_CNTL__DIS_RBF                                  0x02000000
#define AGP_CNTL__AGP_MISC_MASK                            0xfc000000

// BM_STATUS
#define BM_STATUS__BUS_MASTER_STATUS_MASK                  0xffffffff

// VENDOR_ID
#define VENDOR_ID__VENDOR_ID_MASK                          0x0000ffff

// DEVICE_ID
#define DEVICE_ID__DEVICE_ID_MASK                          0x0000ffff

// COMMAND
#define COMMAND__IO_ACCESS_EN_MASK                         0x00000001
#define COMMAND__IO_ACCESS_EN                              0x00000001
#define COMMAND__MEM_ACCESS_EN_MASK                        0x00000002
#define COMMAND__MEM_ACCESS_EN                             0x00000002
#define COMMAND__BUS_MASTER_EN_MASK                        0x00000004
#define COMMAND__BUS_MASTER_EN                             0x00000004
#define COMMAND__SPECIAL_CYCLE_EN_MASK                     0x00000008
#define COMMAND__SPECIAL_CYCLE_EN                          0x00000008
#define COMMAND__MEM_WRITE_INVALIDATE_EN_MASK              0x00000010
#define COMMAND__MEM_WRITE_INVALIDATE_EN                   0x00000010
#define COMMAND__PAL_SNOOP_EN_MASK                         0x00000020
#define COMMAND__PAL_SNOOP_EN                              0x00000020
#define COMMAND__PARITY_ERROR_EN_MASK                      0x00000040
#define COMMAND__PARITY_ERROR_EN                           0x00000040
#define COMMAND__AD_STEPPING_MASK                          0x00000080
#define COMMAND__AD_STEPPING                               0x00000080
#define COMMAND__SERR_EN_MASK                              0x00000100
#define COMMAND__SERR_EN                                   0x00000100
#define COMMAND__FAST_B2B_EN_MASK                          0x00000200
#define COMMAND__FAST_B2B_EN                               0x00000200

// STATUS
#define STATUS__CAP_LIST_MASK                              0x00000010
#define STATUS__CAP_LIST                                   0x00000010
#define STATUS__PCI_66_EN_MASK                             0x00000020
#define STATUS__PCI_66_EN                                  0x00000020
#define STATUS__UDF_EN_MASK                                0x00000040
#define STATUS__UDF_EN                                     0x00000040
#define STATUS__FAST_BACK_CAPABLE_MASK                     0x00000080
#define STATUS__FAST_BACK_CAPABLE                          0x00000080
#define STATUS__DEVSEL_TIMING_MASK                         0x00000600
#define STATUS__SIGNAL_TARGET_ABORT_MASK                   0x00000800
#define STATUS__SIGNAL_TARGET_ABORT                        0x00000800
#define STATUS__RECEIVED_TARGET_ABORT_MASK                 0x00001000
#define STATUS__RECEIVED_TARGET_ABORT                      0x00001000
#define STATUS__RECEIVED_MASTER_ABORT_MASK                 0x00002000
#define STATUS__RECEIVED_MASTER_ABORT                      0x00002000
#define STATUS__SIGNALED_SYSTEM_ERROR_MASK                 0x00004000
#define STATUS__SIGNALED_SYSTEM_ERROR                      0x00004000
#define STATUS__PARITY_ERROR_DETECTED_MASK                 0x00008000
#define STATUS__PARITY_ERROR_DETECTED                      0x00008000

// REVISION_ID
#define REVISION_ID__MINOR_REV_ID_MASK                     0x0000000f
#define REVISION_ID__MAJOR_REV_ID_MASK                     0x000000f0

// REGPROG_INF
#define REGPROG_INF__REG_LEVEL_PROG_INF_MASK               0x000000ff

// SUB_CLASS
#define SUB_CLASS__SUB_CLASS_INF_MASK                      0x00000080
#define SUB_CLASS__SUB_CLASS_INF                           0x00000080

// BASE_CODE
#define BASE_CODE__BASE_CLASS_CODE_MASK                    0x000000ff

// CACHE_LINE
#define CACHE_LINE__CACHE_LINE_SIZE_MASK                   0x000000ff

// LATENCY
#define LATENCY__LATENCY_TIMER_MASK                        0x000000ff

// HEADER
#define HEADER__HEADER_TYPE_MASK                           0x0000007f
#define HEADER__DEVICE_TYPE_MASK                           0x00000080
#define HEADER__DEVICE_TYPE                                0x00000080

// BIST
#define BIST__BIST_COMP_MASK                               0x0000000f
#define BIST__BIST_STRT_MASK                               0x00000040
#define BIST__BIST_STRT                                    0x00000040
#define BIST__BIST_CAP_MASK                                0x00000080
#define BIST__BIST_CAP                                     0x00000080

// MEM_BASE
#define MEM_BASE__PREFETCH_EN_MASK                         0x00000008
#define MEM_BASE__PREFETCH_EN                              0x00000008
#define MEM_BASE__MEM_BASE_MASK                            0xfe000000

// IO_BASE
#define IO_BASE__BLOCK_IO_BIT_MASK                         0x000000ff
#define IO_BASE__IO_BASE_MASK                              0xffffff00

// REG_BASE
#define REG_BASE__REG_BASE_MASK                            0xfff80000

// ADAPTER_ID
#define ADAPTER_ID__SUBSYSTEM_VENDOR_ID_MASK               0x0000ffff
#define ADAPTER_ID__SUBSYSTEM_ID_MASK                      0xffff0000

// BIOS_ROM
#define BIOS_ROM__BIOS_ROM_EN_MASK                         0x00000001
#define BIOS_ROM__BIOS_ROM_EN                              0x00000001
#define BIOS_ROM__BIOS_BASE_ADDR_MASK                      0xfffe0000

// CAPABILITIES_PTR
#define CAPABILITIES_PTR__CAP_PTR_MASK                     0x000000ff

// INTERRUPT_LINE
#define INTERRUPT_LINE__INTERRUPT_LINE_MASK                0x000000ff

// INTERRUPT_PIN
#define INTERRUPT_PIN__INTERRUPT_PIN_MASK                  0x00000001
#define INTERRUPT_PIN__INTERRUPT_PIN                       0x00000001

// MIN_GRANT
#define MIN_GRANT__MIN_GNT_MASK                            0x000000ff

// MAX_LATENCY
#define MAX_LATENCY__MAX_LAT_MASK                          0x000000ff

// ADAPTER_ID_W
#define ADAPTER_ID_W__SUBSYSTEM_VENDOR_ID_MASK             0x0000ffff
#define ADAPTER_ID_W__SUBSYSTEM_ID_MASK                    0xffff0000

// PMI_CAP_ID
#define PMI_CAP_ID__PMI_CAP_ID_MASK                        0x000000ff

// PMI_NXT_CAP_PTR
#define PMI_NXT_CAP_PTR__PMI_NXT_CAP_PTR_MASK              0x000000ff

// PMI_PMC_REG
#define PMI_PMC_REG__PMI_VERSION_MASK                      0x00000007
#define PMI_PMC_REG__PMI_PME_CLOCK_MASK                    0x00000008
#define PMI_PMC_REG__PMI_PME_CLOCK                         0x00000008
#define PMI_PMC_REG__PMI_DEV_SPECIFIC_INIT_MASK            0x00000020
#define PMI_PMC_REG__PMI_DEV_SPECIFIC_INIT                 0x00000020
#define PMI_PMC_REG__PMI_D1_SUPPORT_MASK                   0x00000200
#define PMI_PMC_REG__PMI_D1_SUPPORT                        0x00000200
#define PMI_PMC_REG__PMI_D2_SUPPORT_MASK                   0x00000400
#define PMI_PMC_REG__PMI_D2_SUPPORT                        0x00000400
#define PMI_PMC_REG__PMI_PME_SUPPORT_MASK                  0x0000f800

// PM_STATUS
#define PM_STATUS__PMI_POWER_STATE_MASK                    0x00000003
#define PM_STATUS__PMI_PME_EN_MASK                         0x00000100
#define PM_STATUS__PMI_PME_EN                              0x00000100
#define PM_STATUS__PMI_DATA_SELECT_MASK                    0x00001e00
#define PM_STATUS__PMI_DATA_SCALE_MASK                     0x00006000
#define PM_STATUS__PMI_PME_STATUS_MASK                     0x00008000
#define PM_STATUS__PMI_PME_STATUS                          0x00008000

// PMI_DATA
#define PMI_DATA__PMI_DATA_MASK                            0x000000ff

// AGP_CAP_ID
#define AGP_CAP_ID__CAP_ID_MASK                            0x000000ff
#define AGP_CAP_ID__NEXT_PTR_MASK                          0x0000ff00
#define AGP_CAP_ID__AGP_MINOR_MASK                         0x000f0000
#define AGP_CAP_ID__AGP_MAJOR_MASK                         0x00f00000

// AGP_STATUS
#define AGP_STATUS__RATE1X_MASK                            0x00000001
#define AGP_STATUS__RATE1X                                 0x00000001
#define AGP_STATUS__RATE2X_MASK                            0x00000002
#define AGP_STATUS__RATE2X                                 0x00000002
#define AGP_STATUS__RATE4X_MASK                            0x00000004
#define AGP_STATUS__RATE4X                                 0x00000004
#define AGP_STATUS__SBA_MASK                               0x00000200
#define AGP_STATUS__SBA                                    0x00000200
#define AGP_STATUS__RQ_MASK                                0xff000000

// AGP_COMMAND
#define AGP_COMMAND__DATA_RATE_MASK                        0x00000007
#define AGP_COMMAND__AGP_EN_MASK                           0x00000100
#define AGP_COMMAND__AGP_EN                                0x00000100
#define AGP_COMMAND__SBA_EN_MASK                           0x00000200
#define AGP_COMMAND__SBA_EN                                0x00000200
#define AGP_COMMAND__RQ_DEPTH_MASK                         0xff000000

// AIC_CTRL
#define AIC_CTRL__TRANSLATE_EN_MASK                        0x00000001
#define AIC_CTRL__TRANSLATE_EN                             0x00000001
#define AIC_CTRL__HW_0_DEBUG_MASK                          0x00000002
#define AIC_CTRL__HW_0_DEBUG                               0x00000002
#define AIC_CTRL__HW_1_DEBUG_MASK                          0x00000004
#define AIC_CTRL__HW_1_DEBUG                               0x00000004
#define AIC_CTRL__HW_2_DEBUG_MASK                          0x00000008
#define AIC_CTRL__HW_2_DEBUG                               0x00000008
#define AIC_CTRL__HW_3_DEBUG_MASK                          0x00000010
#define AIC_CTRL__HW_3_DEBUG                               0x00000010
#define AIC_CTRL__HW_4_DEBUG_MASK                          0x00000020
#define AIC_CTRL__HW_4_DEBUG                               0x00000020
#define AIC_CTRL__HW_5_DEBUG_MASK                          0x00000040
#define AIC_CTRL__HW_5_DEBUG                               0x00000040
#define AIC_CTRL__HW_6_DEBUG_MASK                          0x00000080
#define AIC_CTRL__HW_6_DEBUG                               0x00000080
#define AIC_CTRL__HW_7_DEBUG_MASK                          0x00000100
#define AIC_CTRL__HW_7_DEBUG                               0x00000100
#define AIC_CTRL__HW_8_DEBUG_MASK                          0x00000200
#define AIC_CTRL__HW_8_DEBUG                               0x00000200
#define AIC_CTRL__HW_9_DEBUG_MASK                          0x00000400
#define AIC_CTRL__HW_9_DEBUG                               0x00000400
#define AIC_CTRL__HW_A_DEBUG_MASK                          0x00000800
#define AIC_CTRL__HW_A_DEBUG                               0x00000800
#define AIC_CTRL__HW_B_DEBUG_MASK                          0x00001000
#define AIC_CTRL__HW_B_DEBUG                               0x00001000
#define AIC_CTRL__HW_C_DEBUG_MASK                          0x00002000
#define AIC_CTRL__HW_C_DEBUG                               0x00002000
#define AIC_CTRL__HW_D_DEBUG_MASK                          0x00004000
#define AIC_CTRL__HW_D_DEBUG                               0x00004000
#define AIC_CTRL__HW_E_DEBUG_MASK                          0x00008000
#define AIC_CTRL__HW_E_DEBUG                               0x00008000
#define AIC_CTRL__HW_F_DEBUG_MASK                          0x00010000
#define AIC_CTRL__HW_F_DEBUG                               0x00010000
#define AIC_CTRL__HW_10_DEBUG_MASK                         0x00020000
#define AIC_CTRL__HW_10_DEBUG                              0x00020000
#define AIC_CTRL__HW_11_DEBUG_MASK                         0x00040000
#define AIC_CTRL__HW_11_DEBUG                              0x00040000
#define AIC_CTRL__HW_12_DEBUG_MASK                         0x00080000
#define AIC_CTRL__HW_12_DEBUG                              0x00080000
#define AIC_CTRL__HW_13_DEBUG_MASK                         0x00100000
#define AIC_CTRL__HW_13_DEBUG                              0x00100000
#define AIC_CTRL__HW_14_DEBUG_MASK                         0x00200000
#define AIC_CTRL__HW_14_DEBUG                              0x00200000
#define AIC_CTRL__HW_15_DEBUG_MASK                         0x00400000
#define AIC_CTRL__HW_15_DEBUG                              0x00400000
#define AIC_CTRL__HW_16_DEBUG_MASK                         0x00800000
#define AIC_CTRL__HW_16_DEBUG                              0x00800000
#define AIC_CTRL__HW_17_DEBUG_MASK                         0x01000000
#define AIC_CTRL__HW_17_DEBUG                              0x01000000
#define AIC_CTRL__HW_18_DEBUG_MASK                         0x02000000
#define AIC_CTRL__HW_18_DEBUG                              0x02000000
#define AIC_CTRL__HW_19_DEBUG_MASK                         0x04000000
#define AIC_CTRL__HW_19_DEBUG                              0x04000000
#define AIC_CTRL__HW_1A_DEBUG_MASK                         0x08000000
#define AIC_CTRL__HW_1A_DEBUG                              0x08000000
#define AIC_CTRL__HW_1B_DEBUG_MASK                         0x10000000
#define AIC_CTRL__HW_1B_DEBUG                              0x10000000
#define AIC_CTRL__HW_1C_DEBUG_MASK                         0x20000000
#define AIC_CTRL__HW_1C_DEBUG                              0x20000000
#define AIC_CTRL__HW_1D_DEBUG_MASK                         0x40000000
#define AIC_CTRL__HW_1D_DEBUG                              0x40000000
#define AIC_CTRL__HW_1E_DEBUG_MASK                         0x80000000
#define AIC_CTRL__HW_1E_DEBUG                              0x80000000

// AIC_STAT
#define AIC_STAT__AIC_TLB_VLD_MASK                         0x00000001
#define AIC_STAT__AIC_TLB_VLD                              0x00000001
#define AIC_STAT__AIC_STAT1_MASK                           0x00000002
#define AIC_STAT__AIC_STAT1                                0x00000002
#define AIC_STAT__AIC_STAT0_MASK                           0x00000004
#define AIC_STAT__AIC_STAT0                                0x00000004

// AIC_PT_BASE
#define AIC_PT_BASE__AIC_PT_BASE_MASK                      0xfffff000

// AIC_LO_ADDR
#define AIC_LO_ADDR__AIC_LO_ADDR_MASK                      0xfffff000

// AIC_HI_ADDR
#define AIC_HI_ADDR__AIC_HI_ADDR_MASK                      0xfffff000

// AIC_TLB_ADDR
#define AIC_TLB_ADDR__AIC_TLB_ADDR_MASK                    0xfffff000

// AIC_TLB_DATA
#define AIC_TLB_DATA__AIC_TLB_DATA_MASK                    0xfffff000

// GENMO_WT
#define GENMO_WT__GENMO_MONO_ADDRESS_B_MASK                0x00000001
#define GENMO_WT__GENMO_MONO_ADDRESS_B                     0x00000001
#define GENMO_WT__VGA_RAM_EN_MASK                          0x00000002
#define GENMO_WT__VGA_RAM_EN                               0x00000002
#define GENMO_WT__VGA_CKSEL_MASK                           0x0000000c
#define GENMO_WT__ODD_EVEN_MD_PGSEL_MASK                   0x00000020
#define GENMO_WT__ODD_EVEN_MD_PGSEL                        0x00000020
#define GENMO_WT__VGA_HSYNC_POL_MASK                       0x00000040
#define GENMO_WT__VGA_HSYNC_POL                            0x00000040
#define GENMO_WT__VGA_VSYNC_POL_MASK                       0x00000080
#define GENMO_WT__VGA_VSYNC_POL                            0x00000080

// GENMO_RD
#define GENMO_RD__GENMO_MONO_ADDRESS_B_MASK                0x00000001
#define GENMO_RD__GENMO_MONO_ADDRESS_B                     0x00000001
#define GENMO_RD__VGA_RAM_EN_MASK                          0x00000002
#define GENMO_RD__VGA_RAM_EN                               0x00000002
#define GENMO_RD__VGA_CKSEL_MASK                           0x0000000c
#define GENMO_RD__ODD_EVEN_MD_PGSEL_MASK                   0x00000020
#define GENMO_RD__ODD_EVEN_MD_PGSEL                        0x00000020
#define GENMO_RD__VGA_HSYNC_POL_MASK                       0x00000040
#define GENMO_RD__VGA_HSYNC_POL                            0x00000040
#define GENMO_RD__VGA_VSYNC_POL_MASK                       0x00000080
#define GENMO_RD__VGA_VSYNC_POL                            0x00000080

// DAC_CNTL
#define DAC_CNTL__DAC_RANGE_CNTL_MASK                      0x00000003
#define DAC_CNTL__DAC_BLANKING_MASK                        0x00000004
#define DAC_CNTL__DAC_BLANKING                             0x00000004
#define DAC_CNTL__DAC_CMP_EN_MASK                          0x00000008
#define DAC_CNTL__DAC_CMP_EN                               0x00000008
#define DAC_CNTL__DAC_CMP_OUT_R_MASK                       0x00000010
#define DAC_CNTL__DAC_CMP_OUT_R                            0x00000010
#define DAC_CNTL__DAC_CMP_OUT_G_MASK                       0x00000020
#define DAC_CNTL__DAC_CMP_OUT_G                            0x00000020
#define DAC_CNTL__DAC_CMP_OUT_B_MASK                       0x00000040
#define DAC_CNTL__DAC_CMP_OUT_B                            0x00000040
#define DAC_CNTL__DAC_CMP_OUTPUT_MASK                      0x00000080
#define DAC_CNTL__DAC_CMP_OUTPUT                           0x00000080
#define DAC_CNTL__DAC_8BIT_EN_MASK                         0x00000100
#define DAC_CNTL__DAC_8BIT_EN                              0x00000100
#define DAC_CNTL__DAC_4BPP_PIX_ORDER_MASK                  0x00000200
#define DAC_CNTL__DAC_4BPP_PIX_ORDER                       0x00000200
#define DAC_CNTL__DAC_TVO_EN_MASK                          0x00000400
#define DAC_CNTL__DAC_TVO_EN                               0x00000400
#define DAC_CNTL__DAC_VGA_ADR_EN_MASK                      0x00002000
#define DAC_CNTL__DAC_VGA_ADR_EN                           0x00002000
#define DAC_CNTL__DAC_EXPAND_MODE_MASK                     0x00004000
#define DAC_CNTL__DAC_EXPAND_MODE                          0x00004000
#define DAC_CNTL__DAC_PDWN_MASK                            0x00008000
#define DAC_CNTL__DAC_PDWN                                 0x00008000
#define DAC_CNTL__DAC_CRC_CONT_EN_MASK                     0x00040000
#define DAC_CNTL__DAC_CRC_CONT_EN                          0x00040000
#define DAC_CNTL__DAC_CRC_EN_MASK                          0x00080000
#define DAC_CNTL__DAC_CRC_EN                               0x00080000
#define DAC_CNTL__DAC_CRC_FIELD_MASK                       0x00100000
#define DAC_CNTL__DAC_CRC_FIELD                            0x00100000
#define DAC_CNTL__DAC_LUT_COUNTER_LIMIT_MASK               0x00600000
#define DAC_CNTL__DAC_LUT_READ_SEL_MASK                    0x00800000
#define DAC_CNTL__DAC_LUT_READ_SEL                         0x00800000
#define DAC_CNTL__DAC_MASK_MASK                            0xff000000

// CRTC_GEN_CNTL
#define CRTC_GEN_CNTL__CRTC_DBL_SCAN_EN_MASK               0x00000001
#define CRTC_GEN_CNTL__CRTC_DBL_SCAN_EN                    0x00000001
#define CRTC_GEN_CNTL__CRTC_INTERLACE_EN_MASK              0x00000002
#define CRTC_GEN_CNTL__CRTC_INTERLACE_EN                   0x00000002
#define CRTC_GEN_CNTL__CRTC_C_SYNC_EN_MASK                 0x00000010
#define CRTC_GEN_CNTL__CRTC_C_SYNC_EN                      0x00000010
#define CRTC_GEN_CNTL__CRTC_PIX_WIDTH_MASK                 0x00000f00
#define CRTC_GEN_CNTL__CRTC_CUR_EN_MASK                    0x00010000
#define CRTC_GEN_CNTL__CRTC_CUR_EN                         0x00010000
#define CRTC_GEN_CNTL__CRTC_VSTAT_MODE_MASK                0x00060000
#define CRTC_GEN_CNTL__CRTC_CUR_MODE_MASK                  0x00700000
#define CRTC_GEN_CNTL__CRTC_EXT_DISP_EN_MASK               0x01000000
#define CRTC_GEN_CNTL__CRTC_EXT_DISP_EN                    0x01000000
#define CRTC_GEN_CNTL__CRTC_EN_MASK                        0x02000000
#define CRTC_GEN_CNTL__CRTC_EN                             0x02000000
#define CRTC_GEN_CNTL__CRTC_DISP_REQ_EN_B_MASK             0x04000000
#define CRTC_GEN_CNTL__CRTC_DISP_REQ_EN_B                  0x04000000

// MEM_CNTL
#define MEM_CNTL__MEM_NUM_CHANNELS_MASK                    0x00000001
#define MEM_CNTL__MEM_NUM_CHANNELS                         0x00000001
#define MEM_CNTL__MC_USE_B_CH_ONLY_MASK                    0x00000002
#define MEM_CNTL__MC_USE_B_CH_ONLY                         0x00000002
#define MEM_CNTL__DISABLE_AP_MASK                          0x00000004
#define MEM_CNTL__DISABLE_AP                               0x00000004
#define MEM_CNTL__MEM_BANK_MAPPING_A_MASK                  0x000000f0
#define MEM_CNTL__MEM_ADDR_MAPPING_A_MASK                  0x0000ff00
#define MEM_CNTL__MEM_BANK_MAPPING_B_MASK                  0x00f00000
#define MEM_CNTL__MEM_ADDR_MAPPING_B_MASK                  0xff000000

// EXT_MEM_CNTL
#define EXT_MEM_CNTL__MEM_TRP_MASK                         0x00000003
#define EXT_MEM_CNTL__MEM_TRCD_MASK                        0x0000000c
#define EXT_MEM_CNTL__MEM_TRAS_MASK                        0x00000070
#define EXT_MEM_CNTL__MEM_TRRD_MASK                        0x00000300
#define EXT_MEM_CNTL__MEM_TR2W_MASK                        0x00000c00
#define EXT_MEM_CNTL__MEM_TWR_MASK                         0x00003000
#define EXT_MEM_CNTL__MEM_TW2R_MASK                        0x0000c000
#define EXT_MEM_CNTL__MEM_TR2R_MASK                        0x00030000
#define EXT_MEM_CNTL__MEM_TWR_MODE_MASK                    0x00040000
#define EXT_MEM_CNTL__MEM_TWR_MODE                         0x00040000
#define EXT_MEM_CNTL__MEM_REFRESH_DIS_MASK                 0x00100000
#define EXT_MEM_CNTL__MEM_REFRESH_DIS                      0x00100000
#define EXT_MEM_CNTL__MEM_REFRESH_RATE_MASK                0xff000000

// MC_AGP_LOCATION
#define MC_AGP_LOCATION__MC_AGP_START_MASK                 0x0000ffff
#define MC_AGP_LOCATION__MC_AGP_TOP_MASK                   0xffff0000

// MEM_IO_CNTL_A0
#define MEM_IO_CNTL_A0__MEM_N_CKA0_MASK                    0x0000000f
#define MEM_IO_CNTL_A0__MEM_N_CKA1_MASK                    0x000000f0
#define MEM_IO_CNTL_A0__MEM_N_AA_MASK                      0x00000f00
#define MEM_IO_CNTL_A0__MEM_N_DQMA_MASK                    0x0000f000
#define MEM_IO_CNTL_A0__MEM_N_DQSA_MASK                    0x000f0000
#define MEM_IO_CNTL_A0__MC_IO_SSTL_ENA_MASK                0x00100000
#define MEM_IO_CNTL_A0__MC_IO_SSTL_ENA                     0x00100000
#define MEM_IO_CNTL_A0__MEM_SLEW_CKA0_MASK                 0x01000000
#define MEM_IO_CNTL_A0__MEM_SLEW_CKA0                      0x01000000
#define MEM_IO_CNTL_A0__MEM_SLEW_CKA1_MASK                 0x02000000
#define MEM_IO_CNTL_A0__MEM_SLEW_CKA1                      0x02000000
#define MEM_IO_CNTL_A0__MEM_SLEW_AA_MASK                   0x04000000
#define MEM_IO_CNTL_A0__MEM_SLEW_AA                        0x04000000
#define MEM_IO_CNTL_A0__MEM_SLEW_DQMA_MASK                 0x08000000
#define MEM_IO_CNTL_A0__MEM_SLEW_DQMA                      0x08000000
#define MEM_IO_CNTL_A0__MEM_SLEW_DQSA_MASK                 0x10000000
#define MEM_IO_CNTL_A0__MEM_SLEW_DQSA                      0x10000000
#define MEM_IO_CNTL_A0__MEM_PREAMP_DAA_MASK                0x20000000
#define MEM_IO_CNTL_A0__MEM_PREAMP_DAA                     0x20000000
#define MEM_IO_CNTL_A0__MEM_PREAMP_DQMA_MASK               0x40000000
#define MEM_IO_CNTL_A0__MEM_PREAMP_DQMA                    0x40000000
#define MEM_IO_CNTL_A0__MEM_PREAMP_DQSA_MASK               0x80000000
#define MEM_IO_CNTL_A0__MEM_PREAMP_DQSA                    0x80000000

// MEM_INIT_LATENCY_TIMER
#define MEM_INIT_LATENCY_TIMER__MEM_RB0R_INIT_LAT_MASK     0x0000000f
#define MEM_INIT_LATENCY_TIMER__MEM_RB1R_INIT_LAT_MASK     0x000000f0
#define MEM_INIT_LATENCY_TIMER__MEM_PPR_INIT_LAT_MASK      0x00000f00
#define MEM_INIT_LATENCY_TIMER__MEM_DISPR_INIT_LAT_MASK    0x0000f000
#define MEM_INIT_LATENCY_TIMER__MEM_RB0W_INIT_LAT_MASK     0x000f0000
#define MEM_INIT_LATENCY_TIMER__MEM_RB1W_INIT_LAT_MASK     0x00f00000
#define MEM_INIT_LATENCY_TIMER__MEM_FIXED_INIT_LAT_MASK    0x0f000000
#define MEM_INIT_LATENCY_TIMER__SAME_PAGE_PRIO_MASK        0x70000000

// MEM_SDRAM_MODE_REG
#define MEM_SDRAM_MODE_REG__MEM_MODE_REG_MASK              0x00007fff
#define MEM_SDRAM_MODE_REG__MEM_RBS_POSITION_A_MASK        0x00030000
#define MEM_SDRAM_MODE_REG__MEM_RBS_POSITION_B_MASK        0x000c0000
#define MEM_SDRAM_MODE_REG__MEM_CAS_LATENCY_MASK           0x00700000
#define MEM_SDRAM_MODE_REG__WR_LAT_EQ_CAS_LAT_MASK         0x00800000
#define MEM_SDRAM_MODE_REG__WR_LAT_EQ_CAS_LAT              0x00800000
#define MEM_SDRAM_MODE_REG__MEM_ERST_POSITION_A_MASK       0x03000000
#define MEM_SDRAM_MODE_REG__MEM_ERST_POSITION_B_MASK       0x0c000000
#define MEM_SDRAM_MODE_REG__MC_INIT_COMPLETE_MASK          0x10000000
#define MEM_SDRAM_MODE_REG__MC_INIT_COMPLETE               0x10000000
#define MEM_SDRAM_MODE_REG__MEM_NON_JEDEC_MASK             0x20000000
#define MEM_SDRAM_MODE_REG__MEM_NON_JEDEC                  0x20000000
#define MEM_SDRAM_MODE_REG__MEM_CFG_TYPE_MASK              0x40000000
#define MEM_SDRAM_MODE_REG__MEM_CFG_TYPE                   0x40000000
#define MEM_SDRAM_MODE_REG__MEM_SDRAM_RESET_MASK           0x80000000
#define MEM_SDRAM_MODE_REG__MEM_SDRAM_RESET                0x80000000

// AGP_BASE
#define AGP_BASE__AGP_BASE_ADDR_MASK                       0xffffffff

// MEM_IO_CNTL_A1
#define MEM_IO_CNTL_A1__MEM_P_CKA0_MASK                    0x0000000f
#define MEM_IO_CNTL_A1__MEM_P_CKA1_MASK                    0x000000f0
#define MEM_IO_CNTL_A1__MEM_P_AA_MASK                      0x00000f00
#define MEM_IO_CNTL_A1__MEM_P_DQMA_MASK                    0x0000f000
#define MEM_IO_CNTL_A1__MEM_P_DQSA_MASK                    0x000f0000
#define MEM_IO_CNTL_A1__DLL_FB_SLCT_CKA_MASK               0x00300000
#define MEM_IO_CNTL_A1__CLKA0_ENABLE_MASK                  0x00400000
#define MEM_IO_CNTL_A1__CLKA0_ENABLE                       0x00400000
#define MEM_IO_CNTL_A1__CLKA1_ENABLE_MASK                  0x00800000
#define MEM_IO_CNTL_A1__CLKA1_ENABLE                       0x00800000
#define MEM_IO_CNTL_A1__CLKAFB_ENABLE_MASK                 0x01000000
#define MEM_IO_CNTL_A1__CLKAFB_ENABLE                      0x01000000
#define MEM_IO_CNTL_A1__DFR_DQSA_MASK                      0x02000000
#define MEM_IO_CNTL_A1__DFR_DQSA                           0x02000000
#define MEM_IO_CNTL_A1__DFR_CKA_MASK                       0x04000000
#define MEM_IO_CNTL_A1__DFR_CKA                            0x04000000
#define MEM_IO_CNTL_A1__DFR_DQMA_MASK                      0x08000000
#define MEM_IO_CNTL_A1__DFR_DQMA                           0x08000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A0_MASK            0x10000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A0                 0x10000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A1_MASK            0x20000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A1                 0x20000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A2_MASK            0x40000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A2                 0x40000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A3_MASK            0x80000000
#define MEM_IO_CNTL_A1__DQS_DRIVER_SLCT_A3                 0x80000000

// MEM_IO_CNTL_B0
#define MEM_IO_CNTL_B0__MEM_N_CKB0_MASK                    0x0000000f
#define MEM_IO_CNTL_B0__MEM_N_CKB1_MASK                    0x000000f0
#define MEM_IO_CNTL_B0__MEM_N_AB_MASK                      0x00000f00
#define MEM_IO_CNTL_B0__MEM_N_DQMB_MASK                    0x0000f000
#define MEM_IO_CNTL_B0__MEM_N_DQSB_MASK                    0x000f0000
#define MEM_IO_CNTL_B0__MC_IO_SSTL_ENB_MASK                0x00100000
#define MEM_IO_CNTL_B0__MC_IO_SSTL_ENB                     0x00100000
#define MEM_IO_CNTL_B0__MEM_SLEW_CKB0_MASK                 0x01000000
#define MEM_IO_CNTL_B0__MEM_SLEW_CKB0                      0x01000000
#define MEM_IO_CNTL_B0__MEM_SLEW_CKB1_MASK                 0x02000000
#define MEM_IO_CNTL_B0__MEM_SLEW_CKB1                      0x02000000
#define MEM_IO_CNTL_B0__MEM_SLEW_AB_MASK                   0x04000000
#define MEM_IO_CNTL_B0__MEM_SLEW_AB                        0x04000000
#define MEM_IO_CNTL_B0__MEM_SLEW_DQMB_MASK                 0x08000000
#define MEM_IO_CNTL_B0__MEM_SLEW_DQMB                      0x08000000
#define MEM_IO_CNTL_B0__MEM_SLEW_DQSB_MASK                 0x10000000
#define MEM_IO_CNTL_B0__MEM_SLEW_DQSB                      0x10000000
#define MEM_IO_CNTL_B0__MEM_PREAMP_DAB_MASK                0x20000000
#define MEM_IO_CNTL_B0__MEM_PREAMP_DAB                     0x20000000
#define MEM_IO_CNTL_B0__MEM_PREAMP_DQMB_MASK               0x40000000
#define MEM_IO_CNTL_B0__MEM_PREAMP_DQMB                    0x40000000
#define MEM_IO_CNTL_B0__MEM_PREAMP_DQSB_MASK               0x80000000
#define MEM_IO_CNTL_B0__MEM_PREAMP_DQSB                    0x80000000

// MEM_IO_CNTL_B1
#define MEM_IO_CNTL_B1__MEM_P_CKB0_MASK                    0x0000000f
#define MEM_IO_CNTL_B1__MEM_P_CKB1_MASK                    0x000000f0
#define MEM_IO_CNTL_B1__MEM_P_AB_MASK                      0x00000f00
#define MEM_IO_CNTL_B1__MEM_P_DQMB_MASK                    0x0000f000
#define MEM_IO_CNTL_B1__MEM_P_DQSB_MASK                    0x000f0000
#define MEM_IO_CNTL_B1__DLL_FB_SLCT_CKB_MASK               0x00300000
#define MEM_IO_CNTL_B1__CLKB0_ENABLE_MASK                  0x00400000
#define MEM_IO_CNTL_B1__CLKB0_ENABLE                       0x00400000
#define MEM_IO_CNTL_B1__CLKB1_ENABLE_MASK                  0x00800000
#define MEM_IO_CNTL_B1__CLKB1_ENABLE                       0x00800000
#define MEM_IO_CNTL_B1__CLKBFB_ENABLE_MASK                 0x01000000
#define MEM_IO_CNTL_B1__CLKBFB_ENABLE                      0x01000000
#define MEM_IO_CNTL_B1__DFR_DQSB_MASK                      0x02000000
#define MEM_IO_CNTL_B1__DFR_DQSB                           0x02000000
#define MEM_IO_CNTL_B1__DFR_CKB_MASK                       0x04000000
#define MEM_IO_CNTL_B1__DFR_CKB                            0x04000000
#define MEM_IO_CNTL_B1__DFR_DQMB_MASK                      0x08000000
#define MEM_IO_CNTL_B1__DFR_DQMB                           0x08000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B0_MASK            0x10000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B0                 0x10000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B1_MASK            0x20000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B1                 0x20000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B2_MASK            0x40000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B2                 0x40000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B3_MASK            0x80000000
#define MEM_IO_CNTL_B1__DQS_DRIVER_SLCT_B3                 0x80000000

// MC_DEBUG
#define MC_DEBUG__IGNORE_RW_PENALTY_RB0R_MASK              0x00000001
#define MC_DEBUG__IGNORE_RW_PENALTY_RB0R                   0x00000001
#define MC_DEBUG__IGNORE_RW_PENALTY_RB1R_MASK              0x00000002
#define MC_DEBUG__IGNORE_RW_PENALTY_RB1R                   0x00000002
#define MC_DEBUG__IGNORE_RW_PENALTY_RB0W_MASK              0x00000004
#define MC_DEBUG__IGNORE_RW_PENALTY_RB0W                   0x00000004
#define MC_DEBUG__IGNORE_RW_PENALTY_RB1W_MASK              0x00000008
#define MC_DEBUG__IGNORE_RW_PENALTY_RB1W                   0x00000008
#define MC_DEBUG__IGNORE_RW_PENALTY_DISPR_MASK             0x00000010
#define MC_DEBUG__IGNORE_RW_PENALTY_DISPR                  0x00000010
#define MC_DEBUG__IGNORE_RW_PENALTY_PPR_MASK               0x00000020
#define MC_DEBUG__IGNORE_RW_PENALTY_PPR                    0x00000020
#define MC_DEBUG__IGNORE_RW_PENALTY_FIXED_MASK             0x00000040
#define MC_DEBUG__IGNORE_RW_PENALTY_FIXED                  0x00000040
#define MC_DEBUG__MEM_VIPW_PRIORITY_MASK                   0x00000300
#define MC_DEBUG__CLKA0_ENABLEb_MASK                       0x00000400
#define MC_DEBUG__CLKA0_ENABLEb                            0x00000400
#define MC_DEBUG__CLKA1_ENABLEb_MASK                       0x00000800
#define MC_DEBUG__CLKA1_ENABLEb                            0x00000800
#define MC_DEBUG__CLKB0_ENABLEb_MASK                       0x00001000
#define MC_DEBUG__CLKB0_ENABLEb                            0x00001000
#define MC_DEBUG__CLKB1_ENABLEb_MASK                       0x00002000
#define MC_DEBUG__CLKB1_ENABLEb                            0x00002000
#define MC_DEBUG__COHERENCY_FIX_ENABLE_MASK                0x00004000
#define MC_DEBUG__COHERENCY_FIX_ENABLE                     0x00004000
#define MC_DEBUG__MC_DEBUG_MASK                            0xffff8000

// MC_STATUS
#define MC_STATUS__MEM_PWRUP_COMPL_A_MASK                  0x00000001
#define MC_STATUS__MEM_PWRUP_COMPL_A                       0x00000001
#define MC_STATUS__MEM_PWRUP_COMPL_B_MASK                  0x00000002
#define MC_STATUS__MEM_PWRUP_COMPL_B                       0x00000002
#define MC_STATUS__MC_IDLE_MASK                            0x00000004
#define MC_STATUS__MC_IDLE                                 0x00000004
#define MC_STATUS__SPARE_MASK                              0x0000fff8

// MEM_IO_OE_CNTL
#define MEM_IO_OE_CNTL__MEM_DQ_OE_EXTEND_A_MASK            0x00000003
#define MEM_IO_OE_CNTL__MEM_DQ_OE_POSITION_A_MASK          0x0000000c
#define MEM_IO_OE_CNTL__MEM_QS_OE_EXTEND_A_MASK            0x00000030
#define MEM_IO_OE_CNTL__MEM_QS_OE_POSITION_A_MASK          0x000000c0
#define MEM_IO_OE_CNTL__MEM_DQ_OE_EXTEND_B_MASK            0x00000300
#define MEM_IO_OE_CNTL__MEM_DQ_OE_POSITION_B_MASK          0x00000c00
#define MEM_IO_OE_CNTL__MEM_QS_OE_EXTEND_B_MASK            0x00003000
#define MEM_IO_OE_CNTL__MEM_QS_OE_POSITION_B_MASK          0x0000c000
#define MEM_IO_OE_CNTL__MEM_DYNAMIC_CKE_MASK               0x00010000
#define MEM_IO_OE_CNTL__MEM_DYNAMIC_CKE                    0x00010000
#define MEM_IO_OE_CNTL__MEM_SDRAM_TRI_EN_MASK              0x00020000
#define MEM_IO_OE_CNTL__MEM_SDRAM_TRI_EN                   0x00020000

// MC_FB_LOCATION
#define MC_FB_LOCATION__MC_FB_START_MASK                   0x0000ffff
#define MC_FB_LOCATION__MC_FB_TOP_MASK                     0xffff0000

// GRA00
#define GRA00__GRPH_SET_RESET0_MASK                        0x00000001
#define GRA00__GRPH_SET_RESET0                             0x00000001
#define GRA00__GRPH_SET_RESET1_MASK                        0x00000002
#define GRA00__GRPH_SET_RESET1                             0x00000002
#define GRA00__GRPH_SET_RESET2_MASK                        0x00000004
#define GRA00__GRPH_SET_RESET2                             0x00000004
#define GRA00__GRPH_SET_RESET3_MASK                        0x00000008
#define GRA00__GRPH_SET_RESET3                             0x00000008

// GRA01
#define GRA01__GRPH_SET_RESET_ENA0_MASK                    0x00000001
#define GRA01__GRPH_SET_RESET_ENA0                         0x00000001
#define GRA01__GRPH_SET_RESET_ENA1_MASK                    0x00000002
#define GRA01__GRPH_SET_RESET_ENA1                         0x00000002
#define GRA01__GRPH_SET_RESET_ENA2_MASK                    0x00000004
#define GRA01__GRPH_SET_RESET_ENA2                         0x00000004
#define GRA01__GRPH_SET_RESET_ENA3_MASK                    0x00000008
#define GRA01__GRPH_SET_RESET_ENA3                         0x00000008

// GRA02
#define GRA02__GRPH_CCOMP_MASK                             0x0000000f

// GRA03
#define GRA03__GRPH_ROTATE_MASK                            0x00000007
#define GRA03__GRPH_FN_SEL_MASK                            0x00000018

// GRA04
#define GRA04__GRPH_RMAP_MASK                              0x00000003

// GRA06
#define GRA06__GRPH_GRAPHICS_MASK                          0x00000001
#define GRA06__GRPH_GRAPHICS                               0x00000001
#define GRA06__GRPH_ODDEVEN_MASK                           0x00000002
#define GRA06__GRPH_ODDEVEN                                0x00000002
#define GRA06__GRPH_ADRSEL_MASK                            0x0000000c

// GRA07
#define GRA07__GRPH_XCARE0_MASK                            0x00000001
#define GRA07__GRPH_XCARE0                                 0x00000001
#define GRA07__GRPH_XCARE1_MASK                            0x00000002
#define GRA07__GRPH_XCARE1                                 0x00000002
#define GRA07__GRPH_XCARE2_MASK                            0x00000004
#define GRA07__GRPH_XCARE2                                 0x00000004
#define GRA07__GRPH_XCARE3_MASK                            0x00000008
#define GRA07__GRPH_XCARE3                                 0x00000008

// GRA08
#define GRA08__GRPH_BMSK_MASK                              0x000000ff

// SEQ02
#define SEQ02__SEQ_MAP0_EN_MASK                            0x00000001
#define SEQ02__SEQ_MAP0_EN                                 0x00000001
#define SEQ02__SEQ_MAP1_EN_MASK                            0x00000002
#define SEQ02__SEQ_MAP1_EN                                 0x00000002
#define SEQ02__SEQ_MAP2_EN_MASK                            0x00000004
#define SEQ02__SEQ_MAP2_EN                                 0x00000004
#define SEQ02__SEQ_MAP3_EN_MASK                            0x00000008
#define SEQ02__SEQ_MAP3_EN                                 0x00000008

// SEQ04
#define SEQ04__SEQ_256K_MASK                               0x00000002
#define SEQ04__SEQ_256K                                    0x00000002
#define SEQ04__SEQ_ODDEVEN_MASK                            0x00000004
#define SEQ04__SEQ_ODDEVEN                                 0x00000004
#define SEQ04__SEQ_CHAIN_MASK                              0x00000008
#define SEQ04__SEQ_CHAIN                                   0x00000008

// CRT1E
#define CRT1E__GRPH_DEC_RD1_MASK                           0x00000002
#define CRT1E__GRPH_DEC_RD1                                0x00000002

// CRT1F
#define CRT1F__GRPH_DEC_RD0_MASK                           0x000000ff

// CRT22
#define CRT22__GRPH_LATCH_DATA_MASK                        0x000000ff

// CRT1E_S
#define CRT1E_S__GRPH_DEC_RD1_M_MASK                       0x00000002
#define CRT1E_S__GRPH_DEC_RD1_M                            0x00000002

// CRT1F_S
#define CRT1F_S__GRPH_DEC_RD0_M_MASK                       0x000000ff

// CRT22_S
#define CRT22_S__GRPH_LATCH_DATA_M_MASK                    0x000000ff

// HOST_PATH_CNTL
#define HOST_PATH_CNTL__HDP_APER_CNTL_MASK                 0x00800000
#define HOST_PATH_CNTL__HDP_APER_CNTL                      0x00800000
#define HOST_PATH_CNTL__HP_LIN_RD_CACHE_DIS_MASK           0x01000000
#define HOST_PATH_CNTL__HP_LIN_RD_CACHE_DIS                0x01000000
#define HOST_PATH_CNTL__HP_RBBM_LOCK_DIS_MASK              0x02000000
#define HOST_PATH_CNTL__HP_RBBM_LOCK_DIS                   0x02000000
#define HOST_PATH_CNTL__HDP_SOFT_RESET_MASK                0x04000000
#define HOST_PATH_CNTL__HDP_SOFT_RESET                     0x04000000
#define HOST_PATH_CNTL__HDP_WRITE_COMBINER_TIMEOUT_MASK    0x70000000
#define HOST_PATH_CNTL__HP_TEST_RST_CNTL_MASK              0x80000000
#define HOST_PATH_CNTL__HP_TEST_RST_CNTL                   0x80000000

// MEM_VGA_WP_SEL
#define MEM_VGA_WP_SEL__MEM_VGA_WPS0_MASK                  0x00000fff
#define MEM_VGA_WP_SEL__MEM_VGA_WPS1_MASK                  0x0fff0000

// MEM_VGA_RP_SEL
#define MEM_VGA_RP_SEL__MEM_VGA_RPS0_MASK                  0x00000fff
#define MEM_VGA_RP_SEL__MEM_VGA_RPS1_MASK                  0x0fff0000

// HDP_DEBUG
#define HDP_DEBUG__HDP_0_DEBUG_MASK                        0x00000001
#define HDP_DEBUG__HDP_0_DEBUG                             0x00000001
#define HDP_DEBUG__HDP_1_DEBUG_MASK                        0x00000002
#define HDP_DEBUG__HDP_1_DEBUG                             0x00000002
#define HDP_DEBUG__HDP_2_DEBUG_MASK                        0x00000004
#define HDP_DEBUG__HDP_2_DEBUG                             0x00000004
#define HDP_DEBUG__HDP_3_DEBUG_MASK                        0x00000008
#define HDP_DEBUG__HDP_3_DEBUG                             0x00000008
#define HDP_DEBUG__HDP_4_DEBUG_MASK                        0x00000010
#define HDP_DEBUG__HDP_4_DEBUG                             0x00000010
#define HDP_DEBUG__HDP_5_DEBUG_MASK                        0x00000020
#define HDP_DEBUG__HDP_5_DEBUG                             0x00000020
#define HDP_DEBUG__HDP_6_DEBUG_MASK                        0x00000040
#define HDP_DEBUG__HDP_6_DEBUG                             0x00000040
#define HDP_DEBUG__HDP_7_DEBUG_MASK                        0x00000080
#define HDP_DEBUG__HDP_7_DEBUG                             0x00000080

// SW_SEMAPHORE
#define SW_SEMAPHORE__SW_SEMAPHORE_MASK                    0x0000ffff

// SURFACE_CNTL
#define SURFACE_CNTL__SURF_TRANSLATION_DIS_MASK            0x00000100
#define SURFACE_CNTL__SURF_TRANSLATION_DIS                 0x00000100
#define SURFACE_CNTL__NONSURF_AP0_SWP_MASK                 0x00300000
#define SURFACE_CNTL__NONSURF_AP1_SWP_MASK                 0x00c00000

// SURFACE0_LOWER_BOUND
#define SURFACE0_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE1_LOWER_BOUND
#define SURFACE1_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE2_LOWER_BOUND
#define SURFACE2_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE3_LOWER_BOUND
#define SURFACE3_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE4_LOWER_BOUND
#define SURFACE4_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE5_LOWER_BOUND
#define SURFACE5_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE6_LOWER_BOUND
#define SURFACE6_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE7_LOWER_BOUND
#define SURFACE7_LOWER_BOUND__SURF_LOWER_MASK              0x0fffffff

// SURFACE0_UPPER_BOUND
#define SURFACE0_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE1_UPPER_BOUND
#define SURFACE1_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE2_UPPER_BOUND
#define SURFACE2_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE3_UPPER_BOUND
#define SURFACE3_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE4_UPPER_BOUND
#define SURFACE4_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE5_UPPER_BOUND
#define SURFACE5_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE6_UPPER_BOUND
#define SURFACE6_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE7_UPPER_BOUND
#define SURFACE7_UPPER_BOUND__SURF_UPPER_MASK              0x0fffffff

// SURFACE0_INFO
#define SURFACE0_INFO__SURF0_PITCHSEL_MASK                 0x000003ff
#define SURFACE0_INFO__SURF0_TILE_MODE_MASK                0x00030000
#define SURFACE0_INFO__SURF0_AP0_SWP_MASK                  0x00300000
#define SURFACE0_INFO__SURF0_AP1_SWP_MASK                  0x00c00000
#define SURFACE0_INFO__SURF0_WRITE_FLAG_MASK               0x01000000
#define SURFACE0_INFO__SURF0_WRITE_FLAG                    0x01000000
#define SURFACE0_INFO__SURF0_READ_FLAG_MASK                0x02000000
#define SURFACE0_INFO__SURF0_READ_FLAG                     0x02000000

// SURFACE1_INFO
#define SURFACE1_INFO__SURF1_PITCHSEL_MASK                 0x000003ff
#define SURFACE1_INFO__SURF1_TILE_MODE_MASK                0x00030000
#define SURFACE1_INFO__SURF1_AP0_SWP_MASK                  0x00300000
#define SURFACE1_INFO__SURF1_AP1_SWP_MASK                  0x00c00000
#define SURFACE1_INFO__SURF1_WRITE_FLAG_MASK               0x01000000
#define SURFACE1_INFO__SURF1_WRITE_FLAG                    0x01000000
#define SURFACE1_INFO__SURF1_READ_FLAG_MASK                0x02000000
#define SURFACE1_INFO__SURF1_READ_FLAG                     0x02000000

// SURFACE2_INFO
#define SURFACE2_INFO__SURF2_PITCHSEL_MASK                 0x000003ff
#define SURFACE2_INFO__SURF2_TILE_MODE_MASK                0x00030000
#define SURFACE2_INFO__SURF2_AP0_SWP_MASK                  0x00300000
#define SURFACE2_INFO__SURF2_AP1_SWP_MASK                  0x00c00000
#define SURFACE2_INFO__SURF2_WRITE_FLAG_MASK               0x01000000
#define SURFACE2_INFO__SURF2_WRITE_FLAG                    0x01000000
#define SURFACE2_INFO__SURF2_READ_FLAG_MASK                0x02000000
#define SURFACE2_INFO__SURF2_READ_FLAG                     0x02000000

// SURFACE3_INFO
#define SURFACE3_INFO__SURF3_PITCHSEL_MASK                 0x000003ff
#define SURFACE3_INFO__SURF3_TILE_MODE_MASK                0x00030000
#define SURFACE3_INFO__SURF3_AP0_SWP_MASK                  0x00300000
#define SURFACE3_INFO__SURF3_AP1_SWP_MASK                  0x00c00000
#define SURFACE3_INFO__SURF3_WRITE_FLAG_MASK               0x01000000
#define SURFACE3_INFO__SURF3_WRITE_FLAG                    0x01000000
#define SURFACE3_INFO__SURF3_READ_FLAG_MASK                0x02000000
#define SURFACE3_INFO__SURF3_READ_FLAG                     0x02000000

// SURFACE4_INFO
#define SURFACE4_INFO__SURF4_PITCHSEL_MASK                 0x000003ff
#define SURFACE4_INFO__SURF4_TILE_MODE_MASK                0x00030000
#define SURFACE4_INFO__SURF4_AP0_SWP_MASK                  0x00300000
#define SURFACE4_INFO__SURF4_AP1_SWP_MASK                  0x00c00000
#define SURFACE4_INFO__SURF4_WRITE_FLAG_MASK               0x01000000
#define SURFACE4_INFO__SURF4_WRITE_FLAG                    0x01000000
#define SURFACE4_INFO__SURF4_READ_FLAG_MASK                0x02000000
#define SURFACE4_INFO__SURF4_READ_FLAG                     0x02000000

// SURFACE5_INFO
#define SURFACE5_INFO__SURF5_PITCHSEL_MASK                 0x000003ff
#define SURFACE5_INFO__SURF5_TILE_MODE_MASK                0x00030000
#define SURFACE5_INFO__SURF5_AP0_SWP_MASK                  0x00300000
#define SURFACE5_INFO__SURF5_AP1_SWP_MASK                  0x00c00000
#define SURFACE5_INFO__SURF5_WRITE_FLAG_MASK               0x01000000
#define SURFACE5_INFO__SURF5_WRITE_FLAG                    0x01000000
#define SURFACE5_INFO__SURF5_READ_FLAG_MASK                0x02000000
#define SURFACE5_INFO__SURF5_READ_FLAG                     0x02000000

// SURFACE6_INFO
#define SURFACE6_INFO__SURF6_PITCHSEL_MASK                 0x000003ff
#define SURFACE6_INFO__SURF6_TILE_MODE_MASK                0x00030000
#define SURFACE6_INFO__SURF6_AP0_SWP_MASK                  0x00300000
#define SURFACE6_INFO__SURF6_AP1_SWP_MASK                  0x00c00000
#define SURFACE6_INFO__SURF6_WRITE_FLAG_MASK               0x01000000
#define SURFACE6_INFO__SURF6_WRITE_FLAG                    0x01000000
#define SURFACE6_INFO__SURF6_READ_FLAG_MASK                0x02000000
#define SURFACE6_INFO__SURF6_READ_FLAG                     0x02000000

// SURFACE7_INFO
#define SURFACE7_INFO__SURF7_PITCHSEL_MASK                 0x000003ff
#define SURFACE7_INFO__SURF7_TILE_MODE_MASK                0x00030000
#define SURFACE7_INFO__SURF7_AP0_SWP_MASK                  0x00300000
#define SURFACE7_INFO__SURF7_AP1_SWP_MASK                  0x00c00000
#define SURFACE7_INFO__SURF7_WRITE_FLAG_MASK               0x01000000
#define SURFACE7_INFO__SURF7_WRITE_FLAG                    0x01000000
#define SURFACE7_INFO__SURF7_READ_FLAG_MASK                0x02000000
#define SURFACE7_INFO__SURF7_READ_FLAG                     0x02000000

// SURFACE_ACCESS_FLAGS
#define SURFACE_ACCESS_FLAGS__SURF0_WRITE_FLAG_MASK        0x00000001
#define SURFACE_ACCESS_FLAGS__SURF0_WRITE_FLAG             0x00000001
#define SURFACE_ACCESS_FLAGS__SURF1_WRITE_FLAG_MASK        0x00000002
#define SURFACE_ACCESS_FLAGS__SURF1_WRITE_FLAG             0x00000002
#define SURFACE_ACCESS_FLAGS__SURF2_WRITE_FLAG_MASK        0x00000004
#define SURFACE_ACCESS_FLAGS__SURF2_WRITE_FLAG             0x00000004
#define SURFACE_ACCESS_FLAGS__SURF3_WRITE_FLAG_MASK        0x00000008
#define SURFACE_ACCESS_FLAGS__SURF3_WRITE_FLAG             0x00000008
#define SURFACE_ACCESS_FLAGS__SURF4_WRITE_FLAG_MASK        0x00000010
#define SURFACE_ACCESS_FLAGS__SURF4_WRITE_FLAG             0x00000010
#define SURFACE_ACCESS_FLAGS__SURF5_WRITE_FLAG_MASK        0x00000020
#define SURFACE_ACCESS_FLAGS__SURF5_WRITE_FLAG             0x00000020
#define SURFACE_ACCESS_FLAGS__SURF6_WRITE_FLAG_MASK        0x00000040
#define SURFACE_ACCESS_FLAGS__SURF6_WRITE_FLAG             0x00000040
#define SURFACE_ACCESS_FLAGS__SURF7_WRITE_FLAG_MASK        0x00000080
#define SURFACE_ACCESS_FLAGS__SURF7_WRITE_FLAG             0x00000080
#define SURFACE_ACCESS_FLAGS__NONSURF_WRITE_FLAG_MASK      0x00000100
#define SURFACE_ACCESS_FLAGS__NONSURF_WRITE_FLAG           0x00000100
#define SURFACE_ACCESS_FLAGS__LINEAR_WRITE_FLAG_MASK       0x00000200
#define SURFACE_ACCESS_FLAGS__LINEAR_WRITE_FLAG            0x00000200
#define SURFACE_ACCESS_FLAGS__VGA_WRITE_FLAG_MASK          0x00000400
#define SURFACE_ACCESS_FLAGS__VGA_WRITE_FLAG               0x00000400
#define SURFACE_ACCESS_FLAGS__SURF0_READ_FLAG_MASK         0x00010000
#define SURFACE_ACCESS_FLAGS__SURF0_READ_FLAG              0x00010000
#define SURFACE_ACCESS_FLAGS__SURF1_READ_FLAG_MASK         0x00020000
#define SURFACE_ACCESS_FLAGS__SURF1_READ_FLAG              0x00020000
#define SURFACE_ACCESS_FLAGS__SURF2_READ_FLAG_MASK         0x00040000
#define SURFACE_ACCESS_FLAGS__SURF2_READ_FLAG              0x00040000
#define SURFACE_ACCESS_FLAGS__SURF3_READ_FLAG_MASK         0x00080000
#define SURFACE_ACCESS_FLAGS__SURF3_READ_FLAG              0x00080000
#define SURFACE_ACCESS_FLAGS__SURF4_READ_FLAG_MASK         0x00100000
#define SURFACE_ACCESS_FLAGS__SURF4_READ_FLAG              0x00100000
#define SURFACE_ACCESS_FLAGS__SURF5_READ_FLAG_MASK         0x00200000
#define SURFACE_ACCESS_FLAGS__SURF5_READ_FLAG              0x00200000
#define SURFACE_ACCESS_FLAGS__SURF6_READ_FLAG_MASK         0x00400000
#define SURFACE_ACCESS_FLAGS__SURF6_READ_FLAG              0x00400000
#define SURFACE_ACCESS_FLAGS__SURF7_READ_FLAG_MASK         0x00800000
#define SURFACE_ACCESS_FLAGS__SURF7_READ_FLAG              0x00800000
#define SURFACE_ACCESS_FLAGS__NONSURF_READ_FLAG_MASK       0x01000000
#define SURFACE_ACCESS_FLAGS__NONSURF_READ_FLAG            0x01000000
#define SURFACE_ACCESS_FLAGS__LINEAR_READ_FLAG_MASK        0x02000000
#define SURFACE_ACCESS_FLAGS__LINEAR_READ_FLAG             0x02000000
#define SURFACE_ACCESS_FLAGS__VGA_READ_FLAG_MASK           0x04000000
#define SURFACE_ACCESS_FLAGS__VGA_READ_FLAG                0x04000000

// SURFACE_ACCESS_CLR
#define SURFACE_ACCESS_CLR__SURF0_WRITE_FLAG_CLR_MASK      0x00000001
#define SURFACE_ACCESS_CLR__SURF0_WRITE_FLAG_CLR           0x00000001
#define SURFACE_ACCESS_CLR__SURF1_WRITE_FLAG_CLR_MASK      0x00000002
#define SURFACE_ACCESS_CLR__SURF1_WRITE_FLAG_CLR           0x00000002
#define SURFACE_ACCESS_CLR__SURF2_WRITE_FLAG_CLR_MASK      0x00000004
#define SURFACE_ACCESS_CLR__SURF2_WRITE_FLAG_CLR           0x00000004
#define SURFACE_ACCESS_CLR__SURF3_WRITE_FLAG_CLR_MASK      0x00000008
#define SURFACE_ACCESS_CLR__SURF3_WRITE_FLAG_CLR           0x00000008
#define SURFACE_ACCESS_CLR__SURF4_WRITE_FLAG_CLR_MASK      0x00000010
#define SURFACE_ACCESS_CLR__SURF4_WRITE_FLAG_CLR           0x00000010
#define SURFACE_ACCESS_CLR__SURF5_WRITE_FLAG_CLR_MASK      0x00000020
#define SURFACE_ACCESS_CLR__SURF5_WRITE_FLAG_CLR           0x00000020
#define SURFACE_ACCESS_CLR__SURF6_WRITE_FLAG_CLR_MASK      0x00000040
#define SURFACE_ACCESS_CLR__SURF6_WRITE_FLAG_CLR           0x00000040
#define SURFACE_ACCESS_CLR__SURF7_WRITE_FLAG_CLR_MASK      0x00000080
#define SURFACE_ACCESS_CLR__SURF7_WRITE_FLAG_CLR           0x00000080
#define SURFACE_ACCESS_CLR__NONSURF_WRITE_FLAG_CLR_MASK    0x00000100
#define SURFACE_ACCESS_CLR__NONSURF_WRITE_FLAG_CLR         0x00000100
#define SURFACE_ACCESS_CLR__LINEAR_WRITE_FLAG_CLR_MASK     0x00000200
#define SURFACE_ACCESS_CLR__LINEAR_WRITE_FLAG_CLR          0x00000200
#define SURFACE_ACCESS_CLR__VGA_WRITE_FLAG_CLR_MASK        0x00000400
#define SURFACE_ACCESS_CLR__VGA_WRITE_FLAG_CLR             0x00000400
#define SURFACE_ACCESS_CLR__SURF0_READ_FLAG_CLR_MASK       0x00010000
#define SURFACE_ACCESS_CLR__SURF0_READ_FLAG_CLR            0x00010000
#define SURFACE_ACCESS_CLR__SURF1_READ_FLAG_CLR_MASK       0x00020000
#define SURFACE_ACCESS_CLR__SURF1_READ_FLAG_CLR            0x00020000
#define SURFACE_ACCESS_CLR__SURF2_READ_FLAG_CLR_MASK       0x00040000
#define SURFACE_ACCESS_CLR__SURF2_READ_FLAG_CLR            0x00040000
#define SURFACE_ACCESS_CLR__SURF3_READ_FLAG_CLR_MASK       0x00080000
#define SURFACE_ACCESS_CLR__SURF3_READ_FLAG_CLR            0x00080000
#define SURFACE_ACCESS_CLR__SURF4_READ_FLAG_CLR_MASK       0x00100000
#define SURFACE_ACCESS_CLR__SURF4_READ_FLAG_CLR            0x00100000
#define SURFACE_ACCESS_CLR__SURF5_READ_FLAG_CLR_MASK       0x00200000
#define SURFACE_ACCESS_CLR__SURF5_READ_FLAG_CLR            0x00200000
#define SURFACE_ACCESS_CLR__SURF6_READ_FLAG_CLR_MASK       0x00400000
#define SURFACE_ACCESS_CLR__SURF6_READ_FLAG_CLR            0x00400000
#define SURFACE_ACCESS_CLR__SURF7_READ_FLAG_CLR_MASK       0x00800000
#define SURFACE_ACCESS_CLR__SURF7_READ_FLAG_CLR            0x00800000
#define SURFACE_ACCESS_CLR__NONSURF_READ_FLAG_CLR_MASK     0x01000000
#define SURFACE_ACCESS_CLR__NONSURF_READ_FLAG_CLR          0x01000000
#define SURFACE_ACCESS_CLR__LINEAR_READ_FLAG_CLR_MASK      0x02000000
#define SURFACE_ACCESS_CLR__LINEAR_READ_FLAG_CLR           0x02000000
#define SURFACE_ACCESS_CLR__VGA_READ_FLAG_CLR_MASK         0x04000000
#define SURFACE_ACCESS_CLR__VGA_READ_FLAG_CLR              0x04000000

// GRPH8_IDX
#define GRPH8_IDX__GRPH_IDX_MASK                           0x0000000f

// GRPH8_DATA
#define GRPH8_DATA__GRPH_DATA_MASK                         0x000000ff

// GRA05
#define GRA05__GRPH_WRITE_MODE_MASK                        0x00000003
#define GRA05__GRPH_READ1_MASK                             0x00000008
#define GRA05__GRPH_READ1                                  0x00000008
#define GRA05__CGA_ODDEVEN_MASK                            0x00000010
#define GRA05__CGA_ODDEVEN                                 0x00000010
#define GRA05__GRPH_OES_MASK                               0x00000020
#define GRA05__GRPH_OES                                    0x00000020
#define GRA05__GRPH_PACK_MASK                              0x00000040
#define GRA05__GRPH_PACK                                   0x00000040

// SEQ8_IDX
#define SEQ8_IDX__SEQ_IDX_MASK                             0x00000007

// SEQ8_DATA
#define SEQ8_DATA__SEQ_DATA_MASK                           0x000000ff

// CRTC8_IDX
#define CRTC8_IDX__VCRTC_IDX_MASK                          0x0000003f

// CRTC8_DATA
#define CRTC8_DATA__VCRTC_DATA_MASK                        0x000000ff

// CRT14
#define CRT14__UNDRLN_LOC_MASK                             0x0000001f
#define CRT14__ADDR_CNT_BY4_MASK                           0x00000020
#define CRT14__ADDR_CNT_BY4                                0x00000020
#define CRT14__DOUBLE_WORD_MASK                            0x00000040
#define CRT14__DOUBLE_WORD                                 0x00000040

// CRT17
#define CRT17__RA0_AS_A13B_MASK                            0x00000001
#define CRT17__RA0_AS_A13B                                 0x00000001
#define CRT17__RA1_AS_A14B_MASK                            0x00000002
#define CRT17__RA1_AS_A14B                                 0x00000002
#define CRT17__VCOUNT_BY2_MASK                             0x00000004
#define CRT17__VCOUNT_BY2                                  0x00000004
#define CRT17__ADDR_CNT_BY2_MASK                           0x00000008
#define CRT17__ADDR_CNT_BY2                                0x00000008
#define CRT17__WRAP_A15TOA0_MASK                           0x00000020
#define CRT17__WRAP_A15TOA0                                0x00000020
#define CRT17__BYTE_MODE_MASK                              0x00000040
#define CRT17__BYTE_MODE                                   0x00000040
#define CRT17__CRTC_SYNC_EN_MASK                           0x00000080
#define CRT17__CRTC_SYNC_EN                                0x00000080

// CRT14_S
#define CRT14_S__UNDRLN_LOC_S_MASK                         0x0000001f
#define CRT14_S__ADDR_CNT_BY4_M_MASK                       0x00000020
#define CRT14_S__ADDR_CNT_BY4_M                            0x00000020
#define CRT14_S__DOUBLE_WORD_M_MASK                        0x00000040
#define CRT14_S__DOUBLE_WORD_M                             0x00000040

// CRT17_S
#define CRT17_S__RA0_AS_A13B_M_MASK                        0x00000001
#define CRT17_S__RA0_AS_A13B_M                             0x00000001
#define CRT17_S__RA1_AS_A14B_M_MASK                        0x00000002
#define CRT17_S__RA1_AS_A14B_M                             0x00000002
#define CRT17_S__VCOUNT_BY2_S_MASK                         0x00000004
#define CRT17_S__VCOUNT_BY2_S                              0x00000004
#define CRT17_S__ADDR_CNT_BY2_M_MASK                       0x00000008
#define CRT17_S__ADDR_CNT_BY2_M                            0x00000008
#define CRT17_S__WRAP_A15TOA0_M_MASK                       0x00000020
#define CRT17_S__WRAP_A15TOA0_M                            0x00000020
#define CRT17_S__BYTE_MODE_M_MASK                          0x00000040
#define CRT17_S__BYTE_MODE_M                               0x00000040
#define CRT17_S__CRTC_SYNC_EN_M_MASK                       0x00000080
#define CRT17_S__CRTC_SYNC_EN_M                            0x00000080

// GEN_INT_CNTL
#define GEN_INT_CNTL__CRTC_VBLANK_MASK_MASK                0x00000001
#define GEN_INT_CNTL__CRTC_VBLANK_MASK                     0x00000001
#define GEN_INT_CNTL__CRTC_VLINE_MASK_MASK                 0x00000002
#define GEN_INT_CNTL__CRTC_VLINE_MASK                      0x00000002
#define GEN_INT_CNTL__CRTC_VSYNC_MASK_MASK                 0x00000004
#define GEN_INT_CNTL__CRTC_VSYNC_MASK                      0x00000004
#define GEN_INT_CNTL__SNAPSHOT_MASK_MASK                   0x00000008
#define GEN_INT_CNTL__SNAPSHOT_MASK                        0x00000008
#define GEN_INT_CNTL__FP_DETECT_MASK_MASK                  0x00000010
#define GEN_INT_CNTL__FP_DETECT_MASK                       0x00000010
#define GEN_INT_CNTL__DMA_VIPH0_INT_EN_MASK                0x00001000
#define GEN_INT_CNTL__DMA_VIPH0_INT_EN                     0x00001000
#define GEN_INT_CNTL__DMA_VIPH1_INT_EN_MASK                0x00002000
#define GEN_INT_CNTL__DMA_VIPH1_INT_EN                     0x00002000
#define GEN_INT_CNTL__DMA_VIPH2_INT_EN_MASK                0x00004000
#define GEN_INT_CNTL__DMA_VIPH2_INT_EN                     0x00004000
#define GEN_INT_CNTL__DMA_VIPH3_INT_EN_MASK                0x00008000
#define GEN_INT_CNTL__DMA_VIPH3_INT_EN                     0x00008000
#define GEN_INT_CNTL__I2C_INT_EN_MASK                      0x00020000
#define GEN_INT_CNTL__I2C_INT_EN                           0x00020000
#define GEN_INT_CNTL__GUI_IDLE_MASK_MASK                   0x00080000
#define GEN_INT_CNTL__GUI_IDLE_MASK                        0x00080000
#define GEN_INT_CNTL__VIPH_INT_EN_MASK                     0x01000000
#define GEN_INT_CNTL__VIPH_INT_EN                          0x01000000
#define GEN_INT_CNTL__SW_INT_EN_MASK                       0x02000000
#define GEN_INT_CNTL__SW_INT_EN                            0x02000000
#define GEN_INT_CNTL__GUIDMA_MASK_MASK                     0x40000000
#define GEN_INT_CNTL__GUIDMA_MASK                          0x40000000
#define GEN_INT_CNTL__VIDDMA_MASK_MASK                     0x80000000
#define GEN_INT_CNTL__VIDDMA_MASK                          0x80000000

// GEN_INT_STATUS
#define GEN_INT_STATUS__CRTC_VBLANK_STAT_MASK              0x00000001
#define GEN_INT_STATUS__CRTC_VBLANK_STAT                   0x00000001
#define GEN_INT_STATUS__CRTC_VBLANK_STAT_AK_MASK           0x00000001
#define GEN_INT_STATUS__CRTC_VBLANK_STAT_AK                0x00000001
#define GEN_INT_STATUS__CRTC_VLINE_STAT_MASK               0x00000002
#define GEN_INT_STATUS__CRTC_VLINE_STAT                    0x00000002
#define GEN_INT_STATUS__CRTC_VLINE_STAT_AK_MASK            0x00000002
#define GEN_INT_STATUS__CRTC_VLINE_STAT_AK                 0x00000002
#define GEN_INT_STATUS__CRTC_VSYNC_STAT_MASK               0x00000004
#define GEN_INT_STATUS__CRTC_VSYNC_STAT                    0x00000004
#define GEN_INT_STATUS__CRTC_VSYNC_STAT_AK_MASK            0x00000004
#define GEN_INT_STATUS__CRTC_VSYNC_STAT_AK                 0x00000004
#define GEN_INT_STATUS__SNAPSHOT_STAT_MASK                 0x00000008
#define GEN_INT_STATUS__SNAPSHOT_STAT                      0x00000008
#define GEN_INT_STATUS__SNAPSHOT_STAT_AK_MASK              0x00000008
#define GEN_INT_STATUS__SNAPSHOT_STAT_AK                   0x00000008
#define GEN_INT_STATUS__FP_DETECT_STAT_MASK                0x00000010
#define GEN_INT_STATUS__FP_DETECT_STAT                     0x00000010
#define GEN_INT_STATUS__FP_DETECT_STAT_AK_MASK             0x00000010
#define GEN_INT_STATUS__FP_DETECT_STAT_AK                  0x00000010
#define GEN_INT_STATUS__CAP0_INT_ACTIVE_MASK               0x00000100
#define GEN_INT_STATUS__CAP0_INT_ACTIVE                    0x00000100
#define GEN_INT_STATUS__DMA_VIPH0_INT_MASK                 0x00001000
#define GEN_INT_STATUS__DMA_VIPH0_INT                      0x00001000
#define GEN_INT_STATUS__DMA_VIPH0_INT_AK_MASK              0x00001000
#define GEN_INT_STATUS__DMA_VIPH0_INT_AK                   0x00001000
#define GEN_INT_STATUS__DMA_VIPH1_INT_MASK                 0x00002000
#define GEN_INT_STATUS__DMA_VIPH1_INT                      0x00002000
#define GEN_INT_STATUS__DMA_VIPH1_INT_AK_MASK              0x00002000
#define GEN_INT_STATUS__DMA_VIPH1_INT_AK                   0x00002000
#define GEN_INT_STATUS__DMA_VIPH2_INT_MASK                 0x00004000
#define GEN_INT_STATUS__DMA_VIPH2_INT                      0x00004000
#define GEN_INT_STATUS__DMA_VIPH2_INT_AK_MASK              0x00004000
#define GEN_INT_STATUS__DMA_VIPH2_INT_AK                   0x00004000
#define GEN_INT_STATUS__DMA_VIPH3_INT_MASK                 0x00008000
#define GEN_INT_STATUS__DMA_VIPH3_INT                      0x00008000
#define GEN_INT_STATUS__DMA_VIPH3_INT_AK_MASK              0x00008000
#define GEN_INT_STATUS__DMA_VIPH3_INT_AK                   0x00008000
#define GEN_INT_STATUS__I2C_INT_MASK                       0x00020000
#define GEN_INT_STATUS__I2C_INT                            0x00020000
#define GEN_INT_STATUS__I2C_INT_AK_MASK                    0x00020000
#define GEN_INT_STATUS__I2C_INT_AK                         0x00020000
#define GEN_INT_STATUS__GUI_IDLE_STAT_MASK                 0x00080000
#define GEN_INT_STATUS__GUI_IDLE_STAT                      0x00080000
#define GEN_INT_STATUS__GUI_IDLE_STAT_AK_MASK              0x00080000
#define GEN_INT_STATUS__GUI_IDLE_STAT_AK                   0x00080000
#define GEN_INT_STATUS__VIPH_INT_MASK                      0x01000000
#define GEN_INT_STATUS__VIPH_INT                           0x01000000
#define GEN_INT_STATUS__SW_INT_MASK                        0x02000000
#define GEN_INT_STATUS__SW_INT                             0x02000000
#define GEN_INT_STATUS__SW_INT_AK_MASK                     0x02000000
#define GEN_INT_STATUS__SW_INT_AK                          0x02000000
#define GEN_INT_STATUS__SW_INT_SET_MASK                    0x04000000
#define GEN_INT_STATUS__SW_INT_SET                         0x04000000
#define GEN_INT_STATUS__GUIDMA_STAT_MASK                   0x40000000
#define GEN_INT_STATUS__GUIDMA_STAT                        0x40000000
#define GEN_INT_STATUS__GUIDMA_AK_MASK                     0x40000000
#define GEN_INT_STATUS__GUIDMA_AK                          0x40000000
#define GEN_INT_STATUS__VIDDMA_STAT_MASK                   0x80000000
#define GEN_INT_STATUS__VIDDMA_STAT                        0x80000000
#define GEN_INT_STATUS__VIDDMA_AK_MASK                     0x80000000
#define GEN_INT_STATUS__VIDDMA_AK                          0x80000000

// CRTC_EXT_CNTL
#define CRTC_EXT_CNTL__CRTC_VGA_XOVERSCAN_MASK             0x00000001
#define CRTC_EXT_CNTL__CRTC_VGA_XOVERSCAN                  0x00000001
#define CRTC_EXT_CNTL__VGA_BLINK_RATE_MASK                 0x00000006
#define CRTC_EXT_CNTL__VGA_ATI_LINEAR_MASK                 0x00000008
#define CRTC_EXT_CNTL__VGA_ATI_LINEAR                      0x00000008
#define CRTC_EXT_CNTL__VGA_128KAP_PAGING_MASK              0x00000010
#define CRTC_EXT_CNTL__VGA_128KAP_PAGING                   0x00000010
#define CRTC_EXT_CNTL__VGA_TEXT_132_MASK                   0x00000020
#define CRTC_EXT_CNTL__VGA_TEXT_132                        0x00000020
#define CRTC_EXT_CNTL__VGA_XCRT_CNT_EN_MASK                0x00000040
#define CRTC_EXT_CNTL__VGA_XCRT_CNT_EN                     0x00000040
#define CRTC_EXT_CNTL__CRTC_HSYNC_DIS_MASK                 0x00000100
#define CRTC_EXT_CNTL__CRTC_HSYNC_DIS                      0x00000100
#define CRTC_EXT_CNTL__CRTC_VSYNC_DIS_MASK                 0x00000200
#define CRTC_EXT_CNTL__CRTC_VSYNC_DIS                      0x00000200
#define CRTC_EXT_CNTL__CRTC_DISPLAY_DIS_MASK               0x00000400
#define CRTC_EXT_CNTL__CRTC_DISPLAY_DIS                    0x00000400
#define CRTC_EXT_CNTL__CRTC_SYNC_TRISTATE_MASK             0x00000800
#define CRTC_EXT_CNTL__CRTC_SYNC_TRISTATE                  0x00000800
#define CRTC_EXT_CNTL__CRTC_HSYNC_TRISTATE_MASK            0x00001000
#define CRTC_EXT_CNTL__CRTC_HSYNC_TRISTATE                 0x00001000
#define CRTC_EXT_CNTL__CRTC_VSYNC_TRISTATE_MASK            0x00002000
#define CRTC_EXT_CNTL__CRTC_VSYNC_TRISTATE                 0x00002000
#define CRTC_EXT_CNTL__VGA_CUR_B_TEST_MASK                 0x00020000
#define CRTC_EXT_CNTL__VGA_CUR_B_TEST                      0x00020000
#define CRTC_EXT_CNTL__VGA_PACK_DIS_MASK                   0x00040000
#define CRTC_EXT_CNTL__VGA_PACK_DIS                        0x00040000
#define CRTC_EXT_CNTL__VGA_MEM_PS_EN_MASK                  0x00080000
#define CRTC_EXT_CNTL__VGA_MEM_PS_EN                       0x00080000
#define CRTC_EXT_CNTL__VCRTC_IDX_MASTER_MASK               0x7f000000

// WAIT_UNTIL
#define WAIT_UNTIL__WAIT_CRTC_PFLIP_MASK                   0x00000001
#define WAIT_UNTIL__WAIT_CRTC_PFLIP                        0x00000001
#define WAIT_UNTIL__WAIT_RE_CRTC_VLINE_MASK                0x00000002
#define WAIT_UNTIL__WAIT_RE_CRTC_VLINE                     0x00000002
#define WAIT_UNTIL__WAIT_FE_CRTC_VLINE_MASK                0x00000004
#define WAIT_UNTIL__WAIT_FE_CRTC_VLINE                     0x00000004
#define WAIT_UNTIL__WAIT_CRTC_VLINE_MASK                   0x00000008
#define WAIT_UNTIL__WAIT_CRTC_VLINE                        0x00000008
#define WAIT_UNTIL__WAIT_DMA_VIPH0_IDLE_MASK               0x00000010
#define WAIT_UNTIL__WAIT_DMA_VIPH0_IDLE                    0x00000010
#define WAIT_UNTIL__WAIT_DMA_VIPH1_IDLE_MASK               0x00000020
#define WAIT_UNTIL__WAIT_DMA_VIPH1_IDLE                    0x00000020
#define WAIT_UNTIL__WAIT_DMA_VIPH2_IDLE_MASK               0x00000040
#define WAIT_UNTIL__WAIT_DMA_VIPH2_IDLE                    0x00000040
#define WAIT_UNTIL__WAIT_DMA_VIPH3_IDLE_MASK               0x00000080
#define WAIT_UNTIL__WAIT_DMA_VIPH3_IDLE                    0x00000080
#define WAIT_UNTIL__WAIT_DMA_VID_IDLE_MASK                 0x00000100
#define WAIT_UNTIL__WAIT_DMA_VID_IDLE                      0x00000100
#define WAIT_UNTIL__WAIT_DMA_GUI_IDLE_MASK                 0x00000200
#define WAIT_UNTIL__WAIT_DMA_GUI_IDLE                      0x00000200
#define WAIT_UNTIL__WAIT_CMDFIFO_MASK                      0x00000400
#define WAIT_UNTIL__WAIT_CMDFIFO                           0x00000400
#define WAIT_UNTIL__WAIT_OV0_FLIP_MASK                     0x00000800
#define WAIT_UNTIL__WAIT_OV0_FLIP                          0x00000800
#define WAIT_UNTIL__WAIT_OV0_SLICEDONE_MASK                0x00001000
#define WAIT_UNTIL__WAIT_OV0_SLICEDONE                     0x00001000
#define WAIT_UNTIL__WAIT_2D_IDLE_MASK                      0x00004000
#define WAIT_UNTIL__WAIT_2D_IDLE                           0x00004000
#define WAIT_UNTIL__WAIT_3D_IDLE_MASK                      0x00008000
#define WAIT_UNTIL__WAIT_3D_IDLE                           0x00008000
#define WAIT_UNTIL__WAIT_2D_IDLECLEAN_MASK                 0x00010000
#define WAIT_UNTIL__WAIT_2D_IDLECLEAN                      0x00010000
#define WAIT_UNTIL__WAIT_3D_IDLECLEAN_MASK                 0x00020000
#define WAIT_UNTIL__WAIT_3D_IDLECLEAN                      0x00020000
#define WAIT_UNTIL__WAIT_HOST_IDLECLEAN_MASK               0x00040000
#define WAIT_UNTIL__WAIT_HOST_IDLECLEAN                    0x00040000
#define WAIT_UNTIL__WAIT_EXTERN_SIG_MASK                   0x00080000
#define WAIT_UNTIL__WAIT_EXTERN_SIG                        0x00080000
#define WAIT_UNTIL__CMDFIFO_ENTRIES_MASK                   0x07f00000

// ISYNC_CNTL
#define ISYNC_CNTL__ISYNC_ANY2D_IDLE3D_MASK                0x00000001
#define ISYNC_CNTL__ISYNC_ANY2D_IDLE3D                     0x00000001
#define ISYNC_CNTL__ISYNC_ANY3D_IDLE2D_MASK                0x00000002
#define ISYNC_CNTL__ISYNC_ANY3D_IDLE2D                     0x00000002
#define ISYNC_CNTL__ISYNC_TRIG2D_IDLE3D_MASK               0x00000004
#define ISYNC_CNTL__ISYNC_TRIG2D_IDLE3D                    0x00000004
#define ISYNC_CNTL__ISYNC_TRIG3D_IDLE2D_MASK               0x00000008
#define ISYNC_CNTL__ISYNC_TRIG3D_IDLE2D                    0x00000008
#define ISYNC_CNTL__ISYNC_WAIT_IDLEGUI_MASK                0x00000010
#define ISYNC_CNTL__ISYNC_WAIT_IDLEGUI                     0x00000010
#define ISYNC_CNTL__ISYNC_CPSCRATCH_IDLEGUI_MASK           0x00000020
#define ISYNC_CNTL__ISYNC_CPSCRATCH_IDLEGUI                0x00000020

// RBBM_GUICNTL
#define RBBM_GUICNTL__HOST_DATA_SWAP_MASK                  0x00000003

// RBBM_STATUS
#define RBBM_STATUS__CMDFIFO_AVAIL_MASK                    0x0000007f
#define RBBM_STATUS__HIRQ_ON_RBB_MASK                      0x00000100
#define RBBM_STATUS__HIRQ_ON_RBB                           0x00000100
#define RBBM_STATUS__CPRQ_ON_RBB_MASK                      0x00000200
#define RBBM_STATUS__CPRQ_ON_RBB                           0x00000200
#define RBBM_STATUS__CFRQ_ON_RBB_MASK                      0x00000400
#define RBBM_STATUS__CFRQ_ON_RBB                           0x00000400
#define RBBM_STATUS__HIRQ_IN_RTBUF_MASK                    0x00000800
#define RBBM_STATUS__HIRQ_IN_RTBUF                         0x00000800
#define RBBM_STATUS__CPRQ_IN_RTBUF_MASK                    0x00001000
#define RBBM_STATUS__CPRQ_IN_RTBUF                         0x00001000
#define RBBM_STATUS__CFRQ_IN_RTBUF_MASK                    0x00002000
#define RBBM_STATUS__CFRQ_IN_RTBUF                         0x00002000
#define RBBM_STATUS__CF_PIPE_BUSY_MASK                     0x00004000
#define RBBM_STATUS__CF_PIPE_BUSY                          0x00004000
#define RBBM_STATUS__ENG_EV_BUSY_MASK                      0x00008000
#define RBBM_STATUS__ENG_EV_BUSY                           0x00008000
#define RBBM_STATUS__CP_CMDSTRM_BUSY_MASK                  0x00010000
#define RBBM_STATUS__CP_CMDSTRM_BUSY                       0x00010000
#define RBBM_STATUS__E2_BUSY_MASK                          0x00020000
#define RBBM_STATUS__E2_BUSY                               0x00020000
#define RBBM_STATUS__RB2D_BUSY_MASK                        0x00040000
#define RBBM_STATUS__RB2D_BUSY                             0x00040000
#define RBBM_STATUS__RB3D_BUSY_MASK                        0x00080000
#define RBBM_STATUS__RB3D_BUSY                             0x00080000
#define RBBM_STATUS__SE_BUSY_MASK                          0x00100000
#define RBBM_STATUS__SE_BUSY                               0x00100000
#define RBBM_STATUS__RE_BUSY_MASK                          0x00200000
#define RBBM_STATUS__RE_BUSY                               0x00200000
#define RBBM_STATUS__TAM_BUSY_MASK                         0x00400000
#define RBBM_STATUS__TAM_BUSY                              0x00400000
#define RBBM_STATUS__TDM_BUSY_MASK                         0x00800000
#define RBBM_STATUS__TDM_BUSY                              0x00800000
#define RBBM_STATUS__PB_BUSY_MASK                          0x01000000
#define RBBM_STATUS__PB_BUSY                               0x01000000
#define RBBM_STATUS__GUI_ACTIVE_MASK                       0x80000000
#define RBBM_STATUS__GUI_ACTIVE                            0x80000000

// RBBM_CNTL
#define RBBM_CNTL__RB_SETTLE_MASK                          0x0000000f
#define RBBM_CNTL__ABORTCLKS_HI_MASK                       0x00000070
#define RBBM_CNTL__ABORTCLKS_CP_MASK                       0x00000700
#define RBBM_CNTL__ABORTCLKS_CFIFO_MASK                    0x00007000
#define RBBM_CNTL__CPQ_DATA_SWAP_MASK                      0x00020000
#define RBBM_CNTL__CPQ_DATA_SWAP                           0x00020000
#define RBBM_CNTL__NO_ABORT_IDCT_MASK                      0x00200000
#define RBBM_CNTL__NO_ABORT_IDCT                           0x00200000
#define RBBM_CNTL__NO_ABORT_BIOS_MASK                      0x00400000
#define RBBM_CNTL__NO_ABORT_BIOS                           0x00400000
#define RBBM_CNTL__NO_ABORT_FB_MASK                        0x00800000
#define RBBM_CNTL__NO_ABORT_FB                             0x00800000
#define RBBM_CNTL__NO_ABORT_CP_MASK                        0x01000000
#define RBBM_CNTL__NO_ABORT_CP                             0x01000000
#define RBBM_CNTL__NO_ABORT_HI_MASK                        0x02000000
#define RBBM_CNTL__NO_ABORT_HI                             0x02000000
#define RBBM_CNTL__NO_ABORT_HDP_MASK                       0x04000000
#define RBBM_CNTL__NO_ABORT_HDP                            0x04000000
#define RBBM_CNTL__NO_ABORT_MC_MASK                        0x08000000
#define RBBM_CNTL__NO_ABORT_MC                             0x08000000
#define RBBM_CNTL__NO_ABORT_AIC_MASK                       0x10000000
#define RBBM_CNTL__NO_ABORT_AIC                            0x10000000
#define RBBM_CNTL__NO_ABORT_VIP_MASK                       0x20000000
#define RBBM_CNTL__NO_ABORT_VIP                            0x20000000
#define RBBM_CNTL__NO_ABORT_DISP_MASK                      0x40000000
#define RBBM_CNTL__NO_ABORT_DISP                           0x40000000
#define RBBM_CNTL__NO_ABORT_CG_MASK                        0x80000000
#define RBBM_CNTL__NO_ABORT_CG                             0x80000000

// RBBM_SOFT_RESET
#define RBBM_SOFT_RESET__SOFT_RESET_CP_MASK                0x00000001
#define RBBM_SOFT_RESET__SOFT_RESET_CP                     0x00000001
#define RBBM_SOFT_RESET__SOFT_RESET_HI_MASK                0x00000002
#define RBBM_SOFT_RESET__SOFT_RESET_HI                     0x00000002
#define RBBM_SOFT_RESET__SOFT_RESET_SE_MASK                0x00000004
#define RBBM_SOFT_RESET__SOFT_RESET_SE                     0x00000004
#define RBBM_SOFT_RESET__SOFT_RESET_RE_MASK                0x00000008
#define RBBM_SOFT_RESET__SOFT_RESET_RE                     0x00000008
#define RBBM_SOFT_RESET__SOFT_RESET_PP_MASK                0x00000010
#define RBBM_SOFT_RESET__SOFT_RESET_PP                     0x00000010
#define RBBM_SOFT_RESET__SOFT_RESET_E2_MASK                0x00000020
#define RBBM_SOFT_RESET__SOFT_RESET_E2                     0x00000020
#define RBBM_SOFT_RESET__SOFT_RESET_RB_MASK                0x00000040
#define RBBM_SOFT_RESET__SOFT_RESET_RB                     0x00000040
#define RBBM_SOFT_RESET__SOFT_RESET_HDP_MASK               0x00000080
#define RBBM_SOFT_RESET__SOFT_RESET_HDP                    0x00000080
#define RBBM_SOFT_RESET__SOFT_RESET_MC_MASK                0x00000100
#define RBBM_SOFT_RESET__SOFT_RESET_MC                     0x00000100
#define RBBM_SOFT_RESET__SOFT_RESET_AIC_MASK               0x00000200
#define RBBM_SOFT_RESET__SOFT_RESET_AIC                    0x00000200
#define RBBM_SOFT_RESET__SOFT_RESET_VIP_MASK               0x00000400
#define RBBM_SOFT_RESET__SOFT_RESET_VIP                    0x00000400
#define RBBM_SOFT_RESET__SOFT_RESET_DISP_MASK              0x00000800
#define RBBM_SOFT_RESET__SOFT_RESET_DISP                   0x00000800
#define RBBM_SOFT_RESET__SOFT_RESET_CG_MASK                0x00001000
#define RBBM_SOFT_RESET__SOFT_RESET_CG                     0x00001000

// NQWAIT_UNTIL
#define NQWAIT_UNTIL__WAIT_GUI_IDLE_MASK                   0x00000001
#define NQWAIT_UNTIL__WAIT_GUI_IDLE                        0x00000001

// RBBM_DEBUG
#define RBBM_DEBUG__EN_BESWAP_MASK                         0x00000001
#define RBBM_DEBUG__EN_BESWAP                              0x00000001
#define RBBM_DEBUG__RBBM_DEBUG_MASK                        0xfffffffe

// RBBM_CMDFIFO_ADDR
#define RBBM_CMDFIFO_ADDR__CMDFIFO_ADDR_MASK               0x0000003f

// RBBM_CMDFIFO_DATAL
#define RBBM_CMDFIFO_DATAL__CMDFIFO_DATAL_MASK             0xffffffff

// RBBM_CMDFIFO_DATAH
#define RBBM_CMDFIFO_DATAH__CMDFIFO_DATAH_MASK             0x00000fff

// RBBM_CMDFIFO_STAT
#define RBBM_CMDFIFO_STAT__CMDFIFO_RPTR_MASK               0x0000003f
#define RBBM_CMDFIFO_STAT__CMDFIFO_WPTR_MASK               0x00003f00

// GENFC_RD
#define GENFC_RD__VSYNC_SEL_R_MASK                         0x00000008
#define GENFC_RD__VSYNC_SEL_R                              0x00000008

// GENFC_WT
#define GENFC_WT__VSYNC_SEL_W_MASK                         0x00000008
#define GENFC_WT__VSYNC_SEL_W                              0x00000008

// GENS0
#define GENS0__SENSE_SWITCH_MASK                           0x00000010
#define GENS0__SENSE_SWITCH                                0x00000010
#define GENS0__CRT_INTR_MASK                               0x00000080
#define GENS0__CRT_INTR                                    0x00000080

// GENS1
#define GENS1__NO_DIPLAY_MASK                              0x00000001
#define GENS1__NO_DIPLAY                                   0x00000001
#define GENS1__VGA_VSTATUS_MASK                            0x00000008
#define GENS1__VGA_VSTATUS                                 0x00000008
#define GENS1__PIXEL_READ_BACK_MASK                        0x00000030

// DAC_DATA
#define DAC_DATA__DAC_DATA_MASK                            0x000000ff

// DAC_MASK
#define DAC_MASK__DAC_MASK_MASK                            0x000000ff

// DAC_R_INDEX
#define DAC_R_INDEX__DAC_R_INDEX_MASK                      0x000000ff

// DAC_W_INDEX
#define DAC_W_INDEX__DAC_W_INDEX_MASK                      0x000000ff

// SEQ00
#define SEQ00__SEQ_RST0B_MASK                              0x00000001
#define SEQ00__SEQ_RST0B                                   0x00000001
#define SEQ00__SEQ_RST1B_MASK                              0x00000002
#define SEQ00__SEQ_RST1B                                   0x00000002

// SEQ01
#define SEQ01__SEQ_DOT8_MASK                               0x00000001
#define SEQ01__SEQ_DOT8                                    0x00000001
#define SEQ01__SEQ_SHIFT2_MASK                             0x00000004
#define SEQ01__SEQ_SHIFT2                                  0x00000004
#define SEQ01__SEQ_PCLKBY2_MASK                            0x00000008
#define SEQ01__SEQ_PCLKBY2                                 0x00000008
#define SEQ01__SEQ_SHIFT4_MASK                             0x00000010
#define SEQ01__SEQ_SHIFT4                                  0x00000010
#define SEQ01__SEQ_MAXBW_MASK                              0x00000020
#define SEQ01__SEQ_MAXBW                                   0x00000020

// SEQ03
#define SEQ03__SEQ_FONT_B1_MASK                            0x00000001
#define SEQ03__SEQ_FONT_B1                                 0x00000001
#define SEQ03__SEQ_FONT_B2_MASK                            0x00000002
#define SEQ03__SEQ_FONT_B2                                 0x00000002
#define SEQ03__SEQ_FONT_A1_MASK                            0x00000004
#define SEQ03__SEQ_FONT_A1                                 0x00000004
#define SEQ03__SEQ_FONT_A2_MASK                            0x00000008
#define SEQ03__SEQ_FONT_A2                                 0x00000008
#define SEQ03__SEQ_FONT_B0_MASK                            0x00000010
#define SEQ03__SEQ_FONT_B0                                 0x00000010
#define SEQ03__SEQ_FONT_A0_MASK                            0x00000020
#define SEQ03__SEQ_FONT_A0                                 0x00000020

// CRT00
#define CRT00__H_TOTAL_MASK                                0x000000ff

// CRT01
#define CRT01__H_DISP_END_MASK                             0x000000ff

// CRT02
#define CRT02__H_BLANK_START_MASK                          0x000000ff

// CRT03
#define CRT03__H_BLANK_END_MASK                            0x0000001f
#define CRT03__H_DE_SKEW_MASK                              0x00000060
#define CRT03__CR10CR11_R_DIS_B_MASK                       0x00000080
#define CRT03__CR10CR11_R_DIS_B                            0x00000080

// CRT04
#define CRT04__H_SYNC_START_MASK                           0x000000ff

// CRT05
#define CRT05__H_SYNC_END_MASK                             0x0000001f
#define CRT05__H_SYNC_SKEW_MASK                            0x00000060
#define CRT05__H_BLANK_END_B5_MASK                         0x00000080
#define CRT05__H_BLANK_END_B5                              0x00000080

// CRT06
#define CRT06__V_TOTAL_MASK                                0x000000ff

// CRT07
#define CRT07__V_TOTAL_B8_MASK                             0x00000001
#define CRT07__V_TOTAL_B8                                  0x00000001
#define CRT07__V_DISP_END_B8_MASK                          0x00000002
#define CRT07__V_DISP_END_B8                               0x00000002
#define CRT07__V_SYNC_START_B8_MASK                        0x00000004
#define CRT07__V_SYNC_START_B8                             0x00000004
#define CRT07__V_BLANK_START_B8_MASK                       0x00000008
#define CRT07__V_BLANK_START_B8                            0x00000008
#define CRT07__LINE_CMP_B8_MASK                            0x00000010
#define CRT07__LINE_CMP_B8                                 0x00000010
#define CRT07__V_TOTAL_B9_MASK                             0x00000020
#define CRT07__V_TOTAL_B9                                  0x00000020
#define CRT07__V_DISP_END_B9_MASK                          0x00000040
#define CRT07__V_DISP_END_B9                               0x00000040
#define CRT07__V_SYNC_START_B9_MASK                        0x00000080
#define CRT07__V_SYNC_START_B9                             0x00000080

// CRT08
#define CRT08__ROW_SCAN_START_MASK                         0x0000001f
#define CRT08__BYTE_PAN_MASK                               0x00000060

// CRT09
#define CRT09__MAX_ROW_SCAN_MASK                           0x0000001f
#define CRT09__V_BLANK_START_B9_MASK                       0x00000020
#define CRT09__V_BLANK_START_B9                            0x00000020
#define CRT09__LINE_CMP_B9_MASK                            0x00000040
#define CRT09__LINE_CMP_B9                                 0x00000040
#define CRT09__DOUBLE_CHAR_HEIGHT_MASK                     0x00000080
#define CRT09__DOUBLE_CHAR_HEIGHT                          0x00000080

// CRT0A
#define CRT0A__CURSOR_START_MASK                           0x0000001f
#define CRT0A__CURSOR_DISABLE_MASK                         0x00000020
#define CRT0A__CURSOR_DISABLE                              0x00000020

// CRT0B
#define CRT0B__CURSOR_END_MASK                             0x0000001f
#define CRT0B__CURSOR_SKEW_MASK                            0x00000060

// CRT0C
#define CRT0C__DISP_START_MASK                             0x000000ff

// CRT0D
#define CRT0D__DISP_START_MASK                             0x000000ff

// CRT0E
#define CRT0E__CURSOR_LOC_HI_MASK                          0x000000ff

// CRT0F
#define CRT0F__CURSOR_LOC_LO_MASK                          0x000000ff

// CRT10
#define CRT10__V_SYNC_START_MASK                           0x000000ff

// CRT11
#define CRT11__V_SYNC_END_MASK                             0x0000000f
#define CRT11__V_INTR_CLR_MASK                             0x00000010
#define CRT11__V_INTR_CLR                                  0x00000010
#define CRT11__V_INTR_EN_MASK                              0x00000020
#define CRT11__V_INTR_EN                                   0x00000020
#define CRT11__SEL5_REFRESH_CYC_MASK                       0x00000040
#define CRT11__SEL5_REFRESH_CYC                            0x00000040
#define CRT11__C0T7_WR_ONLY_MASK                           0x00000080
#define CRT11__C0T7_WR_ONLY                                0x00000080

// CRT12
#define CRT12__V_DISP_END_MASK                             0x000000ff

// CRT13
#define CRT13__DISP_PITCH_MASK                             0x000000ff

// CRT15
#define CRT15__V_BLANK_START_MASK                          0x000000ff

// CRT16
#define CRT16__V_BLANK_END_MASK                            0x000000ff

// CRT18
#define CRT18__LINE_CMP_MASK                               0x000000ff

// CRT00_S
#define CRT00_S__H_TOTAL_S_MASK                            0x000000ff

// CRT01_S
#define CRT01_S__H_DISP_END_S_MASK                         0x000000ff

// CRT02_S
#define CRT02_S__H_BLANK_START_S_MASK                      0x000000ff

// CRT03_S
#define CRT03_S__H_BLANK_END_S_MASK                        0x0000001f
#define CRT03_S__H_DE_SKEW_S_MASK                          0x00000060
#define CRT03_S__CR10CR11_R_DIS_B_M_MASK                   0x00000080
#define CRT03_S__CR10CR11_R_DIS_B_M                        0x00000080

// CRT04_S
#define CRT04_S__H_SYNC_START_S_MASK                       0x000000ff

// CRT05_S
#define CRT05_S__H_SYNC_END_S_MASK                         0x0000001f
#define CRT05_S__H_SYNC_SKEW_S_MASK                        0x00000060
#define CRT05_S__H_BLANK_END_B5_S_MASK                     0x00000080
#define CRT05_S__H_BLANK_END_B5_S                          0x00000080

// CRT06_S
#define CRT06_S__V_TOTAL_S_MASK                            0x000000ff

// CRT07_S
#define CRT07_S__V_TOTAL_B8_S_MASK                         0x00000001
#define CRT07_S__V_TOTAL_B8_S                              0x00000001
#define CRT07_S__V_DISP_END_B8_S_MASK                      0x00000002
#define CRT07_S__V_DISP_END_B8_S                           0x00000002
#define CRT07_S__V_SYNC_START_B8_S_MASK                    0x00000004
#define CRT07_S__V_SYNC_START_B8_S                         0x00000004
#define CRT07_S__V_BLANK_START_B8_S_MASK                   0x00000008
#define CRT07_S__V_BLANK_START_B8_S                        0x00000008
#define CRT07_S__LINE_CMP_B8_M_MASK                        0x00000010
#define CRT07_S__LINE_CMP_B8_M                             0x00000010
#define CRT07_S__V_TOTAL_B9_S_MASK                         0x00000020
#define CRT07_S__V_TOTAL_B9_S                              0x00000020
#define CRT07_S__V_DISP_END_B9_S_MASK                      0x00000040
#define CRT07_S__V_DISP_END_B9_S                           0x00000040
#define CRT07_S__V_SYNC_START_B9_S_MASK                    0x00000080
#define CRT07_S__V_SYNC_START_B9_S                         0x00000080

// CRT08_S
#define CRT08_S__ROW_SCAN_START_M_MASK                     0x0000001f
#define CRT08_S__BYTE_PAN_M_MASK                           0x00000060

// CRT09_S
#define CRT09_S__MAX_ROW_SCAN_S_MASK                       0x0000001f
#define CRT09_S__V_BLANK_START_B9_S_MASK                   0x00000020
#define CRT09_S__V_BLANK_START_B9_S                        0x00000020
#define CRT09_S__LINE_CMP_B9_M_MASK                        0x00000040
#define CRT09_S__LINE_CMP_B9_M                             0x00000040
#define CRT09_S__DOUBLE_CHAR_HEIGHT_M_MASK                 0x00000080
#define CRT09_S__DOUBLE_CHAR_HEIGHT_M                      0x00000080

// CRT0A_S
#define CRT0A_S__CURSOR_START_S_MASK                       0x0000001f
#define CRT0A_S__CURSOR_DISABLE_M_MASK                     0x00000020
#define CRT0A_S__CURSOR_DISABLE_M                          0x00000020

// CRT0B_S
#define CRT0B_S__CURSOR_END_S_MASK                         0x0000001f
#define CRT0B_S__CURSOR_SKEW_M_MASK                        0x00000060

// CRT0C_S
#define CRT0C_S__DISP_START_M_MASK                         0x000000ff

// CRT0D_S
#define CRT0D_S__DISP_START_M_MASK                         0x000000ff

// CRT0E_S
#define CRT0E_S__CURSOR_LOC_HI_M_MASK                      0x000000ff

// CRT0F_S
#define CRT0F_S__CURSOR_LOC_LO_M_MASK                      0x000000ff

// CRT10_S
#define CRT10_S__V_SYNC_START_S_MASK                       0x000000ff

// CRT11_S
#define CRT11_S__V_SYNC_END_S_MASK                         0x0000000f
#define CRT11_S__V_INTR_CLR_M_MASK                         0x00000010
#define CRT11_S__V_INTR_CLR_M                              0x00000010
#define CRT11_S__V_INTR_EN_M_MASK                          0x00000020
#define CRT11_S__V_INTR_EN_M                               0x00000020
#define CRT11_S__SEL5_REFRESH_CYC_M_MASK                   0x00000040
#define CRT11_S__SEL5_REFRESH_CYC_M                        0x00000040
#define CRT11_S__C0T7_WR_ONLY_M_MASK                       0x00000080
#define CRT11_S__C0T7_WR_ONLY_M                            0x00000080

// CRT12_S
#define CRT12_S__V_DISP_END_S_MASK                         0x000000ff

// CRT13_S
#define CRT13_S__DISP_PITCH_M_MASK                         0x000000ff

// CRT15_S
#define CRT15_S__V_BLANK_START_S_MASK                      0x000000ff

// CRT16_S
#define CRT16_S__V_BLANK_END_S_MASK                        0x000000ff

// CRT18_S
#define CRT18_S__LINE_CMP_M_MASK                           0x000000ff

// ATTRX
#define ATTRX__ATTR_IDX_MASK                               0x0000001f
#define ATTRX__ATTR_PAL_RW_ENB_MASK                        0x00000020
#define ATTRX__ATTR_PAL_RW_ENB                             0x00000020

// ATTRDW
#define ATTRDW__ATTR_DATA_MASK                             0x000000ff

// ATTRDR
#define ATTRDR__ATTR_DATA_MASK                             0x000000ff

// ATTR00
#define ATTR00__ATTR_PAL_MASK                              0x0000003f

// ATTR01
#define ATTR01__ATTR_PAL_MASK                              0x0000003f

// ATTR02
#define ATTR02__ATTR_PAL_MASK                              0x0000003f

// ATTR03
#define ATTR03__ATTR_PAL_MASK                              0x0000003f

// ATTR04
#define ATTR04__ATTR_PAL_MASK                              0x0000003f

// ATTR05
#define ATTR05__ATTR_PAL_MASK                              0x0000003f

// ATTR06
#define ATTR06__ATTR_PAL_MASK                              0x0000003f

// ATTR07
#define ATTR07__ATTR_PAL_MASK                              0x0000003f

// ATTR08
#define ATTR08__ATTR_PAL_MASK                              0x0000003f

// ATTR09
#define ATTR09__ATTR_PAL_MASK                              0x0000003f

// ATTR0A
#define ATTR0A__ATTR_PAL_MASK                              0x0000003f

// ATTR0B
#define ATTR0B__ATTR_PAL_MASK                              0x0000003f

// ATTR0C
#define ATTR0C__ATTR_PAL_MASK                              0x0000003f

// ATTR0D
#define ATTR0D__ATTR_PAL_MASK                              0x0000003f

// ATTR0E
#define ATTR0E__ATTR_PAL_MASK                              0x0000003f

// ATTR0F
#define ATTR0F__ATTR_PAL_MASK                              0x0000003f

// ATTR10
#define ATTR10__ATTR_GRPH_MODE_MASK                        0x00000001
#define ATTR10__ATTR_GRPH_MODE                             0x00000001
#define ATTR10__ATTR_MONO_EN_MASK                          0x00000002
#define ATTR10__ATTR_MONO_EN                               0x00000002
#define ATTR10__ATTR_LGRPH_EN_MASK                         0x00000004
#define ATTR10__ATTR_LGRPH_EN                              0x00000004
#define ATTR10__ATTR_BLINK_EN_MASK                         0x00000008
#define ATTR10__ATTR_BLINK_EN                              0x00000008
#define ATTR10__ATTR_PANTOPONLY_MASK                       0x00000020
#define ATTR10__ATTR_PANTOPONLY                            0x00000020
#define ATTR10__ATTR_PCLKBY2_MASK                          0x00000040
#define ATTR10__ATTR_PCLKBY2                               0x00000040
#define ATTR10__ATTR_CSEL_EN_MASK                          0x00000080
#define ATTR10__ATTR_CSEL_EN                               0x00000080

// ATTR11
#define ATTR11__ATTR_OVSC_MASK                             0x000000ff

// ATTR12
#define ATTR12__ATTR_MAP_EN_MASK                           0x0000000f
#define ATTR12__ATTR_VSMUX_MASK                            0x00000030

// ATTR13
#define ATTR13__ATTR_PPAN_MASK                             0x0000000f

// ATTR14
#define ATTR14__ATTR_CSEL1_MASK                            0x00000003
#define ATTR14__ATTR_CSEL2_MASK                            0x0000000c

// DISP_TEST_MACRO_RW_WRITE
#define DISP_TEST_MACRO_RW_WRITE__TEST_MACRO_RW_WRITE1_MASK 0x00003fff
#define DISP_TEST_MACRO_RW_WRITE__TEST_MACRO_RW_WRITE2_MASK 0x0fffc000

// DISP_TEST_MACRO_RW_READ
#define DISP_TEST_MACRO_RW_READ__TEST_MACRO_RW_READ1_MASK  0x0000ffff
#define DISP_TEST_MACRO_RW_READ__TEST_MACRO_RW_READ2_MASK  0xffff0000

// DISP_TEST_MACRO_RW_DATA
#define DISP_TEST_MACRO_RW_DATA__TEST_MACRO_RW_DATA_MASK   0xffffffff

// DISP_TEST_MACRO_RW_CNTL
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_START_MASK  0x00000001
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_START       0x00000001
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_OP_MASK     0x0000000e
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MODE_MASK   0x00000030
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MISMATCH_SEL_MASK 0x00007fc0
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MISMATCH_MASK 0x00008000
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MISMATCH    0x00008000
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_ENABLE_MASK 0x00010000
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_ENABLE      0x00010000
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_SCLK_NEG_ENABLE_MASK 0x00020000
#define DISP_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_SCLK_NEG_ENABLE 0x00020000

// CRTC_STATUS
#define CRTC_STATUS__CRTC_VBLANK_CUR_MASK                  0x00000001
#define CRTC_STATUS__CRTC_VBLANK_CUR                       0x00000001
#define CRTC_STATUS__CRTC_VBLANK_SAVE_MASK                 0x00000002
#define CRTC_STATUS__CRTC_VBLANK_SAVE                      0x00000002
#define CRTC_STATUS__CRTC_VBLANK_SAVE_CLEAR_MASK           0x00000002
#define CRTC_STATUS__CRTC_VBLANK_SAVE_CLEAR                0x00000002
#define CRTC_STATUS__CRTC_VLINE_SYNC_MASK                  0x00000004
#define CRTC_STATUS__CRTC_VLINE_SYNC                       0x00000004
#define CRTC_STATUS__CRTC_FRAME_MASK                       0x00000008
#define CRTC_STATUS__CRTC_FRAME                            0x00000008

// GPIO_VGA_DDC
#define GPIO_VGA_DDC__VGA_DCC_DATA_OUTPUT_MASK             0x00000001
#define GPIO_VGA_DDC__VGA_DCC_DATA_OUTPUT                  0x00000001
#define GPIO_VGA_DDC__VGA_DDC_CLK_OUTPUT_MASK              0x00000002
#define GPIO_VGA_DDC__VGA_DDC_CLK_OUTPUT                   0x00000002
#define GPIO_VGA_DDC__VGA_DDC_DATA_INPUT_MASK              0x00000100
#define GPIO_VGA_DDC__VGA_DDC_DATA_INPUT                   0x00000100
#define GPIO_VGA_DDC__VGA_DDC_CLK_INPUT_MASK               0x00000200
#define GPIO_VGA_DDC__VGA_DDC_CLK_INPUT                    0x00000200
#define GPIO_VGA_DDC__VGA_DDC_DATA_OUT_EN_MASK             0x00010000
#define GPIO_VGA_DDC__VGA_DDC_DATA_OUT_EN                  0x00010000
#define GPIO_VGA_DDC__VGA_DDC_CLK_OUT_EN_MASK              0x00020000
#define GPIO_VGA_DDC__VGA_DDC_CLK_OUT_EN                   0x00020000

// GPIO_DVI_DDC
#define GPIO_DVI_DDC__DVI_DCC_DATA_OUTPUT_MASK             0x00000001
#define GPIO_DVI_DDC__DVI_DCC_DATA_OUTPUT                  0x00000001
#define GPIO_DVI_DDC__DVI_DDC_CLK_OUTPUT_MASK              0x00000002
#define GPIO_DVI_DDC__DVI_DDC_CLK_OUTPUT                   0x00000002
#define GPIO_DVI_DDC__DVI_DDC_DATA_INPUT_MASK              0x00000100
#define GPIO_DVI_DDC__DVI_DDC_DATA_INPUT                   0x00000100
#define GPIO_DVI_DDC__DVI_DDC_CLK_INPUT_MASK               0x00000200
#define GPIO_DVI_DDC__DVI_DDC_CLK_INPUT                    0x00000200
#define GPIO_DVI_DDC__DVI_DDC_DATA_OUT_EN_MASK             0x00010000
#define GPIO_DVI_DDC__DVI_DDC_DATA_OUT_EN                  0x00010000
#define GPIO_DVI_DDC__DVI_DDC_CLK_OUT_EN_MASK              0x00020000
#define GPIO_DVI_DDC__DVI_DDC_CLK_OUT_EN                   0x00020000

// GPIO_MONID
#define GPIO_MONID__GPIO_MONID_0_OUTPUT_MASK               0x00000001
#define GPIO_MONID__GPIO_MONID_0_OUTPUT                    0x00000001
#define GPIO_MONID__GPIO_MONID_1_OUTPUT_MASK               0x00000002
#define GPIO_MONID__GPIO_MONID_1_OUTPUT                    0x00000002
#define GPIO_MONID__GPIO_MONID_0_INPUT_MASK                0x00000100
#define GPIO_MONID__GPIO_MONID_0_INPUT                     0x00000100
#define GPIO_MONID__GPIO_MONID_1_INPUT_MASK                0x00000200
#define GPIO_MONID__GPIO_MONID_1_INPUT                     0x00000200
#define GPIO_MONID__GPIO_MONID_0_OUT_EN_MASK               0x00010000
#define GPIO_MONID__GPIO_MONID_0_OUT_EN                    0x00010000
#define GPIO_MONID__GPIO_MONID_1_OUT_EN_MASK               0x00020000
#define GPIO_MONID__GPIO_MONID_1_OUT_EN                    0x00020000

// PALETTE_INDEX
#define PALETTE_INDEX__PALETTE_W_INDEX_MASK                0x000000ff
#define PALETTE_INDEX__PALETTE_R_INDEX_MASK                0x00ff0000

// PALETTE_DATA
#define PALETTE_DATA__PALETTE_DATA_B_MASK                  0x000000ff
#define PALETTE_DATA__PALETTE_DATA_G_MASK                  0x0000ff00
#define PALETTE_DATA__PALETTE_DATA_R_MASK                  0x00ff0000

// PALETTE_30_DATA
#define PALETTE_30_DATA__PALETTE_DATA_B_MASK               0x000003ff
#define PALETTE_30_DATA__PALETTE_DATA_G_MASK               0x000ffc00
#define PALETTE_30_DATA__PALETTE_DATA_R_MASK               0x3ff00000

// CRTC_H_TOTAL_DISP
#define CRTC_H_TOTAL_DISP__CRTC_H_TOTAL_MASK               0x000003ff
#define CRTC_H_TOTAL_DISP__CRTC_H_DISP_MASK                0x01ff0000

// CRTC_H_SYNC_STRT_WID
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_STRT_PIX_MASK    0x00000007
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_STRT_CHAR_MASK   0x00001ff8
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_WID_MASK         0x003f0000
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_POL_MASK         0x00800000
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_POL              0x00800000
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_SKEW_TUNE_MASK   0x07000000
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_SKEW_TUNE_MODE_MASK 0x10000000
#define CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_SKEW_TUNE_MODE   0x10000000

// CRTC_V_TOTAL_DISP
#define CRTC_V_TOTAL_DISP__CRTC_V_TOTAL_MASK               0x00000fff
#define CRTC_V_TOTAL_DISP__CRTC_V_DISP_MASK                0x0fff0000

// CRTC_V_SYNC_STRT_WID
#define CRTC_V_SYNC_STRT_WID__CRTC_V_SYNC_STRT_MASK        0x00000fff
#define CRTC_V_SYNC_STRT_WID__CRTC_V_SYNC_WID_MASK         0x001f0000
#define CRTC_V_SYNC_STRT_WID__CRTC_V_SYNC_POL_MASK         0x00800000
#define CRTC_V_SYNC_STRT_WID__CRTC_V_SYNC_POL              0x00800000

// CRTC_VLINE_CRNT_VLINE
#define CRTC_VLINE_CRNT_VLINE__CRTC_VLINE_MASK             0x00000fff
#define CRTC_VLINE_CRNT_VLINE__CRTC_CRNT_VLINE_MASK        0x0fff0000

// CRTC_CRNT_FRAME
#define CRTC_CRNT_FRAME__CRTC_CRNT_FRAME_MASK              0x001fffff

// CRTC_GUI_TRIG_VLINE
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE_START_MASK 0x00000fff
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE_INV_MASK  0x00008000
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE_INV       0x00008000
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE_END_MASK  0x0fff0000
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE_STALL_MASK 0x40000000
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE_STALL     0x40000000
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE_MASK      0x80000000
#define CRTC_GUI_TRIG_VLINE__CRTC_GUI_TRIG_VLINE           0x80000000

// CRTC_DEBUG
#define CRTC_DEBUG__CRTC_GUI_TRIG_BYPASS_EN_MASK           0x00000001
#define CRTC_DEBUG__CRTC_GUI_TRIG_BYPASS_EN                0x00000001
#define CRTC_DEBUG__GUI_TRIG_VLINE_BYPASS_MASK             0x00000002
#define CRTC_DEBUG__GUI_TRIG_VLINE_BYPASS                  0x00000002
#define CRTC_DEBUG__GUI_TRIG_OFFSET_BYPASS_MASK            0x00000004
#define CRTC_DEBUG__GUI_TRIG_OFFSET_BYPASS                 0x00000004
#define CRTC_DEBUG__GUI_TRIG_PITCH_ADD_BYPASS_MASK         0x00000008
#define CRTC_DEBUG__GUI_TRIG_PITCH_ADD_BYPASS              0x00000008

// CRTC_OFFSET_RIGHT
#define CRTC_OFFSET_RIGHT__CRTC_OFFSET_RIGHT_MASK          0x07ffffff
#define CRTC_OFFSET_RIGHT__CRTC_GUI_TRIG_OFFSET_MASK       0x40000000
#define CRTC_OFFSET_RIGHT__CRTC_GUI_TRIG_OFFSET            0x40000000
#define CRTC_OFFSET_RIGHT__CRTC_OFFSET_LOCK_MASK           0x80000000
#define CRTC_OFFSET_RIGHT__CRTC_OFFSET_LOCK                0x80000000

// CRTC_OFFSET
#define CRTC_OFFSET__CRTC_OFFSET_MASK                      0x07ffffff
#define CRTC_OFFSET__CRTC_GUI_TRIG_OFFSET_MASK             0x40000000
#define CRTC_OFFSET__CRTC_GUI_TRIG_OFFSET                  0x40000000
#define CRTC_OFFSET__CRTC_OFFSET_LOCK_MASK                 0x80000000
#define CRTC_OFFSET__CRTC_OFFSET_LOCK                      0x80000000

// CRTC_OFFSET_CNTL
#define CRTC_OFFSET_CNTL__CRTC_TILE_LINE_MASK              0x0000000f
#define CRTC_OFFSET_CNTL__CRTC_TILE_LINE_RIGHT_MASK        0x000000f0
#define CRTC_OFFSET_CNTL__CRTC_TILE_EN_RIGHT_MASK          0x00004000
#define CRTC_OFFSET_CNTL__CRTC_TILE_EN_RIGHT               0x00004000
#define CRTC_OFFSET_CNTL__CRTC_TILE_EN_MASK                0x00008000
#define CRTC_OFFSET_CNTL__CRTC_TILE_EN                     0x00008000
#define CRTC_OFFSET_CNTL__CRTC_OFFSET_FLIP_CNTL_MASK       0x00010000
#define CRTC_OFFSET_CNTL__CRTC_OFFSET_FLIP_CNTL            0x00010000
#define CRTC_OFFSET_CNTL__CRTC_STEREO_OFFSET_EN_MASK       0x00020000
#define CRTC_OFFSET_CNTL__CRTC_STEREO_OFFSET_EN            0x00020000
#define CRTC_OFFSET_CNTL__CRTC_STEREO_SYNC_EN_MASK         0x000c0000
#define CRTC_OFFSET_CNTL__CRTC_STEREO_SYNC_OUT_EN_MASK     0x00100000
#define CRTC_OFFSET_CNTL__CRTC_STEREO_SYNC_OUT_EN          0x00100000
#define CRTC_OFFSET_CNTL__CRTC_STEREO_SYNC_MASK            0x00200000
#define CRTC_OFFSET_CNTL__CRTC_STEREO_SYNC                 0x00200000
#define CRTC_OFFSET_CNTL__CRTC_GUI_TRIG_OFFSET_LEFT_EN_MASK 0x10000000
#define CRTC_OFFSET_CNTL__CRTC_GUI_TRIG_OFFSET_LEFT_EN     0x10000000
#define CRTC_OFFSET_CNTL__CRTC_GUI_TRIG_OFFSET_RIGHT_EN_MASK 0x20000000
#define CRTC_OFFSET_CNTL__CRTC_GUI_TRIG_OFFSET_RIGHT_EN    0x20000000
#define CRTC_OFFSET_CNTL__CRTC_GUI_TRIG_OFFSET_MASK        0x40000000
#define CRTC_OFFSET_CNTL__CRTC_GUI_TRIG_OFFSET             0x40000000
#define CRTC_OFFSET_CNTL__CRTC_OFFSET_LOCK_MASK            0x80000000
#define CRTC_OFFSET_CNTL__CRTC_OFFSET_LOCK                 0x80000000

// CRTC_PITCH
#define CRTC_PITCH__CRTC_PITCH_MASK                        0x000007ff
#define CRTC_PITCH__CRTC_PITCH_RIGHT_MASK                  0x07ff0000

// OVR_CLR
#define OVR_CLR__OVR_CLR_B_MASK                            0x000000ff
#define OVR_CLR__OVR_CLR_G_MASK                            0x0000ff00
#define OVR_CLR__OVR_CLR_R_MASK                            0x00ff0000

// OVR_WID_LEFT_RIGHT
#define OVR_WID_LEFT_RIGHT__OVR_WID_RIGHT_MASK             0x0000007f
#define OVR_WID_LEFT_RIGHT__OVR_WID_LEFT_MASK              0x007f0000

// OVR_WID_TOP_BOTTOM
#define OVR_WID_TOP_BOTTOM__OVR_WID_BOTTOM_MASK            0x000003ff
#define OVR_WID_TOP_BOTTOM__OVR_WID_TOP_MASK               0x03ff0000

// DISPLAY_BASE_ADDR
#define DISPLAY_BASE_ADDR__DISPLAY_BASE_ADDR_MASK          0xffffffff

// SNAPSHOT_VH_COUNTS
#define SNAPSHOT_VH_COUNTS__SNAPSHOT_HCOUNT_MASK           0x000003ff
#define SNAPSHOT_VH_COUNTS__SNAPSHOT_VCOUNT_MASK           0x0fff0000

// SNAPSHOT_F_COUNT
#define SNAPSHOT_F_COUNT__SNAPSHOT_F_COUNT_MASK            0x001fffff

// N_VIF_COUNT
#define N_VIF_COUNT__N_VIF_COUNT_VAL_MASK                  0x000003ff
#define N_VIF_COUNT__GENLOCK_SOURCE_SEL_MASK               0x80000000
#define N_VIF_COUNT__GENLOCK_SOURCE_SEL                    0x80000000

// SNAPSHOT_VIF_COUNT
#define SNAPSHOT_VIF_COUNT__LSNAPSHOT_VIF_COUNT_MASK       0x000003ff
#define SNAPSHOT_VIF_COUNT__USNAPSHOT_VIF_COUNT_MASK       0x001ffc00
#define SNAPSHOT_VIF_COUNT__AUTO_SNAPSHOT_TAKEN_RD_MASK    0x01000000
#define SNAPSHOT_VIF_COUNT__AUTO_SNAPSHOT_TAKEN_RD         0x01000000
#define SNAPSHOT_VIF_COUNT__AUTO_SNAPSHOT_TAKEN_WR_MASK    0x01000000
#define SNAPSHOT_VIF_COUNT__AUTO_SNAPSHOT_TAKEN_WR         0x01000000
#define SNAPSHOT_VIF_COUNT__MANUAL_SNAPSHOT_NOW_MASK       0x02000000
#define SNAPSHOT_VIF_COUNT__MANUAL_SNAPSHOT_NOW            0x02000000

// FP_CRTC_H_TOTAL_DISP
#define FP_CRTC_H_TOTAL_DISP__FP_CRTC_H_TOTAL_MASK         0x000003ff
#define FP_CRTC_H_TOTAL_DISP__FP_CRTC_H_DISP_MASK          0x01ff0000

// FP_CRTC_V_TOTAL_DISP
#define FP_CRTC_V_TOTAL_DISP__FP_CRTC_V_TOTAL_MASK         0x00000fff
#define FP_CRTC_V_TOTAL_DISP__FP_CRTC_V_DISP_MASK          0x0fff0000

// CRT_CRTC_H_SYNC_STRT_WID
#define CRT_CRTC_H_SYNC_STRT_WID__CRT_CRTC_H_SYNC_STRT_CHAR_MASK 0x00001ff8
#define CRT_CRTC_H_SYNC_STRT_WID__CRT_CRTC_H_SYNC_WID_MASK 0x003f0000

// CRT_CRTC_V_SYNC_STRT_WID
#define CRT_CRTC_V_SYNC_STRT_WID__CRT_CRTC_V_SYNC_STRT_MASK 0x00000fff
#define CRT_CRTC_V_SYNC_STRT_WID__CRT_CRTC_V_SYNC_WID_MASK 0x001f0000

// CUR_OFFSET
#define CUR_OFFSET__CUR_OFFSET_MASK                        0x07ffffff
#define CUR_OFFSET__CUR_LOCK_MASK                          0x80000000
#define CUR_OFFSET__CUR_LOCK                               0x80000000

// CUR_HORZ_VERT_POSN
#define CUR_HORZ_VERT_POSN__CUR_VERT_POSN_MASK             0x00000fff
#define CUR_HORZ_VERT_POSN__CUR_HORZ_POSN_MASK             0x3fff0000
#define CUR_HORZ_VERT_POSN__CUR_LOCK_MASK                  0x80000000
#define CUR_HORZ_VERT_POSN__CUR_LOCK                       0x80000000

// CUR_HORZ_VERT_OFF
#define CUR_HORZ_VERT_OFF__CUR_VERT_OFF_MASK               0x0000003f
#define CUR_HORZ_VERT_OFF__CUR_HORZ_OFF_MASK               0x003f0000
#define CUR_HORZ_VERT_OFF__CUR_LOCK_MASK                   0x80000000
#define CUR_HORZ_VERT_OFF__CUR_LOCK                        0x80000000

// CUR_CLR0
#define CUR_CLR0__CUR_CLR0_B_MASK                          0x000000ff
#define CUR_CLR0__CUR_CLR0_G_MASK                          0x0000ff00
#define CUR_CLR0__CUR_CLR0_R_MASK                          0x00ff0000

// CUR_CLR1
#define CUR_CLR1__CUR_CLR1_B_MASK                          0x000000ff
#define CUR_CLR1__CUR_CLR1_G_MASK                          0x0000ff00
#define CUR_CLR1__CUR_CLR1_R_MASK                          0x00ff0000

// FP_HORZ_VERT_ACTIVE
#define FP_HORZ_VERT_ACTIVE__FP_VERT_ACTIVE_SIZE_MASK      0x00000fff
#define FP_HORZ_VERT_ACTIVE__FP_HORZ_ACTIVE_SIZE_MASK      0x01ff0000

// CRTC_MORE_CNTL
#define CRTC_MORE_CNTL__CRTC_HORZ_BLANK_MODE_SEL_MASK      0x00000001
#define CRTC_MORE_CNTL__CRTC_HORZ_BLANK_MODE_SEL           0x00000001
#define CRTC_MORE_CNTL__CRTC_VERT_BLANK_MODE_SEL_MASK      0x00000002
#define CRTC_MORE_CNTL__CRTC_VERT_BLANK_MODE_SEL           0x00000002
#define CRTC_MORE_CNTL__CRTC_AUTO_HORZ_CENTER_EN_MASK      0x00000004
#define CRTC_MORE_CNTL__CRTC_AUTO_HORZ_CENTER_EN           0x00000004
#define CRTC_MORE_CNTL__CRTC_AUTO_VERT_CENTER_EN_MASK      0x00000008
#define CRTC_MORE_CNTL__CRTC_AUTO_VERT_CENTER_EN           0x00000008
#define CRTC_MORE_CNTL__CRTC_H_CUTOFF_ACTIVE_EN_MASK       0x00000010
#define CRTC_MORE_CNTL__CRTC_H_CUTOFF_ACTIVE_EN            0x00000010
#define CRTC_MORE_CNTL__CRTC_V_CUTOFF_ACTIVE_EN_MASK       0x00000020
#define CRTC_MORE_CNTL__CRTC_V_CUTOFF_ACTIVE_EN            0x00000020

// DAC_EXT_CNTL
#define DAC_EXT_CNTL__DAC_FORCE_BLANK_OFF_EN_MASK          0x00000010
#define DAC_EXT_CNTL__DAC_FORCE_BLANK_OFF_EN               0x00000010
#define DAC_EXT_CNTL__DAC_FORCE_DATA_EN_MASK               0x00000020
#define DAC_EXT_CNTL__DAC_FORCE_DATA_EN                    0x00000020
#define DAC_EXT_CNTL__DAC_FORCE_DATA_SEL_MASK              0x000000c0
#define DAC_EXT_CNTL__DAC_FORCE_DATA_MASK                  0x0003ff00

// FP_GEN_CNTL
#define FP_GEN_CNTL__FP_BLANK_EN_MASK                      0x00000002
#define FP_GEN_CNTL__FP_BLANK_EN                           0x00000002
#define FP_GEN_CNTL__TMDS_EN_MASK                          0x00000004
#define FP_GEN_CNTL__TMDS_EN                               0x00000004
#define FP_GEN_CNTL__PANEL_FORMAT_MASK                     0x00000008
#define FP_GEN_CNTL__PANEL_FORMAT                          0x00000008
#define FP_GEN_CNTL__NO_OF_GREY_MASK                       0x00000030
#define FP_GEN_CNTL__FP_RST_FM_MASK                        0x00000040
#define FP_GEN_CNTL__FP_RST_FM                             0x00000040
#define FP_GEN_CNTL__FP_EN_TMDS_MASK                       0x00000080
#define FP_GEN_CNTL__FP_EN_TMDS                            0x00000080
#define FP_GEN_CNTL__FP_DETECT_SENSE_MASK                  0x00000100
#define FP_GEN_CNTL__FP_DETECT_SENSE                       0x00000100
#define FP_GEN_CNTL__FP_DETECT_INT_POL_MASK                0x00000200
#define FP_GEN_CNTL__FP_DETECT_INT_POL                     0x00000200
#define FP_GEN_CNTL__FP_DETECT_EN_MASK                     0x00001000
#define FP_GEN_CNTL__FP_DETECT_EN                          0x00001000
#define FP_GEN_CNTL__FP_USE_VGA_HVSYNC_MASK                0x00004000
#define FP_GEN_CNTL__FP_USE_VGA_HVSYNC                     0x00004000
#define FP_GEN_CNTL__FP_USE_VGA_SYNC_POLARITY_MASK         0x00008000
#define FP_GEN_CNTL__FP_USE_VGA_SYNC_POLARITY              0x00008000
#define FP_GEN_CNTL__CRTC_DONT_SHADOW_VPAR_MASK            0x00010000
#define FP_GEN_CNTL__CRTC_DONT_SHADOW_VPAR                 0x00010000
#define FP_GEN_CNTL__CRTC_DONT_SHADOW_HEND_MASK            0x00020000
#define FP_GEN_CNTL__CRTC_DONT_SHADOW_HEND                 0x00020000
#define FP_GEN_CNTL__CRTC_USE_SHADOWED_VEND_MASK           0x00040000
#define FP_GEN_CNTL__CRTC_USE_SHADOWED_VEND                0x00040000
#define FP_GEN_CNTL__CRTC_USE_SHADOWED_ROWCUR_MASK         0x00080000
#define FP_GEN_CNTL__CRTC_USE_SHADOWED_ROWCUR              0x00080000
#define FP_GEN_CNTL__RMX_HVSYNC_CONTROL_EN_MASK            0x00100000
#define FP_GEN_CNTL__RMX_HVSYNC_CONTROL_EN                 0x00100000
#define FP_GEN_CNTL__DFP_SYNC_SEL_MASK                     0x00200000
#define FP_GEN_CNTL__DFP_SYNC_SEL                          0x00200000
#define FP_GEN_CNTL__CRTC_LOCK_8DOT_MASK                   0x00400000
#define FP_GEN_CNTL__CRTC_LOCK_8DOT                        0x00400000
#define FP_GEN_CNTL__CRT_SYNC_SEL_MASK                     0x00800000
#define FP_GEN_CNTL__CRT_SYNC_SEL                          0x00800000
#define FP_GEN_CNTL__FP_USE_SHADOW_EN_MASK                 0x01000000
#define FP_GEN_CNTL__FP_USE_SHADOW_EN                      0x01000000
#define FP_GEN_CNTL__DONT_RST_CHAREN_MASK                  0x02000000
#define FP_GEN_CNTL__DONT_RST_CHAREN                       0x02000000
#define FP_GEN_CNTL__CRT_SYNC_ALT_SEL_MASK                 0x04000000
#define FP_GEN_CNTL__CRT_SYNC_ALT_SEL                      0x04000000
#define FP_GEN_CNTL__CRTC_USE_NONSHADOW_HPARAMS_FOR_BLANK_MASK 0x08000000
#define FP_GEN_CNTL__CRTC_USE_NONSHADOW_HPARAMS_FOR_BLANK  0x08000000
#define FP_GEN_CNTL__CRTC_USE_NONSHADOW_VPARAMS_FOR_BLANK_MASK 0x10000000
#define FP_GEN_CNTL__CRTC_USE_NONSHADOW_VPARAMS_FOR_BLANK  0x10000000
#define FP_GEN_CNTL__CRTC_VGA_XOVERSCAN_COLOR_MASK         0x20000000
#define FP_GEN_CNTL__CRTC_VGA_XOVERSCAN_COLOR              0x20000000
#define FP_GEN_CNTL__CRTC_VGA_XOVERSCAN_DIVBY2_EN_MASK     0x40000000
#define FP_GEN_CNTL__CRTC_VGA_XOVERSCAN_DIVBY2_EN          0x40000000

// FP_HORZ_STRETCH
#define FP_HORZ_STRETCH__FP_HORZ_STRETCH_RATIO_MASK        0x0000ffff
#define FP_HORZ_STRETCH__FP_HORZ_PANEL_SIZE_MASK           0x01ff0000
#define FP_HORZ_STRETCH__FP_HORZ_STRETCH_EN_MASK           0x02000000
#define FP_HORZ_STRETCH__FP_HORZ_STRETCH_EN                0x02000000
#define FP_HORZ_STRETCH__FP_HORZ_STRETCH_MODE_MASK         0x04000000
#define FP_HORZ_STRETCH__FP_HORZ_STRETCH_MODE              0x04000000
#define FP_HORZ_STRETCH__FP_AUTO_HORZ_RATIO_MASK           0x08000000
#define FP_HORZ_STRETCH__FP_AUTO_HORZ_RATIO                0x08000000
#define FP_HORZ_STRETCH__FP_LOOP_STRETCH_MASK              0x70000000
#define FP_HORZ_STRETCH__RMX_AUTO_RATIO_HORZ_INC_MASK      0x80000000
#define FP_HORZ_STRETCH__RMX_AUTO_RATIO_HORZ_INC           0x80000000

// FP_VERT_STRETCH
#define FP_VERT_STRETCH__FP_VERT_STRETCH_RATIO_MASK        0x00000fff
#define FP_VERT_STRETCH__FP_VERT_PANEL_SIZE_MASK           0x00fff000
#define FP_VERT_STRETCH__FP_VERT_STRETCH_EN_MASK           0x02000000
#define FP_VERT_STRETCH__FP_VERT_STRETCH_EN                0x02000000
#define FP_VERT_STRETCH__FP_VERT_STRETCH_MODE_MASK         0x04000000
#define FP_VERT_STRETCH__FP_VERT_STRETCH_MODE              0x04000000
#define FP_VERT_STRETCH__FP_AUTO_VERT_RATIO_MASK           0x08000000
#define FP_VERT_STRETCH__FP_AUTO_VERT_RATIO                0x08000000
#define FP_VERT_STRETCH__RMX_AUTO_RATIO_VERT_INC_MASK      0x80000000
#define FP_VERT_STRETCH__RMX_AUTO_RATIO_VERT_INC           0x80000000

// FP_H_SYNC_STRT_WID
#define FP_H_SYNC_STRT_WID__FP_H_SYNC_STRT_PIX_MASK        0x00000007
#define FP_H_SYNC_STRT_WID__FP_H_SYNC_STRT_CHAR_MASK       0x00001ff8
#define FP_H_SYNC_STRT_WID__FP_H_SYNC_WID_MASK             0x003f0000
#define FP_H_SYNC_STRT_WID__FP_H_SYNC_POL_MASK             0x00800000
#define FP_H_SYNC_STRT_WID__FP_H_SYNC_POL                  0x00800000

// FP_V_SYNC_STRT_WID
#define FP_V_SYNC_STRT_WID__FP_V_SYNC_STRT_MASK            0x00000fff
#define FP_V_SYNC_STRT_WID__FP_V_SYNC_WID_MASK             0x001f0000
#define FP_V_SYNC_STRT_WID__FP_V_SYNC_POL_MASK             0x00800000
#define FP_V_SYNC_STRT_WID__FP_V_SYNC_POL                  0x00800000

// AUX_WINDOW_HORZ_CNTL
#define AUX_WINDOW_HORZ_CNTL__AUX_WIN_HORZ_START_MASK      0x00000fff
#define AUX_WINDOW_HORZ_CNTL__AUX_WIN_HORZ_END_MASK        0x00fff000
#define AUX_WINDOW_HORZ_CNTL__AUX_WIN_EN_MASK              0x01000000
#define AUX_WINDOW_HORZ_CNTL__AUX_WIN_EN                   0x01000000
#define AUX_WINDOW_HORZ_CNTL__AUX_WIN_OUT_DELAY_MASK       0x1e000000
#define AUX_WINDOW_HORZ_CNTL__RESERVED_MASK                0x40000000
#define AUX_WINDOW_HORZ_CNTL__RESERVED                     0x40000000
#define AUX_WINDOW_HORZ_CNTL__AUX_WIN_LOCK_MASK            0x80000000
#define AUX_WINDOW_HORZ_CNTL__AUX_WIN_LOCK                 0x80000000

// AUX_WINDOW_VERT_CNTL
#define AUX_WINDOW_VERT_CNTL__AUX_WIN_VERT_START_MASK      0x00000fff
#define AUX_WINDOW_VERT_CNTL__AUX_WIN_VERT_END_MASK        0x0fff0000
#define AUX_WINDOW_VERT_CNTL__AUX_WINDOW_POL_MASK          0x40000000
#define AUX_WINDOW_VERT_CNTL__AUX_WINDOW_POL               0x40000000
#define AUX_WINDOW_VERT_CNTL__AUX_WIN_LOCK_MASK            0x80000000
#define AUX_WINDOW_VERT_CNTL__AUX_WIN_LOCK                 0x80000000

// GRPH_BUFFER_CNTL
#define GRPH_BUFFER_CNTL__GRPH_START_REQ_MASK              0x0000007f
#define GRPH_BUFFER_CNTL__GRPH_STOP_REQ_MASK               0x00007f00
#define GRPH_BUFFER_CNTL__GRPH_CRITICAL_POINT_MASK         0x007f0000
#define GRPH_BUFFER_CNTL__GRPH_CRITICAL_CNTL_MASK          0x10000000
#define GRPH_BUFFER_CNTL__GRPH_CRITICAL_CNTL               0x10000000
#define GRPH_BUFFER_CNTL__GRPH_BUFFER_SIZE_MASK            0x20000000
#define GRPH_BUFFER_CNTL__GRPH_BUFFER_SIZE                 0x20000000
#define GRPH_BUFFER_CNTL__GRPH_CRITICAL_AT_SOF_MASK        0x40000000
#define GRPH_BUFFER_CNTL__GRPH_CRITICAL_AT_SOF             0x40000000
#define GRPH_BUFFER_CNTL__GRPH_STOP_CNTL_MASK              0x80000000
#define GRPH_BUFFER_CNTL__GRPH_STOP_CNTL                   0x80000000

// VGA_BUFFER_CNTL
#define VGA_BUFFER_CNTL__VGA_START_REQ_MASK                0x0000003f
#define VGA_BUFFER_CNTL__VGA_STOP_REQ_MASK                 0x00003f00
#define VGA_BUFFER_CNTL__VGA_CRITICAL_POINT_MASK           0x003f0000

// OV0_Y_X_START
#define OV0_Y_X_START__OV0_X_START_MASK                    0x00001fff
#define OV0_Y_X_START__OV0_Y_START_MASK                    0x1fff0000

// OV0_Y_X_END
#define OV0_Y_X_END__OV0_X_END_MASK                        0x00001fff
#define OV0_Y_X_END__OV0_Y_END_MASK                        0x1fff0000

// OV0_PIPELINE_CNTL
#define OV0_PIPELINE_CNTL__OV0_DISP_PIPE_DELAY_MASK        0x0000000f

// OV0_REG_LOAD_CNTL
#define OV0_REG_LOAD_CNTL__OV0_LOCK_MASK                   0x00000001
#define OV0_REG_LOAD_CNTL__OV0_LOCK                        0x00000001
#define OV0_REG_LOAD_CNTL__OV0_VBLANK_DURING_LOCK_MASK     0x00000002
#define OV0_REG_LOAD_CNTL__OV0_VBLANK_DURING_LOCK          0x00000002
#define OV0_REG_LOAD_CNTL__OV0_STALL_GUI_UNTIL_FLIP_MASK   0x00000004
#define OV0_REG_LOAD_CNTL__OV0_STALL_GUI_UNTIL_FLIP        0x00000004
#define OV0_REG_LOAD_CNTL__OV0_LOCK_READBACK_MASK          0x00000008
#define OV0_REG_LOAD_CNTL__OV0_LOCK_READBACK               0x00000008
#define OV0_REG_LOAD_CNTL__OV0_FLIP_READBACK_MASK          0x00000010
#define OV0_REG_LOAD_CNTL__OV0_FLIP_READBACK               0x00000010

// OV0_SCALE_CNTL
#define OV0_SCALE_CNTL__OV0_HORZ_PICK_NEAREST_MASK         0x00000004
#define OV0_SCALE_CNTL__OV0_HORZ_PICK_NEAREST              0x00000004
#define OV0_SCALE_CNTL__OV0_VERT_PICK_NEAREST_MASK         0x00000008
#define OV0_SCALE_CNTL__OV0_VERT_PICK_NEAREST              0x00000008
#define OV0_SCALE_CNTL__OV0_SIGNED_UV_MASK                 0x00000010
#define OV0_SCALE_CNTL__OV0_SIGNED_UV                      0x00000010
#define OV0_SCALE_CNTL__OV0_GAMMA_SEL_MASK                 0x000000e0
#define OV0_SCALE_CNTL__OV0_SURFACE_FORMAT_MASK            0x00000f00
#define OV0_SCALE_CNTL__OV0_ADAPTIVE_DEINT_MASK            0x00001000
#define OV0_SCALE_CNTL__OV0_ADAPTIVE_DEINT                 0x00001000
#define OV0_SCALE_CNTL__OV0_BURST_PER_PLANE_MASK           0x007f0000
#define OV0_SCALE_CNTL__OV0_DOUBLE_BUFFER_REGS_MASK        0x01000000
#define OV0_SCALE_CNTL__OV0_DOUBLE_BUFFER_REGS             0x01000000
#define OV0_SCALE_CNTL__OV0_BANDWIDTH_MASK                 0x04000000
#define OV0_SCALE_CNTL__OV0_BANDWIDTH                      0x04000000
#define OV0_SCALE_CNTL__OV0_INT_EMU_MASK                   0x20000000
#define OV0_SCALE_CNTL__OV0_INT_EMU                        0x20000000
#define OV0_SCALE_CNTL__OV0_OVERLAY_EN_MASK                0x40000000
#define OV0_SCALE_CNTL__OV0_OVERLAY_EN                     0x40000000
#define OV0_SCALE_CNTL__OV0_SOFT_RESET_MASK                0x80000000
#define OV0_SCALE_CNTL__OV0_SOFT_RESET                     0x80000000

// OV0_V_INC
#define OV0_V_INC__OV0_V_INC_MASK                          0x03ffff00

// OV0_P1_V_ACCUM_INIT
#define OV0_P1_V_ACCUM_INIT__OV0_P1_MAX_LN_IN_PER_LN_OUT_MASK 0x00000003
#define OV0_P1_V_ACCUM_INIT__OV0_P1_V_ACCUM_INIT_MASK      0x03ff8000

// OV0_P23_V_ACCUM_INIT
#define OV0_P23_V_ACCUM_INIT__OV0_P23_MAX_LN_IN_PER_LN_OUT_MASK 0x00000003
#define OV0_P23_V_ACCUM_INIT__OV0_P23_V_ACCUM_INIT_MASK    0x01ff8000

// OV0_P1_BLANK_LINES_AT_TOP
#define OV0_P1_BLANK_LINES_AT_TOP__OV0_P1_BLNK_LN_AT_TOP_M1_MASK 0x00000fff
#define OV0_P1_BLANK_LINES_AT_TOP__OV0_P1_ACTIVE_LINES_M1_MASK 0x0fff0000

// OV0_P23_BLANK_LINES_AT_TOP
#define OV0_P23_BLANK_LINES_AT_TOP__OV0_P23_BLNK_LN_AT_TOP_M1_MASK 0x000007ff
#define OV0_P23_BLANK_LINES_AT_TOP__OV0_P23_ACTIVE_LINES_M1_MASK 0x07ff0000

// OV0_BASE_ADDR
#define OV0_BASE_ADDR__OV0_BASE_ADDR_MASK                  0xffffffff

// OV0_VID_BUF0_BASE_ADRS
#define OV0_VID_BUF0_BASE_ADRS__OV0_VID_BUF_PITCH_SEL_MASK 0x00000001
#define OV0_VID_BUF0_BASE_ADRS__OV0_VID_BUF_PITCH_SEL      0x00000001
#define OV0_VID_BUF0_BASE_ADRS__RESERVED_BIT1_MASK         0x00000002
#define OV0_VID_BUF0_BASE_ADRS__RESERVED_BIT1              0x00000002
#define OV0_VID_BUF0_BASE_ADRS__OV0_VID_BUF_BASE_ADRS_MASK 0x07fffff0
#define OV0_VID_BUF0_BASE_ADRS__RESERVED_BIT31_28_MASK     0xf0000000

// OV0_VID_BUF1_BASE_ADRS
#define OV0_VID_BUF1_BASE_ADRS__OV0_VID_BUF_PITCH_SEL_MASK 0x00000001
#define OV0_VID_BUF1_BASE_ADRS__OV0_VID_BUF_PITCH_SEL      0x00000001
#define OV0_VID_BUF1_BASE_ADRS__RESERVED_BIT1_MASK         0x00000002
#define OV0_VID_BUF1_BASE_ADRS__RESERVED_BIT1              0x00000002
#define OV0_VID_BUF1_BASE_ADRS__OV0_VID_BUF_BASE_ADRS_MASK 0x07fffff0
#define OV0_VID_BUF1_BASE_ADRS__RESERVED_BIT31_28_MASK     0xf0000000

// OV0_VID_BUF2_BASE_ADRS
#define OV0_VID_BUF2_BASE_ADRS__OV0_VID_BUF_PITCH_SEL_MASK 0x00000001
#define OV0_VID_BUF2_BASE_ADRS__OV0_VID_BUF_PITCH_SEL      0x00000001
#define OV0_VID_BUF2_BASE_ADRS__RESERVED_BIT1_MASK         0x00000002
#define OV0_VID_BUF2_BASE_ADRS__RESERVED_BIT1              0x00000002
#define OV0_VID_BUF2_BASE_ADRS__OV0_VID_BUF_BASE_ADRS_MASK 0x07fffff0
#define OV0_VID_BUF2_BASE_ADRS__RESERVED_BIT31_28_MASK     0xf0000000

// OV0_VID_BUF3_BASE_ADRS
#define OV0_VID_BUF3_BASE_ADRS__OV0_VID_BUF_PITCH_SEL_MASK 0x00000001
#define OV0_VID_BUF3_BASE_ADRS__OV0_VID_BUF_PITCH_SEL      0x00000001
#define OV0_VID_BUF3_BASE_ADRS__RESERVED_BIT1_MASK         0x00000002
#define OV0_VID_BUF3_BASE_ADRS__RESERVED_BIT1              0x00000002
#define OV0_VID_BUF3_BASE_ADRS__OV0_VID_BUF_BASE_ADRS_MASK 0x07fffff0
#define OV0_VID_BUF3_BASE_ADRS__RESERVED_BIT31_28_MASK     0xf0000000

// OV0_VID_BUF4_BASE_ADRS
#define OV0_VID_BUF4_BASE_ADRS__OV0_VID_BUF_PITCH_SEL_MASK 0x00000001
#define OV0_VID_BUF4_BASE_ADRS__OV0_VID_BUF_PITCH_SEL      0x00000001
#define OV0_VID_BUF4_BASE_ADRS__RESERVED_BIT1_MASK         0x00000002
#define OV0_VID_BUF4_BASE_ADRS__RESERVED_BIT1              0x00000002
#define OV0_VID_BUF4_BASE_ADRS__OV0_VID_BUF_BASE_ADRS_MASK 0x07fffff0
#define OV0_VID_BUF4_BASE_ADRS__RESERVED_BIT31_28_MASK     0xf0000000

// OV0_VID_BUF5_BASE_ADRS
#define OV0_VID_BUF5_BASE_ADRS__OV0_VID_BUF_PITCH_SEL_MASK 0x00000001
#define OV0_VID_BUF5_BASE_ADRS__OV0_VID_BUF_PITCH_SEL      0x00000001
#define OV0_VID_BUF5_BASE_ADRS__RESERVED_BIT1_MASK         0x00000002
#define OV0_VID_BUF5_BASE_ADRS__RESERVED_BIT1              0x00000002
#define OV0_VID_BUF5_BASE_ADRS__OV0_VID_BUF_BASE_ADRS_MASK 0x07fffff0
#define OV0_VID_BUF5_BASE_ADRS__RESERVED_BIT31_28_MASK     0xf0000000

// OV0_VID_BUF_PITCH0_VALUE
#define OV0_VID_BUF_PITCH0_VALUE__OV0_PITCH_VALUE_MASK     0x000ffff0
#define OV0_VID_BUF_PITCH0_VALUE__OV0_PITCH_SKIP_LINES_MASK 0x0c000000
#define OV0_VID_BUF_PITCH0_VALUE__OV0_PITCH_IN_TILES_LSBS_MASK 0xf0000000

// OV0_VID_BUF_PITCH1_VALUE
#define OV0_VID_BUF_PITCH1_VALUE__OV0_PITCH_VALUE_MASK     0x000ffff0
#define OV0_VID_BUF_PITCH1_VALUE__OV0_PITCH_SKIP_LINES_MASK 0x0c000000
#define OV0_VID_BUF_PITCH1_VALUE__OV0_PITCH_IN_TILES_LSBS_MASK 0xf0000000

// OV0_AUTO_FLIP_CNTRL
#define OV0_AUTO_FLIP_CNTRL__OV0_SOFT_BUF_NUM_MASK         0x00000007
#define OV0_AUTO_FLIP_CNTRL__OV0_SOFT_REPEAT_FIELD_MASK    0x00000008
#define OV0_AUTO_FLIP_CNTRL__OV0_SOFT_REPEAT_FIELD         0x00000008
#define OV0_AUTO_FLIP_CNTRL__OV0_SOFT_BUF_ODD_MASK         0x00000010
#define OV0_AUTO_FLIP_CNTRL__OV0_SOFT_BUF_ODD              0x00000010
#define OV0_AUTO_FLIP_CNTRL__OV0_IGNORE_REPEAT_FIELD_MASK  0x00000020
#define OV0_AUTO_FLIP_CNTRL__OV0_IGNORE_REPEAT_FIELD       0x00000020
#define OV0_AUTO_FLIP_CNTRL__OV0_SOFT_EOF_TOGGLE_MASK      0x00000040
#define OV0_AUTO_FLIP_CNTRL__OV0_SOFT_EOF_TOGGLE           0x00000040
#define OV0_AUTO_FLIP_CNTRL__OV0_VID_PORT_SELECT_MASK      0x00000300
#define OV0_AUTO_FLIP_CNTRL__OV0_P1_FIRST_LINE_EVEN_MASK   0x00010000
#define OV0_AUTO_FLIP_CNTRL__OV0_P1_FIRST_LINE_EVEN        0x00010000
#define OV0_AUTO_FLIP_CNTRL__OV0_SHIFT_EVEN_DOWN_MASK      0x00040000
#define OV0_AUTO_FLIP_CNTRL__OV0_SHIFT_EVEN_DOWN           0x00040000
#define OV0_AUTO_FLIP_CNTRL__OV0_SHIFT_ODD_DOWN_MASK       0x00080000
#define OV0_AUTO_FLIP_CNTRL__OV0_SHIFT_ODD_DOWN            0x00080000
#define OV0_AUTO_FLIP_CNTRL__OV0_FIELD_POL_SOURCE_MASK     0x00800000
#define OV0_AUTO_FLIP_CNTRL__OV0_FIELD_POL_SOURCE          0x00800000

// OV0_DEINTERLACE_PATTERN
#define OV0_DEINTERLACE_PATTERN__OV0_DEINT_PAT_MASK        0x000fffff
#define OV0_DEINTERLACE_PATTERN__OV0_DEINT_PAT_PNTR_MASK   0x0f000000
#define OV0_DEINTERLACE_PATTERN__OV0_DEINT_PAT_LEN_M1_MASK 0xf0000000

// OV0_SUBMIT_HISTORY
#define OV0_SUBMIT_HISTORY__OV0_NEXT_BUF_NUM_MASK          0x00000007
#define OV0_SUBMIT_HISTORY__OV0_NEXT_IS_ODD_MASK           0x00000010
#define OV0_SUBMIT_HISTORY__OV0_NEXT_IS_ODD                0x00000010
#define OV0_SUBMIT_HISTORY__OV0_CURR_BUF_NUM_MASK          0x00000700
#define OV0_SUBMIT_HISTORY__OV0_CURR_IS_ODD_MASK           0x00001000
#define OV0_SUBMIT_HISTORY__OV0_CURR_IS_ODD                0x00001000
#define OV0_SUBMIT_HISTORY__OV0_PREV_BUF_NUM_MASK          0x00070000
#define OV0_SUBMIT_HISTORY__OV0_PREV_IS_ODD_MASK           0x00100000
#define OV0_SUBMIT_HISTORY__OV0_PREV_IS_ODD                0x00100000

// OV0_H_INC
#define OV0_H_INC__OV0_P1_H_INC_MASK                       0x00003fff
#define OV0_H_INC__OV0_P23_H_INC_MASK                      0x3fff0000

// OV0_STEP_BY
#define OV0_STEP_BY__OV0_P1_H_STEP_BY_MASK                 0x00000007
#define OV0_STEP_BY__OV0_P1_PREDWNSC_RATIO_MASK            0x00000010
#define OV0_STEP_BY__OV0_P1_PREDWNSC_RATIO                 0x00000010
#define OV0_STEP_BY__OV0_P23_H_STEP_BY_MASK                0x00000700
#define OV0_STEP_BY__OV0_P23_PREDWNSC_RATIO_MASK           0x00001000
#define OV0_STEP_BY__OV0_P23_PREDWNSC_RATIO                0x00001000

// OV0_P1_H_ACCUM_INIT
#define OV0_P1_H_ACCUM_INIT__OV0_P1_H_ACCUM_INIT_MASK      0x000f8000
#define OV0_P1_H_ACCUM_INIT__OV0_PRESHIFT_P1_TO_MASK       0xf0000000

// OV0_P23_H_ACCUM_INIT
#define OV0_P23_H_ACCUM_INIT__OV0_P23_H_ACCUM_INIT_MASK    0x000f8000
#define OV0_P23_H_ACCUM_INIT__OV0_PRESHIFT_P23_TO_MASK     0x70000000

// OV0_P1_X_START_END
#define OV0_P1_X_START_END__OV0_P1_X_END_MASK              0x00000fff
#define OV0_P1_X_START_END__OV0_P1_X_START_MASK            0x000f0000

// OV0_P2_X_START_END
#define OV0_P2_X_START_END__OV0_P2_X_END_MASK              0x000007ff
#define OV0_P2_X_START_END__OV0_P2_X_START_MASK            0x000f0000

// OV0_P3_X_START_END
#define OV0_P3_X_START_END__OV0_P3_X_END_MASK              0x000007ff
#define OV0_P3_X_START_END__OV0_P3_X_START_MASK            0x000f0000

// OV0_FILTER_CNTL
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_HORZ_Y_MASK        0x00000001
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_HORZ_Y             0x00000001
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_HORZ_UV_MASK       0x00000002
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_HORZ_UV            0x00000002
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_VERT_Y_MASK        0x00000004
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_VERT_Y             0x00000004
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_VERT_UV_MASK       0x00000008
#define OV0_FILTER_CNTL__OV0_HC_COEF_ON_VERT_UV            0x00000008

// OV0_FOUR_TAP_COEF_0
#define OV0_FOUR_TAP_COEF_0__OV0_COEF__0TH_TAP_MASK        0x0000000f
#define OV0_FOUR_TAP_COEF_0__OV0_COEF__1ST_TAP_MASK        0x00007f00
#define OV0_FOUR_TAP_COEF_0__OV0_COEF__2ND_TAP_MASK        0x007f0000
#define OV0_FOUR_TAP_COEF_0__OV0_COEF__3RD_TAP_MASK        0x0f000000

// OV0_FOUR_TAP_COEF_1
#define OV0_FOUR_TAP_COEF_1__OV0_COEF__0TH_TAP_MASK        0x0000000f
#define OV0_FOUR_TAP_COEF_1__OV0_COEF__1ST_TAP_MASK        0x00007f00
#define OV0_FOUR_TAP_COEF_1__OV0_COEF__2ND_TAP_MASK        0x007f0000
#define OV0_FOUR_TAP_COEF_1__OV0_COEF__3RD_TAP_MASK        0x0f000000

// OV0_FOUR_TAP_COEF_2
#define OV0_FOUR_TAP_COEF_2__OV0_COEF__0TH_TAP_MASK        0x0000000f
#define OV0_FOUR_TAP_COEF_2__OV0_COEF__1ST_TAP_MASK        0x00007f00
#define OV0_FOUR_TAP_COEF_2__OV0_COEF__2ND_TAP_MASK        0x007f0000
#define OV0_FOUR_TAP_COEF_2__OV0_COEF__3RD_TAP_MASK        0x0f000000

// OV0_FOUR_TAP_COEF_3
#define OV0_FOUR_TAP_COEF_3__OV0_COEF__0TH_TAP_MASK        0x0000000f
#define OV0_FOUR_TAP_COEF_3__OV0_COEF__1ST_TAP_MASK        0x00007f00
#define OV0_FOUR_TAP_COEF_3__OV0_COEF__2ND_TAP_MASK        0x007f0000
#define OV0_FOUR_TAP_COEF_3__OV0_COEF__3RD_TAP_MASK        0x0f000000

// OV0_FOUR_TAP_COEF_4
#define OV0_FOUR_TAP_COEF_4__OV0_COEF__0TH_TAP_MASK        0x0000000f
#define OV0_FOUR_TAP_COEF_4__OV0_COEF__1ST_TAP_MASK        0x00007f00
#define OV0_FOUR_TAP_COEF_4__OV0_COEF__2ND_TAP_MASK        0x007f0000
#define OV0_FOUR_TAP_COEF_4__OV0_COEF__3RD_TAP_MASK        0x0f000000

// OV0_FLAG_CNTRL
#define OV0_FLAG_CNTRL__OV0_HI_PRI_MCREQ_MASK              0x0000000f
#define OV0_FLAG_CNTRL__OV0_HI_PRI_FORCE_MASK              0x00000100
#define OV0_FLAG_CNTRL__OV0_HI_PRI_FORCE                   0x00000100
#define OV0_FLAG_CNTRL__OV0_LUMA_10BIT_EN_MASK             0x00001000
#define OV0_FLAG_CNTRL__OV0_LUMA_10BIT_EN                  0x00001000
#define OV0_FLAG_CNTRL__OV0_CHROMA_10BIT_EN_MASK           0x00002000
#define OV0_FLAG_CNTRL__OV0_CHROMA_10BIT_EN                0x00002000

// OV0_SLICE_CNTL
#define OV0_SLICE_CNTL__OV0_SLICE_LAST_LINE_MASK           0x0000007f
#define OV0_SLICE_CNTL__OV0_SLICEDONE_STAT_MASK            0x40000000
#define OV0_SLICE_CNTL__OV0_SLICEDONE_STAT                 0x40000000
#define OV0_SLICE_CNTL__OV0_MPEG_EOF_TOGGLE_MASK           0x80000000
#define OV0_SLICE_CNTL__OV0_MPEG_EOF_TOGGLE                0x80000000

// OV0_VID_KEY_CLR_LOW
#define OV0_VID_KEY_CLR_LOW__OV0_VID_KEY_Cb_BLUE_LOW_MASK  0x000003ff
#define OV0_VID_KEY_CLR_LOW__OV0_VID_KEY_Y_GREEN_LOW_MASK  0x000ffc00
#define OV0_VID_KEY_CLR_LOW__OV0_VID_KEY_Cr_RED_LOW_MASK   0x3ff00000

// OV0_VID_KEY_CLR_HIGH
#define OV0_VID_KEY_CLR_HIGH__OV0_VID_KEY_Cb_BLUE_HIGH_MASK 0x000003ff
#define OV0_VID_KEY_CLR_HIGH__OV0_VID_KEY_Y_GREEN_HIGH_MASK 0x000ffc00
#define OV0_VID_KEY_CLR_HIGH__OV0_VID_KEY_Cr_RED_HIGH_MASK 0x3ff00000

// OV0_GRPH_KEY_CLR_LOW
#define OV0_GRPH_KEY_CLR_LOW__OV0_GRPH_KEY_BLUE_LOW_MASK   0x000000ff
#define OV0_GRPH_KEY_CLR_LOW__OV0_GRPH_KEY_GREEN_LOW_MASK  0x0000ff00
#define OV0_GRPH_KEY_CLR_LOW__OV0_GRPH_KEY_RED_LOW_MASK    0x00ff0000
#define OV0_GRPH_KEY_CLR_LOW__OV0_GRPH_KEY_ALPHA_LOW_MASK  0xff000000

// OV0_GRPH_KEY_CLR_HIGH
#define OV0_GRPH_KEY_CLR_HIGH__OV0_GRPH_KEY_BLUE_HIGH_MASK 0x000000ff
#define OV0_GRPH_KEY_CLR_HIGH__OV0_GRPH_KEY_GREEN_HIGH_MASK 0x0000ff00
#define OV0_GRPH_KEY_CLR_HIGH__OV0_GRPH_KEY_RED_HIGH_MASK  0x00ff0000
#define OV0_GRPH_KEY_CLR_HIGH__OV0_GRPH_KEY_ALPHA_HIGH_MASK 0xff000000

// OV0_KEY_CNTL
#define OV0_KEY_CNTL__OV0_VIDEO_KEY_FN_MASK                0x00000003
#define OV0_KEY_CNTL__OV0_GRAPHICS_KEY_FN_MASK             0x00000030
#define OV0_KEY_CNTL__OV0_CMP_MIX_MASK                     0x00000100
#define OV0_KEY_CNTL__OV0_CMP_MIX                          0x00000100

// OV0_TEST
#define OV0_TEST__OV0_SUBPIC_ONLY_MASK                     0x00000008
#define OV0_TEST__OV0_SUBPIC_ONLY                          0x00000008
#define OV0_TEST__OV0_SWAP_UV_MASK                         0x00000020
#define OV0_TEST__OV0_SWAP_UV                              0x00000020
#define OV0_TEST__OV0_NOROUNDUP_MASK                       0x00000040
#define OV0_TEST__OV0_NOROUNDUP                            0x00000040
#define OV0_TEST__OV0_ADAPTIVE_DEINT_ADJ_MASK              0x00003000

// SUBPIC_CNTL
#define SUBPIC_CNTL__SUBPIC_ON_MASK                        0x00000001
#define SUBPIC_CNTL__SUBPIC_ON                             0x00000001
#define SUBPIC_CNTL__BTN_HLI_ON_MASK                       0x00000002
#define SUBPIC_CNTL__BTN_HLI_ON                            0x00000002
#define SUBPIC_CNTL__SP_HORZ_MODE_MASK                     0x00000010
#define SUBPIC_CNTL__SP_HORZ_MODE                          0x00000010
#define SUBPIC_CNTL__SP_VERT_MODE_MASK                     0x00000020
#define SUBPIC_CNTL__SP_VERT_MODE                          0x00000020
#define SUBPIC_CNTL__SP_ODD_FIELD_MASK                     0x00000100
#define SUBPIC_CNTL__SP_ODD_FIELD                          0x00000100
#define SUBPIC_CNTL__SP_BUF_SELECT_MASK                    0x00000200
#define SUBPIC_CNTL__SP_BUF_SELECT                         0x00000200
#define SUBPIC_CNTL__SP_NO_R_EDGE_BLEND_MASK               0x00000400
#define SUBPIC_CNTL__SP_NO_R_EDGE_BLEND                    0x00000400

// SUBPIC_DEFCOLCON
#define SUBPIC_DEFCOLCON__BKGD_PIX_CON_MASK                0x0000000f
#define SUBPIC_DEFCOLCON__PATT_PIX_CON_MASK                0x000000f0
#define SUBPIC_DEFCOLCON__EMPH_PIX1_CON_MASK               0x00000f00
#define SUBPIC_DEFCOLCON__EMPH_PIX2_CON_MASK               0x0000f000
#define SUBPIC_DEFCOLCON__BKGD_PIX_CLR_MASK                0x000f0000
#define SUBPIC_DEFCOLCON__PATT_PIX_CLR_MASK                0x00f00000
#define SUBPIC_DEFCOLCON__EMPH_PIX1_CLR_MASK               0x0f000000
#define SUBPIC_DEFCOLCON__EMPH_PIX2_CLR_MASK               0xf0000000

// SUBPIC_Y_X_START
#define SUBPIC_Y_X_START__SP_START_X_MASK                  0x000003ff
#define SUBPIC_Y_X_START__SP_START_Y_MASK                  0x03ff0000

// SUBPIC_Y_X_END
#define SUBPIC_Y_X_END__SP_END_X_MASK                      0x000003ff
#define SUBPIC_Y_X_END__SP_END_Y_MASK                      0x03ff0000

// SUBPIC_V_INC
#define SUBPIC_V_INC__SP_V_INC_FRAC_MASK                   0x0000fff0
#define SUBPIC_V_INC__SP_V_INC_INT_MASK                    0x000f0000

// SUBPIC_H_INC
#define SUBPIC_H_INC__SP_H_INC_FRAC_MASK                   0x0000fff0
#define SUBPIC_H_INC__SP_H_INC_INT_MASK                    0x000f0000

// SUBPIC_BUF0_OFFSET
#define SUBPIC_BUF0_OFFSET__SUBPIC_OFFSET0_MASK            0xffffffff

// SUBPIC_BUF1_OFFSET
#define SUBPIC_BUF1_OFFSET__SUBPIC_OFFSET1_MASK            0xffffffff

// SUBPIC_LC0_OFFSET
#define SUBPIC_LC0_OFFSET__SUBPIC_LC0_OFFSET_MASK          0xffffffff

// SUBPIC_LC1_OFFSET
#define SUBPIC_LC1_OFFSET__SUBPIC_LC1_OFFSET_MASK          0xffffffff

// SUBPIC_PITCH
#define SUBPIC_PITCH__SUBPIC_BUF_PITCH_MASK                0x00000fff
#define SUBPIC_PITCH__SUBPIC_LC_PITCH_MASK                 0x0fff0000

// SUBPIC_BTN_HLI_COLCON
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_BKGD_PIX_CON_MASK   0x0000000f
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_PATT_PIX_CON_MASK   0x000000f0
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_EMPH_PIX1_CON_MASK  0x00000f00
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_EMPH_PIX2_CON_MASK  0x0000f000
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_BKGD_PIX_CLR_MASK   0x000f0000
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_PATT_PIX_CLR_MASK   0x00f00000
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_EMPH_PIX1_CLR_MASK  0x0f000000
#define SUBPIC_BTN_HLI_COLCON__BTN_HLI_EMPH_PIX2_CLR_MASK  0xf0000000

// SUBPIC_BTN_HLI_Y_X_START
#define SUBPIC_BTN_HLI_Y_X_START__BTN_HLI_START_X_MASK     0x000003ff
#define SUBPIC_BTN_HLI_Y_X_START__BTN_HLI_START_Y_MASK     0x03ff0000

// SUBPIC_BTN_HLI_Y_X_END
#define SUBPIC_BTN_HLI_Y_X_END__BTN_HLI_END_X_MASK         0x000003ff
#define SUBPIC_BTN_HLI_Y_X_END__BTN_HLI_END_Y_MASK         0x03ff0000

// SUBPIC_PALETTE_INDEX
#define SUBPIC_PALETTE_INDEX__SP_PAL_ADDR_MASK             0x0000000f

// SUBPIC_PALETTE_DATA
#define SUBPIC_PALETTE_DATA__SP_DATA_MASK                  0x00ffffff

// SUBPIC_0_PAL
#define SUBPIC_0_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_0_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_0_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_1_PAL
#define SUBPIC_1_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_1_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_1_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_2_PAL
#define SUBPIC_2_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_2_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_2_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_3_PAL
#define SUBPIC_3_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_3_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_3_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_4_PAL
#define SUBPIC_4_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_4_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_4_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_5_PAL
#define SUBPIC_5_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_5_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_5_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_6_PAL
#define SUBPIC_6_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_6_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_6_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_7_PAL
#define SUBPIC_7_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_7_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_7_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_8_PAL
#define SUBPIC_8_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_8_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_8_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_9_PAL
#define SUBPIC_9_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_9_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_9_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_A_PAL
#define SUBPIC_A_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_A_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_A_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_B_PAL
#define SUBPIC_B_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_B_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_B_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_C_PAL
#define SUBPIC_C_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_C_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_C_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_D_PAL
#define SUBPIC_D_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_D_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_D_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_E_PAL
#define SUBPIC_E_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_E_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_E_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_F_PAL
#define SUBPIC_F_PAL__SP_CB_MASK                           0x000000ff
#define SUBPIC_F_PAL__SP_CR_MASK                           0x0000ff00
#define SUBPIC_F_PAL__SP_Y_MASK                            0x00ff0000

// SUBPIC_H_ACCUM_INIT
#define SUBPIC_H_ACCUM_INIT__SP_H_ACC_INIT_FRAC_MASK       0x0000fff0
#define SUBPIC_H_ACCUM_INIT__SP_H_ACC_INIT_INT_MASK        0x07ff0000

// SUBPIC_V_ACCUM_INIT
#define SUBPIC_V_ACCUM_INIT__SP_V_ACC_INIT_FRAC_MASK       0x0000fff0
#define SUBPIC_V_ACCUM_INIT__SP_V_ACC_INIT_INT_MASK        0x07ff0000

// DISP_MISC_CNTL
#define DISP_MISC_CNTL__SOFT_RESET_GRPH_PP_MASK            0x00000001
#define DISP_MISC_CNTL__SOFT_RESET_GRPH_PP                 0x00000001
#define DISP_MISC_CNTL__SOFT_RESET_SUBPIC_PP_MASK          0x00000002
#define DISP_MISC_CNTL__SOFT_RESET_SUBPIC_PP               0x00000002
#define DISP_MISC_CNTL__SOFT_RESET_OV0_PP_MASK             0x00000004
#define DISP_MISC_CNTL__SOFT_RESET_OV0_PP                  0x00000004
#define DISP_MISC_CNTL__SOFT_RESET_GRPH_SCLK_MASK          0x00000010
#define DISP_MISC_CNTL__SOFT_RESET_GRPH_SCLK               0x00000010
#define DISP_MISC_CNTL__SOFT_RESET_SUBPIC_SCLK_MASK        0x00000020
#define DISP_MISC_CNTL__SOFT_RESET_SUBPIC_SCLK             0x00000020
#define DISP_MISC_CNTL__SOFT_RESET_OV0_SCLK_MASK           0x00000040
#define DISP_MISC_CNTL__SOFT_RESET_OV0_SCLK                0x00000040
#define DISP_MISC_CNTL__SYNC_STRENGTH_MASK                 0x00000300
#define DISP_MISC_CNTL__PALETTE_MEM_RD_MARGIN_MASK         0x0f000000
#define DISP_MISC_CNTL__RMX_BUF_MEM_RD_MARGIN_MASK         0xf0000000

// DAC_MACRO_CNTL
#define DAC_MACRO_CNTL__DAC_WHITE_CNTL_MASK                0x0000000f
#define DAC_MACRO_CNTL__DAC_BG_ADJ_MASK                    0x00000f00
#define DAC_MACRO_CNTL__DAC_PDWN_R_MASK                    0x00010000
#define DAC_MACRO_CNTL__DAC_PDWN_R                         0x00010000
#define DAC_MACRO_CNTL__DAC_PDWN_G_MASK                    0x00020000
#define DAC_MACRO_CNTL__DAC_PDWN_G                         0x00020000
#define DAC_MACRO_CNTL__DAC_PDWN_B_MASK                    0x00040000
#define DAC_MACRO_CNTL__DAC_PDWN_B                         0x00040000

// DISP_PWR_MAN
#define DISP_PWR_MAN__DISP_PWR_MAN_D3_CRTC_EN_MASK         0x00000001
#define DISP_PWR_MAN__DISP_PWR_MAN_D3_CRTC_EN              0x00000001
#define DISP_PWR_MAN__DISP_PWR_MAN_DPMS_MASK               0x00000300
#define DISP_PWR_MAN__DISP_D3_RST_MASK                     0x00010000
#define DISP_PWR_MAN__DISP_D3_RST                          0x00010000
#define DISP_PWR_MAN__DISP_D3_REG_RST_MASK                 0x00020000
#define DISP_PWR_MAN__DISP_D3_REG_RST                      0x00020000
#define DISP_PWR_MAN__DISP_D3_GRPH_RST_MASK                0x00040000
#define DISP_PWR_MAN__DISP_D3_GRPH_RST                     0x00040000
#define DISP_PWR_MAN__DISP_D3_SUBPIC_RST_MASK              0x00080000
#define DISP_PWR_MAN__DISP_D3_SUBPIC_RST                   0x00080000
#define DISP_PWR_MAN__DISP_D3_OV0_RST_MASK                 0x00100000
#define DISP_PWR_MAN__DISP_D3_OV0_RST                      0x00100000
#define DISP_PWR_MAN__DISP_D1D2_GRPH_RST_MASK              0x00200000
#define DISP_PWR_MAN__DISP_D1D2_GRPH_RST                   0x00200000
#define DISP_PWR_MAN__DISP_D1D2_SUBPIC_RST_MASK            0x00400000
#define DISP_PWR_MAN__DISP_D1D2_SUBPIC_RST                 0x00400000
#define DISP_PWR_MAN__DISP_D1D2_OV0_RST_MASK               0x00800000
#define DISP_PWR_MAN__DISP_D1D2_OV0_RST                    0x00800000

// DISP_TEST_DEBUG_CNTL
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_DISPENG_MASK       0x00000001
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_DISPENG            0x00000001
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_PALETTE_MASK       0x00000002
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_PALETTE            0x00000002
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_DAC_MASK           0x00000004
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_DAC                0x00000004
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_SUBPIC_MASK        0x00000008
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_SUBPIC             0x00000008
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_OV0SCALE_MASK      0x00000010
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_OV0SCALE           0x00000010
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_CLK_MASK           0x00000020
#define DISP_TEST_DEBUG_CNTL__DISP_TEST_CLK                0x00000020
#define DISP_TEST_DEBUG_CNTL__DISP_GRPH_UNDERFLOW_MASK     0x01000000
#define DISP_TEST_DEBUG_CNTL__DISP_GRPH_UNDERFLOW          0x01000000
#define DISP_TEST_DEBUG_CNTL__DISP_GRPH_UNDERFLOW_CLR_MASK 0x01000000
#define DISP_TEST_DEBUG_CNTL__DISP_GRPH_UNDERFLOW_CLR      0x01000000
#define DISP_TEST_DEBUG_CNTL__DISP_CUR_UNDERFLOW_MASK      0x02000000
#define DISP_TEST_DEBUG_CNTL__DISP_CUR_UNDERFLOW           0x02000000
#define DISP_TEST_DEBUG_CNTL__DISP_CUR_UNDERFLOW_CLR_MASK  0x02000000
#define DISP_TEST_DEBUG_CNTL__DISP_CUR_UNDERFLOW_CLR       0x02000000
#define DISP_TEST_DEBUG_CNTL__DISP_OV0_UNDERFLOW_MASK      0x04000000
#define DISP_TEST_DEBUG_CNTL__DISP_OV0_UNDERFLOW           0x04000000
#define DISP_TEST_DEBUG_CNTL__DISP_OV0_UNDERFLOW_CLR_MASK  0x04000000
#define DISP_TEST_DEBUG_CNTL__DISP_OV0_UNDERFLOW_CLR       0x04000000
#define DISP_TEST_DEBUG_CNTL__DISP_SUBPIC_UNDERFLOW_MASK   0x08000000
#define DISP_TEST_DEBUG_CNTL__DISP_SUBPIC_UNDERFLOW        0x08000000
#define DISP_TEST_DEBUG_CNTL__DISP_SUBPIC_UNDERFLOW_CLR_MASK 0x08000000
#define DISP_TEST_DEBUG_CNTL__DISP_SUBPIC_UNDERFLOW_CLR    0x08000000
#define DISP_TEST_DEBUG_CNTL__DISP_SUBPIC_FORCE_HI_PRI_MASK 0x10000000
#define DISP_TEST_DEBUG_CNTL__DISP_SUBPIC_FORCE_HI_PRI     0x10000000

// DISP_HW_DEBUG
#define DISP_HW_DEBUG__DISP_HW_0_DEBUG_MASK                0x00000001
#define DISP_HW_DEBUG__DISP_HW_0_DEBUG                     0x00000001
#define DISP_HW_DEBUG__DISP_HW_1_DEBUG_MASK                0x00000002
#define DISP_HW_DEBUG__DISP_HW_1_DEBUG                     0x00000002
#define DISP_HW_DEBUG__DISP_HW_2_DEBUG_MASK                0x00000004
#define DISP_HW_DEBUG__DISP_HW_2_DEBUG                     0x00000004
#define DISP_HW_DEBUG__DISP_HW_3_DEBUG_MASK                0x00000008
#define DISP_HW_DEBUG__DISP_HW_3_DEBUG                     0x00000008
#define DISP_HW_DEBUG__DISP_HW_4_DEBUG_MASK                0x00000010
#define DISP_HW_DEBUG__DISP_HW_4_DEBUG                     0x00000010
#define DISP_HW_DEBUG__DISP_HW_5_DEBUG_MASK                0x00000020
#define DISP_HW_DEBUG__DISP_HW_5_DEBUG                     0x00000020
#define DISP_HW_DEBUG__DISP_HW_6_DEBUG_MASK                0x00000040
#define DISP_HW_DEBUG__DISP_HW_6_DEBUG                     0x00000040
#define DISP_HW_DEBUG__DISP_HW_7_DEBUG_MASK                0x00000080
#define DISP_HW_DEBUG__DISP_HW_7_DEBUG                     0x00000080
#define DISP_HW_DEBUG__DISP_HW_8_DEBUG_MASK                0x00000100
#define DISP_HW_DEBUG__DISP_HW_8_DEBUG                     0x00000100
#define DISP_HW_DEBUG__DISP_HW_9_DEBUG_MASK                0x00000200
#define DISP_HW_DEBUG__DISP_HW_9_DEBUG                     0x00000200
#define DISP_HW_DEBUG__DISP_HW_A_DEBUG_MASK                0x00000400
#define DISP_HW_DEBUG__DISP_HW_A_DEBUG                     0x00000400
#define DISP_HW_DEBUG__DISP_HW_B_DEBUG_MASK                0x00000800
#define DISP_HW_DEBUG__DISP_HW_B_DEBUG                     0x00000800
#define DISP_HW_DEBUG__DISP_HW_C_DEBUG_MASK                0x00001000
#define DISP_HW_DEBUG__DISP_HW_C_DEBUG                     0x00001000
#define DISP_HW_DEBUG__DISP_HW_D_DEBUG_MASK                0x00002000
#define DISP_HW_DEBUG__DISP_HW_D_DEBUG                     0x00002000
#define DISP_HW_DEBUG__DISP_HW_E_DEBUG_MASK                0x00004000
#define DISP_HW_DEBUG__DISP_HW_E_DEBUG                     0x00004000
#define DISP_HW_DEBUG__DISP_HW_F_DEBUG_MASK                0x00008000
#define DISP_HW_DEBUG__DISP_HW_F_DEBUG                     0x00008000
#define DISP_HW_DEBUG__DISP_HW_10_DEBUG_MASK               0x00010000
#define DISP_HW_DEBUG__DISP_HW_10_DEBUG                    0x00010000
#define DISP_HW_DEBUG__DISP_HW_11_DEBUG_MASK               0x00020000
#define DISP_HW_DEBUG__DISP_HW_11_DEBUG                    0x00020000
#define DISP_HW_DEBUG__DISP_HW_12_DEBUG_MASK               0x00040000
#define DISP_HW_DEBUG__DISP_HW_12_DEBUG                    0x00040000
#define DISP_HW_DEBUG__DISP_HW_13_DEBUG_MASK               0x00080000
#define DISP_HW_DEBUG__DISP_HW_13_DEBUG                    0x00080000
#define DISP_HW_DEBUG__DISP_HW_14_DEBUG_MASK               0x00100000
#define DISP_HW_DEBUG__DISP_HW_14_DEBUG                    0x00100000
#define DISP_HW_DEBUG__DISP_HW_15_DEBUG_MASK               0x00200000
#define DISP_HW_DEBUG__DISP_HW_15_DEBUG                    0x00200000
#define DISP_HW_DEBUG__DISP_HW_16_DEBUG_MASK               0x00400000
#define DISP_HW_DEBUG__DISP_HW_16_DEBUG                    0x00400000
#define DISP_HW_DEBUG__DISP_HW_17_DEBUG_MASK               0x00800000
#define DISP_HW_DEBUG__DISP_HW_17_DEBUG                    0x00800000
#define DISP_HW_DEBUG__DISP_HW_18_DEBUG_MASK               0x01000000
#define DISP_HW_DEBUG__DISP_HW_18_DEBUG                    0x01000000
#define DISP_HW_DEBUG__DISP_HW_19_DEBUG_MASK               0x02000000
#define DISP_HW_DEBUG__DISP_HW_19_DEBUG                    0x02000000
#define DISP_HW_DEBUG__DISP_HW_1A_DEBUG_MASK               0x04000000
#define DISP_HW_DEBUG__DISP_HW_1A_DEBUG                    0x04000000
#define DISP_HW_DEBUG__DISP_HW_1B_DEBUG_MASK               0x08000000
#define DISP_HW_DEBUG__DISP_HW_1B_DEBUG                    0x08000000
#define DISP_HW_DEBUG__DISP_HW_1C_DEBUG_MASK               0x10000000
#define DISP_HW_DEBUG__DISP_HW_1C_DEBUG                    0x10000000
#define DISP_HW_DEBUG__DISP_HW_1D_DEBUG_MASK               0x20000000
#define DISP_HW_DEBUG__DISP_HW_1D_DEBUG                    0x20000000
#define DISP_HW_DEBUG__DISP_HW_1E_DEBUG_MASK               0x40000000
#define DISP_HW_DEBUG__DISP_HW_1E_DEBUG                    0x40000000
#define DISP_HW_DEBUG__DISP_HW_1F_DEBUG_MASK               0x80000000
#define DISP_HW_DEBUG__DISP_HW_1F_DEBUG                    0x80000000

// DAC_CRC_SIG1
#define DAC_CRC_SIG1__DAC_CRC_SIG_B_MASK                   0x000003ff
#define DAC_CRC_SIG1__DAC_CRC_SIG_G_MASK                   0x03ff0000

// DAC_CRC_SIG2
#define DAC_CRC_SIG2__DAC_CRC_SIG_R_MASK                   0x000003ff
#define DAC_CRC_SIG2__DAC_CRC_SIG_C_MASK                   0x003f0000

// OV0_LIN_TRANS_A
#define OV0_LIN_TRANS_A__OV0_LIN_TRANS_Cb_R_MASK           0x0000fffe
#define OV0_LIN_TRANS_A__OV0_LIN_TRANS_Y_R_MASK            0xfffe0000

// OV0_LIN_TRANS_B
#define OV0_LIN_TRANS_B__OV0_LIN_TRANS_OFF_R_MASK          0x00001fff
#define OV0_LIN_TRANS_B__OV0_LIN_TRANS_Cr_R_MASK           0xfffe0000

// OV0_LIN_TRANS_C
#define OV0_LIN_TRANS_C__OV0_LIN_TRANS_Cb_G_MASK           0x0000fffe
#define OV0_LIN_TRANS_C__OV0_LIN_TRANS_Y_G_MASK            0xfffe0000

// OV0_LIN_TRANS_D
#define OV0_LIN_TRANS_D__OV0_LIN_TRANS_OFF_G_MASK          0x00001fff
#define OV0_LIN_TRANS_D__OV0_LIN_TRANS_Cr_G_MASK           0xfffe0000

// OV0_LIN_TRANS_E
#define OV0_LIN_TRANS_E__OV0_LIN_TRANS_Cb_B_MASK           0x0000fffe
#define OV0_LIN_TRANS_E__OV0_LIN_TRANS_Y_B_MASK            0xfffe0000

// OV0_LIN_TRANS_F
#define OV0_LIN_TRANS_F__OV0_LIN_TRANS_OFF_B_MASK          0x00001fff
#define OV0_LIN_TRANS_F__OV0_LIN_TRANS_Cr_B_MASK           0xfffe0000

// OV0_GAMMA_0_F
#define OV0_GAMMA_0_F__OV0_GAMMA_0_F_OFFSET_MASK           0x000001ff
#define OV0_GAMMA_0_F__OV0_GAMMA_0_F_SLOPE_MASK            0x07ff0000

// OV0_GAMMA_10_1F
#define OV0_GAMMA_10_1F__OV0_GAMMA_10_1F_OFFSET_MASK       0x000001ff
#define OV0_GAMMA_10_1F__OV0_GAMMA_10_1F_SLOPE_MASK        0x07ff0000

// OV0_GAMMA_20_3F
#define OV0_GAMMA_20_3F__OV0_GAMMA_20_3F_OFFSET_MASK       0x000003ff
#define OV0_GAMMA_20_3F__OV0_GAMMA_20_3F_SLOPE_MASK        0x03ff0000

// OV0_GAMMA_40_7F
#define OV0_GAMMA_40_7F__OV0_GAMMA_40_7F_OFFSET_MASK       0x000003ff
#define OV0_GAMMA_40_7F__OV0_GAMMA_40_7F_SLOPE_MASK        0x01ff0000

// OV0_GAMMA_380_3BF
#define OV0_GAMMA_380_3BF__OV0_GAMMA_380_3BF_OFFSET_MASK   0x000001ff
#define OV0_GAMMA_380_3BF__OV0_GAMMA_380_3BF_SLOPE_MASK    0x01ff0000

// OV0_GAMMA_3C0_3FF
#define OV0_GAMMA_3C0_3FF__OV0_GAMMA_3C0_3FF_OFFSET_MASK   0x000001ff
#define OV0_GAMMA_3C0_3FF__OV0_GAMMA_3C0_3FF_SLOPE_MASK    0x01ff0000

// DISP_MERGE_CNTL
#define DISP_MERGE_CNTL__DISP_ALPHA_MODE_MASK              0x00000003
#define DISP_MERGE_CNTL__DISP_ALPHA_INV_MASK               0x00000004
#define DISP_MERGE_CNTL__DISP_ALPHA_INV                    0x00000004
#define DISP_MERGE_CNTL__DISP_ALPHA_PREMULT_MASK           0x00000008
#define DISP_MERGE_CNTL__DISP_ALPHA_PREMULT                0x00000008
#define DISP_MERGE_CNTL__DISP_RGB_OFFSET_EN_MASK           0x00000100
#define DISP_MERGE_CNTL__DISP_RGB_OFFSET_EN                0x00000100
#define DISP_MERGE_CNTL__DISP_LIN_TRANS_BYPASS_MASK        0x00000200
#define DISP_MERGE_CNTL__DISP_LIN_TRANS_BYPASS             0x00000200
#define DISP_MERGE_CNTL__DISP_GRPH_ALPHA_MASK              0x00ff0000
#define DISP_MERGE_CNTL__DISP_OV0_ALPHA_MASK               0xff000000

// DISP_OUTPUT_CNTL
#define DISP_OUTPUT_CNTL__DISP_DAC_SOURCE_MASK             0x00000003
#define DISP_OUTPUT_CNTL__DISP_TRANS_MATRIX_SEL_MASK       0x00000030
#define DISP_OUTPUT_CNTL__DISP_RMX_SOURCE_MASK             0x00000100
#define DISP_OUTPUT_CNTL__DISP_RMX_SOURCE                  0x00000100
#define DISP_OUTPUT_CNTL__DISP_RMX_HTAP_SEL_MASK           0x00000200
#define DISP_OUTPUT_CNTL__DISP_RMX_HTAP_SEL                0x00000200
#define DISP_OUTPUT_CNTL__DISP_RMX_DITH_EN_MASK            0x00000400
#define DISP_OUTPUT_CNTL__DISP_RMX_DITH_EN                 0x00000400
#define DISP_OUTPUT_CNTL__DISP_TV_SOURCE_MASK              0x00010000
#define DISP_OUTPUT_CNTL__DISP_TV_SOURCE                   0x00010000
#define DISP_OUTPUT_CNTL__DISP_TV_MODE_MASK                0x00060000
#define DISP_OUTPUT_CNTL__DISP_TV_YG_DITH_EN_MASK          0x00080000
#define DISP_OUTPUT_CNTL__DISP_TV_YG_DITH_EN               0x00080000
#define DISP_OUTPUT_CNTL__DISP_TV_CbB_CrR_DITH_EN_MASK     0x00100000
#define DISP_OUTPUT_CNTL__DISP_TV_CbB_CrR_DITH_EN          0x00100000
#define DISP_OUTPUT_CNTL__DISP_TV_BIT_WIDTH_MASK           0x00200000
#define DISP_OUTPUT_CNTL__DISP_TV_BIT_WIDTH                0x00200000
#define DISP_OUTPUT_CNTL__DISP_TV_SYNC_MODE_MASK           0x00c00000
#define DISP_OUTPUT_CNTL__DISP_TV_SYNC_FORCE_MASK          0x01000000
#define DISP_OUTPUT_CNTL__DISP_TV_SYNC_FORCE               0x01000000
#define DISP_OUTPUT_CNTL__DISP_TV_SYNC_COLOR_MASK          0x06000000
#define DISP_OUTPUT_CNTL__DISP_TV_EVEN_FLAG_CNTL_MASK      0x18000000
#define DISP_OUTPUT_CNTL__DISP_TV_SYNC_STATUS_MASK         0x20000000
#define DISP_OUTPUT_CNTL__DISP_TV_SYNC_STATUS              0x20000000
#define DISP_OUTPUT_CNTL__DISP_TV_H_DOWNSCALE_MASK         0x40000000
#define DISP_OUTPUT_CNTL__DISP_TV_H_DOWNSCALE              0x40000000

// DISP_LIN_TRANS_GRPH_A
#define DISP_LIN_TRANS_GRPH_A__DISP_LIN_TRANS_GRPH_C2_MASK 0x00007ff8
#define DISP_LIN_TRANS_GRPH_A__DISP_LIN_TRANS_GRPH_C1_MASK 0xfff80000

// DISP_LIN_TRANS_GRPH_B
#define DISP_LIN_TRANS_GRPH_B__DISP_LIN_TRANS_GRPH_C4_MASK 0x00001fff
#define DISP_LIN_TRANS_GRPH_B__DISP_LIN_TRANS_GRPH_C3_MASK 0x7ff80000

// DISP_LIN_TRANS_GRPH_C
#define DISP_LIN_TRANS_GRPH_C__DISP_LIN_TRANS_GRPH_C6_MASK 0x0000fff8
#define DISP_LIN_TRANS_GRPH_C__DISP_LIN_TRANS_GRPH_C5_MASK 0x7ff80000

// DISP_LIN_TRANS_GRPH_D
#define DISP_LIN_TRANS_GRPH_D__DISP_LIN_TRANS_GRPH_C8_MASK 0x00001fff
#define DISP_LIN_TRANS_GRPH_D__DISP_LIN_TRANS_GRPH_C7_MASK 0x7ff80000

// DISP_LIN_TRANS_GRPH_E
#define DISP_LIN_TRANS_GRPH_E__DISP_LIN_TRANS_GRPH_C10_MASK 0x00007ff8
#define DISP_LIN_TRANS_GRPH_E__DISP_LIN_TRANS_GRPH_C9_MASK 0x7ff80000

// DISP_LIN_TRANS_GRPH_F
#define DISP_LIN_TRANS_GRPH_F__DISP_LIN_TRANS_GRPH_C12_MASK 0x00001fff
#define DISP_LIN_TRANS_GRPH_F__DISP_LIN_TRANS_GRPH_C11_MASK 0xfff80000

// DISP_LIN_TRANS_VID_A
#define DISP_LIN_TRANS_VID_A__DISP_LIN_TRANS_VID_C2_MASK   0x00007ff8
#define DISP_LIN_TRANS_VID_A__DISP_LIN_TRANS_VID_C1_MASK   0xfff80000

// DISP_LIN_TRANS_VID_B
#define DISP_LIN_TRANS_VID_B__DISP_LIN_TRANS_VID_C4_MASK   0x00001fff
#define DISP_LIN_TRANS_VID_B__DISP_LIN_TRANS_VID_C3_MASK   0x7ff80000

// DISP_LIN_TRANS_VID_C
#define DISP_LIN_TRANS_VID_C__DISP_LIN_TRANS_VID_C6_MASK   0x0000fff8
#define DISP_LIN_TRANS_VID_C__DISP_LIN_TRANS_VID_C5_MASK   0x7ff80000

// DISP_LIN_TRANS_VID_D
#define DISP_LIN_TRANS_VID_D__DISP_LIN_TRANS_VID_C8_MASK   0x00001fff
#define DISP_LIN_TRANS_VID_D__DISP_LIN_TRANS_VID_C7_MASK   0x7ff80000

// DISP_LIN_TRANS_VID_E
#define DISP_LIN_TRANS_VID_E__DISP_LIN_TRANS_VID_C10_MASK  0x00007ff8
#define DISP_LIN_TRANS_VID_E__DISP_LIN_TRANS_VID_C9_MASK   0x7ff80000

// DISP_LIN_TRANS_VID_F
#define DISP_LIN_TRANS_VID_F__DISP_LIN_TRANS_VID_C12_MASK  0x00001fff
#define DISP_LIN_TRANS_VID_F__DISP_LIN_TRANS_VID_C11_MASK  0xfff80000

// RMX_HORZ_FILTER_0TAP_COEF
#define RMX_HORZ_FILTER_0TAP_COEF__RMX_COEF_0POS_0TAP_HORZ_MASK 0x0000001f
#define RMX_HORZ_FILTER_0TAP_COEF__RMX_COEF_1POS_0TAP_HORZ_MASK 0x000003e0
#define RMX_HORZ_FILTER_0TAP_COEF__RMX_COEF_2POS_0TAP_HORZ_MASK 0x00007c00
#define RMX_HORZ_FILTER_0TAP_COEF__RMX_COEF_3POS_0TAP_HORZ_MASK 0x000f8000
#define RMX_HORZ_FILTER_0TAP_COEF__RMX_COEF_4POS_0TAP_HORZ_MASK 0x01f00000

// RMX_HORZ_FILTER_1TAP_COEF
#define RMX_HORZ_FILTER_1TAP_COEF__RMX_COEF_0POS_1TAP_HORZ_MASK 0x0000001f
#define RMX_HORZ_FILTER_1TAP_COEF__RMX_COEF_1POS_1TAP_HORZ_MASK 0x000003e0
#define RMX_HORZ_FILTER_1TAP_COEF__RMX_COEF_2POS_1TAP_HORZ_MASK 0x00007c00
#define RMX_HORZ_FILTER_1TAP_COEF__RMX_COEF_3POS_1TAP_HORZ_MASK 0x000f8000
#define RMX_HORZ_FILTER_1TAP_COEF__RMX_COEF_4POS_1TAP_HORZ_MASK 0x01f00000

// RMX_HORZ_FILTER_2TAP_COEF
#define RMX_HORZ_FILTER_2TAP_COEF__RMX_COEF_0POS_2TAP_HORZ_MASK 0x0000001f
#define RMX_HORZ_FILTER_2TAP_COEF__RMX_COEF_1POS_2TAP_HORZ_MASK 0x000003e0
#define RMX_HORZ_FILTER_2TAP_COEF__RMX_COEF_2POS_2TAP_HORZ_MASK 0x00007c00
#define RMX_HORZ_FILTER_2TAP_COEF__RMX_COEF_3POS_2TAP_HORZ_MASK 0x000f8000
#define RMX_HORZ_FILTER_2TAP_COEF__RMX_COEF_4POS_2TAP_HORZ_MASK 0x01f00000

// RMX_HORZ_PHASE
#define RMX_HORZ_PHASE__RMX_HORZ_START_PHASE_MASK          0x00000fff
#define RMX_HORZ_PHASE__RMX_VERT_START_PHASE_MASK          0x0fff0000

// DAC_EMBEDDED_SYNC_CNTL
#define DAC_EMBEDDED_SYNC_CNTL__DAC_EMBED_SYNC_EN_Y_G_MASK 0x00000001
#define DAC_EMBEDDED_SYNC_CNTL__DAC_EMBED_SYNC_EN_Y_G      0x00000001
#define DAC_EMBEDDED_SYNC_CNTL__DAC_EMBED_SYNC_EN_Cb_B_MASK 0x00000002
#define DAC_EMBEDDED_SYNC_CNTL__DAC_EMBED_SYNC_EN_Cb_B     0x00000002
#define DAC_EMBEDDED_SYNC_CNTL__DAC_EMBED_SYNC_EN_Cr_R_MASK 0x00000004
#define DAC_EMBEDDED_SYNC_CNTL__DAC_EMBED_SYNC_EN_Cr_R     0x00000004
#define DAC_EMBEDDED_SYNC_CNTL__DAC_TRILEVEL_SYNC_EN_MASK  0x00000008
#define DAC_EMBEDDED_SYNC_CNTL__DAC_TRILEVEL_SYNC_EN       0x00000008

// DAC_BROAD_PULSE
#define DAC_BROAD_PULSE__DAC_BROAD_PULSE_START_MASK        0x00001fff
#define DAC_BROAD_PULSE__DAC_BROAD_PULSE_END_MASK          0x1fff0000

// DAC_SKEW_CLKS
#define DAC_SKEW_CLKS__DAC_SKEW_CLKS_MASK                  0x000000ff

// DAC_INCR
#define DAC_INCR__DAC_INCR_Y_G_MASK                        0x000003ff
#define DAC_INCR__DAC_INCR_CrCb_RB_MASK                    0x03ff0000

// DAC_NEG_SYNC_LEVEL
#define DAC_NEG_SYNC_LEVEL__DAC_NEG_SYNC_LEVEL_Y_G_MASK    0x000003ff
#define DAC_NEG_SYNC_LEVEL__DAC_NEG_SYNC_LEVEL_CrCb_RB_MASK 0x03ff0000

// DAC_POS_SYNC_LEVEL
#define DAC_POS_SYNC_LEVEL__DAC_POS_SYNC_LEVEL_Y_G_MASK    0x000003ff
#define DAC_POS_SYNC_LEVEL__DAC_POS_SYNC_LEVEL_CrCb_RB_MASK 0x03ff0000

// DAC_BLANK_LEVEL
#define DAC_BLANK_LEVEL__DAC_BLANK_LEVEL_Y_G_MASK          0x000003ff
#define DAC_BLANK_LEVEL__DAC_BLANK_LEVEL_CrCb_RB_MASK      0x03ff0000

// CLOCK_CNTL_INDEX
#define CLOCK_CNTL_INDEX__PLL_ADDR_MASK                    0x0000001f
#define CLOCK_CNTL_INDEX__PLL_WR_EN_MASK                   0x00000080
#define CLOCK_CNTL_INDEX__PLL_WR_EN                        0x00000080
#define CLOCK_CNTL_INDEX__PPLL_DIV_SEL_MASK                0x00000300

// CLOCK_CNTL_DATA
#define CLOCK_CNTL_DATA__PLL_DATA_MASK                     0xffffffff

// PPLL_DIV_0
#define PPLL_DIV_0__PPLL_FB0_DIV_MASK                      0x000007ff
#define PPLL_DIV_0__PPLL_ATOMIC_UPDATE_W_MASK              0x00008000
#define PPLL_DIV_0__PPLL_ATOMIC_UPDATE_W                   0x00008000
#define PPLL_DIV_0__PPLL_ATOMIC_UPDATE_R_MASK              0x00008000
#define PPLL_DIV_0__PPLL_ATOMIC_UPDATE_R                   0x00008000
#define PPLL_DIV_0__PPLL_POST0_DIV_MASK                    0x00070000

// PPLL_DIV_1
#define PPLL_DIV_1__PPLL_FB1_DIV_MASK                      0x000007ff
#define PPLL_DIV_1__PPLL_ATOMIC_UPDATE_W_MASK              0x00008000
#define PPLL_DIV_1__PPLL_ATOMIC_UPDATE_W                   0x00008000
#define PPLL_DIV_1__PPLL_ATOMIC_UPDATE_R_MASK              0x00008000
#define PPLL_DIV_1__PPLL_ATOMIC_UPDATE_R                   0x00008000
#define PPLL_DIV_1__PPLL_POST1_DIV_MASK                    0x00070000

// PPLL_DIV_2
#define PPLL_DIV_2__PPLL_FB2_DIV_MASK                      0x000007ff
#define PPLL_DIV_2__PPLL_ATOMIC_UPDATE_W_MASK              0x00008000
#define PPLL_DIV_2__PPLL_ATOMIC_UPDATE_W                   0x00008000
#define PPLL_DIV_2__PPLL_ATOMIC_UPDATE_R_MASK              0x00008000
#define PPLL_DIV_2__PPLL_ATOMIC_UPDATE_R                   0x00008000
#define PPLL_DIV_2__PPLL_POST2_DIV_MASK                    0x00070000

// PPLL_DIV_3
#define PPLL_DIV_3__PPLL_FB3_DIV_MASK                      0x000007ff
#define PPLL_DIV_3__PPLL_ATOMIC_UPDATE_W_MASK              0x00008000
#define PPLL_DIV_3__PPLL_ATOMIC_UPDATE_W                   0x00008000
#define PPLL_DIV_3__PPLL_ATOMIC_UPDATE_R_MASK              0x00008000
#define PPLL_DIV_3__PPLL_ATOMIC_UPDATE_R                   0x00008000
#define PPLL_DIV_3__PPLL_POST3_DIV_MASK                    0x00070000

// VCLK_ECP_CNTL
#define VCLK_ECP_CNTL__VCLK_SRC_SEL_MASK                   0x00000003
#define VCLK_ECP_CNTL__VCLK_INVERT_MASK                    0x00000010
#define VCLK_ECP_CNTL__VCLK_INVERT                         0x00000010
#define VCLK_ECP_CNTL__ECP_DIV_MASK                        0x00000300
#define VCLK_ECP_CNTL__BYTE_CLK_POST_DIV_MASK              0x00030000
#define VCLK_ECP_CNTL__ECP_FORCE_ON_MASK                   0x00040000
#define VCLK_ECP_CNTL__ECP_FORCE_ON                        0x00040000
#define VCLK_ECP_CNTL__BYTE_CLK_OUT_EN_MASK                0x00100000
#define VCLK_ECP_CNTL__BYTE_CLK_OUT_EN                     0x00100000
#define VCLK_ECP_CNTL__BYTE_CLK_SKEW_MASK                  0x07000000

// HTOTAL_CNTL
#define HTOTAL_CNTL__HTOT_PIX_SLIP_MASK                    0x0000000f
#define HTOTAL_CNTL__HTOT_VCLK_SLIP_MASK                   0x00000f00
#define HTOTAL_CNTL__HTOT_PPLL_SLIP_MASK                   0x00070000
#define HTOTAL_CNTL__HTOT_CNTL_EDGE_MASK                   0x01000000
#define HTOTAL_CNTL__HTOT_CNTL_EDGE                        0x01000000
#define HTOTAL_CNTL__HTOT_CNTL_VGA_EN_MASK                 0x10000000
#define HTOTAL_CNTL__HTOT_CNTL_VGA_EN                      0x10000000

// PLL_TEST_CNTL
#define PLL_TEST_CNTL__TST_SRC_SEL_MASK                    0x0000001f
#define PLL_TEST_CNTL__TST_SRC_INV_MASK                    0x00000080
#define PLL_TEST_CNTL__TST_SRC_INV                         0x00000080
#define PLL_TEST_CNTL__TST_DIVIDERS_MASK                   0x00000100
#define PLL_TEST_CNTL__TST_DIVIDERS                        0x00000100
#define PLL_TEST_CNTL__PLL_MASK_READ_B_MASK                0x00000200
#define PLL_TEST_CNTL__PLL_MASK_READ_B                     0x00000200
#define PLL_TEST_CNTL__TESTCLK_MUX_SEL_MASK                0x00001000
#define PLL_TEST_CNTL__TESTCLK_MUX_SEL                     0x00001000
#define PLL_TEST_CNTL__ANALOG_MON_MASK                     0x000f0000
#define PLL_TEST_CNTL__TEST_COUNT_MASK                     0xff000000

// CP_RB_CNTL
#define CP_RB_CNTL__RB_BUFSZ_MASK                          0x0000003f
#define CP_RB_CNTL__RB_BLKSZ_MASK                          0x00003f00
#define CP_RB_CNTL__BUF_SWAP_MASK                          0x00030000
#define CP_RB_CNTL__MAX_FETCH_MASK                         0x000c0000
#define CP_RB_CNTL__RB_NO_UPDATE_MASK                      0x08000000
#define CP_RB_CNTL__RB_NO_UPDATE                           0x08000000

// CP_RB_BASE
#define CP_RB_BASE__RB_BASE_MASK                           0xfffffffc

// CP_RB_RPTR_ADDR
#define CP_RB_RPTR_ADDR__RB_RPTR_SWAP_MASK                 0x00000003
#define CP_RB_RPTR_ADDR__RB_RPTR_ADDR_MASK                 0xfffffffc

// CP_RB_RPTR
#define CP_RB_RPTR__RB_RPTR_MASK                           0x007fffff

// CP_RB_WPTR
#define CP_RB_WPTR__RB_WPTR_MASK                           0x007fffff

// CP_RB_WPTR_DELAY
#define CP_RB_WPTR_DELAY__PRE_WRITE_TIMER_MASK             0x0fffffff
#define CP_RB_WPTR_DELAY__PRE_WRITE_LIMIT_MASK             0xf0000000

// CP_IB_BASE
#define CP_IB_BASE__IB_BASE_MASK                           0xfffffffc

// CP_IB_BUFSZ
#define CP_IB_BUFSZ__IB_BUFSZ_MASK                         0x007fffff

// CP_CSQ_CNTL
#define CP_CSQ_CNTL__CSQ_CNT_PRIMARY_MASK                  0x000000ff
#define CP_CSQ_CNTL__CSQ_CNT_INDIRECT_MASK                 0x0000ff00
#define CP_CSQ_CNTL__CSQ_MODE_MASK                         0xf0000000

// CP_CSQ_APER_PRIMARY
#define CP_CSQ_APER_PRIMARY__CP_CSQ_APER_PRIMARY_MASK      0xffffffff

// CP_CSQ_APER_INDIRECT
#define CP_CSQ_APER_INDIRECT__CP_CSQ_APER_INDIRECT_MASK    0xffffffff

// CP_ME_CNTL
#define CP_ME_CNTL__ME_STAT_MASK                           0x0000ffff
#define CP_ME_CNTL__ME_STATMUX_MASK                        0x001f0000
#define CP_ME_CNTL__ME_BUSY_MASK                           0x20000000
#define CP_ME_CNTL__ME_BUSY                                0x20000000
#define CP_ME_CNTL__ME_MODE_MASK                           0x40000000
#define CP_ME_CNTL__ME_MODE                                0x40000000
#define CP_ME_CNTL__ME_STEP_MASK                           0x80000000
#define CP_ME_CNTL__ME_STEP                                0x80000000

// CP_ME_RAM_ADDR
#define CP_ME_RAM_ADDR__ME_RAM_ADDR_MASK                   0x000000ff

// CP_ME_RAM_RADDR
#define CP_ME_RAM_RADDR__ME_RAM_RADDR_MASK                 0x000000ff

// CP_ME_RAM_DATAH
#define CP_ME_RAM_DATAH__ME_RAM_DATAH_MASK                 0x0000003f

// CP_ME_RAM_DATAL
#define CP_ME_RAM_DATAL__ME_RAM_DATAL_MASK                 0xffffffff

// CP_DEBUG
#define CP_DEBUG__CP_DEBUG_MASK                            0xffffffff

// SCRATCH_REG0
#define SCRATCH_REG0__SCRATCH_REG0_MASK                    0xffffffff
#define GUI_SCRATCH_REG0__SCRATCH_REG0_MASK                0xffffffff

// SCRATCH_REG1
#define SCRATCH_REG1__SCRATCH_REG1_MASK                    0xffffffff
#define GUI_SCRATCH_REG1__SCRATCH_REG1_MASK                0xffffffff

// SCRATCH_REG2
#define SCRATCH_REG2__SCRATCH_REG2_MASK                    0xffffffff
#define GUI_SCRATCH_REG2__SCRATCH_REG2_MASK                0xffffffff

// SCRATCH_REG3
#define SCRATCH_REG3__SCRATCH_REG3_MASK                    0xffffffff
#define GUI_SCRATCH_REG3__SCRATCH_REG3_MASK                0xffffffff

// SCRATCH_REG4
#define SCRATCH_REG4__SCRATCH_REG4_MASK                    0xffffffff
#define GUI_SCRATCH_REG4__SCRATCH_REG4_MASK                0xffffffff

// SCRATCH_REG5
#define SCRATCH_REG5__SCRATCH_REG5_MASK                    0xffffffff
#define GUI_SCRATCH_REG5__SCRATCH_REG5_MASK                0xffffffff

// SCRATCH_UMSK
#define SCRATCH_UMSK__SCRATCH_UMSK_MASK                    0x0000003f
#define SCRATCH_UMSK__SCRATCH_SWAP_MASK                    0x00030000

// SCRATCH_ADDR
#define SCRATCH_ADDR__SCRATCH_ADDR_MASK                    0xffffffe0

// DMA_GUI_TABLE_ADDR
#define DMA_GUI_TABLE_ADDR__CP_SYNC_MASK                   0x00000001
#define DMA_GUI_TABLE_ADDR__CP_SYNC                        0x00000001
#define DMA_GUI_TABLE_ADDR__TABLE_ADDR_MASK                0xfffffff0

// DMA_GUI_SRC_ADDR
#define DMA_GUI_SRC_ADDR__SRC_ADDR_MASK                    0xffffffff

// DMA_GUI_DST_ADDR
#define DMA_GUI_DST_ADDR__DST_ADDR_MASK                    0xffffffff

// DMA_GUI_COMMAND
#define DMA_GUI_COMMAND__BYTE_COUNT_MASK                   0x001fffff
#define DMA_GUI_COMMAND__SRC_SWAP_MASK                     0x00c00000
#define DMA_GUI_COMMAND__DST_SWAP_MASK                     0x03000000
#define DMA_GUI_COMMAND__SAS_MASK                          0x04000000
#define DMA_GUI_COMMAND__SAS                               0x04000000
#define DMA_GUI_COMMAND__DAS_MASK                          0x08000000
#define DMA_GUI_COMMAND__DAS                               0x08000000
#define DMA_GUI_COMMAND__SAIC_MASK                         0x10000000
#define DMA_GUI_COMMAND__SAIC                              0x10000000
#define DMA_GUI_COMMAND__DAIC_MASK                         0x20000000
#define DMA_GUI_COMMAND__DAIC                              0x20000000
#define DMA_GUI_COMMAND__INTDIS_MASK                       0x40000000
#define DMA_GUI_COMMAND__INTDIS                            0x40000000
#define DMA_GUI_COMMAND__EOL_MASK                          0x80000000
#define DMA_GUI_COMMAND__EOL                               0x80000000

// DMA_GUI_STATUS
#define DMA_GUI_STATUS__DTAQ_AVAIL_MASK                    0x0000001f
#define DMA_GUI_STATUS__LAST_TABLE_NUM_MASK                0x00000f00
#define DMA_GUI_STATUS__CURRENT_TABLE_NUM_MASK             0x0000f000
#define DMA_GUI_STATUS__ABORT_EN_MASK                      0x00100000
#define DMA_GUI_STATUS__ABORT_EN                           0x00100000
#define DMA_GUI_STATUS__ACTIVE_MASK                        0x00200000
#define DMA_GUI_STATUS__ACTIVE                             0x00200000
#define DMA_GUI_STATUS__SWAP_MASK                          0x00c00000

// DMA_GUI_ACT_DSCRPTR
#define DMA_GUI_ACT_DSCRPTR__TABLE_ADDR_MASK               0xfffffff0

// DMA_VID_TABLE_ADDR
#define DMA_VID_TABLE_ADDR__CP_SYNC_MASK                   0x00000001
#define DMA_VID_TABLE_ADDR__CP_SYNC                        0x00000001
#define DMA_VID_TABLE_ADDR__TABLE_ADDR_MASK                0xfffffff0

// DMA_VID_SRC_ADDR
#define DMA_VID_SRC_ADDR__SRC_ADDR_MASK                    0xffffffff

// DMA_VID_DST_ADDR
#define DMA_VID_DST_ADDR__DST_ADDR_MASK                    0xffffffff

// DMA_VID_COMMAND
#define DMA_VID_COMMAND__BYTE_COUNT_MASK                   0x001fffff
#define DMA_VID_COMMAND__SRC_SWAP_MASK                     0x00c00000
#define DMA_VID_COMMAND__DST_SWAP_MASK                     0x03000000
#define DMA_VID_COMMAND__SAS_MASK                          0x04000000
#define DMA_VID_COMMAND__SAS                               0x04000000
#define DMA_VID_COMMAND__DAS_MASK                          0x08000000
#define DMA_VID_COMMAND__DAS                               0x08000000
#define DMA_VID_COMMAND__SAIC_MASK                         0x10000000
#define DMA_VID_COMMAND__SAIC                              0x10000000
#define DMA_VID_COMMAND__DAIC_MASK                         0x20000000
#define DMA_VID_COMMAND__DAIC                              0x20000000
#define DMA_VID_COMMAND__INTDIS_MASK                       0x40000000
#define DMA_VID_COMMAND__INTDIS                            0x40000000
#define DMA_VID_COMMAND__EOL_MASK                          0x80000000
#define DMA_VID_COMMAND__EOL                               0x80000000

// DMA_VID_STATUS
#define DMA_VID_STATUS__DTAQ_AVAIL_MASK                    0x0000001f
#define DMA_VID_STATUS__LAST_TABLE_NUM_MASK                0x00000f00
#define DMA_VID_STATUS__CURRENT_TABLE_NUM_MASK             0x0000f000
#define DMA_VID_STATUS__ABORT_EN_MASK                      0x00100000
#define DMA_VID_STATUS__ABORT_EN                           0x00100000
#define DMA_VID_STATUS__ACTIVE_MASK                        0x00200000
#define DMA_VID_STATUS__ACTIVE                             0x00200000
#define DMA_VID_STATUS__SWAP_MASK                          0x00c00000

// DMA_VID_ACT_DSCRPTR
#define DMA_VID_ACT_DSCRPTR__TABLE_ADDR_MASK               0xfffffff0

// CP_CSQ_ADDR
#define CP_CSQ_ADDR__CSQ_ADDR_MASK                         0x000003fc

// CP_CSQ_DATA
#define CP_CSQ_DATA__CSQ_DATA_MASK                         0xffffffff

// CP_CSQ_STAT
#define CP_CSQ_STAT__CSQ_RPTR_PRIMARY_MASK                 0x000000ff
#define CP_CSQ_STAT__CSQ_WPTR_PRIMARY_MASK                 0x0000ff00
#define CP_CSQ_STAT__CSQ_RPTR_INDIRECT_MASK                0x00ff0000
#define CP_CSQ_STAT__CSQ_WPTR_INDIRECT_MASK                0xff000000

// CP_STAT
#define CP_STAT__MRU_BUSY_MASK                             0x00000001
#define CP_STAT__MRU_BUSY                                  0x00000001
#define CP_STAT__MWU_BUSY_MASK                             0x00000002
#define CP_STAT__MWU_BUSY                                  0x00000002
#define CP_STAT__RSIU_BUSY_MASK                            0x00000004
#define CP_STAT__RSIU_BUSY                                 0x00000004
#define CP_STAT__RCIU_BUSY_MASK                            0x00000008
#define CP_STAT__RCIU_BUSY                                 0x00000008
#define CP_STAT__CSF_PRIMARY_BUSY_MASK                     0x00000200
#define CP_STAT__CSF_PRIMARY_BUSY                          0x00000200
#define CP_STAT__CSF_INDIRECT_BUSY_MASK                    0x00000400
#define CP_STAT__CSF_INDIRECT_BUSY                         0x00000400
#define CP_STAT__CSQ_PRIMARY_BUSY_MASK                     0x00000800
#define CP_STAT__CSQ_PRIMARY_BUSY                          0x00000800
#define CP_STAT__CSQ_INDIRECT_BUSY_MASK                    0x00001000
#define CP_STAT__CSQ_INDIRECT_BUSY                         0x00001000
#define CP_STAT__CSI_BUSY_MASK                             0x00002000
#define CP_STAT__CSI_BUSY                                  0x00002000
#define CP_STAT__GUIDMA_BUSY_MASK                          0x10000000
#define CP_STAT__GUIDMA_BUSY                               0x10000000
#define CP_STAT__VIDDMA_BUSY_MASK                          0x20000000
#define CP_STAT__VIDDMA_BUSY                               0x20000000
#define CP_STAT__CMDSTRM_BUSY_MASK                         0x40000000
#define CP_STAT__CMDSTRM_BUSY                              0x40000000
#define CP_STAT__CP_BUSY_MASK                              0x80000000
#define CP_STAT__CP_BUSY                                   0x80000000

// SE_PORT_DATA0
#define SE_PORT_DATA0__DATAPORT0_MASK                      0xffffffff

// SE_PORT_DATA1
#define SE_PORT_DATA1__DATAPORT1_MASK                      0xffffffff

// SE_PORT_DATA2
#define SE_PORT_DATA2__DATAPORT2_MASK                      0xffffffff

// SE_PORT_DATA3
#define SE_PORT_DATA3__DATAPORT3_MASK                      0xffffffff

// SE_PORT_DATA4
#define SE_PORT_DATA4__DATAPORT4_MASK                      0xffffffff

// SE_PORT_DATA5
#define SE_PORT_DATA5__DATAPORT5_MASK                      0xffffffff

// SE_PORT_DATA6
#define SE_PORT_DATA6__DATAPORT6_MASK                      0xffffffff

// SE_PORT_DATA7
#define SE_PORT_DATA7__DATAPORT7_MASK                      0xffffffff

// SE_PORT_DATA8
#define SE_PORT_DATA8__DATAPORT8_MASK                      0xffffffff

// SE_PORT_DATA9
#define SE_PORT_DATA9__DATAPORT9_MASK                      0xffffffff

// SE_PORT_DATA10
#define SE_PORT_DATA10__DATAPORT10_MASK                    0xffffffff

// SE_PORT_DATA11
#define SE_PORT_DATA11__DATAPORT11_MASK                    0xffffffff

// SE_PORT_DATA12
#define SE_PORT_DATA12__DATAPORT12_MASK                    0xffffffff

// SE_PORT_DATA13
#define SE_PORT_DATA13__DATAPORT13_MASK                    0xffffffff

// SE_PORT_DATA14
#define SE_PORT_DATA14__DATAPORT14_MASK                    0xffffffff

// SE_PORT_DATA15
#define SE_PORT_DATA15__DATAPORT15_MASK                    0xffffffff

// SE_PORT_IDX0
#define SE_PORT_IDX0__IDXPORT0_MASK                        0xffffffff

// SE_PORT_IDX1
#define SE_PORT_IDX1__IDXPORT1_MASK                        0xffffffff

// SE_PORT_IDX2
#define SE_PORT_IDX2__IDXPORT2_MASK                        0xffffffff

// SE_PORT_IDX3
#define SE_PORT_IDX3__IDXPORT3_MASK                        0xffffffff

// SE_PORT_IDX4
#define SE_PORT_IDX4__IDXPORT4_MASK                        0xffffffff

// SE_PORT_IDX5
#define SE_PORT_IDX5__IDXPORT5_MASK                        0xffffffff

// SE_PORT_IDX6
#define SE_PORT_IDX6__IDXPORT6_MASK                        0xffffffff

// SE_PORT_IDX7
#define SE_PORT_IDX7__IDXPORT7_MASK                        0xffffffff

// SE_PORT_IDX8
#define SE_PORT_IDX8__IDXPORT8_MASK                        0xffffffff

// SE_PORT_IDX9
#define SE_PORT_IDX9__IDXPORT9_MASK                        0xffffffff

// SE_PORT_IDX10
#define SE_PORT_IDX10__IDXPORT10_MASK                      0xffffffff

// SE_PORT_IDX11
#define SE_PORT_IDX11__IDXPORT11_MASK                      0xffffffff

// SE_PORT_IDX12
#define SE_PORT_IDX12__IDXPORT12_MASK                      0xffffffff

// SE_PORT_IDX13
#define SE_PORT_IDX13__IDXPORT13_MASK                      0xffffffff

// SE_PORT_IDX14
#define SE_PORT_IDX14__IDXPORT14_MASK                      0xffffffff

// SE_PORT_IDX15
#define SE_PORT_IDX15__IDXPORT15_MASK                      0xffffffff

// SE_VTX_FMT
#define SE_VTX_FMT__VTX_W0_PRESENT_MASK                    0x00000001
#define SE_VTX_FMT__VTX_W0_PRESENT                         0x00000001
#define SE_VTX_FMT__VTX_FPCOLOR_PRESENT_MASK               0x00000002
#define SE_VTX_FMT__VTX_FPCOLOR_PRESENT                    0x00000002
#define SE_VTX_FMT__VTX_FPALPHA_PRESENT_MASK               0x00000004
#define SE_VTX_FMT__VTX_FPALPHA_PRESENT                    0x00000004
#define SE_VTX_FMT__VTX_PKCOLOR_PRESENT_MASK               0x00000008
#define SE_VTX_FMT__VTX_PKCOLOR_PRESENT                    0x00000008
#define SE_VTX_FMT__VTX_FPSPEC_PRESENT_MASK                0x00000010
#define SE_VTX_FMT__VTX_FPSPEC_PRESENT                     0x00000010
#define SE_VTX_FMT__VTX_FPFOG_PRESENT_MASK                 0x00000020
#define SE_VTX_FMT__VTX_FPFOG_PRESENT                      0x00000020
#define SE_VTX_FMT__VTX_PKSPEC_PRESENT_MASK                0x00000040
#define SE_VTX_FMT__VTX_PKSPEC_PRESENT                     0x00000040
#define SE_VTX_FMT__VTX_ST0_PRESENT_MASK                   0x00000080
#define SE_VTX_FMT__VTX_ST0_PRESENT                        0x00000080
#define SE_VTX_FMT__VTX_ST1_PRESENT_MASK                   0x00000100
#define SE_VTX_FMT__VTX_ST1_PRESENT                        0x00000100
#define SE_VTX_FMT__VTX_Q1_PRESENT_MASK                    0x00000200
#define SE_VTX_FMT__VTX_Q1_PRESENT                         0x00000200
#define SE_VTX_FMT__VTX_ST2_PRESENT_MASK                   0x00000400
#define SE_VTX_FMT__VTX_ST2_PRESENT                        0x00000400
#define SE_VTX_FMT__VTX_Q2_PRESENT_MASK                    0x00000800
#define SE_VTX_FMT__VTX_Q2_PRESENT                         0x00000800
#define SE_VTX_FMT__VTX_ST3_PRESENT_MASK                   0x00001000
#define SE_VTX_FMT__VTX_ST3_PRESENT                        0x00001000
#define SE_VTX_FMT__VTX_Q3_PRESENT_MASK                    0x00002000
#define SE_VTX_FMT__VTX_Q3_PRESENT                         0x00002000
#define SE_VTX_FMT__VTX_Q0_PRESENT_MASK                    0x00004000
#define SE_VTX_FMT__VTX_Q0_PRESENT                         0x00004000
#define SE_VTX_FMT__VTX_BLND_WEIGHT_CNT_MASK               0x00038000
#define SE_VTX_FMT__VTX_N0_PRESENT_MASK                    0x00040000
#define SE_VTX_FMT__VTX_N0_PRESENT                         0x00040000
#define SE_VTX_FMT__VTX_XY1_PRESENT_MASK                   0x08000000
#define SE_VTX_FMT__VTX_XY1_PRESENT                        0x08000000
#define SE_VTX_FMT__VTX_Z1_PRESENT_MASK                    0x10000000
#define SE_VTX_FMT__VTX_Z1_PRESENT                         0x10000000
#define SE_VTX_FMT__VTX_W1_PRESENT_MASK                    0x20000000
#define SE_VTX_FMT__VTX_W1_PRESENT                         0x20000000
#define SE_VTX_FMT__VTX_N1_PRESENT_MASK                    0x40000000
#define SE_VTX_FMT__VTX_N1_PRESENT                         0x40000000
#define SE_VTX_FMT__VTX_Z_PRESENT_MASK                     0x80000000
#define SE_VTX_FMT__VTX_Z_PRESENT                          0x80000000

// SE_VF_CNTL
#define SE_VF_CNTL__PRIM_TYPE_MASK                         0x0000000f
#define SE_VF_CNTL__PRIM_WALK_MASK                         0x00000030
#define SE_VF_CNTL__COLOR_ORDER_MASK                       0x00000040
#define SE_VF_CNTL__COLOR_ORDER                            0x00000040
#define SE_VF_CNTL__EN_MAOS_MASK                           0x00000080
#define SE_VF_CNTL__EN_MAOS                                0x00000080
#define SE_VF_CNTL__VTX_FMT_MODE_MASK                      0x00000100
#define SE_VF_CNTL__VTX_FMT_MODE                           0x00000100
#define SE_VF_CNTL__TCL_ENABLE_MASK                        0x00000200
#define SE_VF_CNTL__TCL_ENABLE                             0x00000200
#define SE_VF_CNTL__NUM_VERTICES_MASK                      0xffff0000

// SE_CNTL
#define SE_CNTL__FFACE_CULL_DIR_MASK                       0x00000001
#define SE_CNTL__FFACE_CULL_DIR                            0x00000001
#define SE_CNTL__BFACE_CULL_FCN_MASK                       0x00000006
#define SE_CNTL__FFACE_CULL_FCN_MASK                       0x00000018
#define SE_CNTL__BADVTX_CULL_DISABLE_MASK                  0x00000020
#define SE_CNTL__BADVTX_CULL_DISABLE                       0x00000020
#define SE_CNTL__FLAT_SHADE_VTX_MASK                       0x000000c0
#define SE_CNTL__DIFFUSE_SHADE_FCN_MASK                    0x00000300
#define SE_CNTL__ALPHA_SHADE_FCN_MASK                      0x00000c00
#define SE_CNTL__SPECULAR_SHADE_FCN_MASK                   0x00003000
#define SE_CNTL__FOG_SHADE_FCN_MASK                        0x0000c000
#define SE_CNTL__ZBIAS_EN_POINT_MASK                       0x00010000
#define SE_CNTL__ZBIAS_EN_POINT                            0x00010000
#define SE_CNTL__ZBIAS_EN_LINE_MASK                        0x00020000
#define SE_CNTL__ZBIAS_EN_LINE                             0x00020000
#define SE_CNTL__ZBIAS_EN_TRI_MASK                         0x00040000
#define SE_CNTL__ZBIAS_EN_TRI                              0x00040000
#define SE_CNTL__WIDELINE_EN_MASK                          0x00100000
#define SE_CNTL__WIDELINE_EN                               0x00100000
#define SE_CNTL__VPORT_XY_XFEN_MASK                        0x01000000
#define SE_CNTL__VPORT_XY_XFEN                             0x01000000
#define SE_CNTL__VPORT_Z_XFEN_MASK                         0x02000000
#define SE_CNTL__VPORT_Z_XFEN                              0x02000000
#define SE_CNTL__VTX_PIXCENTER_MASK                        0x08000000
#define SE_CNTL__VTX_PIXCENTER                             0x08000000
#define SE_CNTL__ROUND_MODE_MASK                           0x30000000
#define SE_CNTL__ROUND_PRECISION_MASK                      0xc0000000

// SE_COORD_FMT
#define SE_COORD_FMT__VTX_XY_FMT_MASK                      0x00000001
#define SE_COORD_FMT__VTX_XY_FMT                           0x00000001
#define SE_COORD_FMT__VTX_Z_FMT_MASK                       0x00000002
#define SE_COORD_FMT__VTX_Z_FMT                            0x00000002
#define SE_COORD_FMT__VTX_ST0_NONPARAMETRIC_MASK           0x00000100
#define SE_COORD_FMT__VTX_ST0_NONPARAMETRIC                0x00000100
#define SE_COORD_FMT__VTX_ST1_NONPARAMETRIC_MASK           0x00000200
#define SE_COORD_FMT__VTX_ST1_NONPARAMETRIC                0x00000200
#define SE_COORD_FMT__VTX_ST2_NONPARAMETRIC_MASK           0x00000400
#define SE_COORD_FMT__VTX_ST2_NONPARAMETRIC                0x00000400
#define SE_COORD_FMT__VTX_ST3_NONPARAMETRIC_MASK           0x00000800
#define SE_COORD_FMT__VTX_ST3_NONPARAMETRIC                0x00000800
#define SE_COORD_FMT__VTX_W0_NORMALIZE_MASK                0x00001000
#define SE_COORD_FMT__VTX_W0_NORMALIZE                     0x00001000
#define SE_COORD_FMT__VTX_W0_FMT_MASK                      0x00010000
#define SE_COORD_FMT__VTX_W0_FMT                           0x00010000
#define SE_COORD_FMT__VTX_ST0_FMT_MASK                     0x00020000
#define SE_COORD_FMT__VTX_ST0_FMT                          0x00020000
#define SE_COORD_FMT__VTX_ST1_FMT_MASK                     0x00080000
#define SE_COORD_FMT__VTX_ST1_FMT                          0x00080000
#define SE_COORD_FMT__VTX_ST2_FMT_MASK                     0x00200000
#define SE_COORD_FMT__VTX_ST2_FMT                          0x00200000
#define SE_COORD_FMT__VTX_ST3_FMT_MASK                     0x00800000
#define SE_COORD_FMT__VTX_ST3_FMT                          0x00800000
#define SE_COORD_FMT__TEX1_W_ROUTING_MASK                  0x04000000
#define SE_COORD_FMT__TEX1_W_ROUTING                       0x04000000

// SE_VPORT_XSCALE
#define SE_VPORT_XSCALE__VPORT_XSCALE_MASK                 0xffffffff

// SE_VPORT_XOFFSET
#define SE_VPORT_XOFFSET__VPORT_XOFFSET_MASK               0xffffffff

// SE_VPORT_YSCALE
#define SE_VPORT_YSCALE__VPORT_YSCALE_MASK                 0xffffffff

// SE_VPORT_YOFFSET
#define SE_VPORT_YOFFSET__VPORT_YOFFSET_MASK               0xffffffff

// SE_VPORT_ZSCALE
#define SE_VPORT_ZSCALE__VPORT_ZSCALE_MASK                 0xffffffff

// SE_VPORT_ZOFFSET
#define SE_VPORT_ZOFFSET__VPORT_ZOFFSET_MASK               0xffffffff

// SE_ZBIAS_FACTOR
#define SE_ZBIAS_FACTOR__ZBIAS_FACTOR_MASK                 0xffffffff

// SE_ZBIAS_CONSTANT
#define SE_ZBIAS_CONSTANT__ZBIAS_CONSTANT_MASK             0xffffffff

// SE_LINE_WIDTH
#define SE_LINE_WIDTH__LINE_WIDTH_MASK                     0x000003ff

// SE_W0_RANGE
#define SE_W0_RANGE__W0_RANGE_MASK                         0xffffffff

// SE_VTX_NUM_ARRAYS
#define SE_VTX_NUM_ARRAYS__VTX_NUM_ARRAYS_MASK             0x0000001f
#define SE_VTX_NUM_ARRAYS__VC_PFETCH_MASK                  0x0000c000

// SE_VTX_AOS_ATTR01
#define SE_VTX_AOS_ATTR01__VTX_AOS_COUNT0_MASK             0x0000003f
#define SE_VTX_AOS_ATTR01__VTX_AOS_STRIDE0_MASK            0x00003f00
#define SE_VTX_AOS_ATTR01__VTX_AOS_COUNT1_MASK             0x003f0000
#define SE_VTX_AOS_ATTR01__VTX_AOS_STRIDE1_MASK            0x3f000000

// SE_VTX_AOS_ADDR0
#define SE_VTX_AOS_ADDR0__VTX_AOS_ADDR0_MASK               0xfffffffc

// SE_VTX_AOS_ADDR1
#define SE_VTX_AOS_ADDR1__VTX_AOS_ADDR1_MASK               0xfffffffc

// SE_VTX_AOS_ATTR23
#define SE_VTX_AOS_ATTR23__VTX_AOS_COUNT2_MASK             0x0000003f
#define SE_VTX_AOS_ATTR23__VTX_AOS_STRIDE2_MASK            0x00003f00
#define SE_VTX_AOS_ATTR23__VTX_AOS_COUNT3_MASK             0x003f0000
#define SE_VTX_AOS_ATTR23__VTX_AOS_STRIDE3_MASK            0x3f000000

// SE_VTX_AOS_ADDR2
#define SE_VTX_AOS_ADDR2__VTX_AOS_ADDR2_MASK               0xfffffffc

// SE_VTX_AOS_ADDR3
#define SE_VTX_AOS_ADDR3__VTX_AOS_ADDR3_MASK               0xfffffffc

// SE_VTX_AOS_ATTR45
#define SE_VTX_AOS_ATTR45__VTX_AOS_COUNT4_MASK             0x0000003f
#define SE_VTX_AOS_ATTR45__VTX_AOS_STRIDE4_MASK            0x00003f00
#define SE_VTX_AOS_ATTR45__VTX_AOS_COUNT5_MASK             0x003f0000
#define SE_VTX_AOS_ATTR45__VTX_AOS_STRIDE5_MASK            0x3f000000

// SE_VTX_AOS_ADDR4
#define SE_VTX_AOS_ADDR4__VTX_AOS_ADDR4_MASK               0xfffffffc

// SE_VTX_AOS_ADDR5
#define SE_VTX_AOS_ADDR5__VTX_AOS_ADDR5_MASK               0xfffffffc

// SE_VTX_AOS_ATTR67
#define SE_VTX_AOS_ATTR67__VTX_AOS_COUNT6_MASK             0x0000003f
#define SE_VTX_AOS_ATTR67__VTX_AOS_STRIDE6_MASK            0x00003f00
#define SE_VTX_AOS_ATTR67__VTX_AOS_COUNT7_MASK             0x003f0000
#define SE_VTX_AOS_ATTR67__VTX_AOS_STRIDE7_MASK            0x3f000000

// SE_VTX_AOS_ADDR6
#define SE_VTX_AOS_ADDR6__VTX_AOS_ADDR6_MASK               0xfffffffc

// SE_VTX_AOS_ADDR7
#define SE_VTX_AOS_ADDR7__VTX_AOS_ADDR7_MASK               0xfffffffc

// SE_VTX_AOS_ATTR89
#define SE_VTX_AOS_ATTR89__VTX_AOS_COUNT8_MASK             0x0000003f
#define SE_VTX_AOS_ATTR89__VTX_AOS_STRIDE8_MASK            0x00003f00
#define SE_VTX_AOS_ATTR89__VTX_AOS_COUNT9_MASK             0x003f0000
#define SE_VTX_AOS_ATTR89__VTX_AOS_STRIDE9_MASK            0x3f000000

// SE_VTX_AOS_ADDR8
#define SE_VTX_AOS_ADDR8__VTX_AOS_ADDR8_MASK               0xfffffffc

// SE_VTX_AOS_ADDR9
#define SE_VTX_AOS_ADDR9__VTX_AOS_ADDR9_MASK               0xfffffffc

// SE_VTX_AOS_ATTR1011
#define SE_VTX_AOS_ATTR1011__VTX_AOS_COUNT10_MASK          0x0000003f
#define SE_VTX_AOS_ATTR1011__VTX_AOS_STRIDE10_MASK         0x00003f00
#define SE_VTX_AOS_ATTR1011__VTX_AOS_COUNT11_MASK          0x003f0000
#define SE_VTX_AOS_ATTR1011__VTX_AOS_STRIDE11_MASK         0x3f000000

// SE_VTX_AOS_ADDR10
#define SE_VTX_AOS_ADDR10__VTX_AOS_ADDR10_MASK             0xfffffffc

// SE_VTX_AOS_ADDR11
#define SE_VTX_AOS_ADDR11__VTX_AOS_ADDR11_MASK             0xfffffffc

// SE_PERF_CNTL
#define SE_PERF_CNTL__PERFSEL1_MASK                        0x0000000f
#define SE_PERF_CNTL__CLR_PERF1_MASK                       0x00000040
#define SE_PERF_CNTL__CLR_PERF1                            0x00000040
#define SE_PERF_CNTL__EN_PERF1_MASK                        0x00000080
#define SE_PERF_CNTL__EN_PERF1                             0x00000080
#define SE_PERF_CNTL__PERFSEL2_MASK                        0x00000f00
#define SE_PERF_CNTL__CLR_PERF2_MASK                       0x00004000
#define SE_PERF_CNTL__CLR_PERF2                            0x00004000
#define SE_PERF_CNTL__EN_PERF2_MASK                        0x00008000
#define SE_PERF_CNTL__EN_PERF2                             0x00008000

// SE_PERF_COUNT1
#define SE_PERF_COUNT1__PERF_COUNT1_MASK                   0xffffffff

// SE_PERF_COUNT2
#define SE_PERF_COUNT2__PERF_COUNT2_MASK                   0xffffffff

// SE_DEBUG
#define SE_DEBUG__SE_DEBUG_MASK                            0xffffffff

// SE_CNTL_STATUS
#define SE_CNTL_STATUS__VC_SWAP_MASK                       0x00000003
#define SE_CNTL_STATUS__SINGLESTEP_MASK                    0x00000040
#define SE_CNTL_STATUS__SINGLESTEP                         0x00000040
#define SE_CNTL_STATUS__EN_SINGLESTEP_MASK                 0x00000080
#define SE_CNTL_STATUS__EN_SINGLESTEP                      0x00000080
#define SE_CNTL_STATUS__TCL_BYPASS_MASK                    0x00000100
#define SE_CNTL_STATUS__TCL_BYPASS                         0x00000100
#define SE_CNTL_STATUS__TCL_BUSY_MASK                      0x00000800
#define SE_CNTL_STATUS__TCL_BUSY                           0x00000800
#define SE_CNTL_STATUS__BADVTX_DETECTED_MASK               0x00008000
#define SE_CNTL_STATUS__BADVTX_DETECTED                    0x00008000
#define SE_CNTL_STATUS__PERF_BUSY_MASK                     0x00010000
#define SE_CNTL_STATUS__PERF_BUSY                          0x00010000
#define SE_CNTL_STATUS__MCSE_BUSY_MASK                     0x00020000
#define SE_CNTL_STATUS__MCSE_BUSY                          0x00020000
#define SE_CNTL_STATUS__REIU_BUSY_MASK                     0x00040000
#define SE_CNTL_STATUS__REIU_BUSY                          0x00040000
#define SE_CNTL_STATUS__LSUB_BUSY_MASK                     0x00080000
#define SE_CNTL_STATUS__LSUB_BUSY                          0x00080000
#define SE_CNTL_STATUS__PSUB_BUSY_MASK                     0x00100000
#define SE_CNTL_STATUS__PSUB_BUSY                          0x00100000
#define SE_CNTL_STATUS__LE_BUSY_MASK                       0x00200000
#define SE_CNTL_STATUS__LE_BUSY                            0x00200000
#define SE_CNTL_STATUS__PE_BUSY_MASK                       0x00400000
#define SE_CNTL_STATUS__PE_BUSY                            0x00400000
#define SE_CNTL_STATUS__EE_BUSY_MASK                       0x00800000
#define SE_CNTL_STATUS__EE_BUSY                            0x00800000
#define SE_CNTL_STATUS__VS_BUSY_MASK                       0x01000000
#define SE_CNTL_STATUS__VS_BUSY                            0x01000000
#define SE_CNTL_STATUS__RE_BUSY_MASK                       0x02000000
#define SE_CNTL_STATUS__RE_BUSY                            0x02000000
#define SE_CNTL_STATUS__XE_BUSY_MASK                       0x04000000
#define SE_CNTL_STATUS__XE_BUSY                            0x04000000
#define SE_CNTL_STATUS__MIU_BUSY_MASK                      0x08000000
#define SE_CNTL_STATUS__MIU_BUSY                           0x08000000
#define SE_CNTL_STATUS__VC_BUSY_MASK                       0x10000000
#define SE_CNTL_STATUS__VC_BUSY                            0x10000000
#define SE_CNTL_STATUS__VF_BUSY_MASK                       0x20000000
#define SE_CNTL_STATUS__VF_BUSY                            0x20000000
#define SE_CNTL_STATUS__REGPIPE_BUSY_MASK                  0x40000000
#define SE_CNTL_STATUS__REGPIPE_BUSY                       0x40000000
#define SE_CNTL_STATUS__SE_BUSY_MASK                       0x80000000
#define SE_CNTL_STATUS__SE_BUSY                            0x80000000

// SE_SERE_WCNTL
#define SE_SERE_WCNTL__SERE_WA0_MASK                       0x000003fc
#define SE_SERE_WCNTL__SERE_BLKID0_MASK                    0x00003000
#define SE_SERE_WCNTL__SERE_VALID0_MASK                    0x00004000
#define SE_SERE_WCNTL__SERE_VALID0                         0x00004000
#define SE_SERE_WCNTL__SERE_WA1_MASK                       0x03fc0000
#define SE_SERE_WCNTL__SERE_VALID1_MASK                    0x40000000
#define SE_SERE_WCNTL__SERE_VALID1                         0x40000000
#define SE_SERE_WCNTL__SERE_RTS_MASK                       0x80000000
#define SE_SERE_WCNTL__SERE_RTS                            0x80000000

// SE_SERE_WD0_0
#define SE_SERE_WD0_0__SERE_WD0_0_MASK                     0xffffffff

// SE_SERE_WD0_1
#define SE_SERE_WD0_1__SERE_WD0_1_MASK                     0xffffffff

// SE_SERE_WD0_2
#define SE_SERE_WD0_2__SERE_WD0_2_MASK                     0xffffffff

// SE_SERE_WD0_3
#define SE_SERE_WD0_3__SERE_WD0_3_MASK                     0xffffffff

// SE_SERE_WD1_0
#define SE_SERE_WD1_0__SERE_WD1_0_MASK                     0xffffffff

// SE_SERE_WD1_1
#define SE_SERE_WD1_1__SERE_WD1_1_MASK                     0xffffffff

// SE_SERE_WD1_2
#define SE_SERE_WD1_2__SERE_WD1_2_MASK                     0xffffffff

// SE_SERE_WD1_3
#define SE_SERE_WD1_3__SERE_WD1_3_MASK                     0xffffffff

// SE_MC_SRC2_CNTL
#define SE_MC_SRC2_CNTL__MC_X2_MASK                        0x00001fff
#define SE_MC_SRC2_CNTL__MC_Y2_MASK                        0x0fff0000
#define SE_MC_SRC2_CNTL__MC_SEC_SRC_PITCH_MUL_MASK         0xc0000000

// SE_MC_SRC1_CNTL
#define SE_MC_SRC1_CNTL__MC_X1_MASK                        0x00001fff
#define SE_MC_SRC1_CNTL__MC_Y1_MASK                        0x0fff0000
#define SE_MC_SRC1_CNTL__MC_IDCT_ENB_MASK                  0x10000000
#define SE_MC_SRC1_CNTL__MC_IDCT_ENB                       0x10000000
#define SE_MC_SRC1_CNTL__MC_SEC_TEX_ENB_MASK               0x20000000
#define SE_MC_SRC1_CNTL__MC_SEC_TEX_ENB                    0x20000000
#define SE_MC_SRC1_CNTL__MC_SRC_PITCH_MUL_MASK             0xc0000000

// SE_MC_DST_CNTL
#define SE_MC_DST_CNTL__MC_DST_Y_MASK                      0x00003fff
#define SE_MC_DST_CNTL__MC_DST_X_MASK                      0x3fff0000
#define SE_MC_DST_CNTL__MC_DST_PITCH_MUL_MASK              0xc0000000

// SE_MC_CNTL_START
#define SE_MC_CNTL_START__MC_CNTL_SRC_1_INDEX_MASK         0x0000000f
#define SE_MC_CNTL_START__MC_CNTL_DST_OFFSET_MASK          0x01fffff0
#define SE_MC_CNTL_START__MC_ALPHA_ENB_MASK                0x02000000
#define SE_MC_CNTL_START__MC_ALPHA_ENB                     0x02000000
#define SE_MC_CNTL_START__MC_CNTL_SRC_2_INDEX_MASK         0x1c000000
#define SE_MC_CNTL_START__MC_CNTL_WIDTH_HEIGHT_SEL_MASK    0xe0000000

// SE_MC_BUF_BASE
#define SE_MC_BUF_BASE__MC_BUF_BASE_MASK                   0xff000000

// SE_TCL_VECTOR_INDX_REG
#define SE_TCL_VECTOR_INDX_REG__OCTWORD_OFFSET_MASK        0x0000007f
#define SE_TCL_VECTOR_INDX_REG__OCTWORD_STRIDE_MASK        0x007f0000
#define SE_TCL_VECTOR_INDX_REG__DWORD_COUNT_MASK           0x30000000

// SE_TCL_VECTOR_DATA_REG
#define SE_TCL_VECTOR_DATA_REG__DATA_REGISTER_MASK         0xffffffff

// SE_TCL_SCALAR_INDX_REG
#define SE_TCL_SCALAR_INDX_REG__DWORD_OFFSET_MASK          0x0000007f
#define SE_TCL_SCALAR_INDX_REG__DWORD_STRIDE_MASK          0x007f0000

// SE_TCL_SCALAR_DATA_REG
#define SE_TCL_SCALAR_DATA_REG__DATA_REGISTER_MASK         0xffffffff

// SE_TCL_MATERIAL_EMISSIVE_RED
#define SE_TCL_MATERIAL_EMISSIVE_RED__MATERIAL_EMISSIVE_RED_MASK 0xffffffff

// SE_TCL_MATERIAL_EMISSIVE_GREEN
#define SE_TCL_MATERIAL_EMISSIVE_GREEN__MATERIAL_EMISSIVE_GREEN_MASK 0xffffffff

// SE_TCL_MATERIAL_EMISSIVE_BLUE
#define SE_TCL_MATERIAL_EMISSIVE_BLUE__MATERIAL_EMISSIVE_BLUE_MASK 0xffffffff

// SE_TCL_MATERIAL_EMISSIVE_ALPHA
#define SE_TCL_MATERIAL_EMISSIVE_ALPHA__MATERIAL_EMISSIVE_ALPHA_MASK 0xffffffff

// SE_TCL_MATERIAL_AMBIENT_RED
#define SE_TCL_MATERIAL_AMBIENT_RED__MATERIAL_AMBIENT_RED_MASK 0xffffffff

// SE_TCL_MATERIAL_AMBIENT_GREEN
#define SE_TCL_MATERIAL_AMBIENT_GREEN__MATERIAL_AMBIENT_GREEN_MASK 0xffffffff

// SE_TCL_MATERIAL_AMBIENT_BLUE
#define SE_TCL_MATERIAL_AMBIENT_BLUE__MATERIAL_AMBIENT_BLUE_MASK 0xffffffff

// SE_TCL_MATERIAL_AMBIENT_ALPHA
#define SE_TCL_MATERIAL_AMBIENT_ALPHA__MATERIAL_AMBIENT_ALPHA_MASK 0xffffffff

// SE_TCL_MATERIAL_DIFFUSE_RED
#define SE_TCL_MATERIAL_DIFFUSE_RED__MATERIAL_DIFFUSE_RED_MASK 0xffffffff

// SE_TCL_MATERIAL_DIFFUSE_GREEN
#define SE_TCL_MATERIAL_DIFFUSE_GREEN__MATERIAL_DIFFUSE_GREEN_MASK 0xffffffff

// SE_TCL_MATERIAL_DIFFUSE_BLUE
#define SE_TCL_MATERIAL_DIFFUSE_BLUE__MATERIAL_DIFFUSE_BLUE_MASK 0xffffffff

// SE_TCL_MATERIAL_DIFFUSE_ALPHA
#define SE_TCL_MATERIAL_DIFFUSE_ALPHA__MATERIAL_DIFFUSE_ALPHA_MASK 0xffffffff

// SE_TCL_MATERIAL_SPECULAR_RED
#define SE_TCL_MATERIAL_SPECULAR_RED__MATERIAL_SPECULAR_RED_MASK 0xffffffff

// SE_TCL_MATERIAL_SPECULAR_GREEN
#define SE_TCL_MATERIAL_SPECULAR_GREEN__MATERIAL_SPECULAR_GREEN_MASK 0xffffffff

// SE_TCL_MATERIAL_SPECULAR_BLUE
#define SE_TCL_MATERIAL_SPECULAR_BLUE__MATERIAL_SPECULAR_BLUE_MASK 0xffffffff

// SE_TCL_MATERIAL_SPECULAR_ALPHA
#define SE_TCL_MATERIAL_SPECULAR_ALPHA__MATERIAL_SPECULAR_ALPHA_MASK 0xffffffff

// SE_TCL_SHININESS
#define SE_TCL_SHININESS__SE_TCL_SHININESS_MASK            0xffffffff

// SE_TCL_OUTPUT_VTX_FMT
#define SE_TCL_OUTPUT_VTX_FMT__VTX_W0_PRESENT_MASK         0x00000001
#define SE_TCL_OUTPUT_VTX_FMT__VTX_W0_PRESENT              0x00000001
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPCOLOR_PRESENT_MASK    0x00000002
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPCOLOR_PRESENT         0x00000002
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPALPHA_PRESENT_MASK    0x00000004
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPALPHA_PRESENT         0x00000004
#define SE_TCL_OUTPUT_VTX_FMT__VTX_PKCOLOR_PRESENT_MASK    0x00000008
#define SE_TCL_OUTPUT_VTX_FMT__VTX_PKCOLOR_PRESENT         0x00000008
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPSPEC_PRESENT_MASK     0x00000010
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPSPEC_PRESENT          0x00000010
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPFOG_PRESENT_MASK      0x00000020
#define SE_TCL_OUTPUT_VTX_FMT__VTX_FPFOG_PRESENT           0x00000020
#define SE_TCL_OUTPUT_VTX_FMT__VTX_PKSPEC_PRESENT_MASK     0x00000040
#define SE_TCL_OUTPUT_VTX_FMT__VTX_PKSPEC_PRESENT          0x00000040
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST0_PRESENT_MASK        0x00000080
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST0_PRESENT             0x00000080
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST1_PRESENT_MASK        0x00000100
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST1_PRESENT             0x00000100
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q1_PRESENT_MASK         0x00000200
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q1_PRESENT              0x00000200
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST2_PRESENT_MASK        0x00000400
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST2_PRESENT             0x00000400
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q2_PRESENT_MASK         0x00000800
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q2_PRESENT              0x00000800
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST3_PRESENT_MASK        0x00001000
#define SE_TCL_OUTPUT_VTX_FMT__VTX_ST3_PRESENT             0x00001000
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q3_PRESENT_MASK         0x00002000
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q3_PRESENT              0x00002000
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q0_PRESENT_MASK         0x00004000
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Q0_PRESENT              0x00004000
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Z_PRESENT_MASK          0x80000000
#define SE_TCL_OUTPUT_VTX_FMT__VTX_Z_PRESENT               0x80000000

// SE_TCL_OUTPUT_VTX_SEL
#define SE_TCL_OUTPUT_VTX_SEL__VTX_XYZW_SELECT_MASK        0x00000001
#define SE_TCL_OUTPUT_VTX_SEL__VTX_XYZW_SELECT             0x00000001
#define SE_TCL_OUTPUT_VTX_SEL__VTX_PKDIFFUSE_SELECT_MASK   0x00000002
#define SE_TCL_OUTPUT_VTX_SEL__VTX_PKDIFFUSE_SELECT        0x00000002
#define SE_TCL_OUTPUT_VTX_SEL__VTX_PKSPEC_SELECT_MASK      0x00000004
#define SE_TCL_OUTPUT_VTX_SEL__VTX_PKSPEC_SELECT           0x00000004
#define SE_TCL_OUTPUT_VTX_SEL__FORCE_NAN_IF_CCOLOR_NAN_MASK 0x00000008
#define SE_TCL_OUTPUT_VTX_SEL__FORCE_NAN_IF_CCOLOR_NAN     0x00000008
#define SE_TCL_OUTPUT_VTX_SEL__FORCE_INORDER_PROC_MASK     0x00000010
#define SE_TCL_OUTPUT_VTX_SEL__FORCE_INORDER_PROC          0x00000010
#define SE_TCL_OUTPUT_VTX_SEL__RSVD_1BIT_NUM0_MASK         0x00000020
#define SE_TCL_OUTPUT_VTX_SEL__RSVD_1BIT_NUM0              0x00000020
#define SE_TCL_OUTPUT_VTX_SEL__RSVD_3BIT_NUM0_MASK         0x000001c0
#define SE_TCL_OUTPUT_VTX_SEL__RSVD_3BIT_NUM1_MASK         0x00000e00
#define SE_TCL_OUTPUT_VTX_SEL__RSVD_4BIT_NUM0_MASK         0x0000f000
#define SE_TCL_OUTPUT_VTX_SEL__VTX_TEX0_SELECT_MASK        0x000f0000
#define SE_TCL_OUTPUT_VTX_SEL__VTX_TEX1_SELECT_MASK        0x00f00000
#define SE_TCL_OUTPUT_VTX_SEL__VTX_TEX2_SELECT_MASK        0x0f000000
#define SE_TCL_OUTPUT_VTX_SEL__VTX_TEX3_SELECT_MASK        0xf0000000

// SE_TCL_MATRIX_SELECT_0
#define SE_TCL_MATRIX_SELECT_0__MODELVIEW_MTX_0_SEL_MASK   0x0000000f
#define SE_TCL_MATRIX_SELECT_0__MODELVIEW_MTX_1_SEL_MASK   0x000000f0
#define SE_TCL_MATRIX_SELECT_0__MODELVIEW_MTX_2_SEL_MASK   0x00000f00
#define SE_TCL_MATRIX_SELECT_0__MODELVIEW_MTX_3_SEL_MASK   0x0000f000
#define SE_TCL_MATRIX_SELECT_0__IT_MODELVIEW_MTX_0_SEL_MASK 0x000f0000
#define SE_TCL_MATRIX_SELECT_0__IT_MODELVIEW_MTX_1_SEL_MASK 0x00f00000
#define SE_TCL_MATRIX_SELECT_0__IT_MODELVIEW_MTX_2_SEL_MASK 0x0f000000
#define SE_TCL_MATRIX_SELECT_0__IT_MODELVIEW_MTX_3_SEL_MASK 0xf0000000

// SE_TCL_MATRIX_SELECT_1
#define SE_TCL_MATRIX_SELECT_1__MODEL2CLIP_MTX_0_SEL_MASK  0x0000000f
#define SE_TCL_MATRIX_SELECT_1__MODEL2CLIP_MTX_1_SEL_MASK  0x000000f0
#define SE_TCL_MATRIX_SELECT_1__MODEL2CLIP_MTX_2_SEL_MASK  0x00000f00
#define SE_TCL_MATRIX_SELECT_1__MODEL2CLIP_MTX_3_SEL_MASK  0x0000f000
#define SE_TCL_MATRIX_SELECT_1__TEX_XFORM_MTX_0_SEL_MASK   0x000f0000
#define SE_TCL_MATRIX_SELECT_1__TEX_XFORM_MTX_1_SEL_MASK   0x00f00000
#define SE_TCL_MATRIX_SELECT_1__TEX_XFORM_MTX_2_SEL_MASK   0x0f000000
#define SE_TCL_MATRIX_SELECT_1__TEX_XFORM_MTX_3_SEL_MASK   0xf0000000

// SE_TCL_UCP_VERT_BLEND_CTL
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_IN_CLIP_SPACE_MASK  0x00000001
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_IN_CLIP_SPACE       0x00000001
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_IN_MODEL_SPACE_MASK 0x00000002
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_IN_MODEL_SPACE      0x00000002
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_0_MASK          0x00000004
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_0               0x00000004
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_1_MASK          0x00000008
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_1               0x00000008
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_2_MASK          0x00000010
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_2               0x00000010
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_3_MASK          0x00000020
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_3               0x00000020
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_4_MASK          0x00000040
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_4               0x00000040
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_5_MASK          0x00000080
#define SE_TCL_UCP_VERT_BLEND_CTL__UCP_ENA_5               0x00000080
#define SE_TCL_UCP_VERT_BLEND_CTL__FOG_MODE_MASK           0x00000300
#define SE_TCL_UCP_VERT_BLEND_CTL__RNG_BASED_FOG_MASK      0x00000400
#define SE_TCL_UCP_VERT_BLEND_CTL__RNG_BASED_FOG           0x00000400
#define SE_TCL_UCP_VERT_BLEND_CTL__TWO_SIDED_LIGHTING_ENA_MASK 0x00000800
#define SE_TCL_UCP_VERT_BLEND_CTL__TWO_SIDED_LIGHTING_ENA  0x00000800
#define SE_TCL_UCP_VERT_BLEND_CTL__BLEND_OP_CNT_MASK       0x00007000
#define SE_TCL_UCP_VERT_BLEND_CTL__USE_ST_BLEND_OP_CNT_MASK 0x00008000
#define SE_TCL_UCP_VERT_BLEND_CTL__USE_ST_BLEND_OP_CNT     0x00008000
#define SE_TCL_UCP_VERT_BLEND_CTL__POSITION_BLEND_OPERATION_MASK 0x00010000
#define SE_TCL_UCP_VERT_BLEND_CTL__POSITION_BLEND_OPERATION 0x00010000
#define SE_TCL_UCP_VERT_BLEND_CTL__NORMAL_BLEND_OPERATION_MASK 0x00020000
#define SE_TCL_UCP_VERT_BLEND_CTL__NORMAL_BLEND_OPERATION  0x00020000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_0_MASK 0x00040000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_0   0x00040000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_1_MASK 0x00080000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_1   0x00080000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_2_MASK 0x00100000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_2   0x00100000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_3_MASK 0x00200000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERTEX_BLEND_SOURCE_3   0x00200000
#define SE_TCL_UCP_VERT_BLEND_CTL__BLEND_WEIGHT_MINUS_ONE_MASK 0x00400000
#define SE_TCL_UCP_VERT_BLEND_CTL__BLEND_WEIGHT_MINUS_ONE  0x00400000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERT_BLEND_USE_PROJ_MTX_MASK 0x00800000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERT_BLEND_USE_PROJ_MTX 0x00800000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERT_BLEND_2_OPTIMIZE_MASK 0x01000000
#define SE_TCL_UCP_VERT_BLEND_CTL__VERT_BLEND_2_OPTIMIZE   0x01000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULL_EQ_0_ENA_MASK      0x02000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULL_EQ_0_ENA           0x02000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULL_METHOD_MASK        0x0c000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULLING_FF_DIR_MASK     0x10000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULLING_FF_DIR          0x10000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULL_FF_ENA_MASK        0x20000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULL_FF_ENA             0x20000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULL_BF_ENA_MASK        0x40000000
#define SE_TCL_UCP_VERT_BLEND_CTL__CULL_BF_ENA             0x40000000
#define SE_TCL_UCP_VERT_BLEND_CTL__FORCE_W_TO_ONE_MASK     0x80000000
#define SE_TCL_UCP_VERT_BLEND_CTL__FORCE_W_TO_ONE          0x80000000

// SE_TCL_TEXTURE_PROC_CTL
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_0_MASK    0x00000001
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_0         0x00000001
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_1_MASK    0x00000002
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_1         0x00000002
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_2_MASK    0x00000004
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_2         0x00000004
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_3_MASK    0x00000008
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_ENA_3         0x00000008
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_0_MASK      0x00000010
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_0           0x00000010
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_1_MASK      0x00000020
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_1           0x00000020
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_2_MASK      0x00000040
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_2           0x00000040
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_3_MASK      0x00000080
#define SE_TCL_TEXTURE_PROC_CTL__TEX_XFORM_ENA_3           0x00000080
#define SE_TCL_TEXTURE_PROC_CTL__RSVD_2BIT_NUM3_MASK       0x00000300
#define SE_TCL_TEXTURE_PROC_CTL__RSVD_2BIT_NUM4_MASK       0x00000c00
#define SE_TCL_TEXTURE_PROC_CTL__RSVD_2BIT_NUM5_MASK       0x00003000
#define SE_TCL_TEXTURE_PROC_CTL__RSVD_2BIT_NUM6_MASK       0x0000c000
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_SRC_0_MASK    0x000f0000
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_SRC_1_MASK    0x00f00000
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_SRC_2_MASK    0x0f000000
#define SE_TCL_TEXTURE_PROC_CTL__TEX_CS_PROC_SRC_3_MASK    0xf0000000

// SE_TCL_LIGHT_MODEL_CTL
#define SE_TCL_LIGHT_MODEL_CTL__LIGHTING_ENA_MASK          0x00000001
#define SE_TCL_LIGHT_MODEL_CTL__LIGHTING_ENA               0x00000001
#define SE_TCL_LIGHT_MODEL_CTL__LIGHTING_IN_MODEL_MASK     0x00000002
#define SE_TCL_LIGHT_MODEL_CTL__LIGHTING_IN_MODEL          0x00000002
#define SE_TCL_LIGHT_MODEL_CTL__LOCAL_VIEWER_MASK          0x00000004
#define SE_TCL_LIGHT_MODEL_CTL__LOCAL_VIEWER               0x00000004
#define SE_TCL_LIGHT_MODEL_CTL__NORMALIZE_NORMAL_MASK      0x00000008
#define SE_TCL_LIGHT_MODEL_CTL__NORMALIZE_NORMAL           0x00000008
#define SE_TCL_LIGHT_MODEL_CTL__RESCALE_NORMAL_MASK        0x00000010
#define SE_TCL_LIGHT_MODEL_CTL__RESCALE_NORMAL             0x00000010
#define SE_TCL_LIGHT_MODEL_CTL__SPECULAR_ENA_MASK          0x00000020
#define SE_TCL_LIGHT_MODEL_CTL__SPECULAR_ENA               0x00000020
#define SE_TCL_LIGHT_MODEL_CTL__DIFFUSE_SPECULAR_COMBINE_MASK 0x00000040
#define SE_TCL_LIGHT_MODEL_CTL__DIFFUSE_SPECULAR_COMBINE   0x00000040
#define SE_TCL_LIGHT_MODEL_CTL__ALPHA_LIGHTING_MASK        0x00000080
#define SE_TCL_LIGHT_MODEL_CTL__ALPHA_LIGHTING             0x00000080
#define SE_TCL_LIGHT_MODEL_CTL__LOC_LIGHT_W_SCALE_SUB_MASK 0x00000100
#define SE_TCL_LIGHT_MODEL_CTL__LOC_LIGHT_W_SCALE_SUB      0x00000100
#define SE_TCL_LIGHT_MODEL_CTL__NO_NORMAL_DO_AMB_ONLY_MASK 0x00000200
#define SE_TCL_LIGHT_MODEL_CTL__NO_NORMAL_DO_AMB_ONLY      0x00000200
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM0_MASK     0x00000400
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM0          0x00000400
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM1_MASK     0x00000800
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM1          0x00000800
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM2_MASK     0x00001000
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM2          0x00001000
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM3_MASK     0x00002000
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_1BIT_NUM3          0x00002000
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_2BIT_NUM0_MASK     0x0000c000
#define SE_TCL_LIGHT_MODEL_CTL__EMISSIVE_SOURCE_MASK       0x00030000
#define SE_TCL_LIGHT_MODEL_CTL__AMBIENT_SOURCE_MASK        0x000c0000
#define SE_TCL_LIGHT_MODEL_CTL__DIFFUSE_SOURCE_MASK        0x00300000
#define SE_TCL_LIGHT_MODEL_CTL__SPECULAR_SOURCE_MASK       0x00c00000
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_2BIT_NUM1_MASK     0x03000000
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_3BIT_NUM0_MASK     0x1c000000
#define SE_TCL_LIGHT_MODEL_CTL__RSVD_LT_3BIT_NUM1_MASK     0xe0000000

// SE_TCL_PER_LIGHT_CTL_0
#define SE_TCL_PER_LIGHT_CTL_0__LIGHT_ENA_0_MASK           0x00000001
#define SE_TCL_PER_LIGHT_CTL_0__LIGHT_ENA_0                0x00000001
#define SE_TCL_PER_LIGHT_CTL_0__AMBIENT_ENA_0_MASK         0x00000002
#define SE_TCL_PER_LIGHT_CTL_0__AMBIENT_ENA_0              0x00000002
#define SE_TCL_PER_LIGHT_CTL_0__SPECULAR_ENA_0_MASK        0x00000004
#define SE_TCL_PER_LIGHT_CTL_0__SPECULAR_ENA_0             0x00000004
#define SE_TCL_PER_LIGHT_CTL_0__LOCAL_LIGHT_0_MASK         0x00000008
#define SE_TCL_PER_LIGHT_CTL_0__LOCAL_LIGHT_0              0x00000008
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_ENA_0_MASK            0x00000010
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_ENA_0                 0x00000010
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_DUAL_CONE_0_MASK      0x00000020
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_DUAL_CONE_0           0x00000020
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_ENA_0_MASK         0x00000040
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_ENA_0              0x00000040
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_CONSTANT_ENA_0_MASK 0x00000080
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_CONSTANT_ENA_0     0x00000080
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM0_MASK    0x00000100
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM0         0x00000100
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM1_MASK    0x00000200
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM1         0x00000200
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM2_MASK    0x00000400
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM2         0x00000400
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM3_MASK    0x00000800
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_1BIT_NUM3         0x00000800
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_2BIT_NUM0_MASK    0x00003000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT0_2BIT_NUM1_MASK    0x0000c000
#define SE_TCL_PER_LIGHT_CTL_0__LIGHT_ENA_1_MASK           0x00010000
#define SE_TCL_PER_LIGHT_CTL_0__LIGHT_ENA_1                0x00010000
#define SE_TCL_PER_LIGHT_CTL_0__AMBIENT_ENA_1_MASK         0x00020000
#define SE_TCL_PER_LIGHT_CTL_0__AMBIENT_ENA_1              0x00020000
#define SE_TCL_PER_LIGHT_CTL_0__SPECULAR_ENA_1_MASK        0x00040000
#define SE_TCL_PER_LIGHT_CTL_0__SPECULAR_ENA_1             0x00040000
#define SE_TCL_PER_LIGHT_CTL_0__LOCAL_LIGHT_1_MASK         0x00080000
#define SE_TCL_PER_LIGHT_CTL_0__LOCAL_LIGHT_1              0x00080000
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_ENA_1_MASK            0x00100000
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_ENA_1                 0x00100000
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_DUAL_CONE_1_MASK      0x00200000
#define SE_TCL_PER_LIGHT_CTL_0__SPOT_DUAL_CONE_1           0x00200000
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_ENA_1_MASK         0x00400000
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_ENA_1              0x00400000
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_CONSTANT_ENA_1_MASK 0x00800000
#define SE_TCL_PER_LIGHT_CTL_0__RNG_ATT_CONSTANT_ENA_1     0x00800000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM0_MASK    0x01000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM0         0x01000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM1_MASK    0x02000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM1         0x02000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM2_MASK    0x04000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM2         0x04000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM3_MASK    0x08000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_1BIT_NUM3         0x08000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_2BIT_NUM0_MASK    0x30000000
#define SE_TCL_PER_LIGHT_CTL_0__RSVD_LT1_2BIT_NUM1_MASK    0xc0000000

// SE_TCL_PER_LIGHT_CTL_1
#define SE_TCL_PER_LIGHT_CTL_1__LIGHT_ENA_2_MASK           0x00000001
#define SE_TCL_PER_LIGHT_CTL_1__LIGHT_ENA_2                0x00000001
#define SE_TCL_PER_LIGHT_CTL_1__AMBIENT_ENA_2_MASK         0x00000002
#define SE_TCL_PER_LIGHT_CTL_1__AMBIENT_ENA_2              0x00000002
#define SE_TCL_PER_LIGHT_CTL_1__SPECULAR_ENA_2_MASK        0x00000004
#define SE_TCL_PER_LIGHT_CTL_1__SPECULAR_ENA_2             0x00000004
#define SE_TCL_PER_LIGHT_CTL_1__LOCAL_LIGHT_2_MASK         0x00000008
#define SE_TCL_PER_LIGHT_CTL_1__LOCAL_LIGHT_2              0x00000008
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_ENA_2_MASK            0x00000010
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_ENA_2                 0x00000010
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_DUAL_CONE_2_MASK      0x00000020
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_DUAL_CONE_2           0x00000020
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_ENA_2_MASK         0x00000040
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_ENA_2              0x00000040
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_CONSTANT_ENA_2_MASK 0x00000080
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_CONSTANT_ENA_2     0x00000080
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM0_MASK    0x00000100
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM0         0x00000100
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM1_MASK    0x00000200
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM1         0x00000200
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM2_MASK    0x00000400
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM2         0x00000400
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM3_MASK    0x00000800
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_1BIT_NUM3         0x00000800
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_2BIT_NUM0_MASK    0x00003000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT2_2BIT_NUM1_MASK    0x0000c000
#define SE_TCL_PER_LIGHT_CTL_1__LIGHT_ENA_3_MASK           0x00010000
#define SE_TCL_PER_LIGHT_CTL_1__LIGHT_ENA_3                0x00010000
#define SE_TCL_PER_LIGHT_CTL_1__AMBIENT_ENA_3_MASK         0x00020000
#define SE_TCL_PER_LIGHT_CTL_1__AMBIENT_ENA_3              0x00020000
#define SE_TCL_PER_LIGHT_CTL_1__SPECULAR_ENA_3_MASK        0x00040000
#define SE_TCL_PER_LIGHT_CTL_1__SPECULAR_ENA_3             0x00040000
#define SE_TCL_PER_LIGHT_CTL_1__LOCAL_LIGHT_3_MASK         0x00080000
#define SE_TCL_PER_LIGHT_CTL_1__LOCAL_LIGHT_3              0x00080000
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_ENA_3_MASK            0x00100000
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_ENA_3                 0x00100000
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_DUAL_CONE_3_MASK      0x00200000
#define SE_TCL_PER_LIGHT_CTL_1__SPOT_DUAL_CONE_3           0x00200000
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_ENA_3_MASK         0x00400000
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_ENA_3              0x00400000
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_CONSTANT_ENA_3_MASK 0x00800000
#define SE_TCL_PER_LIGHT_CTL_1__RNG_ATT_CONSTANT_ENA_3     0x00800000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM0_MASK    0x01000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM0         0x01000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM1_MASK    0x02000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM1         0x02000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM2_MASK    0x04000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM2         0x04000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM3_MASK    0x08000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_1BIT_NUM3         0x08000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_2BIT_NUM0_MASK    0x30000000
#define SE_TCL_PER_LIGHT_CTL_1__RSVD_LT3_2BIT_NUM1_MASK    0xc0000000

// SE_TCL_PER_LIGHT_CTL_2
#define SE_TCL_PER_LIGHT_CTL_2__LIGHT_ENA_4_MASK           0x00000001
#define SE_TCL_PER_LIGHT_CTL_2__LIGHT_ENA_4                0x00000001
#define SE_TCL_PER_LIGHT_CTL_2__AMBIENT_ENA_4_MASK         0x00000002
#define SE_TCL_PER_LIGHT_CTL_2__AMBIENT_ENA_4              0x00000002
#define SE_TCL_PER_LIGHT_CTL_2__SPECULAR_ENA_4_MASK        0x00000004
#define SE_TCL_PER_LIGHT_CTL_2__SPECULAR_ENA_4             0x00000004
#define SE_TCL_PER_LIGHT_CTL_2__LOCAL_LIGHT_4_MASK         0x00000008
#define SE_TCL_PER_LIGHT_CTL_2__LOCAL_LIGHT_4              0x00000008
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_ENA_4_MASK            0x00000010
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_ENA_4                 0x00000010
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_DUAL_CONE_4_MASK      0x00000020
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_DUAL_CONE_4           0x00000020
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_ENA_4_MASK         0x00000040
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_ENA_4              0x00000040
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_CONSTANT_ENA_4_MASK 0x00000080
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_CONSTANT_ENA_4     0x00000080
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM0_MASK    0x00000100
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM0         0x00000100
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM1_MASK    0x00000200
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM1         0x00000200
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM2_MASK    0x00000400
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM2         0x00000400
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM3_MASK    0x00000800
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_1BIT_NUM3         0x00000800
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_2BIT_NUM0_MASK    0x00003000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT4_2BIT_NUM1_MASK    0x0000c000
#define SE_TCL_PER_LIGHT_CTL_2__LIGHT_ENA_5_MASK           0x00010000
#define SE_TCL_PER_LIGHT_CTL_2__LIGHT_ENA_5                0x00010000
#define SE_TCL_PER_LIGHT_CTL_2__AMBIENT_ENA_5_MASK         0x00020000
#define SE_TCL_PER_LIGHT_CTL_2__AMBIENT_ENA_5              0x00020000
#define SE_TCL_PER_LIGHT_CTL_2__SPECULAR_ENA_5_MASK        0x00040000
#define SE_TCL_PER_LIGHT_CTL_2__SPECULAR_ENA_5             0x00040000
#define SE_TCL_PER_LIGHT_CTL_2__LOCAL_LIGHT_5_MASK         0x00080000
#define SE_TCL_PER_LIGHT_CTL_2__LOCAL_LIGHT_5              0x00080000
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_ENA_5_MASK            0x00100000
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_ENA_5                 0x00100000
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_DUAL_CONE_5_MASK      0x00200000
#define SE_TCL_PER_LIGHT_CTL_2__SPOT_DUAL_CONE_5           0x00200000
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_ENA_5_MASK         0x00400000
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_ENA_5              0x00400000
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_CONSTANT_ENA_5_MASK 0x00800000
#define SE_TCL_PER_LIGHT_CTL_2__RNG_ATT_CONSTANT_ENA_5     0x00800000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM0_MASK    0x01000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM0         0x01000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM1_MASK    0x02000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM1         0x02000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM2_MASK    0x04000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM2         0x04000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM3_MASK    0x08000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_1BIT_NUM3         0x08000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_2BIT_NUM0_MASK    0x30000000
#define SE_TCL_PER_LIGHT_CTL_2__RSVD_LT5_2BIT_NUM1_MASK    0xc0000000

// SE_TCL_PER_LIGHT_CTL_3
#define SE_TCL_PER_LIGHT_CTL_3__LIGHT_ENA_6_MASK           0x00000001
#define SE_TCL_PER_LIGHT_CTL_3__LIGHT_ENA_6                0x00000001
#define SE_TCL_PER_LIGHT_CTL_3__AMBIENT_ENA_6_MASK         0x00000002
#define SE_TCL_PER_LIGHT_CTL_3__AMBIENT_ENA_6              0x00000002
#define SE_TCL_PER_LIGHT_CTL_3__SPECULAR_ENA_6_MASK        0x00000004
#define SE_TCL_PER_LIGHT_CTL_3__SPECULAR_ENA_6             0x00000004
#define SE_TCL_PER_LIGHT_CTL_3__LOCAL_LIGHT_6_MASK         0x00000008
#define SE_TCL_PER_LIGHT_CTL_3__LOCAL_LIGHT_6              0x00000008
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_ENA_6_MASK            0x00000010
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_ENA_6                 0x00000010
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_DUAL_CONE_6_MASK      0x00000020
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_DUAL_CONE_6           0x00000020
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_ENA_6_MASK         0x00000040
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_ENA_6              0x00000040
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_CONSTANT_ENA_6_MASK 0x00000080
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_CONSTANT_ENA_6     0x00000080
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM0_MASK    0x00000100
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM0         0x00000100
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM1_MASK    0x00000200
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM1         0x00000200
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM2_MASK    0x00000400
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM2         0x00000400
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM3_MASK    0x00000800
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_1BIT_NUM3         0x00000800
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_2BIT_NUM0_MASK    0x00003000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT6_2BIT_NUM1_MASK    0x0000c000
#define SE_TCL_PER_LIGHT_CTL_3__LIGHT_ENA_7_MASK           0x00010000
#define SE_TCL_PER_LIGHT_CTL_3__LIGHT_ENA_7                0x00010000
#define SE_TCL_PER_LIGHT_CTL_3__AMBIENT_ENA_7_MASK         0x00020000
#define SE_TCL_PER_LIGHT_CTL_3__AMBIENT_ENA_7              0x00020000
#define SE_TCL_PER_LIGHT_CTL_3__SPECULAR_ENA_7_MASK        0x00040000
#define SE_TCL_PER_LIGHT_CTL_3__SPECULAR_ENA_7             0x00040000
#define SE_TCL_PER_LIGHT_CTL_3__LOCAL_LIGHT_7_MASK         0x00080000
#define SE_TCL_PER_LIGHT_CTL_3__LOCAL_LIGHT_7              0x00080000
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_ENA_7_MASK            0x00100000
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_ENA_7                 0x00100000
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_DUAL_CONE_7_MASK      0x00200000
#define SE_TCL_PER_LIGHT_CTL_3__SPOT_DUAL_CONE_7           0x00200000
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_ENA_7_MASK         0x00400000
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_ENA_7              0x00400000
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_CONSTANT_ENA_7_MASK 0x00800000
#define SE_TCL_PER_LIGHT_CTL_3__RNG_ATT_CONSTANT_ENA_7     0x00800000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM0_MASK    0x01000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM0         0x01000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM1_MASK    0x02000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM1         0x02000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM2_MASK    0x04000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM2         0x04000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM3_MASK    0x08000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_1BIT_NUM3         0x08000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_2BIT_NUM0_MASK    0x30000000
#define SE_TCL_PER_LIGHT_CTL_3__RSVD_LT7_2BIT_NUM1_MASK    0xc0000000

// SE_TCL_DEBUG
#define SE_TCL_DEBUG__TCL_DEBUG_MASK                       0xffffffff

// SE_TCL_PERF_CNTL
#define SE_TCL_PERF_CNTL__TCL_PERFSEL1_MASK                0x0000000f
#define SE_TCL_PERF_CNTL__TCL_CLR_PERF1_MASK               0x00000040
#define SE_TCL_PERF_CNTL__TCL_CLR_PERF1                    0x00000040
#define SE_TCL_PERF_CNTL__TCL_EN_PERF1_MASK                0x00000080
#define SE_TCL_PERF_CNTL__TCL_EN_PERF1                     0x00000080
#define SE_TCL_PERF_CNTL__TCL_PERFSEL2_MASK                0x00000f00
#define SE_TCL_PERF_CNTL__TCL_CLR_PERF2_MASK               0x00004000
#define SE_TCL_PERF_CNTL__TCL_CLR_PERF2                    0x00004000
#define SE_TCL_PERF_CNTL__TCL_EN_PERF2_MASK                0x00008000
#define SE_TCL_PERF_CNTL__TCL_EN_PERF2                     0x00008000
#define SE_TCL_PERF_CNTL__TCL_SYNC_SELECT_MASK             0x00030000
#define SE_TCL_PERF_CNTL__RSVD_PERF_BITS_MASK              0xfffc0000

// SE_TCL_PERF_COUNT1
#define SE_TCL_PERF_COUNT1__TCL_PERF_COUNT1_MASK           0xffffffff

// SE_TCL_PERF_COUNT2
#define SE_TCL_PERF_COUNT2__TCL_PERF_COUNT2_MASK           0xffffffff

// SE_TCL_FPU_LATENCY
#define SE_TCL_FPU_LATENCY__VE_ENG_LATENCY_MASK            0x0000001f
#define SE_TCL_FPU_LATENCY__VE_MULT_LATENCY_MASK           0x000003e0
#define SE_TCL_FPU_LATENCY__VE_ACCUM_LATENCY_MASK          0x00007c00
#define SE_TCL_FPU_LATENCY__SC_ENG_LATENCY_MASK            0x000f8000
#define SE_TCL_FPU_LATENCY__VE_OUT_LATENCY_MASK            0x01f00000
#define SE_TCL_FPU_LATENCY__SC_MATH_LATENCY_MASK           0x3e000000
#define SE_TCL_FPU_LATENCY__RSVD_LTNCY_2BIT_MASK           0xc0000000

// RE_STIPPLE_ADDR
#define RE_STIPPLE_ADDR__STIPPLE_ADDR_MASK                 0x0000001f

// RE_STIPPLE_DATA
#define RE_STIPPLE_DATA__STIPPLE_DATA_MASK                 0xffffffff

// RE_MISC
#define RE_MISC__STIPPLE_X_OFFSET_MASK                     0x0000001f
#define RE_MISC__STIPPLE_Y_OFFSET_MASK                     0x00001f00
#define RE_MISC__STIPPLE_BIT_ORDER_MASK                    0x00010000
#define RE_MISC__STIPPLE_BIT_ORDER                         0x00010000

// RE_SOLID_COLOR
#define RE_SOLID_COLOR__SOLID_COLOR_MASK                   0xffffffff

// RE_WIDTH_HEIGHT
#define RE_WIDTH_HEIGHT__WIDTH_MASK                        0x000007ff
#define RE_WIDTH_HEIGHT__HEIGHT_MASK                       0x07ff0000

// RE_TOP_LEFT
#define RE_TOP_LEFT__X_LEFT_MASK                           0x000007ff
#define RE_TOP_LEFT__Y_TOP_MASK                            0x07ff0000

// RE_SCISSOR_TL_0
#define RE_SCISSOR_TL_0__X_LEFT_MASK                       0x000007ff
#define RE_SCISSOR_TL_0__Y_TOP_MASK                        0x07ff0000

// RE_SCISSOR_BR_0
#define RE_SCISSOR_BR_0__X_RIGHT_MASK                      0x000007ff
#define RE_SCISSOR_BR_0__Y_BOTTOM_MASK                     0x07ff0000

// RE_SCISSOR_TL_1
#define RE_SCISSOR_TL_1__X_LEFT_MASK                       0x000007ff
#define RE_SCISSOR_TL_1__Y_TOP_MASK                        0x07ff0000

// RE_SCISSOR_BR_1
#define RE_SCISSOR_BR_1__X_RIGHT_MASK                      0x000007ff
#define RE_SCISSOR_BR_1__Y_BOTTOM_MASK                     0x07ff0000

// RE_SCISSOR_TL_2
#define RE_SCISSOR_TL_2__X_LEFT_MASK                       0x000007ff
#define RE_SCISSOR_TL_2__Y_TOP_MASK                        0x07ff0000

// RE_SCISSOR_BR_2
#define RE_SCISSOR_BR_2__X_RIGHT_MASK                      0x000007ff
#define RE_SCISSOR_BR_2__Y_BOTTOM_MASK                     0x07ff0000

// RE_AUX_SCISSOR_CNTL
#define RE_AUX_SCISSOR_CNTL__EXCLUSIVE_SCISSOR_0_MASK      0x01000000
#define RE_AUX_SCISSOR_CNTL__EXCLUSIVE_SCISSOR_0           0x01000000
#define RE_AUX_SCISSOR_CNTL__EXCLUSIVE_SCISSOR_1_MASK      0x02000000
#define RE_AUX_SCISSOR_CNTL__EXCLUSIVE_SCISSOR_1           0x02000000
#define RE_AUX_SCISSOR_CNTL__EXCLUSIVE_SCISSOR_2_MASK      0x04000000
#define RE_AUX_SCISSOR_CNTL__EXCLUSIVE_SCISSOR_2           0x04000000
#define RE_AUX_SCISSOR_CNTL__SCISSOR_ENABLE_0_MASK         0x10000000
#define RE_AUX_SCISSOR_CNTL__SCISSOR_ENABLE_0              0x10000000
#define RE_AUX_SCISSOR_CNTL__SCISSOR_ENABLE_1_MASK         0x20000000
#define RE_AUX_SCISSOR_CNTL__SCISSOR_ENABLE_1              0x20000000
#define RE_AUX_SCISSOR_CNTL__SCISSOR_ENABLE_2_MASK         0x40000000
#define RE_AUX_SCISSOR_CNTL__SCISSOR_ENABLE_2              0x40000000

// RE_LINE_PATTERN
#define RE_LINE_PATTERN__LINE_PATTERN_MASK                 0x0000ffff
#define RE_LINE_PATTERN__REPEAT_COUNT_MASK                 0x00ff0000
#define RE_LINE_PATTERN__PATTERN_START_MASK                0x0f000000
#define RE_LINE_PATTERN__PATTERN_BIT_ORDER_MASK            0x10000000
#define RE_LINE_PATTERN__PATTERN_BIT_ORDER                 0x10000000
#define RE_LINE_PATTERN__AUTO_RESET_ENABLE_MASK            0x20000000
#define RE_LINE_PATTERN__AUTO_RESET_ENABLE                 0x20000000

// RE_LINE_STATE
#define RE_LINE_STATE__CURRENT_PTR_MASK                    0x0000000f
#define RE_LINE_STATE__CURRENT_COUNT_MASK                  0x0000ff00

// RE_PIX_COUNT
#define RE_PIX_COUNT__RE_PIX_COUNT_MASK                    0xffffffff

// RE_PAIR_COUNT
#define RE_PAIR_COUNT__RE_PAIR_COUNT_MASK                  0xffffffff

// RE_CYC_COUNT
#define RE_CYC_COUNT__RE_CYC_COUNT_MASK                    0xffffffff

// RE_OUTSIDE
#define RE_OUTSIDE__RE_OUTSIDE_MASK                        0xffffffff

// RE_STALLED
#define RE_STALLED__RE_STALLED_MASK                        0xffffffff

// RE_PERF
#define RE_PERF__BLOCK_DISABLE_MASK                        0x00000010
#define RE_PERF__BLOCK_DISABLE                             0x00000010
#define RE_PERF__DUAL_PIXEL_DISABLE_MASK                   0x00000020
#define RE_PERF__DUAL_PIXEL_DISABLE                        0x00000020
#define RE_PERF__HIEARCHICAL_Z_OVERRIDE_MASK               0x00000040
#define RE_PERF__HIEARCHICAL_Z_OVERRIDE                    0x00000040

// RE_DEBUG_0
#define RE_DEBUG_0__RE_DWORD_MASK                          0xffffffff

// RE_DEBUG_1
#define RE_DEBUG_1__RE_DWORD_MASK                          0xffffffff

// RE_DEBUG_2
#define RE_DEBUG_2__RE_DWORD_MASK                          0xffffffff

// RE_DEBUG_3
#define RE_DEBUG_3__RE_DWORD_MASK                          0xffffffff

// RE_DEBUG_4
#define RE_DEBUG_4__RE_DWORD_MASK                          0xffffffff

// RE_DEBUG_5
#define RE_DEBUG_5__RE_DWORD_MASK                          0xffffffff

// RE_DEBUG_6
#define RE_DEBUG_6__RE_DWORD_MASK                          0xffffffff

// RE_DEBUG_7
#define RE_DEBUG_7__RE_DWORD_MASK                          0xffffffff

// RE_E2_0
#define RE_E2_0__XSTART_MASK                               0x00000fff
#define RE_E2_0__YSTART_MASK                               0x00fff000
#define RE_E2_0__XEND_LO_MASK                              0xff000000

// RE_E2_1
#define RE_E2_1__XEND_HI_MASK                              0x0000000f
#define RE_E2_1__YEND_MASK                                 0x0000fff0
#define RE_E2_1__XDIR_MASK                                 0x00010000
#define RE_E2_1__XDIR                                      0x00010000
#define RE_E2_1__YDIR_MASK                                 0x00020000
#define RE_E2_1__YDIR                                      0x00020000
#define RE_E2_1__XMAJ_MASK                                 0x00040000
#define RE_E2_1__XMAJ                                      0x00040000
#define RE_E2_1__XOFFSET_MASK                              0x00f80000
#define RE_E2_1__OPCODE_MASK                               0x03000000
#define RE_E2_1__SUB_PRIM_MASK                             0x08000000
#define RE_E2_1__SUB_PRIM                                  0x08000000
#define RE_E2_1__END_PATTERN_MASK                          0x10000000
#define RE_E2_1__END_PATTERN                               0x10000000
#define RE_E2_1__DYE2_LO_MASK                              0xe0000000

// RE_E2_2
#define RE_E2_2__DYE2_HI_MASK                              0x00003fff
#define RE_E2_2__DXE2_MASK                                 0x7fffc000
#define RE_E2_2__E2_LO_MASK                                0x80000000
#define RE_E2_2__E2_LO                                     0x80000000

// RE_E2_3
#define RE_E2_3__E2_HI_MASK                                0xffffffff

// RE_E0E1_0
#define RE_E0E1_0__DYE1_MASK                               0x0001ffff
#define RE_E0E1_0__DXE1_LO_MASK                            0xfffe0000

// RE_E0E1_1
#define RE_E0E1_1__DXE1_HI_MASK                            0x00000003
#define RE_E0E1_1__E1_MASK                                 0xfffffffc

// RE_E0E1_2
#define RE_E0E1_2__DYE0_MASK                               0x0001ffff
#define RE_E0E1_2__DXE0_LO_MASK                            0xfffe0000

// RE_E0E1_3
#define RE_E0E1_3__DXE0_HI_MASK                            0x00000003
#define RE_E0E1_3__E0_MASK                                 0xfffffffc

// RE_NULL_PRIM

// RE_Z_MINMAX_0
#define RE_Z_MINMAX_0__Z_MINMAX_LO_MASK                    0xffffffff

// RE_Z_MINMAX_1
#define RE_Z_MINMAX_1__Z_MINMAX_HI_MASK                    0x000001ff

// RE_Z_MINMAX_3
#define RE_Z_MINMAX_3__ZEXPO_MASK                          0xff000000

// RE_DDA_Z_0
#define RE_DDA_Z_0__ZSTART_LO_MASK                         0xffffffff

// RE_DDA_Z_1
#define RE_DDA_Z_1__ZSTART_H_MASK                          0x000001ff
#define RE_DDA_Z_1__DZDX_L_MASK                            0xfffffe00

// RE_DDA_Z_2
#define RE_DDA_Z_2__DZDX_H_MASK                            0x0000ffff
#define RE_DDA_Z_2__DZDY_L_MASK                            0xffff0000

// RE_DDA_Z_3
#define RE_DDA_Z_3__DZDY_H_MASK                            0x007fffff
#define RE_DDA_Z_3__ZEXPO_MASK                             0xff000000

// RE_DDA_RHW_0
#define RE_DDA_RHW_0__RHWSTART_LO_MASK                     0xffffffff

// RE_DDA_RHW_1
#define RE_DDA_RHW_1__RHWSTART_H_MASK                      0x000001ff
#define RE_DDA_RHW_1__DRHWDX_L_MASK                        0xfffffe00

// RE_DDA_RHW_2
#define RE_DDA_RHW_2__DRHWDX_H_MASK                        0x0000ffff
#define RE_DDA_RHW_2__DRHWDY_L_MASK                        0xffff0000

// RE_DDA_RHW_3
#define RE_DDA_RHW_3__DRHWDY_H_MASK                        0x007fffff
#define RE_DDA_RHW_3__RHWEXPO_MASK                         0xff000000

// RE_DDA_A_0
#define RE_DDA_A_0__ASTART_MASK                            0x01ffffff

// RE_DDA_A_1
#define RE_DDA_A_1__DADX_MASK                              0x01ffffff

// RE_DDA_A_2
#define RE_DDA_A_2__DADY_MASK                              0x01ffffff

// RE_DDA_R_0
#define RE_DDA_R_0__RSTART_MASK                            0x01ffffff

// RE_DDA_R_1
#define RE_DDA_R_1__DRDX_MASK                              0x01ffffff

// RE_DDA_R_2
#define RE_DDA_R_2__DRDY_MASK                              0x01ffffff

// RE_DDA_G_0
#define RE_DDA_G_0__GSTART_MASK                            0x01ffffff

// RE_DDA_G_1
#define RE_DDA_G_1__DGDX_MASK                              0x01ffffff

// RE_DDA_G_2
#define RE_DDA_G_2__DGDY_MASK                              0x01ffffff

// RE_DDA_B_0
#define RE_DDA_B_0__BSTART_MASK                            0x01ffffff

// RE_DDA_B_1
#define RE_DDA_B_1__DBDX_MASK                              0x01ffffff

// RE_DDA_B_2
#define RE_DDA_B_2__DBDY_MASK                              0x01ffffff

// RE_DDA_SA_0
#define RE_DDA_SA_0__SASTART_MASK                          0x01ffffff

// RE_DDA_SA_1
#define RE_DDA_SA_1__DSADX_MASK                            0x01ffffff

// RE_DDA_SA_2
#define RE_DDA_SA_2__DSADY_MASK                            0x01ffffff

// RE_DDA_SR_0
#define RE_DDA_SR_0__SRSTART_MASK                          0x01ffffff

// RE_DDA_SR_1
#define RE_DDA_SR_1__DSRDX_MASK                            0x01ffffff

// RE_DDA_SR_2
#define RE_DDA_SR_2__DSRDY_MASK                            0x01ffffff

// RE_DDA_SG_0
#define RE_DDA_SG_0__SGSTART_MASK                          0x01ffffff

// RE_DDA_SG_1
#define RE_DDA_SG_1__DSGDX_MASK                            0x01ffffff

// RE_DDA_SG_2
#define RE_DDA_SG_2__DSGDY_MASK                            0x01ffffff

// RE_DDA_SB_0
#define RE_DDA_SB_0__SBSTART_MASK                          0x01ffffff

// RE_DDA_SB_1
#define RE_DDA_SB_1__DSBDX_MASK                            0x01ffffff

// RE_DDA_SB_2
#define RE_DDA_SB_2__DSBDY_MASK                            0x01ffffff

// RE_DDA_S0_0
#define RE_DDA_S0_0__S0START_L_MASK                        0xffffffff

// RE_DDA_S0_1
#define RE_DDA_S0_1__S0START_H_MASK                        0x000001ff
#define RE_DDA_S0_1__DS0DX_L_MASK                          0xfffffe00

// RE_DDA_S0_2
#define RE_DDA_S0_2__DS0DX_H_MASK                          0x0000ffff
#define RE_DDA_S0_2__DS0DY_L_MASK                          0xffff0000

// RE_DDA_S0_3
#define RE_DDA_S0_3__DS0DY_H_MASK                          0x007fffff
#define RE_DDA_S0_3__NON_PARAMETRIC_MASK                   0x00800000
#define RE_DDA_S0_3__NON_PARAMETRIC                        0x00800000
#define RE_DDA_S0_3__S0EXPO_MASK                           0xff000000

// RE_DDA_T0_0
#define RE_DDA_T0_0__T0START_L_MASK                        0xffffffff

// RE_DDA_T0_1
#define RE_DDA_T0_1__T0START_H_MASK                        0x000001ff
#define RE_DDA_T0_1__DT0DX_L_MASK                          0xfffffe00

// RE_DDA_T0_2
#define RE_DDA_T0_2__DT0DX_H_MASK                          0x0000ffff
#define RE_DDA_T0_2__DT0DY_L_MASK                          0xffff0000

// RE_DDA_T0_3
#define RE_DDA_T0_3__DT0DY_H_MASK                          0x007fffff
#define RE_DDA_T0_3__T0EXPO_MASK                           0xff000000

// RE_DDA_Q0_0
#define RE_DDA_Q0_0__Q0START_L_MASK                        0xffffffff

// RE_DDA_Q0_1
#define RE_DDA_Q0_1__Q0START_H_MASK                        0x000001ff
#define RE_DDA_Q0_1__DQ0DX_L_MASK                          0xfffffe00

// RE_DDA_Q0_2
#define RE_DDA_Q0_2__DQ0DX_H_MASK                          0x0000ffff
#define RE_DDA_Q0_2__DQ0DY_L_MASK                          0xffff0000

// RE_DDA_Q0_3
#define RE_DDA_Q0_3__DQ0DY_H_MASK                          0x007fffff
#define RE_DDA_Q0_3__Q0EXPO_MASK                           0xff000000

// RE_DDA_S1_0
#define RE_DDA_S1_0__S1START_L_MASK                        0xffffffff

// RE_DDA_S1_1
#define RE_DDA_S1_1__S1START_H_MASK                        0x000001ff
#define RE_DDA_S1_1__DS1DX_L_MASK                          0xfffffe00

// RE_DDA_S1_2
#define RE_DDA_S1_2__DS1DX_H_MASK                          0x0000ffff
#define RE_DDA_S1_2__DS1DY_L_MASK                          0xffff0000

// RE_DDA_S1_3
#define RE_DDA_S1_3__DS1DY_H_MASK                          0x007fffff
#define RE_DDA_S1_3__NON_PARAMETRIC_MASK                   0x00800000
#define RE_DDA_S1_3__NON_PARAMETRIC                        0x00800000
#define RE_DDA_S1_3__S1EXPO_MASK                           0xff000000

// RE_DDA_T1_0
#define RE_DDA_T1_0__T1START_L_MASK                        0xffffffff

// RE_DDA_T1_1
#define RE_DDA_T1_1__T1START_H_MASK                        0x000001ff
#define RE_DDA_T1_1__DT1DX_L_MASK                          0xfffffe00

// RE_DDA_T1_2
#define RE_DDA_T1_2__DT1DX_H_MASK                          0x0000ffff
#define RE_DDA_T1_2__DT1DY_L_MASK                          0xffff0000

// RE_DDA_T1_3
#define RE_DDA_T1_3__DT1DY_H_MASK                          0x007fffff
#define RE_DDA_T1_3__T1EXPO_MASK                           0xff000000

// RE_DDA_Q1_0
#define RE_DDA_Q1_0__Q1START_L_MASK                        0xffffffff

// RE_DDA_Q1_1
#define RE_DDA_Q1_1__Q1START_H_MASK                        0x000001ff
#define RE_DDA_Q1_1__DQ1DX_L_MASK                          0xfffffe00

// RE_DDA_Q1_2
#define RE_DDA_Q1_2__DQ1DX_H_MASK                          0x0000ffff
#define RE_DDA_Q1_2__DQ1DY_L_MASK                          0xffff0000

// RE_DDA_Q1_3
#define RE_DDA_Q1_3__DQ1DY_H_MASK                          0x007fffff
#define RE_DDA_Q1_3__Q1EXPO_MASK                           0xff000000

// RE_DDA_S2_0
#define RE_DDA_S2_0__S2START_L_MASK                        0xffffffff

// RE_DDA_S2_1
#define RE_DDA_S2_1__S2START_H_MASK                        0x000001ff
#define RE_DDA_S2_1__DS2DX_L_MASK                          0xfffffe00

// RE_DDA_S2_2
#define RE_DDA_S2_2__DS2DX_H_MASK                          0x0000ffff
#define RE_DDA_S2_2__DS2DY_L_MASK                          0xffff0000

// RE_DDA_S2_3
#define RE_DDA_S2_3__DS2DY_H_MASK                          0x007fffff
#define RE_DDA_S2_3__NON_PARAMETRIC_MASK                   0x00800000
#define RE_DDA_S2_3__NON_PARAMETRIC                        0x00800000
#define RE_DDA_S2_3__S2EXPO_MASK                           0xff000000

// RE_DDA_T2_0
#define RE_DDA_T2_0__T2START_L_MASK                        0xffffffff

// RE_DDA_T2_1
#define RE_DDA_T2_1__T2START_H_MASK                        0x000001ff
#define RE_DDA_T2_1__DT2DX_L_MASK                          0xfffffe00

// RE_DDA_T2_2
#define RE_DDA_T2_2__DT2DX_H_MASK                          0x0000ffff
#define RE_DDA_T2_2__DT2DY_L_MASK                          0xffff0000

// RE_DDA_T2_3
#define RE_DDA_T2_3__DT2DY_H_MASK                          0x007fffff
#define RE_DDA_T2_3__T2EXPO_MASK                           0xff000000

// RE_DDA_Q2_0
#define RE_DDA_Q2_0__Q2START_L_MASK                        0xffffffff

// RE_DDA_Q2_1
#define RE_DDA_Q2_1__Q2START_H_MASK                        0x000001ff
#define RE_DDA_Q2_1__DQ2DX_L_MASK                          0xfffffe00

// RE_DDA_Q2_2
#define RE_DDA_Q2_2__DQ2DX_H_MASK                          0x0000ffff
#define RE_DDA_Q2_2__DQ2DY_L_MASK                          0xffff0000

// RE_DDA_Q2_3
#define RE_DDA_Q2_3__DQ2DY_H_MASK                          0x007fffff
#define RE_DDA_Q2_3__Q2EXPO_MASK                           0xff000000

// RE_DDA_S3_0
#define RE_DDA_S3_0__S3START_L_MASK                        0xffffffff

// RE_DDA_S3_1
#define RE_DDA_S3_1__S3START_H_MASK                        0x000001ff
#define RE_DDA_S3_1__DS3DX_L_MASK                          0xfffffe00

// RE_DDA_S3_2
#define RE_DDA_S3_2__DS3DX_H_MASK                          0x0000ffff
#define RE_DDA_S3_2__DS3DY_L_MASK                          0xffff0000

// RE_DDA_S3_3
#define RE_DDA_S3_3__DS3DY_H_MASK                          0x007fffff
#define RE_DDA_S3_3__NON_PARAMETRIC_MASK                   0x00800000
#define RE_DDA_S3_3__NON_PARAMETRIC                        0x00800000
#define RE_DDA_S3_3__S3EXPO_MASK                           0xff000000

// RE_DDA_T3_0
#define RE_DDA_T3_0__T3START_L_MASK                        0xffffffff

// RE_DDA_T3_1
#define RE_DDA_T3_1__T3START_H_MASK                        0x000001ff
#define RE_DDA_T3_1__DT3DX_L_MASK                          0xfffffe00

// RE_DDA_T3_2
#define RE_DDA_T3_2__DT3DX_H_MASK                          0x0000ffff
#define RE_DDA_T3_2__DT3DY_L_MASK                          0xffff0000

// RE_DDA_T3_3
#define RE_DDA_T3_3__DT3DY_H_MASK                          0x007fffff
#define RE_DDA_T3_3__T3EXPO_MASK                           0xff000000

// RE_DDA_Q3_0
#define RE_DDA_Q3_0__Q3START_L_MASK                        0xffffffff

// RE_DDA_Q3_1
#define RE_DDA_Q3_1__Q3START_H_MASK                        0x000001ff
#define RE_DDA_Q3_1__DQ3DX_L_MASK                          0xfffffe00

// RE_DDA_Q3_2
#define RE_DDA_Q3_2__DQ3DX_H_MASK                          0x0000ffff
#define RE_DDA_Q3_2__DQ3DY_L_MASK                          0xffff0000

// RE_DDA_Q3_3
#define RE_DDA_Q3_3__DQ3DY_H_MASK                          0x007fffff
#define RE_DDA_Q3_3__Q3EXPO_MASK                           0xff000000

// RE_DDA_DS0_0
#define RE_DDA_DS0_0__DS0XSTART_MASK                       0xffffffff

// RE_DDA_DS0_1
#define RE_DDA_DS0_1__DS0YSTART_MASK                       0xffffffff

// RE_DDA_DS0_2
#define RE_DDA_DS0_2__DS0INC_MASK                          0xffffffff

// RE_DDA_DS0_3
#define RE_DDA_DS0_3__DS0EXPO_MASK                         0xff000000

// RE_DDA_DT0_0
#define RE_DDA_DT0_0__DT0XSTART_MASK                       0xffffffff

// RE_DDA_DT0_1
#define RE_DDA_DT0_1__DT0YSTART_MASK                       0xffffffff

// RE_DDA_DT0_2
#define RE_DDA_DT0_2__DT0INC_MASK                          0xffffffff

// RE_DDA_DT0_3
#define RE_DDA_DT0_3__DT0EXPO_MASK                         0xff000000

// RE_DDA_DS1_0
#define RE_DDA_DS1_0__DS1XSTART_MASK                       0xffffffff

// RE_DDA_DS1_1
#define RE_DDA_DS1_1__DS1YSTART_MASK                       0xffffffff

// RE_DDA_DS1_2
#define RE_DDA_DS1_2__DS1INC_MASK                          0xffffffff

// RE_DDA_DS1_3
#define RE_DDA_DS1_3__DS1EXPO_MASK                         0xff000000

// RE_DDA_DT1_0
#define RE_DDA_DT1_0__DT1XSTART_MASK                       0xffffffff

// RE_DDA_DT1_1
#define RE_DDA_DT1_1__DT1YSTART_MASK                       0xffffffff

// RE_DDA_DT1_2
#define RE_DDA_DT1_2__DT1INC_MASK                          0xffffffff

// RE_DDA_DT1_3
#define RE_DDA_DT1_3__DT1EXPO_MASK                         0xff000000

// RE_DDA_DS2_0
#define RE_DDA_DS2_0__DS2XSTART_MASK                       0xffffffff

// RE_DDA_DS2_1
#define RE_DDA_DS2_1__DS2YSTART_MASK                       0xffffffff

// RE_DDA_DS2_2
#define RE_DDA_DS2_2__DS2INC_MASK                          0xffffffff

// RE_DDA_DS2_3
#define RE_DDA_DS2_3__DS2EXPO_MASK                         0xff000000

// RE_DDA_DT2_0
#define RE_DDA_DT2_0__DT2XSTART_MASK                       0xffffffff

// RE_DDA_DT2_1
#define RE_DDA_DT2_1__DT2YSTART_MASK                       0xffffffff

// RE_DDA_DT2_2
#define RE_DDA_DT2_2__DT2INC_MASK                          0xffffffff

// RE_DDA_DT2_3
#define RE_DDA_DT2_3__DT2EXPO_MASK                         0xff000000

// RE_DDA_DS3_0
#define RE_DDA_DS3_0__DS3XSTART_MASK                       0xffffffff

// RE_DDA_DS3_1
#define RE_DDA_DS3_1__DS3YSTART_MASK                       0xffffffff

// RE_DDA_DS3_2
#define RE_DDA_DS3_2__DS3INC_MASK                          0xffffffff

// RE_DDA_DS3_3
#define RE_DDA_DS3_3__DS3EXPO_MASK                         0xff000000

// RE_DDA_DT3_0
#define RE_DDA_DT3_0__DT3XSTART_MASK                       0xffffffff

// RE_DDA_DT3_1
#define RE_DDA_DT3_1__DT3YSTART_MASK                       0xffffffff

// RE_DDA_DT3_2
#define RE_DDA_DT3_2__DT3INC_MASK                          0xffffffff

// RE_DDA_DT3_3
#define RE_DDA_DT3_3__DT3EXPO_MASK                         0xff000000

// PP_MC_CONTEXT
#define PP_MC_CONTEXT__MC_BUF_BASE_MASK                    0x000000ff
#define PP_MC_CONTEXT__SRC1_INDEX_MASK                     0x00000f00
#define PP_MC_CONTEXT__SRC2_INDEX_MASK                     0x00007000
#define PP_MC_CONTEXT__MC_FUNC_MASK                        0x00038000
#define PP_MC_CONTEXT__DST_PITCH_MUL_MASK                  0x000c0000
#define PP_MC_CONTEXT__SRC_2_PITCH_MUL_MASK                0x00300000
#define PP_MC_CONTEXT__SRC_1_PITCH_MUL_MASK                0x00c00000

// PP_SRC_OFFSET_0
#define PP_SRC_OFFSET_0__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_1
#define PP_SRC_OFFSET_1__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_2
#define PP_SRC_OFFSET_2__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_3
#define PP_SRC_OFFSET_3__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_4
#define PP_SRC_OFFSET_4__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_5
#define PP_SRC_OFFSET_5__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_6
#define PP_SRC_OFFSET_6__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_7
#define PP_SRC_OFFSET_7__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_8
#define PP_SRC_OFFSET_8__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_9
#define PP_SRC_OFFSET_9__MC_OFFSET_MASK                    0x01ffffe0

// PP_SRC_OFFSET_10
#define PP_SRC_OFFSET_10__MC_OFFSET_MASK                   0x01ffffe0

// PP_SRC_OFFSET_11
#define PP_SRC_OFFSET_11__MC_OFFSET_MASK                   0x01ffffe0

// PP_SRC_OFFSET_12
#define PP_SRC_OFFSET_12__MC_OFFSET_MASK                   0x01ffffe0

// PP_SRC_OFFSET_13
#define PP_SRC_OFFSET_13__MC_OFFSET_MASK                   0x01ffffe0

// PP_SRC_OFFSET_14
#define PP_SRC_OFFSET_14__MC_OFFSET_MASK                   0x01ffffe0

// PP_SRC_OFFSET_15
#define PP_SRC_OFFSET_15__MC_OFFSET_MASK                   0x01ffffe0

// PP_SRC_OFFSET_16
#define PP_SRC_OFFSET_16__MC_OFFSET_MASK                   0x01ffffe0

// PP_SRC_OFFSET_17
#define PP_SRC_OFFSET_17__MC_OFFSET_MASK                   0x01ffffe0

// PP_CNTL
#define PP_CNTL__STIPPLE_ENABLE_MASK                       0x00000001
#define PP_CNTL__STIPPLE_ENABLE                            0x00000001
#define PP_CNTL__SCISSOR_ENABLE_MASK                       0x00000002
#define PP_CNTL__SCISSOR_ENABLE                            0x00000002
#define PP_CNTL__PATTERN_ENABLE_MASK                       0x00000004
#define PP_CNTL__PATTERN_ENABLE                            0x00000004
#define PP_CNTL__SHADOW_ENABLE_MASK                        0x00000008
#define PP_CNTL__SHADOW_ENABLE                             0x00000008
#define PP_CNTL__TEX_0_ENABLE_MASK                         0x00000010
#define PP_CNTL__TEX_0_ENABLE                              0x00000010
#define PP_CNTL__TEX_1_ENABLE_MASK                         0x00000020
#define PP_CNTL__TEX_1_ENABLE                              0x00000020
#define PP_CNTL__TEX_2_ENABLE_MASK                         0x00000040
#define PP_CNTL__TEX_2_ENABLE                              0x00000040
#define PP_CNTL__TEX_3_ENABLE_MASK                         0x00000080
#define PP_CNTL__TEX_3_ENABLE                              0x00000080
#define PP_CNTL__TEX_BLEND_0_ENABLE_MASK                   0x00001000
#define PP_CNTL__TEX_BLEND_0_ENABLE                        0x00001000
#define PP_CNTL__TEX_BLEND_1_ENABLE_MASK                   0x00002000
#define PP_CNTL__TEX_BLEND_1_ENABLE                        0x00002000
#define PP_CNTL__TEX_BLEND_2_ENABLE_MASK                   0x00004000
#define PP_CNTL__TEX_BLEND_2_ENABLE                        0x00004000
#define PP_CNTL__TEX_BLEND_3_ENABLE_MASK                   0x00008000
#define PP_CNTL__TEX_BLEND_3_ENABLE                        0x00008000
#define PP_CNTL__PLANAR_YUV_ENABLE_MASK                    0x00100000
#define PP_CNTL__PLANAR_YUV_ENABLE                         0x00100000
#define PP_CNTL__SPECULAR_ENABLE_MASK                      0x00200000
#define PP_CNTL__SPECULAR_ENABLE                           0x00200000
#define PP_CNTL__FOG_ENABLE_MASK                           0x00400000
#define PP_CNTL__FOG_ENABLE                                0x00400000
#define PP_CNTL__ALPHA_TEST_ENABLE_MASK                    0x00800000
#define PP_CNTL__ALPHA_TEST_ENABLE                         0x00800000
#define PP_CNTL__ANTI_ALIAS_CTL_MASK                       0x03000000
#define PP_CNTL__BUMP_MAP_ENABLE_MASK                      0x04000000
#define PP_CNTL__BUMP_MAP_ENABLE                           0x04000000
#define PP_CNTL__BUMPED_MAP_MASK                           0x18000000
#define PP_CNTL__TEX_3D_ENABLE_0_MASK                      0x20000000
#define PP_CNTL__TEX_3D_ENABLE_0                           0x20000000
#define PP_CNTL__TEX_3D_ENABLE_1_MASK                      0x40000000
#define PP_CNTL__TEX_3D_ENABLE_1                           0x40000000
#define PP_CNTL__MC_ENABLE_MASK                            0x80000000
#define PP_CNTL__MC_ENABLE                                 0x80000000

// PP_TXFILTER_0
#define PP_TXFILTER_0__MAG_FILTER_MASK                     0x00000001
#define PP_TXFILTER_0__MAG_FILTER                          0x00000001
#define PP_TXFILTER_0__MIN_FILTER_MASK                     0x0000001e
#define PP_TXFILTER_0__MAX_ANIS_MASK                       0x000000e0
#define PP_TXFILTER_0__LOD_BIAS_MASK                       0x0000ff00
#define PP_TXFILTER_0__MAX_MIP_LEVEL_MASK                  0x000f0000
#define PP_TXFILTER_0__YUV_TO_RGB_MASK                     0x00100000
#define PP_TXFILTER_0__YUV_TO_RGB                          0x00100000
#define PP_TXFILTER_0__YUV_TEMPERATURE_MASK                0x00200000
#define PP_TXFILTER_0__YUV_TEMPERATURE                     0x00200000
#define PP_TXFILTER_0__WRAPEN_S_MASK                       0x00400000
#define PP_TXFILTER_0__WRAPEN_S                            0x00400000
#define PP_TXFILTER_0__CLAMP_S_MASK                        0x03800000
#define PP_TXFILTER_0__WRAPEN_T_MASK                       0x04000000
#define PP_TXFILTER_0__WRAPEN_T                            0x04000000
#define PP_TXFILTER_0__CLAMP_T_MASK                        0x38000000
#define PP_TXFILTER_0__BORDER_MODE_MASK                    0x80000000
#define PP_TXFILTER_0__BORDER_MODE                         0x80000000

// PP_TXFILTER_1
#define PP_TXFILTER_1__MAG_FILTER_MASK                     0x00000001
#define PP_TXFILTER_1__MAG_FILTER                          0x00000001
#define PP_TXFILTER_1__MIN_FILTER_MASK                     0x0000001e
#define PP_TXFILTER_1__LOD_BIAS_MASK                       0x0000ff00
#define PP_TXFILTER_1__MAX_MIP_LEVEL_MASK                  0x000f0000
#define PP_TXFILTER_1__WRAPEN_S_MASK                       0x00400000
#define PP_TXFILTER_1__WRAPEN_S                            0x00400000
#define PP_TXFILTER_1__CLAMP_S_MASK                        0x03800000
#define PP_TXFILTER_1__WRAPEN_T_MASK                       0x04000000
#define PP_TXFILTER_1__WRAPEN_T                            0x04000000
#define PP_TXFILTER_1__CLAMP_T_MASK                        0x38000000
#define PP_TXFILTER_1__BORDER_MODE_MASK                    0x80000000
#define PP_TXFILTER_1__BORDER_MODE                         0x80000000

// PP_TXFILTER_2
#define PP_TXFILTER_2__MAG_FILTER_MASK                     0x00000001
#define PP_TXFILTER_2__MAG_FILTER                          0x00000001
#define PP_TXFILTER_2__MIN_FILTER_MASK                     0x0000001e
#define PP_TXFILTER_2__LOD_BIAS_MASK                       0x0000ff00
#define PP_TXFILTER_2__MAX_MIP_LEVEL_MASK                  0x000f0000
#define PP_TXFILTER_2__WRAPEN_S_MASK                       0x00400000
#define PP_TXFILTER_2__WRAPEN_S                            0x00400000
#define PP_TXFILTER_2__CLAMP_S_MASK                        0x03800000
#define PP_TXFILTER_2__WRAPEN_T_MASK                       0x04000000
#define PP_TXFILTER_2__WRAPEN_T                            0x04000000
#define PP_TXFILTER_2__CLAMP_T_MASK                        0x38000000
#define PP_TXFILTER_2__BORDER_MODE_MASK                    0x80000000
#define PP_TXFILTER_2__BORDER_MODE                         0x80000000

// PP_TXFORMAT_0
#define PP_TXFORMAT_0__TXFORMAT_MASK                       0x0000001f
#define PP_TXFORMAT_0__APPLE_YUV_MASK                      0x00000020
#define PP_TXFORMAT_0__APPLE_YUV                           0x00000020
#define PP_TXFORMAT_0__ALPHA_ENABLE_MASK                   0x00000040
#define PP_TXFORMAT_0__ALPHA_ENABLE                        0x00000040
#define PP_TXFORMAT_0__NON_POWER2_MASK                     0x00000080
#define PP_TXFORMAT_0__NON_POWER2                          0x00000080
#define PP_TXFORMAT_0__TXWIDTH_MASK                        0x00000f00
#define PP_TXFORMAT_0__TXHEIGHT_MASK                       0x0000f000
#define PP_TXFORMAT_0__FACE_WIDTH_5_MASK                   0x000f0000
#define PP_TXFORMAT_0__FACE_HEIGHT_5_MASK                  0x00f00000
#define PP_TXFORMAT_0__ST_ROUTE_MASK                       0x03000000
#define PP_TXFORMAT_0__ALPHA_MASK_ENABLE_MASK              0x10000000
#define PP_TXFORMAT_0__ALPHA_MASK_ENABLE                   0x10000000
#define PP_TXFORMAT_0__CHROMA_KEY_ENABLE_MASK              0x20000000
#define PP_TXFORMAT_0__CHROMA_KEY_ENABLE                   0x20000000
#define PP_TXFORMAT_0__CUBIC_MAP_ENABLE_MASK               0x40000000
#define PP_TXFORMAT_0__CUBIC_MAP_ENABLE                    0x40000000
#define PP_TXFORMAT_0__PERSPECTIVE_ENABLE_MASK             0x80000000
#define PP_TXFORMAT_0__PERSPECTIVE_ENABLE                  0x80000000

// PP_TXFORMAT_1
#define PP_TXFORMAT_1__TXFORMAT_MASK                       0x0000001f
#define PP_TXFORMAT_1__APPLE_YUV_MASK                      0x00000020
#define PP_TXFORMAT_1__APPLE_YUV                           0x00000020
#define PP_TXFORMAT_1__ALPHA_ENABLE_MASK                   0x00000040
#define PP_TXFORMAT_1__ALPHA_ENABLE                        0x00000040
#define PP_TXFORMAT_1__NON_POWER2_MASK                     0x00000080
#define PP_TXFORMAT_1__NON_POWER2                          0x00000080
#define PP_TXFORMAT_1__TXWIDTH_MASK                        0x00000f00
#define PP_TXFORMAT_1__TXHEIGHT_MASK                       0x0000f000
#define PP_TXFORMAT_1__FACE_WIDTH_5_MASK                   0x000f0000
#define PP_TXFORMAT_1__FACE_HEIGHT_5_MASK                  0x00f00000
#define PP_TXFORMAT_1__ST_ROUTE_MASK                       0x03000000
#define PP_TXFORMAT_1__ALPHA_MASK_ENABLE_MASK              0x10000000
#define PP_TXFORMAT_1__ALPHA_MASK_ENABLE                   0x10000000
#define PP_TXFORMAT_1__CHROMA_KEY_ENABLE_MASK              0x20000000
#define PP_TXFORMAT_1__CHROMA_KEY_ENABLE                   0x20000000
#define PP_TXFORMAT_1__CUBIC_MAP_ENABLE_MASK               0x40000000
#define PP_TXFORMAT_1__CUBIC_MAP_ENABLE                    0x40000000
#define PP_TXFORMAT_1__PERSPECTIVE_ENABLE_MASK             0x80000000
#define PP_TXFORMAT_1__PERSPECTIVE_ENABLE                  0x80000000

// PP_TXFORMAT_2
#define PP_TXFORMAT_2__TXFORMAT_MASK                       0x0000001f
#define PP_TXFORMAT_2__APPLE_YUV_MASK                      0x00000020
#define PP_TXFORMAT_2__APPLE_YUV                           0x00000020
#define PP_TXFORMAT_2__ALPHA_ENABLE_MASK                   0x00000040
#define PP_TXFORMAT_2__ALPHA_ENABLE                        0x00000040
#define PP_TXFORMAT_2__NON_POWER2_MASK                     0x00000080
#define PP_TXFORMAT_2__NON_POWER2                          0x00000080
#define PP_TXFORMAT_2__TXWIDTH_MASK                        0x00000f00
#define PP_TXFORMAT_2__TXHEIGHT_MASK                       0x0000f000
#define PP_TXFORMAT_2__FACE_WIDTH_5_MASK                   0x000f0000
#define PP_TXFORMAT_2__FACE_HEIGHT_5_MASK                  0x00f00000
#define PP_TXFORMAT_2__ST_ROUTE_MASK                       0x03000000
#define PP_TXFORMAT_2__ALPHA_MASK_ENABLE_MASK              0x10000000
#define PP_TXFORMAT_2__ALPHA_MASK_ENABLE                   0x10000000
#define PP_TXFORMAT_2__CHROMA_KEY_ENABLE_MASK              0x20000000
#define PP_TXFORMAT_2__CHROMA_KEY_ENABLE                   0x20000000
#define PP_TXFORMAT_2__CUBIC_MAP_ENABLE_MASK               0x40000000
#define PP_TXFORMAT_2__CUBIC_MAP_ENABLE                    0x40000000
#define PP_TXFORMAT_2__PERSPECTIVE_ENABLE_MASK             0x80000000
#define PP_TXFORMAT_2__PERSPECTIVE_ENABLE                  0x80000000

// PP_TXOFFSET_0
#define PP_TXOFFSET_0__ENDIAN_SWAP_MASK                    0x00000003
#define PP_TXOFFSET_0__MACRO_TILE_MASK                     0x00000004
#define PP_TXOFFSET_0__MACRO_TILE                          0x00000004
#define PP_TXOFFSET_0__MICRO_TILE_MASK                     0x00000018
#define PP_TXOFFSET_0__TXOFFSET_MASK                       0xffffffe0

// PP_TXOFFSET_1
#define PP_TXOFFSET_1__ENDIAN_SWAP_MASK                    0x00000003
#define PP_TXOFFSET_1__MACRO_TILE_MASK                     0x00000004
#define PP_TXOFFSET_1__MACRO_TILE                          0x00000004
#define PP_TXOFFSET_1__MICRO_TILE_MASK                     0x00000018
#define PP_TXOFFSET_1__TXOFFSET_MASK                       0xffffffe0

// PP_TXOFFSET_2
#define PP_TXOFFSET_2__ENDIAN_SWAP_MASK                    0x00000003
#define PP_TXOFFSET_2__MACRO_TILE_MASK                     0x00000004
#define PP_TXOFFSET_2__MACRO_TILE                          0x00000004
#define PP_TXOFFSET_2__MICRO_TILE_MASK                     0x00000018
#define PP_TXOFFSET_2__TXOFFSET_MASK                       0xffffffe0

// PP_TEX_SIZE_0
#define PP_TEX_SIZE_0__TEX_USIZE_MASK                      0x000007ff
#define PP_TEX_SIZE_0__TEX_VSIZE_MASK                      0x07ff0000

// PP_TEX_SIZE_1
#define PP_TEX_SIZE_1__TEX_USIZE_MASK                      0x000007ff
#define PP_TEX_SIZE_1__TEX_VSIZE_MASK                      0x07ff0000

// PP_TEX_SIZE_2
#define PP_TEX_SIZE_2__TEX_USIZE_MASK                      0x000007ff
#define PP_TEX_SIZE_2__TEX_VSIZE_MASK                      0x07ff0000

// PP_TXPITCH_0
#define PP_TXPITCH_0__TXPITCH_MASK                         0x00003fe0

// PP_TXPITCH_1
#define PP_TXPITCH_1__TXPITCH_MASK                         0x00003fe0

// PP_TXPITCH_2
#define PP_TXPITCH_2__TXPITCH_MASK                         0x00003fe0

// PP_CUBIC_FACES_0
#define PP_CUBIC_FACES_0__FACE_WIDTH_1_MASK                0x0000000f
#define PP_CUBIC_FACES_0__FACE_HEIGHT_1_MASK               0x000000f0
#define PP_CUBIC_FACES_0__FACE_WIDTH_2_MASK                0x00000f00
#define PP_CUBIC_FACES_0__FACE_HEIGHT_2_MASK               0x0000f000
#define PP_CUBIC_FACES_0__FACE_WIDTH_3_MASK                0x000f0000
#define PP_CUBIC_FACES_0__FACE_HEIGHT_3_MASK               0x00f00000
#define PP_CUBIC_FACES_0__FACE_WIDTH_4_MASK                0x0f000000
#define PP_CUBIC_FACES_0__FACE_HEIGHT_4_MASK               0xf0000000

// PP_CUBIC_FACES_1
#define PP_CUBIC_FACES_1__FACE_WIDTH_1_MASK                0x0000000f
#define PP_CUBIC_FACES_1__FACE_HEIGHT_1_MASK               0x000000f0
#define PP_CUBIC_FACES_1__FACE_WIDTH_2_MASK                0x00000f00
#define PP_CUBIC_FACES_1__FACE_HEIGHT_2_MASK               0x0000f000
#define PP_CUBIC_FACES_1__FACE_WIDTH_3_MASK                0x000f0000
#define PP_CUBIC_FACES_1__FACE_HEIGHT_3_MASK               0x00f00000
#define PP_CUBIC_FACES_1__FACE_WIDTH_4_MASK                0x0f000000
#define PP_CUBIC_FACES_1__FACE_HEIGHT_4_MASK               0xf0000000

// PP_CUBIC_FACES_2
#define PP_CUBIC_FACES_2__FACE_WIDTH_1_MASK                0x0000000f
#define PP_CUBIC_FACES_2__FACE_HEIGHT_1_MASK               0x000000f0
#define PP_CUBIC_FACES_2__FACE_WIDTH_2_MASK                0x00000f00
#define PP_CUBIC_FACES_2__FACE_HEIGHT_2_MASK               0x0000f000
#define PP_CUBIC_FACES_2__FACE_WIDTH_3_MASK                0x000f0000
#define PP_CUBIC_FACES_2__FACE_HEIGHT_3_MASK               0x00f00000
#define PP_CUBIC_FACES_2__FACE_WIDTH_4_MASK                0x0f000000
#define PP_CUBIC_FACES_2__FACE_HEIGHT_4_MASK               0xf0000000

// PP_CUBIC_OFFSET_T0_0
#define PP_CUBIC_OFFSET_T0_0__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T0_1
#define PP_CUBIC_OFFSET_T0_1__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T0_2
#define PP_CUBIC_OFFSET_T0_2__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T0_3
#define PP_CUBIC_OFFSET_T0_3__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T0_4
#define PP_CUBIC_OFFSET_T0_4__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T1_0
#define PP_CUBIC_OFFSET_T1_0__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T1_1
#define PP_CUBIC_OFFSET_T1_1__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T1_2
#define PP_CUBIC_OFFSET_T1_2__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T1_3
#define PP_CUBIC_OFFSET_T1_3__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T1_4
#define PP_CUBIC_OFFSET_T1_4__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T2_0
#define PP_CUBIC_OFFSET_T2_0__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T2_1
#define PP_CUBIC_OFFSET_T2_1__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T2_2
#define PP_CUBIC_OFFSET_T2_2__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T2_3
#define PP_CUBIC_OFFSET_T2_3__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T2_4
#define PP_CUBIC_OFFSET_T2_4__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T3_0
#define PP_CUBIC_OFFSET_T3_0__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T3_1
#define PP_CUBIC_OFFSET_T3_1__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T3_2
#define PP_CUBIC_OFFSET_T3_2__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T3_3
#define PP_CUBIC_OFFSET_T3_3__FACE_OFFSET_MASK             0xffffffe0

// PP_CUBIC_OFFSET_T3_4
#define PP_CUBIC_OFFSET_T3_4__FACE_OFFSET_MASK             0xffffffe0

// PP_SHADOW_ID
#define PP_SHADOW_ID__SHADOW_ID_MASK                       0x00ffffff

// PP_CHROMA_COLOR
#define PP_CHROMA_COLOR__CHROMA_COLOR_MASK                 0xffffffff

// PP_CHROMA_MASK
#define PP_CHROMA_MASK__CHROMA_MASK_MASK                   0xffffffff

// PP_BORDER_COLOR_0
#define PP_BORDER_COLOR_0__BORD_COLOR_MASK                 0xffffffff

// PP_BORDER_COLOR_1
#define PP_BORDER_COLOR_1__BORD_COLOR_MASK                 0xffffffff

// PP_BORDER_COLOR_2
#define PP_BORDER_COLOR_2__BORD_COLOR_MASK                 0xffffffff

// PP_MISC
#define PP_MISC__REF_ALPHA_MASK                            0x000000ff
#define PP_MISC__ALPHA_TEST_OP_MASK                        0x00000700
#define PP_MISC__CHROMA_FUNC_MASK                          0x00030000
#define PP_MISC__CHROMA_KEY_MODE_MASK                      0x00040000
#define PP_MISC__CHROMA_KEY_MODE                           0x00040000
#define PP_MISC__SHADOW_AUTO_INC_MASK                      0x00100000
#define PP_MISC__SHADOW_AUTO_INC                           0x00100000
#define PP_MISC__SHADOW_FUNC_MASK                          0x00200000
#define PP_MISC__SHADOW_FUNC                               0x00200000
#define PP_MISC__SHADOW_PASS_MASK                          0x00400000
#define PP_MISC__SHADOW_PASS                               0x00400000
#define PP_MISC__RIGHT_HAND_CUBE_MASK                      0x01000000
#define PP_MISC__RIGHT_HAND_CUBE                           0x01000000

// PP_TXCBLEND_0
#define PP_TXCBLEND_0__COLOR_ARG_A_MASK                    0x0000001f
#define PP_TXCBLEND_0__COLOR_ARG_B_MASK                    0x000003e0
#define PP_TXCBLEND_0__COLOR_ARG_C_MASK                    0x00007c00
#define PP_TXCBLEND_0__COMP_ARG_A_MASK                     0x00008000
#define PP_TXCBLEND_0__COMP_ARG_A                          0x00008000
#define PP_TXCBLEND_0__COMP_ARG_B_MASK                     0x00010000
#define PP_TXCBLEND_0__COMP_ARG_B                          0x00010000
#define PP_TXCBLEND_0__COMP_ARG_C_MASK                     0x00020000
#define PP_TXCBLEND_0__COMP_ARG_C                          0x00020000
#define PP_TXCBLEND_0__BLEND_CTL_MASK                      0x001c0000
#define PP_TXCBLEND_0__SCALE_TX_MASK                       0x00600000
#define PP_TXCBLEND_0__CLAMP_TX_MASK                       0x00800000
#define PP_TXCBLEND_0__CLAMP_TX                            0x00800000
#define PP_TXCBLEND_0__T0_EQ_TCUR_MASK                     0x01000000
#define PP_TXCBLEND_0__T0_EQ_TCUR                          0x01000000
#define PP_TXCBLEND_0__T1_EQ_TCUR_MASK                     0x02000000
#define PP_TXCBLEND_0__T1_EQ_TCUR                          0x02000000
#define PP_TXCBLEND_0__T2_EQ_TCUR_MASK                     0x04000000
#define PP_TXCBLEND_0__T2_EQ_TCUR                          0x04000000
#define PP_TXCBLEND_0__T3_EQ_TCUR_MASK                     0x08000000
#define PP_TXCBLEND_0__T3_EQ_TCUR                          0x08000000

// PP_TXCBLEND_1
#define PP_TXCBLEND_1__COLOR_ARG_A_MASK                    0x0000001f
#define PP_TXCBLEND_1__COLOR_ARG_B_MASK                    0x000003e0
#define PP_TXCBLEND_1__COLOR_ARG_C_MASK                    0x00007c00
#define PP_TXCBLEND_1__COMP_ARG_A_MASK                     0x00008000
#define PP_TXCBLEND_1__COMP_ARG_A                          0x00008000
#define PP_TXCBLEND_1__COMP_ARG_B_MASK                     0x00010000
#define PP_TXCBLEND_1__COMP_ARG_B                          0x00010000
#define PP_TXCBLEND_1__COMP_ARG_C_MASK                     0x00020000
#define PP_TXCBLEND_1__COMP_ARG_C                          0x00020000
#define PP_TXCBLEND_1__BLEND_CTL_MASK                      0x001c0000
#define PP_TXCBLEND_1__SCALE_TX_MASK                       0x00600000
#define PP_TXCBLEND_1__CLAMP_TX_MASK                       0x00800000
#define PP_TXCBLEND_1__CLAMP_TX                            0x00800000
#define PP_TXCBLEND_1__T0_EQ_TCUR_MASK                     0x01000000
#define PP_TXCBLEND_1__T0_EQ_TCUR                          0x01000000
#define PP_TXCBLEND_1__T1_EQ_TCUR_MASK                     0x02000000
#define PP_TXCBLEND_1__T1_EQ_TCUR                          0x02000000
#define PP_TXCBLEND_1__T2_EQ_TCUR_MASK                     0x04000000
#define PP_TXCBLEND_1__T2_EQ_TCUR                          0x04000000
#define PP_TXCBLEND_1__T3_EQ_TCUR_MASK                     0x08000000
#define PP_TXCBLEND_1__T3_EQ_TCUR                          0x08000000

// PP_TXCBLEND_2
#define PP_TXCBLEND_2__COLOR_ARG_A_MASK                    0x0000001f
#define PP_TXCBLEND_2__COLOR_ARG_B_MASK                    0x000003e0
#define PP_TXCBLEND_2__COLOR_ARG_C_MASK                    0x00007c00
#define PP_TXCBLEND_2__COMP_ARG_A_MASK                     0x00008000
#define PP_TXCBLEND_2__COMP_ARG_A                          0x00008000
#define PP_TXCBLEND_2__COMP_ARG_B_MASK                     0x00010000
#define PP_TXCBLEND_2__COMP_ARG_B                          0x00010000
#define PP_TXCBLEND_2__COMP_ARG_C_MASK                     0x00020000
#define PP_TXCBLEND_2__COMP_ARG_C                          0x00020000
#define PP_TXCBLEND_2__BLEND_CTL_MASK                      0x001c0000
#define PP_TXCBLEND_2__SCALE_TX_MASK                       0x00600000
#define PP_TXCBLEND_2__CLAMP_TX_MASK                       0x00800000
#define PP_TXCBLEND_2__CLAMP_TX                            0x00800000
#define PP_TXCBLEND_2__T0_EQ_TCUR_MASK                     0x01000000
#define PP_TXCBLEND_2__T0_EQ_TCUR                          0x01000000
#define PP_TXCBLEND_2__T1_EQ_TCUR_MASK                     0x02000000
#define PP_TXCBLEND_2__T1_EQ_TCUR                          0x02000000
#define PP_TXCBLEND_2__T2_EQ_TCUR_MASK                     0x04000000
#define PP_TXCBLEND_2__T2_EQ_TCUR                          0x04000000
#define PP_TXCBLEND_2__T3_EQ_TCUR_MASK                     0x08000000
#define PP_TXCBLEND_2__T3_EQ_TCUR                          0x08000000

// PP_TXABLEND_0
#define PP_TXABLEND_0__ALPHA_ARG_A_MASK                    0x0000000f
#define PP_TXABLEND_0__ALPHA_ARG_B_MASK                    0x000000f0
#define PP_TXABLEND_0__ALPHA_ARG_C_MASK                    0x00000f00
#define PP_TXABLEND_0__DOT_ALPHA_MASK                      0x00001000
#define PP_TXABLEND_0__DOT_ALPHA                           0x00001000
#define PP_TXABLEND_0__COMP_ARG_A_MASK                     0x00008000
#define PP_TXABLEND_0__COMP_ARG_A                          0x00008000
#define PP_TXABLEND_0__COMP_ARG_B_MASK                     0x00010000
#define PP_TXABLEND_0__COMP_ARG_B                          0x00010000
#define PP_TXABLEND_0__COMP_ARG_C_MASK                     0x00020000
#define PP_TXABLEND_0__COMP_ARG_C                          0x00020000
#define PP_TXABLEND_0__BLEND_CTL_MASK                      0x001c0000
#define PP_TXABLEND_0__SCALE_TX_MASK                       0x00600000
#define PP_TXABLEND_0__CLAMP_TX_MASK                       0x00800000
#define PP_TXABLEND_0__CLAMP_TX                            0x00800000
#define PP_TXABLEND_0__T0_EQ_TCUR_MASK                     0x01000000
#define PP_TXABLEND_0__T0_EQ_TCUR                          0x01000000
#define PP_TXABLEND_0__T1_EQ_TCUR_MASK                     0x02000000
#define PP_TXABLEND_0__T1_EQ_TCUR                          0x02000000
#define PP_TXABLEND_0__T2_EQ_TCUR_MASK                     0x04000000
#define PP_TXABLEND_0__T2_EQ_TCUR                          0x04000000
#define PP_TXABLEND_0__T3_EQ_TCUR_MASK                     0x08000000
#define PP_TXABLEND_0__T3_EQ_TCUR                          0x08000000

// PP_TXABLEND_1
#define PP_TXABLEND_1__ALPHA_ARG_A_MASK                    0x0000000f
#define PP_TXABLEND_1__ALPHA_ARG_B_MASK                    0x000000f0
#define PP_TXABLEND_1__ALPHA_ARG_C_MASK                    0x00000f00
#define PP_TXABLEND_1__DOT_ALPHA_MASK                      0x00001000
#define PP_TXABLEND_1__DOT_ALPHA                           0x00001000
#define PP_TXABLEND_1__COMP_ARG_A_MASK                     0x00008000
#define PP_TXABLEND_1__COMP_ARG_A                          0x00008000
#define PP_TXABLEND_1__COMP_ARG_B_MASK                     0x00010000
#define PP_TXABLEND_1__COMP_ARG_B                          0x00010000
#define PP_TXABLEND_1__COMP_ARG_C_MASK                     0x00020000
#define PP_TXABLEND_1__COMP_ARG_C                          0x00020000
#define PP_TXABLEND_1__BLEND_CTL_MASK                      0x001c0000
#define PP_TXABLEND_1__SCALE_TX_MASK                       0x00600000
#define PP_TXABLEND_1__CLAMP_TX_MASK                       0x00800000
#define PP_TXABLEND_1__CLAMP_TX                            0x00800000
#define PP_TXABLEND_1__T0_EQ_TCUR_MASK                     0x01000000
#define PP_TXABLEND_1__T0_EQ_TCUR                          0x01000000
#define PP_TXABLEND_1__T1_EQ_TCUR_MASK                     0x02000000
#define PP_TXABLEND_1__T1_EQ_TCUR                          0x02000000
#define PP_TXABLEND_1__T2_EQ_TCUR_MASK                     0x04000000
#define PP_TXABLEND_1__T2_EQ_TCUR                          0x04000000
#define PP_TXABLEND_1__T3_EQ_TCUR_MASK                     0x08000000
#define PP_TXABLEND_1__T3_EQ_TCUR                          0x08000000

// PP_TXABLEND_2
#define PP_TXABLEND_2__ALPHA_ARG_A_MASK                    0x0000000f
#define PP_TXABLEND_2__ALPHA_ARG_B_MASK                    0x000000f0
#define PP_TXABLEND_2__ALPHA_ARG_C_MASK                    0x00000f00
#define PP_TXABLEND_2__DOT_ALPHA_MASK                      0x00001000
#define PP_TXABLEND_2__DOT_ALPHA                           0x00001000
#define PP_TXABLEND_2__COMP_ARG_A_MASK                     0x00008000
#define PP_TXABLEND_2__COMP_ARG_A                          0x00008000
#define PP_TXABLEND_2__COMP_ARG_B_MASK                     0x00010000
#define PP_TXABLEND_2__COMP_ARG_B                          0x00010000
#define PP_TXABLEND_2__COMP_ARG_C_MASK                     0x00020000
#define PP_TXABLEND_2__COMP_ARG_C                          0x00020000
#define PP_TXABLEND_2__BLEND_CTL_MASK                      0x001c0000
#define PP_TXABLEND_2__SCALE_TX_MASK                       0x00600000
#define PP_TXABLEND_2__CLAMP_TX_MASK                       0x00800000
#define PP_TXABLEND_2__CLAMP_TX                            0x00800000
#define PP_TXABLEND_2__T0_EQ_TCUR_MASK                     0x01000000
#define PP_TXABLEND_2__T0_EQ_TCUR                          0x01000000
#define PP_TXABLEND_2__T1_EQ_TCUR_MASK                     0x02000000
#define PP_TXABLEND_2__T1_EQ_TCUR                          0x02000000
#define PP_TXABLEND_2__T2_EQ_TCUR_MASK                     0x04000000
#define PP_TXABLEND_2__T2_EQ_TCUR                          0x04000000
#define PP_TXABLEND_2__T3_EQ_TCUR_MASK                     0x08000000
#define PP_TXABLEND_2__T3_EQ_TCUR                          0x08000000

// PP_TFACTOR_0
#define PP_TFACTOR_0__TFACTOR_MASK                         0xffffffff

// PP_TFACTOR_1
#define PP_TFACTOR_1__TFACTOR_MASK                         0xffffffff

// PP_TFACTOR_2
#define PP_TFACTOR_2__TFACTOR_MASK                         0xffffffff

// PP_ROT_MATRIX_0
#define PP_ROT_MATRIX_0__M00_MAN_MASK                      0x000007ff
#define PP_ROT_MATRIX_0__M10_MAN_MASK                      0x07ff0000
#define PP_ROT_MATRIX_0__Mx0_EXPO_MASK                     0x78000000

// PP_ROT_MATRIX_1
#define PP_ROT_MATRIX_1__M01_MAN_MASK                      0x000007ff
#define PP_ROT_MATRIX_1__M11_MAN_MASK                      0x07ff0000
#define PP_ROT_MATRIX_1__Mx1_EXPO_MASK                     0x78000000

// PP_LUM_MATRIX
#define PP_LUM_MATRIX__LSCALE_MASK                         0x000000ff
#define PP_LUM_MATRIX__LOFFSET_MASK                        0x0000ff00

// PP_FOG_COLOR
#define PP_FOG_COLOR__FOG_COLOR_MASK                       0x00ffffff
#define PP_FOG_COLOR__FOG_TABLE_MASK                       0x01000000
#define PP_FOG_COLOR__FOG_TABLE                            0x01000000
#define PP_FOG_COLOR__FOG_INDEX_SEL_MASK                   0x06000000

// PP_FOG_TABLE_INDEX
#define PP_FOG_TABLE_INDEX__FOG_INDEX_MASK                 0x000000ff

// PP_FOG_TABLE_DATA
#define PP_FOG_TABLE_DATA__FOG_DATA_MASK                   0x000000ff

// PP_PERF
#define PP_PERF__RR_FIFO_SCALE_MASK                        0x00000003
#define PP_PERF__REQUEST_SCALE_MASK                        0x00000004
#define PP_PERF__REQUEST_SCALE                             0x00000004
#define PP_PERF__PP_COUNT_CTL_MASK                         0x00000030

// PP_TRI_JUICE
#define PP_TRI_JUICE__TRI_JUICE_MASK                       0x0000001f

// PP_PERF_COUNT_0
#define PP_PERF_COUNT_0__PP_CACHE_COUNT_MASK               0xffffffff

// PP_PERF_COUNT_1
#define PP_PERF_COUNT_1__PP_CACHE_COUNT_MASK               0xffffffff

// PP_PERF_COUNT_2
#define PP_PERF_COUNT_2__PP_CACHE_COUNT_MASK               0xffffffff

// PP_TAM_DEBUG_0
#define PP_TAM_DEBUG_0__TAM_DWORD_MASK                     0xffffffff

// PP_TAM_DEBUG_1
#define PP_TAM_DEBUG_1__TAM_DWORD_MASK                     0xffffffff

// PP_TAM_DEBUG_2
#define PP_TAM_DEBUG_2__TAM_DWORD_MASK                     0xffffffff

// PP_TAM_DEBUG_3
#define PP_TAM_DEBUG_3__TAM_DWORD_MASK                     0xffffffff

// PP_TDM_DEBUG_0
#define PP_TDM_DEBUG_0__TDM_DWORD_MASK                     0xffffffff

// PP_TDM_DEBUG_1
#define PP_TDM_DEBUG_1__TDM_DWORD_MASK                     0xffffffff

// PP_TDM_DEBUG_2
#define PP_TDM_DEBUG_2__TDM_DWORD_MASK                     0xffffffff

// PP_TDM_DEBUG_3
#define PP_TDM_DEBUG_3__TDM_DWORD_MASK                     0xffffffff

// PP_PB_DEBUG_0
#define PP_PB_DEBUG_0__PB_DWORD_MASK                       0xffffffff

// PP_PB_DEBUG_1
#define PP_PB_DEBUG_1__PB_DWORD_MASK                       0xffffffff

// PP_PB_DEBUG_2
#define PP_PB_DEBUG_2__PB_DWORD_MASK                       0xffffffff

// PP_PB_DEBUG_3
#define PP_PB_DEBUG_3__PB_DWORD_MASK                       0xffffffff

// RB2D_ROP
#define RB2D_ROP__ROP_MASK                                 0x00ff0000

// RB2D_CLRCMP_SRC
#define RB2D_CLRCMP_SRC__CLRCMP_SRC_MASK                   0xffffffff

// RB2D_CLRCMP_DST
#define RB2D_CLRCMP_DST__CLRCMP_DST_MASK                   0xffffffff

// RB2D_CLRCMP_FLIPE
#define RB2D_CLRCMP_FLIPE__CLRCMP_FLIPE_MASK               0xffffffff

// RB2D_CLRCMP_CNTL
#define RB2D_CLRCMP_CNTL__FCN_SRC_MASK                     0x00000007
#define RB2D_CLRCMP_CNTL__FCN_DST_MASK                     0x00000700
#define RB2D_CLRCMP_CNTL__SRC_MASK                         0x03000000

// RB2D_CLRCMP_MSK
#define RB2D_CLRCMP_MSK__CLRCMP_MSK_MASK                   0xffffffff

// RB2D_WRITEMASK
#define RB2D_WRITEMASK__WRITEMASK_MASK                     0xffffffff

// RB2D_DATATYPE
#define RB2D_DATATYPE__DP_DST_DATATYPE_MASK                0x0000000f
#define RB2D_DATATYPE__DP_BRUSH_DATATYPE_MASK              0x00000f00

// RB2D_GUI_MASTER_CNTL
#define RB2D_GUI_MASTER_CNTL__GMC_BRUSH_DATATYPE_MASK      0x000000f0
#define RB2D_GUI_MASTER_CNTL__GMC_DST_DATATYPE_MASK        0x00000f00
#define RB2D_GUI_MASTER_CNTL__GMC_ROP_MASK                 0x00ff0000
#define RB2D_GUI_MASTER_CNTL__GMC_CLR_CMP_FCN_DIS_MASK     0x10000000
#define RB2D_GUI_MASTER_CNTL__GMC_CLR_CMP_FCN_DIS          0x10000000
#define RB2D_GUI_MASTER_CNTL__GMC_WR_MSK_DIS_MASK          0x40000000
#define RB2D_GUI_MASTER_CNTL__GMC_WR_MSK_DIS               0x40000000

// RB2D_BRUSHDATA_0
#define RB2D_BRUSHDATA_0__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_1
#define RB2D_BRUSHDATA_1__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_2
#define RB2D_BRUSHDATA_2__BRUSHDATA_MASK                   0xffffffff
#define RB2D_BRUSH_FRGD_CLR__BRUSHDATA_MASK                0xffffffff

// RB2D_BRUSHDATA_3
#define RB2D_BRUSHDATA_3__BRUSHDATA_MASK                   0xffffffff
#define RB2D_BRUSH_BKGD_CLR__BRUSHDATA_MASK                0xffffffff

// RB2D_BRUSHDATA_4
#define RB2D_BRUSHDATA_4__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_5
#define RB2D_BRUSHDATA_5__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_6
#define RB2D_BRUSHDATA_6__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_7
#define RB2D_BRUSHDATA_7__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_8
#define RB2D_BRUSHDATA_8__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_9
#define RB2D_BRUSHDATA_9__BRUSHDATA_MASK                   0xffffffff

// RB2D_BRUSHDATA_10
#define RB2D_BRUSHDATA_10__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_11
#define RB2D_BRUSHDATA_11__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_12
#define RB2D_BRUSHDATA_12__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_13
#define RB2D_BRUSHDATA_13__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_14
#define RB2D_BRUSHDATA_14__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_15
#define RB2D_BRUSHDATA_15__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_16
#define RB2D_BRUSHDATA_16__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_17
#define RB2D_BRUSHDATA_17__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_18
#define RB2D_BRUSHDATA_18__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_19
#define RB2D_BRUSHDATA_19__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_20
#define RB2D_BRUSHDATA_20__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_21
#define RB2D_BRUSHDATA_21__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_22
#define RB2D_BRUSHDATA_22__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_23
#define RB2D_BRUSHDATA_23__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_24
#define RB2D_BRUSHDATA_24__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_25
#define RB2D_BRUSHDATA_25__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_26
#define RB2D_BRUSHDATA_26__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_27
#define RB2D_BRUSHDATA_27__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_28
#define RB2D_BRUSHDATA_28__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_29
#define RB2D_BRUSHDATA_29__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_30
#define RB2D_BRUSHDATA_30__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_31
#define RB2D_BRUSHDATA_31__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_32
#define RB2D_BRUSHDATA_32__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_33
#define RB2D_BRUSHDATA_33__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_34
#define RB2D_BRUSHDATA_34__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_35
#define RB2D_BRUSHDATA_35__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_36
#define RB2D_BRUSHDATA_36__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_37
#define RB2D_BRUSHDATA_37__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_38
#define RB2D_BRUSHDATA_38__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_39
#define RB2D_BRUSHDATA_39__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_40
#define RB2D_BRUSHDATA_40__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_41
#define RB2D_BRUSHDATA_41__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_42
#define RB2D_BRUSHDATA_42__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_43
#define RB2D_BRUSHDATA_43__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_44
#define RB2D_BRUSHDATA_44__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_45
#define RB2D_BRUSHDATA_45__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_46
#define RB2D_BRUSHDATA_46__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_47
#define RB2D_BRUSHDATA_47__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_48
#define RB2D_BRUSHDATA_48__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_49
#define RB2D_BRUSHDATA_49__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_50
#define RB2D_BRUSHDATA_50__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_51
#define RB2D_BRUSHDATA_51__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_52
#define RB2D_BRUSHDATA_52__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_53
#define RB2D_BRUSHDATA_53__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_54
#define RB2D_BRUSHDATA_54__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_55
#define RB2D_BRUSHDATA_55__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_56
#define RB2D_BRUSHDATA_56__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_57
#define RB2D_BRUSHDATA_57__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_58
#define RB2D_BRUSHDATA_58__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_59
#define RB2D_BRUSHDATA_59__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_60
#define RB2D_BRUSHDATA_60__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_61
#define RB2D_BRUSHDATA_61__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_62
#define RB2D_BRUSHDATA_62__BRUSHDATA_MASK                  0xffffffff

// RB2D_BRUSHDATA_63
#define RB2D_BRUSHDATA_63__BRUSHDATA_MASK                  0xffffffff

// RB2D_DSTCACHE_MODE
#define RB2D_DSTCACHE_MODE__DC_BYPASS_MASK                 0x00000003
#define RB2D_DSTCACHE_MODE__DC_LINE_SIZE_MASK              0x0000000c
#define RB2D_DSTCACHE_MODE__DC_AUTOFLUSH_ENABLE_MASK       0x00000300
#define RB2D_DSTCACHE_MODE__DC_FORCE_RMW_MASK              0x00010000
#define RB2D_DSTCACHE_MODE__DC_FORCE_RMW                   0x00010000
#define RB2D_DSTCACHE_MODE__DC_DISABLE_RI_FILL_MASK        0x01000000
#define RB2D_DSTCACHE_MODE__DC_DISABLE_RI_FILL             0x01000000
#define RB2D_DSTCACHE_MODE__DC_DISABLE_RI_READ_MASK        0x02000000
#define RB2D_DSTCACHE_MODE__DC_DISABLE_RI_READ             0x02000000

// RB2D_DSTCACHE_CTLSTAT
#define RB2D_DSTCACHE_CTLSTAT__DC_FLUSH_MASK               0x00000003
#define RB2D_DSTCACHE_CTLSTAT__DC_FREE_MASK                0x0000000c
#define RB2D_DSTCACHE_CTLSTAT__DC_BUSY_MASK                0x80000000
#define RB2D_DSTCACHE_CTLSTAT__DC_BUSY                     0x80000000

// RB2D_SRC_ENDIAN
#define RB2D_SRC_ENDIAN__SRC_ENDIAN_MASK                   0x00000003

// RB2D_DST_ENDIAN
#define RB2D_DST_ENDIAN__DST_ENDIAN_MASK                   0x00000003

// RB2D_PD1_DATA
#define RB2D_PD1_DATA__PD1_DATA_MASK                       0xffffffff

// RB2D_PD1_ADDR
#define RB2D_PD1_ADDR__PD1_ADDR_MASK                       0x00000007

// RB2D_WRITEBACK_DATA_LO
#define RB2D_WRITEBACK_DATA_LO__WRITEBACK_DATA_LO_MASK     0xffffffff

// RB2D_WRITEBACK_DATA_HI
#define RB2D_WRITEBACK_DATA_HI__WRITEBACK_DATA_HI_MASK     0xffffffff

// RB2D_WRITEBACK_ADDR
#define RB2D_WRITEBACK_ADDR__WRITEBACK_ADDR_MASK           0xfffffff8

// RB3D_BLENDCNTL
#define RB3D_BLENDCNTL__COMB_FCN_MASK                      0x00003000
#define RB3D_BLENDCNTL__SRCBLEND_MASK                      0x003f0000
#define RB3D_BLENDCNTL__DESTBLEND_MASK                     0x3f000000

// RB3D_DEPTHOFFSET
#define RB3D_DEPTHOFFSET__DEPTHOFFSET_MASK                 0xfffffff0

// RB3D_DEPTHPITCH
#define RB3D_DEPTHPITCH__DEPTHPITCH_MASK                   0x00001ff8
#define RB3D_DEPTHPITCH__DEPTHENDIAN_MASK                  0x000c0000

// RB3D_ZSTENCILCNTL
#define RB3D_ZSTENCILCNTL__DEPTHFORMAT_MASK                0x0000000f
#define RB3D_ZSTENCILCNTL__ZFUNC_MASK                      0x00000070
#define RB3D_ZSTENCILCNTL__HIERARCHYENABLE_MASK            0x00000100
#define RB3D_ZSTENCILCNTL__HIERARCHYENABLE                 0x00000100
#define RB3D_ZSTENCILCNTL__STENCILFUNC_MASK                0x00007000
#define RB3D_ZSTENCILCNTL__STENCILFAIL_MASK                0x00070000
#define RB3D_ZSTENCILCNTL__STENCILZPASS_MASK               0x00700000
#define RB3D_ZSTENCILCNTL__STENCILZFAIL_MASK               0x07000000
#define RB3D_ZSTENCILCNTL__ZCOMPRESSION_MASK               0x10000000
#define RB3D_ZSTENCILCNTL__ZCOMPRESSION                    0x10000000
#define RB3D_ZSTENCILCNTL__FORCEZDIRTY_MASK                0x20000000
#define RB3D_ZSTENCILCNTL__FORCEZDIRTY                     0x20000000
#define RB3D_ZSTENCILCNTL__ZWRITEENABLE_MASK               0x40000000
#define RB3D_ZSTENCILCNTL__ZWRITEENABLE                    0x40000000
#define RB3D_ZSTENCILCNTL__ZDECOMPRESSION_MASK             0x80000000
#define RB3D_ZSTENCILCNTL__ZDECOMPRESSION                  0x80000000

// RB3D_DEPTHCLEARVALUE
#define RB3D_DEPTHCLEARVALUE__DEPTHCLEARVALUE_MASK         0xffffffff

// RB3D_ZMASKOFFSET
#define RB3D_ZMASKOFFSET__ZMASKOFFSET_MASK                 0x00007ffe

// RB3D_CNTL
#define RB3D_CNTL__ALPHA_BLEND_ENABLE_MASK                 0x00000001
#define RB3D_CNTL__ALPHA_BLEND_ENABLE                      0x00000001
#define RB3D_CNTL__PLANE_MASK_ENABLE_MASK                  0x00000002
#define RB3D_CNTL__PLANE_MASK_ENABLE                       0x00000002
#define RB3D_CNTL__DITHER_ENABLE_MASK                      0x00000004
#define RB3D_CNTL__DITHER_ENABLE                           0x00000004
#define RB3D_CNTL__ROUND_ENABLE_MASK                       0x00000008
#define RB3D_CNTL__ROUND_ENABLE                            0x00000008
#define RB3D_CNTL__SCALE_DITHER_MASK                       0x00000010
#define RB3D_CNTL__SCALE_DITHER                            0x00000010
#define RB3D_CNTL__DITHER_INIT_MASK                        0x00000020
#define RB3D_CNTL__DITHER_INIT                             0x00000020
#define RB3D_CNTL__ROP_ENABLE_MASK                         0x00000040
#define RB3D_CNTL__ROP_ENABLE                              0x00000040
#define RB3D_CNTL__STENCIL_ENABLE_MASK                     0x00000080
#define RB3D_CNTL__STENCIL_ENABLE                          0x00000080
#define RB3D_CNTL__Z_ENABLE_MASK                           0x00000100
#define RB3D_CNTL__Z_ENABLE                                0x00000100
#define RB3D_CNTL__DEPTHXY_OFFSET_ENABLE_MASK              0x00000200
#define RB3D_CNTL__DEPTHXY_OFFSET_ENABLE                   0x00000200
#define RB3D_CNTL__COLORFORMAT_MASK                        0x00003c00
#define RB3D_CNTL__CLRCMP_FLIPE_ENABLE_MASK                0x00004000
#define RB3D_CNTL__CLRCMP_FLIPE_ENABLE                     0x00004000
#define RB3D_CNTL__ZBLOCK16_MASK                           0x00008000
#define RB3D_CNTL__ZBLOCK16                                0x00008000

// RB3D_COLOROFFSET
#define RB3D_COLOROFFSET__COLOROFFSET_MASK                 0xfffffff0

// RB3D_COLORPITCH
#define RB3D_COLORPITCH__COLORPITCH_MASK                   0x00001ff8
#define RB3D_COLORPITCH__COLORTILE_MASK                    0x00010000
#define RB3D_COLORPITCH__COLORTILE                         0x00010000
#define RB3D_COLORPITCH__COLORMICROTILE_MASK               0x00020000
#define RB3D_COLORPITCH__COLORMICROTILE                    0x00020000
#define RB3D_COLORPITCH__COLORENDIAN_MASK                  0x000c0000

// RB3D_DEPTHXY_OFFSET
#define RB3D_DEPTHXY_OFFSET__DEPTHX_OFFSET_MASK            0x00000fff
#define RB3D_DEPTHXY_OFFSET__DEPTHY_OFFSET_MASK            0x0fff0000

// RB3D_CLRCMP_FLIPE
#define RB3D_CLRCMP_FLIPE__CLRCMP_FLIPE_MASK               0xffffffff

// RB3D_CLRCMP_CLR
#define RB3D_CLRCMP_CLR__CLRCMP_CLR_MASK                   0xffffffff

// RB3D_CLRCMP_MSK
#define RB3D_CLRCMP_MSK__CLRCMP_MSK_MASK                   0xffffffff

// RB3D_ZMASK_WRINDEX
#define RB3D_ZMASK_WRINDEX__ZMASK_WRINDEX_MASK             0x00007ff8

// RB3D_ZMASK_DWORD
#define RB3D_ZMASK_DWORD__ZCLEAR0_MASK                     0x00003fc0
#define RB3D_ZMASK_DWORD__ZMASK0_MASK                      0x0000c000
#define RB3D_ZMASK_DWORD__ZCLEAR1_MASK                     0x3fc00000
#define RB3D_ZMASK_DWORD__ZMASK1_MASK                      0xc0000000

// RB3D_ZMASK_RDINDEX
#define RB3D_ZMASK_RDINDEX__ZMASK_RDINDEX_MASK             0x00007ffe

// RB3D_STENCILREFMASK
#define RB3D_STENCILREFMASK__STENCILREF_MASK               0x000000ff
#define RB3D_STENCILREFMASK__STENCILMASK_MASK              0x00ff0000
#define RB3D_STENCILREFMASK__STENCILWRITEMASK_MASK         0xff000000

// RB3D_ROPCNTL
#define RB3D_ROPCNTL__ROP_MASK                             0x00000f00

// RB3D_PLANEMASK
#define RB3D_PLANEMASK__PLANEMASK_MASK                     0xffffffff

// RB3D_ZCACHE_MODE
#define RB3D_ZCACHE_MODE__ZC_BYPASS_MASK                   0x00000001
#define RB3D_ZCACHE_MODE__ZC_BYPASS                        0x00000001
#define RB3D_ZCACHE_MODE__ZMASK_RAM_RM_MASK                0x00000f00
#define RB3D_ZCACHE_MODE__ZC_DISABLE_RI_FILL_MASK          0x01000000
#define RB3D_ZCACHE_MODE__ZC_DISABLE_RI_FILL               0x01000000
#define RB3D_ZCACHE_MODE__ZC_DISABLE_RI_READ_MASK          0x02000000
#define RB3D_ZCACHE_MODE__ZC_DISABLE_RI_READ               0x02000000
#define RB3D_ZCACHE_MODE__DISABLE_DR_L0_RD_MASK            0x04000000
#define RB3D_ZCACHE_MODE__DISABLE_DR_L0_RD                 0x04000000
#define RB3D_ZCACHE_MODE__DISABLE_DR_L0_WR_MASK            0x08000000
#define RB3D_ZCACHE_MODE__DISABLE_DR_L0_WR                 0x08000000

// RB3D_ZCACHE_CTLSTAT
#define RB3D_ZCACHE_CTLSTAT__ZC_FLUSH_MASK                 0x00000001
#define RB3D_ZCACHE_CTLSTAT__ZC_FLUSH                      0x00000001
#define RB3D_ZCACHE_CTLSTAT__ZC_FREE_MASK                  0x00000004
#define RB3D_ZCACHE_CTLSTAT__ZC_FREE                       0x00000004
#define RB3D_ZCACHE_CTLSTAT__ZC_BUSY_MASK                  0x80000000
#define RB3D_ZCACHE_CTLSTAT__ZC_BUSY                       0x80000000

// RB3D_DSTCACHE_MODE
#define RB3D_DSTCACHE_MODE__DC_BYPASS_MASK                 0x00000003
#define RB3D_DSTCACHE_MODE__DC_LINE_SIZE_MASK              0x0000000c
#define RB3D_DSTCACHE_MODE__DC_AUTOFLUSH_ENABLE_MASK       0x00000300
#define RB3D_DSTCACHE_MODE__DC_FORCE_RMW_MASK              0x00010000
#define RB3D_DSTCACHE_MODE__DC_FORCE_RMW                   0x00010000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_RI_FILL_MASK        0x01000000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_RI_FILL             0x01000000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_RI_READ_MASK        0x02000000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_RI_READ             0x02000000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_L0_RD_MASK          0x04000000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_L0_RD               0x04000000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_L0_WR_MASK          0x08000000
#define RB3D_DSTCACHE_MODE__DC_DISABLE_L0_WR               0x08000000

// RB3D_DSTCACHE_CTLSTAT
#define RB3D_DSTCACHE_CTLSTAT__DC_FLUSH_MASK               0x00000003
#define RB3D_DSTCACHE_CTLSTAT__DC_FREE_MASK                0x0000000c
#define RB3D_DSTCACHE_CTLSTAT__DC_BUSY_MASK                0x80000000
#define RB3D_DSTCACHE_CTLSTAT__DC_BUSY                     0x80000000

// RB3D_PD0_DATA
#define RB3D_PD0_DATA__DISABLE_RB_MASK                     0x00000001
#define RB3D_PD0_DATA__DISABLE_RB                          0x00000001

// RB3D_PD1_DATA
#define RB3D_PD1_DATA__PD1_DATA_MASK                       0xffffffff

// RB3D_PD1_ADDR
#define RB3D_PD1_ADDR__PD1_ADDR_MASK                       0x00000007

// RB3D_WRITEBACK_DATA_LO
#define RB3D_WRITEBACK_DATA_LO__WRITEBACK_DATA_LO_MASK     0xffffffff

// RB3D_WRITEBACK_DATA_HI
#define RB3D_WRITEBACK_DATA_HI__WRITEBACK_DATA_HI_MASK     0xffffffff

// RB3D_WRITEBACK_ADDR
#define RB3D_WRITEBACK_ADDR__WRITEBACK_ADDR_MASK           0xfffffff8

// RB3D_ZPASS_DATA
#define RB3D_ZPASS_DATA__ZPASS_DATA_MASK                   0xffffffff

// RB3D_ZPASS_ADDR
#define RB3D_ZPASS_ADDR__ZPASS_ADDR_MASK                   0xfffffffc

// DST_OFFSET
#define DST_OFFSET__DST_OFFSET_MASK                        0xffffffff

// DST_PITCH
#define DST_PITCH__DST_PITCH_MASK                          0x00003fff

// DST_TILE
#define DST_TILE__DST_TILE_MASK                            0x00000003

// DST_PITCH_OFFSET
#define DST_PITCH_OFFSET__DST_OFFSET_MASK                  0x003fffff
#define DST_PITCH_OFFSET__DST_PITCH_MASK                   0x3fc00000
#define DST_PITCH_OFFSET__DST_TILE_MASK                    0xc0000000

// DST_X
#define DST_X__DST_X_MASK                                  0x00003fff

// DST_Y
#define DST_Y__DST_Y_MASK                                  0x00003fff

// DST_X_Y
#define DST_X_Y__DST_Y_MASK                                0x00003fff
#define DST_X_Y__DST_X_MASK                                0x3fff0000

// DST_Y_X
#define DST_Y_X__DST_X_MASK                                0x00003fff
#define DST_Y_X__DST_Y_MASK                                0x3fff0000

// DST_WIDTH
#define DST_WIDTH__DST_WIDTH_MASK                          0x00003fff

// DST_HEIGHT
#define DST_HEIGHT__DST_HEIGHT_MASK                        0x00003fff

// DST_WIDTH_HEIGHT
#define DST_WIDTH_HEIGHT__DST_HEIGHT_MASK                  0x00003fff
#define DST_WIDTH_HEIGHT__DST_WIDTH_MASK                   0x3fff0000

// DST_HEIGHT_WIDTH
#define DST_HEIGHT_WIDTH__DST_WIDTH_MASK                   0x00003fff
#define DST_HEIGHT_WIDTH__DST_HEIGHT_MASK                  0x3fff0000

// DST_HEIGHT_WIDTH_8
#define DST_HEIGHT_WIDTH_8__DST_WIDTH_MASK                 0x00ff0000
#define DST_HEIGHT_WIDTH_8__DST_HEIGHT_MASK                0xff000000

// DST_HEIGHT_Y
#define DST_HEIGHT_Y__DST_Y_MASK                           0x00003fff
#define DST_HEIGHT_Y__DST_HEIGHT_MASK                      0x3fff0000

// DST_WIDTH_X
#define DST_WIDTH_X__DST_X_MASK                            0x00003fff
#define DST_WIDTH_X__DST_WIDTH_MASK                        0x3fff0000

// DST_WIDTH_X_INCY
#define DST_WIDTH_X_INCY__DST_X_MASK                       0x00003fff
#define DST_WIDTH_X_INCY__DST_WIDTH_MASK                   0x3fff0000

// DST_LINE_START
#define DST_LINE_START__DST_START_X_MASK                   0x00003fff
#define DST_LINE_START__DST_START_Y_MASK                   0x3fff0000

// DST_LINE_END
#define DST_LINE_END__DST_END_X_MASK                       0x00003fff
#define DST_LINE_END__DST_END_Y_MASK                       0x3fff0000

// DST_LINE_PATCOUNT
#define DST_LINE_PATCOUNT__LINE_PATCOUNT_MASK              0x0000001f

// DP_DST_ENDIAN
#define DP_DST_ENDIAN__DST_ENDIAN_MASK                     0x00000003

// BRUSH_Y_X
#define BRUSH_Y_X__BRUSH_X_MASK                            0x00000007
#define BRUSH_Y_X__BRUSH_Y_MASK                            0x00000700

// BRUSH_DATA0
#define BRUSH_DATA0__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA1
#define BRUSH_DATA1__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA2
#define BRUSH_DATA2__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA3
#define BRUSH_DATA3__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA4
#define BRUSH_DATA4__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA5
#define BRUSH_DATA5__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA6
#define BRUSH_DATA6__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA7
#define BRUSH_DATA7__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA8
#define BRUSH_DATA8__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA9
#define BRUSH_DATA9__BRUSH_DATA_MASK                       0xffffffff

// BRUSH_DATA10
#define BRUSH_DATA10__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA11
#define BRUSH_DATA11__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA12
#define BRUSH_DATA12__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA13
#define BRUSH_DATA13__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA14
#define BRUSH_DATA14__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA15
#define BRUSH_DATA15__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA16
#define BRUSH_DATA16__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA17
#define BRUSH_DATA17__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA18
#define BRUSH_DATA18__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA19
#define BRUSH_DATA19__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA20
#define BRUSH_DATA20__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA21
#define BRUSH_DATA21__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA22
#define BRUSH_DATA22__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA23
#define BRUSH_DATA23__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA24
#define BRUSH_DATA24__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA25
#define BRUSH_DATA25__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA26
#define BRUSH_DATA26__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA27
#define BRUSH_DATA27__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA28
#define BRUSH_DATA28__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA29
#define BRUSH_DATA29__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA30
#define BRUSH_DATA30__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA31
#define BRUSH_DATA31__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA32
#define BRUSH_DATA32__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA33
#define BRUSH_DATA33__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA34
#define BRUSH_DATA34__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA35
#define BRUSH_DATA35__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA36
#define BRUSH_DATA36__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA37
#define BRUSH_DATA37__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA38
#define BRUSH_DATA38__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA39
#define BRUSH_DATA39__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA40
#define BRUSH_DATA40__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA41
#define BRUSH_DATA41__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA42
#define BRUSH_DATA42__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA43
#define BRUSH_DATA43__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA44
#define BRUSH_DATA44__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA45
#define BRUSH_DATA45__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA46
#define BRUSH_DATA46__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA47
#define BRUSH_DATA47__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA48
#define BRUSH_DATA48__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA49
#define BRUSH_DATA49__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA50
#define BRUSH_DATA50__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA51
#define BRUSH_DATA51__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA52
#define BRUSH_DATA52__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA53
#define BRUSH_DATA53__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA54
#define BRUSH_DATA54__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA55
#define BRUSH_DATA55__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA56
#define BRUSH_DATA56__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA57
#define BRUSH_DATA57__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA58
#define BRUSH_DATA58__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA59
#define BRUSH_DATA59__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA60
#define BRUSH_DATA60__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA61
#define BRUSH_DATA61__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA62
#define BRUSH_DATA62__BRUSH_DATA_MASK                      0xffffffff

// BRUSH_DATA63
#define BRUSH_DATA63__BRUSH_DATA_MASK                      0xffffffff

// DP_BRUSH_FRGD_CLR
#define DP_BRUSH_FRGD_CLR__DP_BRUSH_FRGD_CLR_MASK          0xffffffff

// DP_BRUSH_BKGD_CLR
#define DP_BRUSH_BKGD_CLR__DP_BRUSH_BKGD_CLR_MASK          0xffffffff

// SRC_OFFSET
#define SRC_OFFSET__SRC_OFFSET_MASK                        0xffffffff

// SRC_PITCH
#define SRC_PITCH__SRC_PITCH_MASK                          0x00003fff

// SRC_TILE
#define SRC_TILE__SRC_TILE_MASK                            0x00000001
#define SRC_TILE__SRC_TILE                                 0x00000001

// SRC_PITCH_OFFSET
#define SRC_PITCH_OFFSET__SRC_OFFSET_MASK                  0x003fffff
#define SRC_PITCH_OFFSET__SRC_PITCH_MASK                   0x3fc00000
#define SRC_PITCH_OFFSET__SRC_TILE_MASK                    0x40000000
#define SRC_PITCH_OFFSET__SRC_TILE                         0x40000000

// SRC_X
#define SRC_X__SRC_X_MASK                                  0x00003fff

// SRC_Y
#define SRC_Y__SRC_Y_MASK                                  0x00003fff

// SRC_X_Y
#define SRC_X_Y__SRC_Y_MASK                                0x00003fff
#define SRC_X_Y__SRC_X_MASK                                0x3fff0000

// SRC_Y_X
#define SRC_Y_X__SRC_X_MASK                                0x00003fff
#define SRC_Y_X__SRC_Y_MASK                                0x3fff0000

// SRC_CLUT_ADDRESS
#define SRC_CLUT_ADDRESS__SRC_CLUT_ADDRESS_MASK            0x000000ff

// SRC_CLUT_DATA
#define SRC_CLUT_DATA__SRC_CLUT_DATA_MASK                  0xffffffff

// SRC_CLUT_DATA_RD
#define SRC_CLUT_DATA_RD__SRC_CLUT_DATA_MASK               0xffffffff

// HOST_DATA0
#define HOST_DATA0__HOST_DATA_MASK                         0xffffffff

// HOST_DATA1
#define HOST_DATA1__HOST_DATA_MASK                         0xffffffff

// HOST_DATA2
#define HOST_DATA2__HOST_DATA_MASK                         0xffffffff

// HOST_DATA3
#define HOST_DATA3__HOST_DATA_MASK                         0xffffffff

// HOST_DATA4
#define HOST_DATA4__HOST_DATA_MASK                         0xffffffff

// HOST_DATA5
#define HOST_DATA5__HOST_DATA_MASK                         0xffffffff

// HOST_DATA6
#define HOST_DATA6__HOST_DATA_MASK                         0xffffffff

// HOST_DATA7
#define HOST_DATA7__HOST_DATA_MASK                         0xffffffff

// HOST_DATA_LAST
#define HOST_DATA_LAST__HOST_DATA_LAST_MASK                0xffffffff

// DP_SRC_ENDIAN
#define DP_SRC_ENDIAN__SRC_ENDIAN_MASK                     0x00000003

// DP_SRC_FRGD_CLR
#define DP_SRC_FRGD_CLR__DP_SRC_FRGD_CLR_MASK              0xffffffff

// DP_SRC_BKGD_CLR
#define DP_SRC_BKGD_CLR__DP_SRC_BKGD_CLR_MASK              0xffffffff

// SC_LEFT
#define SC_LEFT__SC_LEFT_MASK                              0x00003fff

// SC_RIGHT
#define SC_RIGHT__SC_RIGHT_MASK                            0x00003fff

// SC_TOP
#define SC_TOP__SC_TOP_MASK                                0x00003fff

// SC_BOTTOM
#define SC_BOTTOM__SC_BOTTOM_MASK                          0x00003fff

// SRC_SC_RIGHT
#define SRC_SC_RIGHT__SC_RIGHT_MASK                        0x00003fff

// SRC_SC_BOTTOM
#define SRC_SC_BOTTOM__SC_BOTTOM_MASK                      0x00003fff

// DP_CNTL
#define DP_CNTL__DST_X_DIR_MASK                            0x00000001
#define DP_CNTL__DST_X_DIR                                 0x00000001
#define DP_CNTL__DST_Y_DIR_MASK                            0x00000002
#define DP_CNTL__DST_Y_DIR                                 0x00000002
#define DP_CNTL__DST_TILE_MASK                             0x00000018

// DP_CNTL_XDIR_YDIR_YMAJOR
#define DP_CNTL_XDIR_YDIR_YMAJOR__DST_Y_DIR_MASK           0x00008000
#define DP_CNTL_XDIR_YDIR_YMAJOR__DST_Y_DIR                0x00008000
#define DP_CNTL_XDIR_YDIR_YMAJOR__DST_X_DIR_MASK           0x80000000
#define DP_CNTL_XDIR_YDIR_YMAJOR__DST_X_DIR                0x80000000

// DP_DATATYPE
#define DP_DATATYPE__DP_DST_DATATYPE_MASK                  0x0000000f
#define DP_DATATYPE__DP_BRUSH_DATATYPE_MASK                0x00000f00
#define DP_DATATYPE__DP_SRC_DATATYPE_MASK                  0x00070000
#define DP_DATATYPE__DP_BYTE_PIX_ORDER_MASK                0x40000000
#define DP_DATATYPE__DP_BYTE_PIX_ORDER                     0x40000000

// DP_MIX
#define DP_MIX__DP_SRC_SOURCE_MASK                         0x00000700
#define DP_MIX__DP_ROP3_MASK                               0x00ff0000

// DP_WRITE_MSK
#define DP_WRITE_MSK__DP_WRITE_MSK_MASK                    0xffffffff

// DP_XOP
#define DP_XOP__XOP_A_MASK                                 0x000000ff
#define DP_XOP__XOP_B_MASK                                 0x0000ff00
#define DP_XOP__XOP_C_MASK                                 0x00ff0000
#define DP_XOP__XOP_OP_MASK                                0x03000000

// CLR_CMP_CLR_SRC
#define CLR_CMP_CLR_SRC__CLR_CMP_CLR_SRC_MASK              0xffffffff

// CLR_CMP_CLR_DST
#define CLR_CMP_CLR_DST__CLR_CMP_CLR_DST_MASK              0xffffffff

// CLR_CMP_CNTL
#define CLR_CMP_CNTL__CLR_CMP_FCN_SRC_MASK                 0x00000007
#define CLR_CMP_CNTL__CLR_CMP_FCN_DST_MASK                 0x00000700
#define CLR_CMP_CNTL__CLR_CMP_SRC_MASK                     0x03000000

// CLR_CMP_MSK
#define CLR_CMP_MSK__CLR_CMP_MSK_MASK                      0xffffffff

// DSTCACHE_MODE
#define DSTCACHE_MODE__DSTCACHE_MODE_MASK                  0xffffffff

// DSTCACHE_CTLSTAT
#define DSTCACHE_CTLSTAT__DSTCACHE_CTLSTAT_MASK            0xffffffff

// DEFAULT_PITCH_OFFSET
#define DEFAULT_PITCH_OFFSET__DEFAULT_OFFSET_MASK          0x003fffff
#define DEFAULT_PITCH_OFFSET__DEFAULT_PITCH_MASK           0x3fc00000
#define DEFAULT_PITCH_OFFSET__DEFAULT_TILE_MASK            0xc0000000

// DEFAULT_SC_BOTTOM_RIGHT
#define DEFAULT_SC_BOTTOM_RIGHT__DEFAULT_SC_RIGHT_MASK     0x00003fff
#define DEFAULT_SC_BOTTOM_RIGHT__DEFAULT_SC_BOTTOM_MASK    0x3fff0000

// DP_GUI_MASTER_CNTL
#define DP_GUI_MASTER_CNTL__GMC_SRC_PITCH_OFFSET_CNTL_MASK 0x00000001
#define DP_GUI_MASTER_CNTL__GMC_SRC_PITCH_OFFSET_CNTL      0x00000001
#define DP_GUI_MASTER_CNTL__GMC_DST_PITCH_OFFSET_CNTL_MASK 0x00000002
#define DP_GUI_MASTER_CNTL__GMC_DST_PITCH_OFFSET_CNTL      0x00000002
#define DP_GUI_MASTER_CNTL__GMC_SRC_CLIPPING_MASK          0x00000004
#define DP_GUI_MASTER_CNTL__GMC_SRC_CLIPPING               0x00000004
#define DP_GUI_MASTER_CNTL__GMC_DST_CLIPPING_MASK          0x00000008
#define DP_GUI_MASTER_CNTL__GMC_DST_CLIPPING               0x00000008
#define DP_GUI_MASTER_CNTL__GMC_BRUSH_DATATYPE_MASK        0x000000f0
#define DP_GUI_MASTER_CNTL__GMC_DST_DATATYPE_MASK          0x00000f00
#define DP_GUI_MASTER_CNTL__GMC_SRC_DATATYPE_MASK          0x00003000
#define DP_GUI_MASTER_CNTL__GMC_BYTE_PIX_ORDER_MASK        0x00004000
#define DP_GUI_MASTER_CNTL__GMC_BYTE_PIX_ORDER             0x00004000
#define DP_GUI_MASTER_CNTL__GMC_ROP3_MASK                  0x00ff0000
#define DP_GUI_MASTER_CNTL__GMC_DP_SRC_SOURCE_MASK         0x07000000
#define DP_GUI_MASTER_CNTL__GMC_SRC_DATATYPE2_MASK         0x08000000
#define DP_GUI_MASTER_CNTL__GMC_SRC_DATATYPE2              0x08000000
#define DP_GUI_MASTER_CNTL__GMC_CLR_CMP_FCN_DIS_MASK       0x10000000
#define DP_GUI_MASTER_CNTL__GMC_CLR_CMP_FCN_DIS            0x10000000
#define DP_GUI_MASTER_CNTL__GMC_WR_MSK_DIS_MASK            0x40000000
#define DP_GUI_MASTER_CNTL__GMC_WR_MSK_DIS                 0x40000000

// SC_TOP_LEFT
#define SC_TOP_LEFT__SC_LEFT_MASK                          0x00003fff
#define SC_TOP_LEFT__SC_TOP_MASK                           0x3fff0000

// SC_BOTTOM_RIGHT
#define SC_BOTTOM_RIGHT__SC_RIGHT_MASK                     0x00003fff
#define SC_BOTTOM_RIGHT__SC_BOTTOM_MASK                    0x3fff0000

// SRC_SC_BOTTOM_RIGHT
#define SRC_SC_BOTTOM_RIGHT__SC_RIGHT_MASK                 0x00003fff
#define SRC_SC_BOTTOM_RIGHT__SC_BOTTOM_MASK                0x3fff0000

// DEBUG0

// DEBUG1

// DEBUG2

// DEBUG3

// DEBUG4

// DEBUG5

// DEBUG6

// DEBUG7

// DEBUG8

// DEBUG9

// DEBUG10

// DEBUG11

// DEBUG12

// DEBUG13

// DEBUG14

// DEBUG15

// BIOS_0_SCRATCH
#define BIOS_0_SCRATCH__BIOS_SCRATCH_MASK                  0xffffffff

// BIOS_1_SCRATCH
#define BIOS_1_SCRATCH__BIOS_SCRATCH_MASK                  0xffffffff

// BIOS_2_SCRATCH
#define BIOS_2_SCRATCH__BIOS_SCRATCH_MASK                  0xffffffff

// BIOS_3_SCRATCH
#define BIOS_3_SCRATCH__BIOS_SCRATCH_MASK                  0xffffffff

// TVOUT_0_SCRATCH
#define TVOUT_0_SCRATCH__TVOUT_0_SCRATCH_MASK              0xffffffff

// TVOUT_1_SCRATCH
#define TVOUT_1_SCRATCH__TVOUT_1_SCRATCH_MASK              0xffffffff

// I2C_CNTL_0
#define I2C_CNTL_0__I2C_DONE_MASK                          0x00000001
#define I2C_CNTL_0__I2C_DONE                               0x00000001
#define I2C_CNTL_0__I2C_NACK_MASK                          0x00000002
#define I2C_CNTL_0__I2C_NACK                               0x00000002
#define I2C_CNTL_0__I2C_HALT_MASK                          0x00000004
#define I2C_CNTL_0__I2C_HALT                               0x00000004
#define I2C_CNTL_0__I2C_SOFT_RST_MASK                      0x00000020
#define I2C_CNTL_0__I2C_SOFT_RST                           0x00000020
#define I2C_CNTL_0__I2C_DRIVE_EN_MASK                      0x00000040
#define I2C_CNTL_0__I2C_DRIVE_EN                           0x00000040
#define I2C_CNTL_0__I2C_DRIVE_SEL_MASK                     0x00000080
#define I2C_CNTL_0__I2C_DRIVE_SEL                          0x00000080
#define I2C_CNTL_0__I2C_START_MASK                         0x00000100
#define I2C_CNTL_0__I2C_START                              0x00000100
#define I2C_CNTL_0__I2C_STOP_MASK                          0x00000200
#define I2C_CNTL_0__I2C_STOP                               0x00000200
#define I2C_CNTL_0__I2C_RECEIVE_MASK                       0x00000400
#define I2C_CNTL_0__I2C_RECEIVE                            0x00000400
#define I2C_CNTL_0__I2C_ABORT_MASK                         0x00000800
#define I2C_CNTL_0__I2C_ABORT                              0x00000800
#define I2C_CNTL_0__I2C_GO_MASK                            0x00001000
#define I2C_CNTL_0__I2C_GO                                 0x00001000
#define I2C_CNTL_0__I2C_PRESCALE_MASK                      0xffff0000

// I2C_CNTL_1
#define I2C_CNTL_1__I2C_DATA_COUNT_MASK                    0x0000000f
#define I2C_CNTL_1__I2C_ADDR_COUNT_MASK                    0x00000700
#define I2C_CNTL_1__I2C_SEL_MASK                           0x00010000
#define I2C_CNTL_1__I2C_SEL                                0x00010000
#define I2C_CNTL_1__I2C_EN_MASK                            0x00020000
#define I2C_CNTL_1__I2C_EN                                 0x00020000
#define I2C_CNTL_1__I2C_TIME_LIMIT_MASK                    0xff000000

// I2C_DATA
#define I2C_DATA__I2C_DATA_MASK                            0x000000ff

// CONFIG_XSTRAP
#define CONFIG_XSTRAP__VGA_DISABLE_MASK                    0x00000001
#define CONFIG_XSTRAP__VGA_DISABLE                         0x00000001
#define CONFIG_XSTRAP__ENINTB_MASK                         0x00000008
#define CONFIG_XSTRAP__ENINTB                              0x00000008
#define CONFIG_XSTRAP__AGPSKEW_MASK                        0x000000c0
#define CONFIG_XSTRAP__X1CLK_SKEW_MASK                     0x00000300
#define CONFIG_XSTRAP__VIP_DEVICE_MASK                     0x00002000
#define CONFIG_XSTRAP__VIP_DEVICE                          0x00002000
#define CONFIG_XSTRAP__ID_DISABLE_MASK                     0x00004000
#define CONFIG_XSTRAP__ID_DISABLE                          0x00004000
#define CONFIG_XSTRAP__CRIPPLE_PANELb_MASK                 0x00008000
#define CONFIG_XSTRAP__CRIPPLE_PANELb                      0x00008000
#define CONFIG_XSTRAP__AP_SIZE_MASK                        0x00030000
#define CONFIG_XSTRAP__ROMIDCFG_MASK                       0x00700000
#define CONFIG_XSTRAP__BUSCFG_MASK                         0x07000000

// TEST_DEBUG_CNTL
#define TEST_DEBUG_CNTL__TEST_DEBUG_OUT_EN_MASK            0x00000001
#define TEST_DEBUG_CNTL__TEST_DEBUG_OUT_EN                 0x00000001
#define TEST_DEBUG_CNTL__TEST_DEBUG_IN_EN_MASK             0x00000002
#define TEST_DEBUG_CNTL__TEST_DEBUG_IN_EN                  0x00000002
#define TEST_DEBUG_CNTL__TEST_IDDQ_EN_MASK                 0x00000004
#define TEST_DEBUG_CNTL__TEST_IDDQ_EN                      0x00000004
#define TEST_DEBUG_CNTL__TEST_BLOCK_SEL_MASK               0x00003f00
#define TEST_DEBUG_CNTL__TEST_ENABLE_MASK                  0x00008000
#define TEST_DEBUG_CNTL__TEST_ENABLE                       0x00008000
#define TEST_DEBUG_CNTL__TEST_DELAY_IN_MASK                0x00010000
#define TEST_DEBUG_CNTL__TEST_DELAY_IN                     0x00010000

// TEST_DEBUG_MUX
#define TEST_DEBUG_MUX__TEST_DEBUG_SEL_MASK                0x0000003f
#define TEST_DEBUG_MUX__TEST_CLK0_MASK                     0x00001f00
#define TEST_DEBUG_MUX__TEST_CLK0_INV_MASK                 0x00008000
#define TEST_DEBUG_MUX__TEST_CLK0_INV                      0x00008000
#define TEST_DEBUG_MUX__TEST_CLK1_MASK                     0x001f0000
#define TEST_DEBUG_MUX__TEST_CLK1_INV_MASK                 0x00800000
#define TEST_DEBUG_MUX__TEST_CLK1_INV                      0x00800000

// TEST_DEBUG_OUT
#define TEST_DEBUG_OUT__TEST_DEBUG_OUTR_MASK               0x00000fff

// VIDEOMUX_CNTL
#define VIDEOMUX_CNTL__VIPH_INT_SEL_MASK                   0x00000001
#define VIDEOMUX_CNTL__VIPH_INT_SEL                        0x00000001
#define VIDEOMUX_CNTL__ROM_CLK_DIVIDE_MASK                 0x001f0000
#define VIDEOMUX_CNTL__STR_ROMCLK_MASK                     0x00200000
#define VIDEOMUX_CNTL__STR_ROMCLK                          0x00200000
#define VIDEOMUX_CNTL__VIP_INTERNAL_DEBUG_SEL_MASK         0x01c00000

// VIPPAD_STRENGTH
#define VIPPAD_STRENGTH__I2C_STRENGTH_MASK                 0x00000003
#define VIPPAD_STRENGTH__TVDAT_STRENGTH_MASK               0x00000300
#define VIPPAD_STRENGTH__TVCLK_STRENGTH_MASK               0x00003000
#define VIPPAD_STRENGTH__VIPHDAT_STRENGTH_MASK             0x00030000
#define VIPPAD_STRENGTH__VIPHCLK_STRENGTH_MASK             0x00300000

// VIPPAD_MASK
#define VIPPAD_MASK__VIPPAD_MASK_MASK                      0x0007ffff

// VIPPAD_A
#define VIPPAD_A__VIPPAD_A_MASK                            0x0007ffff

// VIPPAD_EN
#define VIPPAD_EN__VIPPAD_EN_MASK                          0x0007ffff

// VIPPAD_Y
#define VIPPAD_Y__VIPPAD_Y_MASK                            0x0007ffff

// VIPPAD1_MASK
#define VIPPAD1_MASK__VIPPAD1_MASK_MASK                    0x0003ffff

// VIPPAD1_A
#define VIPPAD1_A__VIPPAD1_A_MASK                          0x0003ffff

// VIPPAD1_EN
#define VIPPAD1_EN__VIPPAD1_EN_MASK                        0x0003ffff

// VIPPAD1_Y
#define VIPPAD1_Y__VIPPAD1_Y_MASK                          0x0003ffff

// EXTERN_TRIG_CNTL
#define EXTERN_TRIG_CNTL__EXTERN_TRIG_CLR_MASK             0x00000001
#define EXTERN_TRIG_CNTL__EXTERN_TRIG_CLR                  0x00000001
#define EXTERN_TRIG_CNTL__EXTERN_TRIG_READ_MASK            0x00000002
#define EXTERN_TRIG_CNTL__EXTERN_TRIG_READ                 0x00000002

// SEPROM_CNTL1
#define SEPROM_CNTL1__WRITE_ENABLE_MASK                    0x00000001
#define SEPROM_CNTL1__WRITE_ENABLE                         0x00000001
#define SEPROM_CNTL1__WRITE_DISABLE_MASK                   0x00000002
#define SEPROM_CNTL1__WRITE_DISABLE                        0x00000002
#define SEPROM_CNTL1__READ_CONFIG_MASK                     0x00000004
#define SEPROM_CNTL1__READ_CONFIG                          0x00000004
#define SEPROM_CNTL1__WRITE_CONFIG_MASK                    0x00000008
#define SEPROM_CNTL1__WRITE_CONFIG                         0x00000008
#define SEPROM_CNTL1__READ_STATUS_MASK                     0x00000010
#define SEPROM_CNTL1__READ_STATUS                          0x00000010
#define SEPROM_CNTL1__SECT_TO_SRAM_MASK                    0x00000020
#define SEPROM_CNTL1__SECT_TO_SRAM                         0x00000020
#define SEPROM_CNTL1__READY_BUSY_MASK                      0x00000080
#define SEPROM_CNTL1__READY_BUSY                           0x00000080
#define SEPROM_CNTL1__SEPROM_BUSY_MASK                     0x00000100
#define SEPROM_CNTL1__SEPROM_BUSY                          0x00000100
#define SEPROM_CNTL1__BCNT_OVER_WTE_EN_MASK                0x00000200
#define SEPROM_CNTL1__BCNT_OVER_WTE_EN                     0x00000200
#define SEPROM_CNTL1__RB_MASKB_MASK                        0x00000400
#define SEPROM_CNTL1__RB_MASKB                             0x00000400
#define SEPROM_CNTL1__SOFT_RESET_MASK                      0x00000800
#define SEPROM_CNTL1__SOFT_RESET                           0x00000800
#define SEPROM_CNTL1__STATE_IDLEb_MASK                     0x00001000
#define SEPROM_CNTL1__STATE_IDLEb                          0x00001000
#define SEPROM_CNTL1__BYTE_CNT_MASK                        0x00ff0000
#define SEPROM_CNTL1__SCK_PRESCALE_MASK                    0xff000000

// SEPROM_CNTL2
#define SEPROM_CNTL2__WAIT_CYCLE_MASK                      0x000000ff
#define SEPROM_CNTL2__AUTO_ADDR_SAMPLE_MASK                0x00000100
#define SEPROM_CNTL2__AUTO_ADDR_SAMPLE                     0x00000100
#define SEPROM_CNTL2__SEC_COMMAND_MASK                     0x00ff0000

// VIP_HW_DEBUG
#define VIP_HW_DEBUG__VIP_HW_0_DEBUG_MASK                  0x00000001
#define VIP_HW_DEBUG__VIP_HW_0_DEBUG                       0x00000001
#define VIP_HW_DEBUG__VIP_HW_1_DEBUG_MASK                  0x00000002
#define VIP_HW_DEBUG__VIP_HW_1_DEBUG                       0x00000002
#define VIP_HW_DEBUG__VIP_HW_2_DEBUG_MASK                  0x00000004
#define VIP_HW_DEBUG__VIP_HW_2_DEBUG                       0x00000004
#define VIP_HW_DEBUG__VIP_HW_3_DEBUG_MASK                  0x00000008
#define VIP_HW_DEBUG__VIP_HW_3_DEBUG                       0x00000008
#define VIP_HW_DEBUG__VIP_HW_4_DEBUG_MASK                  0x00000010
#define VIP_HW_DEBUG__VIP_HW_4_DEBUG                       0x00000010
#define VIP_HW_DEBUG__VIP_HW_5_DEBUG_MASK                  0x00000020
#define VIP_HW_DEBUG__VIP_HW_5_DEBUG                       0x00000020
#define VIP_HW_DEBUG__VIP_HW_6_DEBUG_MASK                  0x00000040
#define VIP_HW_DEBUG__VIP_HW_6_DEBUG                       0x00000040
#define VIP_HW_DEBUG__VIP_HW_7_DEBUG_MASK                  0x00000080
#define VIP_HW_DEBUG__VIP_HW_7_DEBUG                       0x00000080
#define VIP_HW_DEBUG__VIP_HW_8_DEBUG_MASK                  0x00000100
#define VIP_HW_DEBUG__VIP_HW_8_DEBUG                       0x00000100
#define VIP_HW_DEBUG__VIP_HW_9_DEBUG_MASK                  0x00000200
#define VIP_HW_DEBUG__VIP_HW_9_DEBUG                       0x00000200
#define VIP_HW_DEBUG__VIP_HW_A_DEBUG_MASK                  0x00000400
#define VIP_HW_DEBUG__VIP_HW_A_DEBUG                       0x00000400
#define VIP_HW_DEBUG__VIP_HW_B_DEBUG_MASK                  0x00000800
#define VIP_HW_DEBUG__VIP_HW_B_DEBUG                       0x00000800
#define VIP_HW_DEBUG__VIP_HW_C_DEBUG_MASK                  0x00001000
#define VIP_HW_DEBUG__VIP_HW_C_DEBUG                       0x00001000
#define VIP_HW_DEBUG__VIP_HW_D_DEBUG_MASK                  0x00002000
#define VIP_HW_DEBUG__VIP_HW_D_DEBUG                       0x00002000
#define VIP_HW_DEBUG__VIP_HW_E_DEBUG_MASK                  0x00004000
#define VIP_HW_DEBUG__VIP_HW_E_DEBUG                       0x00004000
#define VIP_HW_DEBUG__VIP_HW_F_DEBUG_MASK                  0x00008000
#define VIP_HW_DEBUG__VIP_HW_F_DEBUG                       0x00008000

// MEDIA_0_SCRATCH
#define MEDIA_0_SCRATCH__MEDIA_0_SCRATCH_MASK              0xffffffff

// MEDIA_1_SCRATCH
#define MEDIA_1_SCRATCH__MEDIA_1_SCRACH_MASK               0xffffffff

// VID_BUFFER_CONTROL
#define VID_BUFFER_CONTROL__CAP0_BUFFER_WATER_MARK_MASK    0x0000001f
#define VID_BUFFER_CONTROL__FULL_BUFFER_EN_MASK            0x00010000
#define VID_BUFFER_CONTROL__FULL_BUFFER_EN                 0x00010000
#define VID_BUFFER_CONTROL__CAP0_ANC_VBI_QUAD_BUF_MASK     0x00020000
#define VID_BUFFER_CONTROL__CAP0_ANC_VBI_QUAD_BUF          0x00020000
#define VID_BUFFER_CONTROL__VID_BUFFER_RESET_MASK          0x00100000
#define VID_BUFFER_CONTROL__VID_BUFFER_RESET               0x00100000
#define VID_BUFFER_CONTROL__CAP_SWAP_MASK                  0x00600000
#define VID_BUFFER_CONTROL__CAP0_BUFFER_EMPTY_MASK         0x01000000
#define VID_BUFFER_CONTROL__CAP0_BUFFER_EMPTY              0x01000000

// CAP_INT_CNTL
#define CAP_INT_CNTL__CAP0_BUF0_INT_EN_MASK                0x00000001
#define CAP_INT_CNTL__CAP0_BUF0_INT_EN                     0x00000001
#define CAP_INT_CNTL__CAP0_BUF0_EVEN_INT_EN_MASK           0x00000002
#define CAP_INT_CNTL__CAP0_BUF0_EVEN_INT_EN                0x00000002
#define CAP_INT_CNTL__CAP0_BUF1_INT_EN_MASK                0x00000004
#define CAP_INT_CNTL__CAP0_BUF1_INT_EN                     0x00000004
#define CAP_INT_CNTL__CAP0_BUF1_EVEN_INT_EN_MASK           0x00000008
#define CAP_INT_CNTL__CAP0_BUF1_EVEN_INT_EN                0x00000008
#define CAP_INT_CNTL__CAP0_VBI0_INT_EN_MASK                0x00000010
#define CAP_INT_CNTL__CAP0_VBI0_INT_EN                     0x00000010
#define CAP_INT_CNTL__CAP0_VBI1_INT_EN_MASK                0x00000020
#define CAP_INT_CNTL__CAP0_VBI1_INT_EN                     0x00000020
#define CAP_INT_CNTL__CAP0_ONESHOT_INT_EN_MASK             0x00000040
#define CAP_INT_CNTL__CAP0_ONESHOT_INT_EN                  0x00000040
#define CAP_INT_CNTL__CAP0_ANC0_INT_EN_MASK                0x00000080
#define CAP_INT_CNTL__CAP0_ANC0_INT_EN                     0x00000080
#define CAP_INT_CNTL__CAP0_ANC1_INT_EN_MASK                0x00000100
#define CAP_INT_CNTL__CAP0_ANC1_INT_EN                     0x00000100
#define CAP_INT_CNTL__CAP0_VBI2_INT_EN_MASK                0x00000200
#define CAP_INT_CNTL__CAP0_VBI2_INT_EN                     0x00000200
#define CAP_INT_CNTL__CAP0_VBI3_INT_EN_MASK                0x00000400
#define CAP_INT_CNTL__CAP0_VBI3_INT_EN                     0x00000400
#define CAP_INT_CNTL__CAP0_ANC2_INT_EN_MASK                0x00000800
#define CAP_INT_CNTL__CAP0_ANC2_INT_EN                     0x00000800
#define CAP_INT_CNTL__CAP0_ANC3_INT_EN_MASK                0x00001000
#define CAP_INT_CNTL__CAP0_ANC3_INT_EN                     0x00001000

// CAP_INT_STATUS
#define CAP_INT_STATUS__CAP0_BUF0_INT_MASK                 0x00000001
#define CAP_INT_STATUS__CAP0_BUF0_INT                      0x00000001
#define CAP_INT_STATUS__CAP0_BUF0_INT_AK_MASK              0x00000001
#define CAP_INT_STATUS__CAP0_BUF0_INT_AK                   0x00000001
#define CAP_INT_STATUS__CAP0_BUF0_EVEN_INT_MASK            0x00000002
#define CAP_INT_STATUS__CAP0_BUF0_EVEN_INT                 0x00000002
#define CAP_INT_STATUS__CAP0_BUF0_EVEN_INT_AK_MASK         0x00000002
#define CAP_INT_STATUS__CAP0_BUF0_EVEN_INT_AK              0x00000002
#define CAP_INT_STATUS__CAP0_BUF1_INT_MASK                 0x00000004
#define CAP_INT_STATUS__CAP0_BUF1_INT                      0x00000004
#define CAP_INT_STATUS__CAP0_BUF1_INT_AK_MASK              0x00000004
#define CAP_INT_STATUS__CAP0_BUF1_INT_AK                   0x00000004
#define CAP_INT_STATUS__CAP0_BUF1_EVEN_INT_MASK            0x00000008
#define CAP_INT_STATUS__CAP0_BUF1_EVEN_INT                 0x00000008
#define CAP_INT_STATUS__CAP0_BUF1_EVEN_INT_AK_MASK         0x00000008
#define CAP_INT_STATUS__CAP0_BUF1_EVEN_INT_AK              0x00000008
#define CAP_INT_STATUS__CAP0_VBI0_INT_MASK                 0x00000010
#define CAP_INT_STATUS__CAP0_VBI0_INT                      0x00000010
#define CAP_INT_STATUS__CAP0_VBI0_INT_AK_MASK              0x00000010
#define CAP_INT_STATUS__CAP0_VBI0_INT_AK                   0x00000010
#define CAP_INT_STATUS__CAP0_VBI1_INT_MASK                 0x00000020
#define CAP_INT_STATUS__CAP0_VBI1_INT                      0x00000020
#define CAP_INT_STATUS__CAP0_VBI1_INT_AK_MASK              0x00000020
#define CAP_INT_STATUS__CAP0_VBI1_INT_AK                   0x00000020
#define CAP_INT_STATUS__CAP0_ONESHOT_INT_MASK              0x00000040
#define CAP_INT_STATUS__CAP0_ONESHOT_INT                   0x00000040
#define CAP_INT_STATUS__CAP0_ONESHOT_INT_AK_MASK           0x00000040
#define CAP_INT_STATUS__CAP0_ONESHOT_INT_AK                0x00000040
#define CAP_INT_STATUS__CAP0_ANC0_INT_MASK                 0x00000080
#define CAP_INT_STATUS__CAP0_ANC0_INT                      0x00000080
#define CAP_INT_STATUS__CAP0_ANC0_INT_AK_MASK              0x00000080
#define CAP_INT_STATUS__CAP0_ANC0_INT_AK                   0x00000080
#define CAP_INT_STATUS__CAP0_ANC1_INT_MASK                 0x00000100
#define CAP_INT_STATUS__CAP0_ANC1_INT                      0x00000100
#define CAP_INT_STATUS__CAP0_ANC1_INT_AK_MASK              0x00000100
#define CAP_INT_STATUS__CAP0_ANC1_INT_AK                   0x00000100
#define CAP_INT_STATUS__CAP0_VBI2_INT_MASK                 0x00000200
#define CAP_INT_STATUS__CAP0_VBI2_INT                      0x00000200
#define CAP_INT_STATUS__CAP0_VBI2_INT_AK_MASK              0x00000200
#define CAP_INT_STATUS__CAP0_VBI2_INT_AK                   0x00000200
#define CAP_INT_STATUS__CAP0_VBI3_INT_MASK                 0x00000400
#define CAP_INT_STATUS__CAP0_VBI3_INT                      0x00000400
#define CAP_INT_STATUS__CAP0_VBI3_INT_AK_MASK              0x00000400
#define CAP_INT_STATUS__CAP0_VBI3_INT_AK                   0x00000400
#define CAP_INT_STATUS__CAP0_ANC2_INT_MASK                 0x00000800
#define CAP_INT_STATUS__CAP0_ANC2_INT                      0x00000800
#define CAP_INT_STATUS__CAP0_ANC2_INT_AK_MASK              0x00000800
#define CAP_INT_STATUS__CAP0_ANC2_INT_AK                   0x00000800
#define CAP_INT_STATUS__CAP0_ANC3_INT_MASK                 0x00001000
#define CAP_INT_STATUS__CAP0_ANC3_INT                      0x00001000
#define CAP_INT_STATUS__CAP0_ANC3_INT_AK_MASK              0x00001000
#define CAP_INT_STATUS__CAP0_ANC3_INT_AK                   0x00001000

// FCP_CNTL
#define FCP_CNTL__FCP0_SRC_SEL_MASK                        0x00000007

// CAP0_BUF0_OFFSET
#define CAP0_BUF0_OFFSET__CAP_BUF0_OFFSET_MASK             0xffffffff

// CAP0_BUF1_OFFSET
#define CAP0_BUF1_OFFSET__CAP_BUF1_OFFSET_MASK             0xffffffff

// CAP0_BUF0_EVEN_OFFSET
#define CAP0_BUF0_EVEN_OFFSET__CAP_BUF0_EVEN_OFFSET_MASK   0xffffffff

// CAP0_BUF1_EVEN_OFFSET
#define CAP0_BUF1_EVEN_OFFSET__CAP_BUF1_EVEN_OFFSET_MASK   0xffffffff

// CAP0_BUF_PITCH
#define CAP0_BUF_PITCH__CAP_BUF_PITCH_MASK                 0x00000fff

// CAP0_V_WINDOW
#define CAP0_V_WINDOW__CAP_V_START_MASK                    0x00000fff
#define CAP0_V_WINDOW__CAP_V_END_MASK                      0x0fff0000

// CAP0_H_WINDOW
#define CAP0_H_WINDOW__CAP_H_START_MASK                    0x00000fff
#define CAP0_H_WINDOW__CAP_H_WIDTH_MASK                    0x0fff0000

// CAP0_VBI0_OFFSET
#define CAP0_VBI0_OFFSET__CAP_VBI0_OFFSET_MASK             0xffffffff

// CAP0_VBI1_OFFSET
#define CAP0_VBI1_OFFSET__CAP_VBI1_OFFSET_MASK             0xffffffff

// CAP0_VBI_V_WINDOW
#define CAP0_VBI_V_WINDOW__CAP_VBI_V_START_MASK            0x00000fff
#define CAP0_VBI_V_WINDOW__CAP_VBI_V_END_MASK              0x0fff0000

// CAP0_VBI_H_WINDOW
#define CAP0_VBI_H_WINDOW__CAP_VBI_H_START_MASK            0x00000fff
#define CAP0_VBI_H_WINDOW__CAP_VBI_H_WIDTH_MASK            0x0fff0000

// CAP0_PORT_MODE_CNTL
#define CAP0_PORT_MODE_CNTL__CAP_PORT_WIDTH_MASK           0x00000002
#define CAP0_PORT_MODE_CNTL__CAP_PORT_WIDTH                0x00000002
#define CAP0_PORT_MODE_CNTL__CAP_PORT_BYTE_USED_MASK       0x00000004
#define CAP0_PORT_MODE_CNTL__CAP_PORT_BYTE_USED            0x00000004

// CAP0_TRIG_CNTL
#define CAP0_TRIG_CNTL__CAP_TRIGGER_R_MASK                 0x00000003
#define CAP0_TRIG_CNTL__CAP_TRIGGER_W_MASK                 0x00000001
#define CAP0_TRIG_CNTL__CAP_TRIGGER_W                      0x00000001
#define CAP0_TRIG_CNTL__CAP_EN_MASK                        0x00000010
#define CAP0_TRIG_CNTL__CAP_EN                             0x00000010
#define CAP0_TRIG_CNTL__CAP_VSYNC_CNT_MASK                 0x0000ff00
#define CAP0_TRIG_CNTL__CAP_VSYNC_CLR_MASK                 0x00010000
#define CAP0_TRIG_CNTL__CAP_VSYNC_CLR                      0x00010000

// CAP0_DEBUG
#define CAP0_DEBUG__CAP_H_STATUS_MASK                      0x00000fff
#define CAP0_DEBUG__CAP_V_STATUS_MASK                      0x0fff0000
#define CAP0_DEBUG__CAP_V_SYNC_MASK                        0x10000000
#define CAP0_DEBUG__CAP_V_SYNC                             0x10000000

// CAP0_CONFIG
#define CAP0_CONFIG__CAP_INPUT_MODE_MASK                   0x00000001
#define CAP0_CONFIG__CAP_INPUT_MODE                        0x00000001
#define CAP0_CONFIG__CAP_START_FIELD_MASK                  0x00000002
#define CAP0_CONFIG__CAP_START_FIELD                       0x00000002
#define CAP0_CONFIG__CAP_START_BUF_R_MASK                  0x00000004
#define CAP0_CONFIG__CAP_START_BUF_R                       0x00000004
#define CAP0_CONFIG__CAP_START_BUF_W_MASK                  0x00000008
#define CAP0_CONFIG__CAP_START_BUF_W                       0x00000008
#define CAP0_CONFIG__CAP_BUF_TYPE_MASK                     0x00000030
#define CAP0_CONFIG__CAP_ONESHOT_MODE_MASK                 0x00000040
#define CAP0_CONFIG__CAP_ONESHOT_MODE                      0x00000040
#define CAP0_CONFIG__CAP_BUF_MODE_MASK                     0x00000180
#define CAP0_CONFIG__CAP_MIRROR_EN_MASK                    0x00000200
#define CAP0_CONFIG__CAP_MIRROR_EN                         0x00000200
#define CAP0_CONFIG__CAP_ONESHOT_MIRROR_EN_MASK            0x00000400
#define CAP0_CONFIG__CAP_ONESHOT_MIRROR_EN                 0x00000400
#define CAP0_CONFIG__CAP_VIDEO_SIGNED_UV_MASK              0x00000800
#define CAP0_CONFIG__CAP_VIDEO_SIGNED_UV                   0x00000800
#define CAP0_CONFIG__CAP_ANC_DECODE_EN_MASK                0x00001000
#define CAP0_CONFIG__CAP_ANC_DECODE_EN                     0x00001000
#define CAP0_CONFIG__CAP_VBI_EN_MASK                       0x00002000
#define CAP0_CONFIG__CAP_VBI_EN                            0x00002000
#define CAP0_CONFIG__CAP_SOFT_PULL_DOWN_EN_MASK            0x00004000
#define CAP0_CONFIG__CAP_SOFT_PULL_DOWN_EN                 0x00004000
#define CAP0_CONFIG__CAP_VIP_EXTEND_FLAG_EN_MASK           0x00008000
#define CAP0_CONFIG__CAP_VIP_EXTEND_FLAG_EN                0x00008000
#define CAP0_CONFIG__CAP_FAKE_FIELD_EN_MASK                0x00010000
#define CAP0_CONFIG__CAP_FAKE_FIELD_EN                     0x00010000
#define CAP0_CONFIG__CAP_FIELD_START_LINE_DIFF_MASK        0x00060000
#define CAP0_CONFIG__CAP_HORZ_DOWN_MASK                    0x00180000
#define CAP0_CONFIG__CAP_VERT_DOWN_MASK                    0x00600000
#define CAP0_CONFIG__CAP_STREAM_FORMAT_MASK                0x03800000
#define CAP0_CONFIG__CAP_HDWNS_DEC_MASK                    0x04000000
#define CAP0_CONFIG__CAP_HDWNS_DEC                         0x04000000
#define CAP0_CONFIG__CAP_VIDEO_IN_FORMAT_MASK              0x20000000
#define CAP0_CONFIG__CAP_VIDEO_IN_FORMAT                   0x20000000
#define CAP0_CONFIG__VBI_HORZ_DOWN_MASK                    0xc0000000

// CAP0_ANC0_OFFSET
#define CAP0_ANC0_OFFSET__CAP_ANC0_OFFSET_MASK             0xffffffff

// CAP0_ANC1_OFFSET
#define CAP0_ANC1_OFFSET__CAP_ANC1_OFFSET_MASK             0xffffffff

// CAP0_ANC_H_WINDOW
#define CAP0_ANC_H_WINDOW__CAP_ANC_WIDTH_MASK              0x00000fff

// CAP0_VIDEO_SYNC_TEST
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_SOF_MASK        0x00000001
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_SOF             0x00000001
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_EOF_MASK        0x00000002
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_EOF             0x00000002
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_EOL_MASK        0x00000004
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_EOL             0x00000004
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_FIELD_MASK      0x00000008
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_VID_FIELD           0x00000008
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_SYNC_EN_MASK        0x00000020
#define CAP0_VIDEO_SYNC_TEST__CAP_TEST_SYNC_EN             0x00000020

// CAP0_ONESHOT_BUF_OFFSET
#define CAP0_ONESHOT_BUF_OFFSET__CAP_ONESHOT_BUF_OFFSET_MASK 0xffffffff

// CAP0_BUF_STATUS
#define CAP0_BUF_STATUS__CAP_PRE_VID_BUF_MASK              0x00000003
#define CAP0_BUF_STATUS__CAP_CUR_VID_BUF_MASK              0x0000000c
#define CAP0_BUF_STATUS__CAP_PRE_FIELD_MASK                0x00000010
#define CAP0_BUF_STATUS__CAP_PRE_FIELD                     0x00000010
#define CAP0_BUF_STATUS__CAP_CUR_FIELD_MASK                0x00000020
#define CAP0_BUF_STATUS__CAP_CUR_FIELD                     0x00000020
#define CAP0_BUF_STATUS__CAP_PRE_VBI_BUF_MASK              0x000000c0
#define CAP0_BUF_STATUS__CAP_CUR_VBI_BUF_MASK              0x00000300
#define CAP0_BUF_STATUS__CAP_VBI_BUF_STATUS_MASK           0x00000400
#define CAP0_BUF_STATUS__CAP_VBI_BUF_STATUS                0x00000400
#define CAP0_BUF_STATUS__CAP_PRE_ANC_BUF_MASK              0x00001800
#define CAP0_BUF_STATUS__CAP_CUR_ANC_BUF_MASK              0x00006000
#define CAP0_BUF_STATUS__CAP_ANC_BUF_STATUS_MASK           0x00008000
#define CAP0_BUF_STATUS__CAP_ANC_BUF_STATUS                0x00008000
#define CAP0_BUF_STATUS__CAP_ANC_PRE_BUF_CNT_MASK          0x0fff0000
#define CAP0_BUF_STATUS__CAP_VIP_INC_MASK                  0x10000000
#define CAP0_BUF_STATUS__CAP_VIP_INC                       0x10000000
#define CAP0_BUF_STATUS__CAP_VIP_PRE_REPEAT_FIELD_MASK     0x20000000
#define CAP0_BUF_STATUS__CAP_VIP_PRE_REPEAT_FIELD          0x20000000
#define CAP0_BUF_STATUS__CAP_CAP_BUF_STATUS_MASK           0x40000000
#define CAP0_BUF_STATUS__CAP_CAP_BUF_STATUS                0x40000000

// CAP0_ANC_BUF01_BLOCK_CNT
#define CAP0_ANC_BUF01_BLOCK_CNT__CAP0_ANC_BUF0_BLOCK_CNT_MASK 0x00000fff
#define CAP0_ANC_BUF01_BLOCK_CNT__CAP0_ANC_BUF1_BLOCK_CNT_MASK 0x0fff0000

// CAP0_ANC_BUF23_BLOCK_CNT
#define CAP0_ANC_BUF23_BLOCK_CNT__CAP0_ANC_BUF2_BLOCK_CNT_MASK 0x00000fff
#define CAP0_ANC_BUF23_BLOCK_CNT__CAP0_ANC_BUF3_BLOCK_CNT_MASK 0x0fff0000

// CAP0_VBI2_OFFSET
#define CAP0_VBI2_OFFSET__CAP_VBI2_OFFSET_MASK             0xffffffff

// CAP0_VBI3_OFFSET
#define CAP0_VBI3_OFFSET__CAP_VBI3_OFFSET_MASK             0xffffffff

// CAP0_ANC2_OFFSET
#define CAP0_ANC2_OFFSET__CAP_ANC2_OFFSET_MASK             0xffffffff

// CAP0_ANC3_OFFSET
#define CAP0_ANC3_OFFSET__CAP_ANC3_OFFSET_MASK             0xffffffff

// DMA_VIPH0_COMMAND
#define DMA_VIPH0_COMMAND__BYTE_COUNT_MASK                 0x001fffff
#define DMA_VIPH0_COMMAND__SWAP_CONTROL_MASK               0x03000000
#define DMA_VIPH0_COMMAND__TRANSFER_SOURCE_MASK            0x04000000
#define DMA_VIPH0_COMMAND__TRANSFER_SOURCE                 0x04000000
#define DMA_VIPH0_COMMAND__TRANSFER_DEST_MASK              0x08000000
#define DMA_VIPH0_COMMAND__TRANSFER_DEST                   0x08000000
#define DMA_VIPH0_COMMAND__SOURCE_OFFSET_HOLD_MASK         0x10000000
#define DMA_VIPH0_COMMAND__SOURCE_OFFSET_HOLD              0x10000000
#define DMA_VIPH0_COMMAND__DEST_OFFSET_HOLD_MASK           0x20000000
#define DMA_VIPH0_COMMAND__DEST_OFFSET_HOLD                0x20000000
#define DMA_VIPH0_COMMAND__INTERRUPT_DIS_MASK              0x40000000
#define DMA_VIPH0_COMMAND__INTERRUPT_DIS                   0x40000000
#define DMA_VIPH0_COMMAND__END_OF_LIST_STATUS_MASK         0x80000000
#define DMA_VIPH0_COMMAND__END_OF_LIST_STATUS              0x80000000

// DMA_VIPH1_COMMAND
#define DMA_VIPH1_COMMAND__BYTE_COUNT_MASK                 0x001fffff
#define DMA_VIPH1_COMMAND__SWAP_CONTROL_MASK               0x03000000
#define DMA_VIPH1_COMMAND__TRANSFER_SOURCE_MASK            0x04000000
#define DMA_VIPH1_COMMAND__TRANSFER_SOURCE                 0x04000000
#define DMA_VIPH1_COMMAND__TRANSFER_DEST_MASK              0x08000000
#define DMA_VIPH1_COMMAND__TRANSFER_DEST                   0x08000000
#define DMA_VIPH1_COMMAND__SOURCE_OFFSET_HOLD_MASK         0x10000000
#define DMA_VIPH1_COMMAND__SOURCE_OFFSET_HOLD              0x10000000
#define DMA_VIPH1_COMMAND__DEST_OFFSET_HOLD_MASK           0x20000000
#define DMA_VIPH1_COMMAND__DEST_OFFSET_HOLD                0x20000000
#define DMA_VIPH1_COMMAND__INTERRUPT_DIS_MASK              0x40000000
#define DMA_VIPH1_COMMAND__INTERRUPT_DIS                   0x40000000
#define DMA_VIPH1_COMMAND__END_OF_LIST_STATUS_MASK         0x80000000
#define DMA_VIPH1_COMMAND__END_OF_LIST_STATUS              0x80000000

// DMA_VIPH2_COMMAND
#define DMA_VIPH2_COMMAND__BYTE_COUNT_MASK                 0x001fffff
#define DMA_VIPH2_COMMAND__SWAP_CONTROL_MASK               0x03000000
#define DMA_VIPH2_COMMAND__TRANSFER_SOURCE_MASK            0x04000000
#define DMA_VIPH2_COMMAND__TRANSFER_SOURCE                 0x04000000
#define DMA_VIPH2_COMMAND__TRANSFER_DEST_MASK              0x08000000
#define DMA_VIPH2_COMMAND__TRANSFER_DEST                   0x08000000
#define DMA_VIPH2_COMMAND__SOURCE_OFFSET_HOLD_MASK         0x10000000
#define DMA_VIPH2_COMMAND__SOURCE_OFFSET_HOLD              0x10000000
#define DMA_VIPH2_COMMAND__DEST_OFFSET_HOLD_MASK           0x20000000
#define DMA_VIPH2_COMMAND__DEST_OFFSET_HOLD                0x20000000
#define DMA_VIPH2_COMMAND__INTERRUPT_DIS_MASK              0x40000000
#define DMA_VIPH2_COMMAND__INTERRUPT_DIS                   0x40000000
#define DMA_VIPH2_COMMAND__END_OF_LIST_STATUS_MASK         0x80000000
#define DMA_VIPH2_COMMAND__END_OF_LIST_STATUS              0x80000000

// DMA_VIPH3_COMMAND
#define DMA_VIPH3_COMMAND__BYTE_COUNT_MASK                 0x001fffff
#define DMA_VIPH3_COMMAND__SWAP_CONTROL_MASK               0x03000000
#define DMA_VIPH3_COMMAND__TRANSFER_SOURCE_MASK            0x04000000
#define DMA_VIPH3_COMMAND__TRANSFER_SOURCE                 0x04000000
#define DMA_VIPH3_COMMAND__TRANSFER_DEST_MASK              0x08000000
#define DMA_VIPH3_COMMAND__TRANSFER_DEST                   0x08000000
#define DMA_VIPH3_COMMAND__SOURCE_OFFSET_HOLD_MASK         0x10000000
#define DMA_VIPH3_COMMAND__SOURCE_OFFSET_HOLD              0x10000000
#define DMA_VIPH3_COMMAND__DEST_OFFSET_HOLD_MASK           0x20000000
#define DMA_VIPH3_COMMAND__DEST_OFFSET_HOLD                0x20000000
#define DMA_VIPH3_COMMAND__INTERRUPT_DIS_MASK              0x40000000
#define DMA_VIPH3_COMMAND__INTERRUPT_DIS                   0x40000000
#define DMA_VIPH3_COMMAND__END_OF_LIST_STATUS_MASK         0x80000000
#define DMA_VIPH3_COMMAND__END_OF_LIST_STATUS              0x80000000

// DMA_VIPH_STATUS
#define DMA_VIPH_STATUS__DMA_VIPH0_AVAIL_MASK              0x0000000f
#define DMA_VIPH_STATUS__DMA_VIPH1_AVAIL_MASK              0x000000f0
#define DMA_VIPH_STATUS__DMA_VIPH2_AVAIL_MASK              0x00000f00
#define DMA_VIPH_STATUS__DMA_VIPH3_AVAIL_MASK              0x0000f000
#define DMA_VIPH_STATUS__DMA_VIPH0_CURRENT_MASK            0x00030000
#define DMA_VIPH_STATUS__DMA_VIPH1_CURRENT_MASK            0x000c0000
#define DMA_VIPH_STATUS__DMA_VIPH2_CURRENT_MASK            0x00300000
#define DMA_VIPH_STATUS__DMA_VIPH3_CURRENT_MASK            0x00c00000
#define DMA_VIPH_STATUS__DMA_VIPH0_ACTIVE_MASK             0x01000000
#define DMA_VIPH_STATUS__DMA_VIPH0_ACTIVE                  0x01000000
#define DMA_VIPH_STATUS__DMA_VIPH1_ACTIVE_MASK             0x02000000
#define DMA_VIPH_STATUS__DMA_VIPH1_ACTIVE                  0x02000000
#define DMA_VIPH_STATUS__DMA_VIPH2_ACTIVE_MASK             0x04000000
#define DMA_VIPH_STATUS__DMA_VIPH2_ACTIVE                  0x04000000
#define DMA_VIPH_STATUS__DMA_VIPH3_ACTIVE_MASK             0x08000000
#define DMA_VIPH_STATUS__DMA_VIPH3_ACTIVE                  0x08000000

// DMA_VIPH_CHUNK_0
#define DMA_VIPH_CHUNK_0__DMA_VIPH3_TABLE_SWAP_MASK        0x00000003
#define DMA_VIPH_CHUNK_0__DMA_VIPH2_TABLE_SWAP_MASK        0x0000000c
#define DMA_VIPH_CHUNK_0__DMA_VIPH1_TABLE_SWAP_MASK        0x00000030
#define DMA_VIPH_CHUNK_0__DMA_VIPH0_TABLE_SWAP_MASK        0x000000c0
#define DMA_VIPH_CHUNK_0__DMA_VIPH3_NOCHUNK_MASK           0x10000000
#define DMA_VIPH_CHUNK_0__DMA_VIPH3_NOCHUNK                0x10000000
#define DMA_VIPH_CHUNK_0__DMA_VIPH2_NOCHUNK_MASK           0x20000000
#define DMA_VIPH_CHUNK_0__DMA_VIPH2_NOCHUNK                0x20000000
#define DMA_VIPH_CHUNK_0__DMA_VIPH1_NOCHUNK_MASK           0x40000000
#define DMA_VIPH_CHUNK_0__DMA_VIPH1_NOCHUNK                0x40000000
#define DMA_VIPH_CHUNK_0__DMA_VIPH0_NOCHUNK_MASK           0x80000000
#define DMA_VIPH_CHUNK_0__DMA_VIPH0_NOCHUNK                0x80000000

// DMA_VIPH_CHUNK_1_VAL
#define DMA_VIPH_CHUNK_1_VAL__DMA_VIP0_CHUNK_MASK          0x000000ff
#define DMA_VIPH_CHUNK_1_VAL__DMA_VIP1_CHUNK_MASK          0x0000ff00
#define DMA_VIPH_CHUNK_1_VAL__DMA_VIP2_CHUNK_MASK          0x00ff0000
#define DMA_VIPH_CHUNK_1_VAL__DMA_VIP3_CHUNK_MASK          0xff000000

// DMA_VIP0_TABLE_ADDR
#define DMA_VIP0_TABLE_ADDR__DMA_VIPH_TABLE_ADDR_MASK      0xffffffff

// DMA_VIP1_TABLE_ADDR
#define DMA_VIP1_TABLE_ADDR__DMA_VIPH_TABLE_ADDR_MASK      0xffffffff

// DMA_VIP2_TABLE_ADDR
#define DMA_VIP2_TABLE_ADDR__DMA_VIPH_TABLE_ADDR_MASK      0xffffffff

// DMA_VIP3_TABLE_ADDR
#define DMA_VIP3_TABLE_ADDR__DMA_VIPH_TABLE_ADDR_MASK      0xffffffff

// DMA_VIPH0_ACTIVE
#define DMA_VIPH0_ACTIVE__DMA_VIPH_TABLE_ADDR_ACT_MASK     0xffffffff

// DMA_VIPH1_ACTIVE
#define DMA_VIPH1_ACTIVE__DMA_VIPH_TABLE_ADDR_ACT_MASK     0xffffffff

// DMA_VIPH2_ACTIVE
#define DMA_VIPH2_ACTIVE__DMA_VIPH_TABLE_ADDR_ACT_MASK     0xffffffff

// DMA_VIPH3_ACTIVE
#define DMA_VIPH3_ACTIVE__DMA_VIPH_TABLE_ADDR_ACT_MASK     0xffffffff

// DMA_VIPH_ABORT
#define DMA_VIPH_ABORT__DMA_VIPH0_ABORT_QUE_MASK           0x00000007
#define DMA_VIPH_ABORT__DMA_VIPH0_ABORT_EN_MASK            0x00000008
#define DMA_VIPH_ABORT__DMA_VIPH0_ABORT_EN                 0x00000008
#define DMA_VIPH_ABORT__DMA_VIPH1_ABORT_QUE_MASK           0x00000070
#define DMA_VIPH_ABORT__DMA_VIPH1_ABORT_EN_MASK            0x00000080
#define DMA_VIPH_ABORT__DMA_VIPH1_ABORT_EN                 0x00000080
#define DMA_VIPH_ABORT__DMA_VIPH2_ABORT_QUE_MASK           0x00000700
#define DMA_VIPH_ABORT__DMA_VIPH2_ABORT_EN_MASK            0x00000800
#define DMA_VIPH_ABORT__DMA_VIPH2_ABORT_EN                 0x00000800
#define DMA_VIPH_ABORT__DMA_VIPH3_ABORT_QUE_MASK           0x00007000
#define DMA_VIPH_ABORT__DMA_VIPH3_ABORT_EN_MASK            0x00008000
#define DMA_VIPH_ABORT__DMA_VIPH3_ABORT_EN                 0x00008000
#define DMA_VIPH_ABORT__DMA_VIPH0_RESET_MASK               0x00100000
#define DMA_VIPH_ABORT__DMA_VIPH0_RESET                    0x00100000
#define DMA_VIPH_ABORT__DMA_VIPH1_RESET_MASK               0x00200000
#define DMA_VIPH_ABORT__DMA_VIPH1_RESET                    0x00200000
#define DMA_VIPH_ABORT__DMA_VIPH2_RESET_MASK               0x00400000
#define DMA_VIPH_ABORT__DMA_VIPH2_RESET                    0x00400000
#define DMA_VIPH_ABORT__DMA_VIPH3_RESET_MASK               0x00800000
#define DMA_VIPH_ABORT__DMA_VIPH3_RESET                    0x00800000

// VIPH_REG_ADDR
#define VIPH_REG_ADDR__VIPH_REG_AD_MASK                    0x0000ffff

// VIPH_REG_DATA
#define VIPH_REG_DATA__VIPH_REG_DT_R_MASK                  0xffffffff
#define VIPH_REG_DATA__VIPH_REG_DT_W_MASK                  0xffffffff

// VIPH_CH0_DATA
#define VIPH_CH0_DATA__VIPH_CH0_DT_MASK                    0xffffffff

// VIPH_CH1_DATA
#define VIPH_CH1_DATA__VIPH_CH1_DT_MASK                    0xffffffff

// VIPH_CH2_DATA
#define VIPH_CH2_DATA__VIPH_CH2_DT_MASK                    0xffffffff

// VIPH_CH3_DATA
#define VIPH_CH3_DATA__VIPH_CH3_DT_MASK                    0xffffffff

// VIPH_CH0_ADDR
#define VIPH_CH0_ADDR__VIPH_CH0_AD_MASK                    0x000000ff

// VIPH_CH1_ADDR
#define VIPH_CH1_ADDR__VIPH_CH1_AD_MASK                    0x000000ff

// VIPH_CH2_ADDR
#define VIPH_CH2_ADDR__VIPH_CH2_AD_MASK                    0x000000ff

// VIPH_CH3_ADDR
#define VIPH_CH3_ADDR__VIPH_CH3_AD_MASK                    0x000000ff

// VIPH_CH0_SBCNT
#define VIPH_CH0_SBCNT__VIPH_CH0_SCNT_MASK                 0x000fffff

// VIPH_CH1_SBCNT
#define VIPH_CH1_SBCNT__VIPH_CH1_SCNT_MASK                 0x000fffff

// VIPH_CH2_SBCNT
#define VIPH_CH2_SBCNT__VIPH_CH2_SCNT_MASK                 0x000fffff

// VIPH_CH3_SBCNT
#define VIPH_CH3_SBCNT__VIPH_CH3_SCNT_MASK                 0x000fffff

// VIPH_CH0_ABCNT
#define VIPH_CH0_ABCNT__VIPH_CH0_ACNT_MASK                 0x000fffff

// VIPH_CH1_ABCNT
#define VIPH_CH1_ABCNT__VIPH_CH1_ACNT_MASK                 0x000fffff

// VIPH_CH2_ABCNT
#define VIPH_CH2_ABCNT__VIPH_CH2_ACNT_MASK                 0x000fffff

// VIPH_CH3_ABCNT
#define VIPH_CH3_ABCNT__VIPH_CH3_ACNT_MASK                 0x000fffff

// VIPH_CONTROL
#define VIPH_CONTROL__VIPH_CLK_SEL_MASK                    0x000000ff
#define VIPH_CONTROL__VIPH_REG_RDY_MASK                    0x00002000
#define VIPH_CONTROL__VIPH_REG_RDY                         0x00002000
#define VIPH_CONTROL__VIPH_MAX_WAIT_MASK                   0x000f0000
#define VIPH_CONTROL__VIPH_DMA_MODE_MASK                   0x00100000
#define VIPH_CONTROL__VIPH_DMA_MODE                        0x00100000
#define VIPH_CONTROL__VIPH_EN_MASK                         0x00200000
#define VIPH_CONTROL__VIPH_EN                              0x00200000
#define VIPH_CONTROL__VIPH_DV0_WID_MASK                    0x01000000
#define VIPH_CONTROL__VIPH_DV0_WID                         0x01000000
#define VIPH_CONTROL__VIPH_DV1_WID_MASK                    0x02000000
#define VIPH_CONTROL__VIPH_DV1_WID                         0x02000000
#define VIPH_CONTROL__VIPH_DV2_WID_MASK                    0x04000000
#define VIPH_CONTROL__VIPH_DV2_WID                         0x04000000
#define VIPH_CONTROL__VIPH_DV3_WID_MASK                    0x08000000
#define VIPH_CONTROL__VIPH_DV3_WID                         0x08000000
#define VIPH_CONTROL__VIPH_PWR_DOWN_MASK                   0x10000000
#define VIPH_CONTROL__VIPH_PWR_DOWN                        0x10000000
#define VIPH_CONTROL__VIPH_PWR_DOWN_AK_MASK                0x10000000
#define VIPH_CONTROL__VIPH_PWR_DOWN_AK                     0x10000000
#define VIPH_CONTROL__VIPH_VIPCLK_DIS_MASK                 0x20000000
#define VIPH_CONTROL__VIPH_VIPCLK_DIS                      0x20000000

// VIPH_DV_LAT
#define VIPH_DV_LAT__VIPH_TIME_UNIT_MASK                   0x00000fff
#define VIPH_DV_LAT__VIPH_DV0_LAT_MASK                     0x000f0000
#define VIPH_DV_LAT__VIPH_DV1_LAT_MASK                     0x00f00000
#define VIPH_DV_LAT__VIPH_DV2_LAT_MASK                     0x0f000000
#define VIPH_DV_LAT__VIPH_DV3_LAT_MASK                     0xf0000000

// VIPH_DMA_CHUNK
#define VIPH_DMA_CHUNK__VIPH_CH0_CHUNK_MASK                0x0000000f
#define VIPH_DMA_CHUNK__VIPH_CH1_CHUNK_MASK                0x00000030
#define VIPH_DMA_CHUNK__VIPH_CH2_CHUNK_MASK                0x000000c0
#define VIPH_DMA_CHUNK__VIPH_CH3_CHUNK_MASK                0x00000300
#define VIPH_DMA_CHUNK__VIPH_CH0_ABORT_MASK                0x00010000
#define VIPH_DMA_CHUNK__VIPH_CH0_ABORT                     0x00010000
#define VIPH_DMA_CHUNK__VIPH_CH1_ABORT_MASK                0x00020000
#define VIPH_DMA_CHUNK__VIPH_CH1_ABORT                     0x00020000
#define VIPH_DMA_CHUNK__VIPH_CH2_ABORT_MASK                0x00040000
#define VIPH_DMA_CHUNK__VIPH_CH2_ABORT                     0x00040000
#define VIPH_DMA_CHUNK__VIPH_CH3_ABORT_MASK                0x00080000
#define VIPH_DMA_CHUNK__VIPH_CH3_ABORT                     0x00080000

// VIPH_DV_INT
#define VIPH_DV_INT__VIPH_DV0_INT_EN_MASK                  0x00000001
#define VIPH_DV_INT__VIPH_DV0_INT_EN                       0x00000001
#define VIPH_DV_INT__VIPH_DV1_INT_EN_MASK                  0x00000002
#define VIPH_DV_INT__VIPH_DV1_INT_EN                       0x00000002
#define VIPH_DV_INT__VIPH_DV2_INT_EN_MASK                  0x00000004
#define VIPH_DV_INT__VIPH_DV2_INT_EN                       0x00000004
#define VIPH_DV_INT__VIPH_DV3_INT_EN_MASK                  0x00000008
#define VIPH_DV_INT__VIPH_DV3_INT_EN                       0x00000008
#define VIPH_DV_INT__VIPH_DV0_INT_MASK                     0x00000010
#define VIPH_DV_INT__VIPH_DV0_INT                          0x00000010
#define VIPH_DV_INT__VIPH_DV0_AK_MASK                      0x00000010
#define VIPH_DV_INT__VIPH_DV0_AK                           0x00000010
#define VIPH_DV_INT__VIPH_DV1_INT_MASK                     0x00000020
#define VIPH_DV_INT__VIPH_DV1_INT                          0x00000020
#define VIPH_DV_INT__VIPH_DV1_AK_MASK                      0x00000020
#define VIPH_DV_INT__VIPH_DV1_AK                           0x00000020
#define VIPH_DV_INT__VIPH_DV2_INT_MASK                     0x00000040
#define VIPH_DV_INT__VIPH_DV2_INT                          0x00000040
#define VIPH_DV_INT__VIPH_DV2_AK_MASK                      0x00000040
#define VIPH_DV_INT__VIPH_DV2_AK                           0x00000040
#define VIPH_DV_INT__VIPH_DV3_INT_MASK                     0x00000080
#define VIPH_DV_INT__VIPH_DV3_INT                          0x00000080
#define VIPH_DV_INT__VIPH_DV3_AK_MASK                      0x00000080
#define VIPH_DV_INT__VIPH_DV3_AK                           0x00000080

// VIPH_TIMEOUT_STAT
#define VIPH_TIMEOUT_STAT__VIPH_FIFO0_STAT_MASK            0x00000001
#define VIPH_TIMEOUT_STAT__VIPH_FIFO0_STAT                 0x00000001
#define VIPH_TIMEOUT_STAT__VIPH_FIFO0_AK_MASK              0x00000001
#define VIPH_TIMEOUT_STAT__VIPH_FIFO0_AK                   0x00000001
#define VIPH_TIMEOUT_STAT__VIPH_FIFO1_STAT_MASK            0x00000002
#define VIPH_TIMEOUT_STAT__VIPH_FIFO1_STAT                 0x00000002
#define VIPH_TIMEOUT_STAT__VIPH_FIFO1_AK_MASK              0x00000002
#define VIPH_TIMEOUT_STAT__VIPH_FIFO1_AK                   0x00000002
#define VIPH_TIMEOUT_STAT__VIPH_FIFO2_STAT_MASK            0x00000004
#define VIPH_TIMEOUT_STAT__VIPH_FIFO2_STAT                 0x00000004
#define VIPH_TIMEOUT_STAT__VIPH_FIFO2_AK_MASK              0x00000004
#define VIPH_TIMEOUT_STAT__VIPH_FIFO2_AK                   0x00000004
#define VIPH_TIMEOUT_STAT__VIPH_FIFO3_STAT_MASK            0x00000008
#define VIPH_TIMEOUT_STAT__VIPH_FIFO3_STAT                 0x00000008
#define VIPH_TIMEOUT_STAT__VIPH_FIFO3_AK_MASK              0x00000008
#define VIPH_TIMEOUT_STAT__VIPH_FIFO3_AK                   0x00000008
#define VIPH_TIMEOUT_STAT__VIPH_REG_STAT_MASK              0x00000010
#define VIPH_TIMEOUT_STAT__VIPH_REG_STAT                   0x00000010
#define VIPH_TIMEOUT_STAT__VIPH_REG_AK_MASK                0x00000010
#define VIPH_TIMEOUT_STAT__VIPH_REG_AK                     0x00000010
#define VIPH_TIMEOUT_STAT__VIPH_AUTO_INT_STAT_MASK         0x00000020
#define VIPH_TIMEOUT_STAT__VIPH_AUTO_INT_STAT              0x00000020
#define VIPH_TIMEOUT_STAT__VIPH_AUTO_INT_AK_MASK           0x00000020
#define VIPH_TIMEOUT_STAT__VIPH_AUTO_INT_AK                0x00000020
#define VIPH_TIMEOUT_STAT__VIPH_FIFO0_MASK_MASK            0x00000100
#define VIPH_TIMEOUT_STAT__VIPH_FIFO0_MASK                 0x00000100
#define VIPH_TIMEOUT_STAT__VIPH_FIFO1_MASK_MASK            0x00000200
#define VIPH_TIMEOUT_STAT__VIPH_FIFO1_MASK                 0x00000200
#define VIPH_TIMEOUT_STAT__VIPH_FIFO2_MASK_MASK            0x00000400
#define VIPH_TIMEOUT_STAT__VIPH_FIFO2_MASK                 0x00000400
#define VIPH_TIMEOUT_STAT__VIPH_FIFO3_MASK_MASK            0x00000800
#define VIPH_TIMEOUT_STAT__VIPH_FIFO3_MASK                 0x00000800
#define VIPH_TIMEOUT_STAT__VIPH_REG_MASK_MASK              0x00001000
#define VIPH_TIMEOUT_STAT__VIPH_REG_MASK                   0x00001000
#define VIPH_TIMEOUT_STAT__VIPH_AUTO_INT_MASK_MASK         0x00002000
#define VIPH_TIMEOUT_STAT__VIPH_AUTO_INT_MASK              0x00002000
#define VIPH_TIMEOUT_STAT__VIPH_DV0_INT_MASK_MASK          0x00010000
#define VIPH_TIMEOUT_STAT__VIPH_DV0_INT_MASK               0x00010000
#define VIPH_TIMEOUT_STAT__VIPH_DV1_INT_MASK_MASK          0x00020000
#define VIPH_TIMEOUT_STAT__VIPH_DV1_INT_MASK               0x00020000
#define VIPH_TIMEOUT_STAT__VIPH_DV2_INT_MASK_MASK          0x00040000
#define VIPH_TIMEOUT_STAT__VIPH_DV2_INT_MASK               0x00040000
#define VIPH_TIMEOUT_STAT__VIPH_DV3_INT_MASK_MASK          0x00080000
#define VIPH_TIMEOUT_STAT__VIPH_DV3_INT_MASK               0x00080000
#define VIPH_TIMEOUT_STAT__VIPH_INTPIN_EN_MASK             0x00100000
#define VIPH_TIMEOUT_STAT__VIPH_INTPIN_EN                  0x00100000
#define VIPH_TIMEOUT_STAT__VIPH_INTPIN_INT_MASK            0x00200000
#define VIPH_TIMEOUT_STAT__VIPH_INTPIN_INT                 0x00200000
#define VIPH_TIMEOUT_STAT__VIPH_REGR_DIS_MASK              0x01000000
#define VIPH_TIMEOUT_STAT__VIPH_REGR_DIS                   0x01000000

// TMDS_CNTL
#define TMDS_CNTL__TMDS_CTL0_MASK                          0x00000001
#define TMDS_CNTL__TMDS_CTL0                               0x00000001
#define TMDS_CNTL__TMDS_CTL1_MASK                          0x00000002
#define TMDS_CNTL__TMDS_CTL1                               0x00000002
#define TMDS_CNTL__TMDS_CTL2_MASK                          0x00000004
#define TMDS_CNTL__TMDS_CTL2                               0x00000004
#define TMDS_CNTL__TMDS_CTL3_MASK                          0x00000008
#define TMDS_CNTL__TMDS_CTL3                               0x00000008
#define TMDS_CNTL__TMDS_DEBUG_HSYNC_MASK                   0x00000010
#define TMDS_CNTL__TMDS_DEBUG_HSYNC                        0x00000010
#define TMDS_CNTL__TMDS_DEBUG_VSYNC_MASK                   0x00000020
#define TMDS_CNTL__TMDS_DEBUG_VSYNC                        0x00000020
#define TMDS_CNTL__TMDS_DEBUG_DE_MASK                      0x00000040
#define TMDS_CNTL__TMDS_DEBUG_DE                           0x00000040
#define TMDS_CNTL__TMDS_DEBUG_EN_MASK                      0x00000080
#define TMDS_CNTL__TMDS_DEBUG_EN                           0x00000080
#define TMDS_CNTL__TMDS_CTL_FB_SEL_MASK                    0x00000300
#define TMDS_CNTL__TMDS_CTL_FB_DEL_MASK                    0x00000c00
#define TMDS_CNTL__TMDS_CTL3_SEL_MASK                      0x00001000
#define TMDS_CNTL__TMDS_CTL3_SEL                           0x00001000
#define TMDS_CNTL__TMDS_SYNC_CHAR_EN_MASK                  0x000f0000
#define TMDS_CNTL__TMDS_SYNC_CONT_MASK                     0x01000000
#define TMDS_CNTL__TMDS_SYNC_CONT                          0x01000000
#define TMDS_CNTL__TMDS_DPCUM_TST_MASK                     0x02000000
#define TMDS_CNTL__TMDS_DPCUM_TST                          0x02000000
#define TMDS_CNTL__TMDS_DPCUM_IN_MASK                      0x3c000000
#define TMDS_CNTL__TMDS_CRC_EN_MASK                        0x40000000
#define TMDS_CNTL__TMDS_CRC_EN                             0x40000000
#define TMDS_CNTL__TMDS_RB_SWITCH_EN_MASK                  0x80000000
#define TMDS_CNTL__TMDS_RB_SWITCH_EN                       0x80000000

// TMDS_SYNC_CHAR_SETA
#define TMDS_SYNC_CHAR_SETA__TMDS_SYNC_CHAR0_MASK          0x000003ff
#define TMDS_SYNC_CHAR_SETA__TMDS_SYNC_CHAR1_MASK          0x03ff0000

// TMDS_SYNC_CHAR_SETB
#define TMDS_SYNC_CHAR_SETB__TMDS_SYNC_CHAR2_MASK          0x000003ff
#define TMDS_SYNC_CHAR_SETB__TMDS_SYNC_CHAR3_MASK          0x03ff0000

// TMDS_CRC
#define TMDS_CRC__TMDS_CRCRGB_MASK                         0x3fffffff

// TMDS_TRANSMITTER_CNTL
#define TMDS_TRANSMITTER_CNTL__TMDS_PLLEN_MASK             0x00000001
#define TMDS_TRANSMITTER_CNTL__TMDS_PLLEN                  0x00000001
#define TMDS_TRANSMITTER_CNTL__TMDS_PLLRST_MASK            0x00000002
#define TMDS_TRANSMITTER_CNTL__TMDS_PLLRST                 0x00000002
#define TMDS_TRANSMITTER_CNTL__TMDS_MODE_SEL_MASK          0x0000000c
#define TMDS_TRANSMITTER_CNTL__TMDS_REGSEL_MASK            0x00000030
#define TMDS_TRANSMITTER_CNTL__TMDS_HALF_CLK_RST_MASK      0x00000040
#define TMDS_TRANSMITTER_CNTL__TMDS_HALF_CLK_RST           0x00000040
#define TMDS_TRANSMITTER_CNTL__TMDS_RAN_PAT_RST_MASK       0x00000080
#define TMDS_TRANSMITTER_CNTL__TMDS_RAN_PAT_RST            0x00000080
#define TMDS_TRANSMITTER_CNTL__TMDS_TSTPIX_MASK            0x0003ff00
#define TMDS_TRANSMITTER_CNTL__TMDS_REG_MASK               0x0ffc0000
#define TMDS_TRANSMITTER_CNTL__ICHCSEL_MASK                0x10000000
#define TMDS_TRANSMITTER_CNTL__ICHCSEL                     0x10000000
#define TMDS_TRANSMITTER_CNTL__ITCLKSEL_MASK               0x20000000
#define TMDS_TRANSMITTER_CNTL__ITCLKSEL                    0x20000000
#define TMDS_TRANSMITTER_CNTL__TMDS_RAN_PAT_SEL_MASK       0x40000000
#define TMDS_TRANSMITTER_CNTL__TMDS_RAN_PAT_SEL            0x40000000

// TMDS_PLL_CNTL
#define TMDS_PLL_CNTL__TMDS_PLLPCP_MASK                    0x00000007
#define TMDS_PLL_CNTL__TMDS_PLLPVG_MASK                    0x00000038
#define TMDS_PLL_CNTL__TMDS_PLLPDC_MASK                    0x000000c0
#define TMDS_PLL_CNTL__TMDS_PLLPVS_MASK                    0x00000f00

// TMDS_PATTERN_GEN_SEED
#define TMDS_PATTERN_GEN_SEED__PATTERN_SEED_MASK           0x00ffffff

// CLK_PWRMGT_CNTL
#define CLK_PWRMGT_CNTL__MPLL_PWRMGT_OFF_MASK              0x00000001
#define CLK_PWRMGT_CNTL__MPLL_PWRMGT_OFF                   0x00000001
#define CLK_PWRMGT_CNTL__SPLL_PWRMGT_OFF_MASK              0x00000002
#define CLK_PWRMGT_CNTL__SPLL_PWRMGT_OFF                   0x00000002
#define CLK_PWRMGT_CNTL__PPLL_PWRMGT_OFF_MASK              0x00000004
#define CLK_PWRMGT_CNTL__PPLL_PWRMGT_OFF                   0x00000004
#define CLK_PWRMGT_CNTL__MCLK_TURNOFF_MASK                 0x00000010
#define CLK_PWRMGT_CNTL__MCLK_TURNOFF                      0x00000010
#define CLK_PWRMGT_CNTL__SCLK_TURNOFF_MASK                 0x00000020
#define CLK_PWRMGT_CNTL__SCLK_TURNOFF                      0x00000020
#define CLK_PWRMGT_CNTL__PCLK_TURNOFF_MASK                 0x00000040
#define CLK_PWRMGT_CNTL__PCLK_TURNOFF                      0x00000040
#define CLK_PWRMGT_CNTL__MC_CH_MODE_MASK                   0x00000100
#define CLK_PWRMGT_CNTL__MC_CH_MODE                        0x00000100
#define CLK_PWRMGT_CNTL__TEST_MODE_MASK                    0x00000200
#define CLK_PWRMGT_CNTL__TEST_MODE                         0x00000200
#define CLK_PWRMGT_CNTL__GLOBAL_PMAN_EN_MASK               0x00000400
#define CLK_PWRMGT_CNTL__GLOBAL_PMAN_EN                    0x00000400
#define CLK_PWRMGT_CNTL__ENGINE_DYNCLK_MODE_MASK           0x00001000
#define CLK_PWRMGT_CNTL__ENGINE_DYNCLK_MODE                0x00001000
#define CLK_PWRMGT_CNTL__MC_BUSY_MASK                      0x00010000
#define CLK_PWRMGT_CNTL__MC_BUSY                           0x00010000
#define CLK_PWRMGT_CNTL__MC_INT_CNTL_MASK                  0x00020000
#define CLK_PWRMGT_CNTL__MC_INT_CNTL                       0x00020000
#define CLK_PWRMGT_CNTL__MC_SWITCH_MASK                    0x00040000
#define CLK_PWRMGT_CNTL__MC_SWITCH                         0x00040000
#define CLK_PWRMGT_CNTL__DLL_READY_MASK                    0x00080000
#define CLK_PWRMGT_CNTL__DLL_READY                         0x00080000
#define CLK_PWRMGT_CNTL__CG_NO1_DEBUG_MASK                 0xff000000

// PLL_PWRMGT_CNTL
#define PLL_PWRMGT_CNTL__MPLL_TURNOFF_MASK                 0x00000001
#define PLL_PWRMGT_CNTL__MPLL_TURNOFF                      0x00000001
#define PLL_PWRMGT_CNTL__SPLL_TURNOFF_MASK                 0x00000002
#define PLL_PWRMGT_CNTL__SPLL_TURNOFF                      0x00000002
#define PLL_PWRMGT_CNTL__PPLL_TURNOFF_MASK                 0x00000004
#define PLL_PWRMGT_CNTL__PPLL_TURNOFF                      0x00000004
#define PLL_PWRMGT_CNTL__CG_NO2_DEBUG_MASK                 0xff000000

// CLK_PIN_CNTL
#define CLK_PIN_CNTL__OSC_EN_MASK                          0x00000001
#define CLK_PIN_CNTL__OSC_EN                               0x00000001
#define CLK_PIN_CNTL__XTL_LOW_GAIN_MASK                    0x00000004
#define CLK_PIN_CNTL__XTL_LOW_GAIN                         0x00000004

// PPLL_CNTL
#define PPLL_CNTL__PPLL_RESET_MASK                         0x00000001
#define PPLL_CNTL__PPLL_RESET                              0x00000001
#define PPLL_CNTL__PPLL_SLEEP_MASK                         0x00000002
#define PPLL_CNTL__PPLL_SLEEP                              0x00000002
#define PPLL_CNTL__PPLL_TST_EN_MASK                        0x00000004
#define PPLL_CNTL__PPLL_TST_EN                             0x00000004
#define PPLL_CNTL__PPLL_REFCLK_SEL_MASK                    0x00000010
#define PPLL_CNTL__PPLL_REFCLK_SEL                         0x00000010
#define PPLL_CNTL__PPLL_FBCLK_SEL_MASK                     0x00000020
#define PPLL_CNTL__PPLL_FBCLK_SEL                          0x00000020
#define PPLL_CNTL__PPLL_TCPOFF_MASK                        0x00000040
#define PPLL_CNTL__PPLL_TCPOFF                             0x00000040
#define PPLL_CNTL__PPLL_TVCOMAX_MASK                       0x00000080
#define PPLL_CNTL__PPLL_TVCOMAX                            0x00000080
#define PPLL_CNTL__PPLL_PCP_MASK                           0x00000700
#define PPLL_CNTL__PPLL_PVG_MASK                           0x00003800
#define PPLL_CNTL__PPLL_PDC_MASK                           0x0000c000
#define PPLL_CNTL__PPLL_ATOMIC_UPDATE_EN_MASK              0x00010000
#define PPLL_CNTL__PPLL_ATOMIC_UPDATE_EN                   0x00010000
#define PPLL_CNTL__PPLL_VGA_ATOMIC_UPDATE_EN_MASK          0x00020000
#define PPLL_CNTL__PPLL_VGA_ATOMIC_UPDATE_EN               0x00020000
#define PPLL_CNTL__PPLL_ATOMIC_UPDATE_SYNC_MASK            0x00040000
#define PPLL_CNTL__PPLL_ATOMIC_UPDATE_SYNC                 0x00040000

// PPLL_REF_DIV
#define PPLL_REF_DIV__PPLL_REF_DIV_MASK                    0x000003ff
#define PPLL_REF_DIV__PPLL_ATOMIC_UPDATE_W_MASK            0x00008000
#define PPLL_REF_DIV__PPLL_ATOMIC_UPDATE_W                 0x00008000
#define PPLL_REF_DIV__PPLL_ATOMIC_UPDATE_R_MASK            0x00008000
#define PPLL_REF_DIV__PPLL_ATOMIC_UPDATE_R                 0x00008000
#define PPLL_REF_DIV__PPLL_REF_DIV_SRC_MASK                0x00030000

// M_SPLL_REF_FB_DIV
#define M_SPLL_REF_FB_DIV__M_SPLL_REF_DIV_MASK             0x000000ff
#define M_SPLL_REF_FB_DIV__MPLL_FB_DIV_MASK                0x0000ff00
#define M_SPLL_REF_FB_DIV__SPLL_FB_DIV_MASK                0x00ff0000
#define M_SPLL_REF_FB_DIV__MPLL_REF_SRC_SEL_MASK           0x01000000
#define M_SPLL_REF_FB_DIV__MPLL_REF_SRC_SEL                0x01000000

// SPLL_CNTL
#define SPLL_CNTL__SPLL_SLEEP_MASK                         0x00000001
#define SPLL_CNTL__SPLL_SLEEP                              0x00000001
#define SPLL_CNTL__SPLL_RESET_MASK                         0x00000002
#define SPLL_CNTL__SPLL_RESET                              0x00000002
#define SPLL_CNTL__SPLL_TST_EN_MASK                        0x00000004
#define SPLL_CNTL__SPLL_TST_EN                             0x00000004
#define SPLL_CNTL__SPLL_REFCLK_SEL_MASK                    0x00000010
#define SPLL_CNTL__SPLL_REFCLK_SEL                         0x00000010
#define SPLL_CNTL__SPLL_FBCLK_SEL_MASK                     0x00000020
#define SPLL_CNTL__SPLL_FBCLK_SEL                          0x00000020
#define SPLL_CNTL__SPLL_TCPOFF_MASK                        0x00000040
#define SPLL_CNTL__SPLL_TCPOFF                             0x00000040
#define SPLL_CNTL__SPLL_TVCOMAX_MASK                       0x00000080
#define SPLL_CNTL__SPLL_TVCOMAX                            0x00000080
#define SPLL_CNTL__SPLL_PCP_MASK                           0x00000700
#define SPLL_CNTL__SPLL_PVG_MASK                           0x00003800
#define SPLL_CNTL__SPLL_PDC_MASK                           0x0000c000
#define SPLL_CNTL__SPLL_X1_CLK_SKEW_MASK                   0x00070000
#define SPLL_CNTL__SPLL_X2_CLK_SKEW_MASK                   0x00700000
#define SPLL_CNTL__SPLL_MODE_MASK                          0x0f000000

// SCLK_CNTL
#define SCLK_CNTL__SCLK_SRC_SEL_MASK                       0x00000007
#define SCLK_CNTL__TCLK_SRC_SEL_MASK                       0x00000700
#define SCLK_CNTL__FORCE_CP_MASK                           0x00010000
#define SCLK_CNTL__FORCE_CP                                0x00010000
#define SCLK_CNTL__FORCE_HDP_MASK                          0x00020000
#define SCLK_CNTL__FORCE_HDP                               0x00020000
#define SCLK_CNTL__FORCE_DISP_MASK                         0x00040000
#define SCLK_CNTL__FORCE_DISP                              0x00040000
#define SCLK_CNTL__FORCE_TOP_MASK                          0x00080000
#define SCLK_CNTL__FORCE_TOP                               0x00080000
#define SCLK_CNTL__FORCE_E2_MASK                           0x00100000
#define SCLK_CNTL__FORCE_E2                                0x00100000
#define SCLK_CNTL__FORCE_SE_MASK                           0x00200000
#define SCLK_CNTL__FORCE_SE                                0x00200000
#define SCLK_CNTL__FORCE_IDCT_MASK                         0x00400000
#define SCLK_CNTL__FORCE_IDCT                              0x00400000
#define SCLK_CNTL__FORCE_VIP_MASK                          0x00800000
#define SCLK_CNTL__FORCE_VIP                               0x00800000
#define SCLK_CNTL__FORCE_RE_MASK                           0x01000000
#define SCLK_CNTL__FORCE_RE                                0x01000000
#define SCLK_CNTL__FORCE_PB_MASK                           0x02000000
#define SCLK_CNTL__FORCE_PB                                0x02000000
#define SCLK_CNTL__FORCE_TAM_MASK                          0x04000000
#define SCLK_CNTL__FORCE_TAM                               0x04000000
#define SCLK_CNTL__FORCE_TDM_MASK                          0x08000000
#define SCLK_CNTL__FORCE_TDM                               0x08000000
#define SCLK_CNTL__FORCE_RB_MASK                           0x10000000
#define SCLK_CNTL__FORCE_RB                                0x10000000

// AGP_PLL_CNTL
#define AGP_PLL_CNTL__APLL_SLEEP_MASK                      0x00000001
#define AGP_PLL_CNTL__APLL_SLEEP                           0x00000001
#define AGP_PLL_CNTL__APLL_RESET_MASK                      0x00000002
#define AGP_PLL_CNTL__APLL_RESET                           0x00000002
#define AGP_PLL_CNTL__APLL_XSEL_MASK                       0x0000000c
#define AGP_PLL_CNTL__APLL_TST_EN_MASK                     0x00000010
#define AGP_PLL_CNTL__APLL_TST_EN                          0x00000010
#define AGP_PLL_CNTL__APLL_TCPOFF_MASK                     0x00000020
#define AGP_PLL_CNTL__APLL_TCPOFF                          0x00000020
#define AGP_PLL_CNTL__APLL_TVCOMAX_MASK                    0x00000040
#define AGP_PLL_CNTL__APLL_TVCOMAX                         0x00000040
#define AGP_PLL_CNTL__APLL_REF_SKEW_MASK                   0x00000380
#define AGP_PLL_CNTL__APLL_FB_SKEW_MASK                    0x00001c00
#define AGP_PLL_CNTL__APLL_X0_CLK_SKEW_MASK                0x0000e000
#define AGP_PLL_CNTL__APLL_X1_CLK_SKEW_MASK                0x00070000
#define AGP_PLL_CNTL__APLL_X2_CLK_SKEW_MASK                0x00380000
#define AGP_PLL_CNTL__APLL_X4_CLK_SKEW_MASK                0x01c00000
#define AGP_PLL_CNTL__APLL_PUMP_GAIN_MASK                  0x0e000000
#define AGP_PLL_CNTL__APLL_VCO_GAIN_MASK                   0x70000000

// CG_TEST_MACRO_RW_WRITE
#define CG_TEST_MACRO_RW_WRITE__TEST_MACRO_RW_WRITE1_MASK  0x00003fff
#define CG_TEST_MACRO_RW_WRITE__TEST_MACRO_RW_WRITE2_MASK  0x0fffc000

// CG_TEST_MACRO_RW_READ
#define CG_TEST_MACRO_RW_READ__TEST_MACRO_RW_READ1_MASK    0x0000ffff
#define CG_TEST_MACRO_RW_READ__TEST_MACRO_RW_READ2_MASK    0xffff0000

// CG_TEST_MACRO_RW_DATA
#define CG_TEST_MACRO_RW_DATA__TEST_MACRO_RW_DATA_MASK     0xffffffff

// CG_TEST_MACRO_RW_CNTL
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_START_MASK    0x00000001
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_START         0x00000001
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_OP_MASK       0x0000000e
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MODE_MASK     0x00000030
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MISMATCH_SEL_MASK 0x00007fc0
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MISMATCH_MASK 0x00008000
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_MISMATCH      0x00008000
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_ENABLE_MASK   0x00010000
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_ENABLE        0x00010000
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_SCLK_NEG_ENABLE_MASK 0x00020000
#define CG_TEST_MACRO_RW_CNTL__TEST_MACRO_RW_SCLK_NEG_ENABLE 0x00020000

// MPLL_CNTL
#define MPLL_CNTL__MPLL_RESET_MASK                         0x00000001
#define MPLL_CNTL__MPLL_RESET                              0x00000001
#define MPLL_CNTL__MPLL_SLEEP_MASK                         0x00000002
#define MPLL_CNTL__MPLL_SLEEP                              0x00000002
#define MPLL_CNTL__MPLL_TST_EN_MASK                        0x00000004
#define MPLL_CNTL__MPLL_TST_EN                             0x00000004
#define MPLL_CNTL__MPLL_REFCLK_SEL_MASK                    0x00000010
#define MPLL_CNTL__MPLL_REFCLK_SEL                         0x00000010
#define MPLL_CNTL__MPLL_FBCLK_SEL_MASK                     0x00000020
#define MPLL_CNTL__MPLL_FBCLK_SEL                          0x00000020
#define MPLL_CNTL__MPLL_TCPOFF_MASK                        0x00000040
#define MPLL_CNTL__MPLL_TCPOFF                             0x00000040
#define MPLL_CNTL__MPLL_TVCOMAX_MASK                       0x00000080
#define MPLL_CNTL__MPLL_TVCOMAX                            0x00000080
#define MPLL_CNTL__MPLL_PCP_MASK                           0x00000700
#define MPLL_CNTL__MPLL_PVG_MASK                           0x00003800
#define MPLL_CNTL__MPLL_PDC_MASK                           0x0000c000
#define MPLL_CNTL__MPLL_X1_CLK_SKEW_MASK                   0x00070000
#define MPLL_CNTL__MPLL_X2_CLK_SKEW_MASK                   0x00700000
#define MPLL_CNTL__MPLL_MODE_MASK                          0x0f000000

// MDLL_CKO
#define MDLL_CKO__MCKOA_SLEEP_MASK                         0x00000001
#define MDLL_CKO__MCKOA_SLEEP                              0x00000001
#define MDLL_CKO__MCKOA_RESET_MASK                         0x00000002
#define MDLL_CKO__MCKOA_RESET                              0x00000002
#define MDLL_CKO__MCKOA_RANGE_MASK                         0x0000000c
#define MDLL_CKO__ERSTA_SOUTSEL_MASK                       0x00000030
#define MDLL_CKO__MCKOA_FB_SEL_MASK                        0x000000c0
#define MDLL_CKO__MCKOA_REF_SKEW_MASK                      0x00000700
#define MDLL_CKO__MCKOA_FB_SKEW_MASK                       0x00007000
#define MDLL_CKO__MCKOA_BP_SEL_MASK                        0x00008000
#define MDLL_CKO__MCKOA_BP_SEL                             0x00008000
#define MDLL_CKO__MCKOB_SLEEP_MASK                         0x00010000
#define MDLL_CKO__MCKOB_SLEEP                              0x00010000
#define MDLL_CKO__MCKOB_RESET_MASK                         0x00020000
#define MDLL_CKO__MCKOB_RESET                              0x00020000
#define MDLL_CKO__MCKOB_RANGE_MASK                         0x000c0000
#define MDLL_CKO__ERSTB_SOUTSEL_MASK                       0x00300000
#define MDLL_CKO__MCKOB_FB_SEL_MASK                        0x00c00000
#define MDLL_CKO__MCKOB_REF_SKEW_MASK                      0x07000000
#define MDLL_CKO__MCKOB_FB_SKEW_MASK                       0x70000000
#define MDLL_CKO__MCKOB_BP_SEL_MASK                        0x80000000
#define MDLL_CKO__MCKOB_BP_SEL                             0x80000000

// MDLL_RDCKA
#define MDLL_RDCKA__MRDCKA0_SLEEP_MASK                     0x00000001
#define MDLL_RDCKA__MRDCKA0_SLEEP                          0x00000001
#define MDLL_RDCKA__MRDCKA0_RESET_MASK                     0x00000002
#define MDLL_RDCKA__MRDCKA0_RESET                          0x00000002
#define MDLL_RDCKA__MRDCKA0_RANGE_MASK                     0x0000000c
#define MDLL_RDCKA__MRDCKA0_REF_SEL_MASK                   0x00000030
#define MDLL_RDCKA__MRDCKA0_FB_SEL_MASK                    0x000000c0
#define MDLL_RDCKA__MRDCKA0_REF_SKEW_MASK                  0x00000700
#define MDLL_RDCKA__MRDCKA0_SINSEL_MASK                    0x00000800
#define MDLL_RDCKA__MRDCKA0_SINSEL                         0x00000800
#define MDLL_RDCKA__MRDCKA0_FB_SKEW_MASK                   0x00007000
#define MDLL_RDCKA__MRDCKA0_BP_SEL_MASK                    0x00008000
#define MDLL_RDCKA__MRDCKA0_BP_SEL                         0x00008000
#define MDLL_RDCKA__MRDCKA1_SLEEP_MASK                     0x00010000
#define MDLL_RDCKA__MRDCKA1_SLEEP                          0x00010000
#define MDLL_RDCKA__MRDCKA1_RESET_MASK                     0x00020000
#define MDLL_RDCKA__MRDCKA1_RESET                          0x00020000
#define MDLL_RDCKA__MRDCKA1_RANGE_MASK                     0x000c0000
#define MDLL_RDCKA__MRDCKA1_REF_SEL_MASK                   0x00300000
#define MDLL_RDCKA__MRDCKA1_FB_SEL_MASK                    0x00c00000
#define MDLL_RDCKA__MRDCKA1_REF_SKEW_MASK                  0x07000000
#define MDLL_RDCKA__MRDCKA1_SINSEL_MASK                    0x08000000
#define MDLL_RDCKA__MRDCKA1_SINSEL                         0x08000000
#define MDLL_RDCKA__MRDCKA1_FB_SKEW_MASK                   0x70000000
#define MDLL_RDCKA__MRDCKA1_BP_SEL_MASK                    0x80000000
#define MDLL_RDCKA__MRDCKA1_BP_SEL                         0x80000000

// MDLL_RDCKB
#define MDLL_RDCKB__MRDCKB0_SLEEP_MASK                     0x00000001
#define MDLL_RDCKB__MRDCKB0_SLEEP                          0x00000001
#define MDLL_RDCKB__MRDCKB0_RESET_MASK                     0x00000002
#define MDLL_RDCKB__MRDCKB0_RESET                          0x00000002
#define MDLL_RDCKB__MRDCKB0_RANGE_MASK                     0x0000000c
#define MDLL_RDCKB__MRDCKB0_REF_SEL_MASK                   0x00000030
#define MDLL_RDCKB__MRDCKB0_FB_SEL_MASK                    0x000000c0
#define MDLL_RDCKB__MRDCKB0_REF_SKEW_MASK                  0x00000700
#define MDLL_RDCKB__MRDCKB0_SINSEL_MASK                    0x00000800
#define MDLL_RDCKB__MRDCKB0_SINSEL                         0x00000800
#define MDLL_RDCKB__MRDCKB0_FB_SKEW_MASK                   0x00007000
#define MDLL_RDCKB__MRDCKB0_BP_SEL_MASK                    0x00008000
#define MDLL_RDCKB__MRDCKB0_BP_SEL                         0x00008000
#define MDLL_RDCKB__MRDCKB1_SLEEP_MASK                     0x00010000
#define MDLL_RDCKB__MRDCKB1_SLEEP                          0x00010000
#define MDLL_RDCKB__MRDCKB1_RESET_MASK                     0x00020000
#define MDLL_RDCKB__MRDCKB1_RESET                          0x00020000
#define MDLL_RDCKB__MRDCKB1_RANGE_MASK                     0x000c0000
#define MDLL_RDCKB__MRDCKB1_REF_SEL_MASK                   0x00300000
#define MDLL_RDCKB__MRDCKB1_FB_SEL_MASK                    0x00c00000
#define MDLL_RDCKB__MRDCKB1_REF_SKEW_MASK                  0x07000000
#define MDLL_RDCKB__MRDCKB1_SINSEL_MASK                    0x08000000
#define MDLL_RDCKB__MRDCKB1_SINSEL                         0x08000000
#define MDLL_RDCKB__MRDCKB1_FB_SKEW_MASK                   0x70000000
#define MDLL_RDCKB__MRDCKB1_BP_SEL_MASK                    0x80000000
#define MDLL_RDCKB__MRDCKB1_BP_SEL                         0x80000000

// MCLK_CNTL
#define MCLK_CNTL__MCLKA_SRC_SEL_MASK                      0x00000007
#define MCLK_CNTL__YCLKA_SRC_SEL_MASK                      0x00000070
#define MCLK_CNTL__MCLKB_SRC_SEL_MASK                      0x00000700
#define MCLK_CNTL__YCLKB_SRC_SEL_MASK                      0x00007000
#define MCLK_CNTL__FORCE_MCLKA_MASK                        0x00010000
#define MCLK_CNTL__FORCE_MCLKA                             0x00010000
#define MCLK_CNTL__FORCE_MCLKB_MASK                        0x00020000
#define MCLK_CNTL__FORCE_MCLKB                             0x00020000
#define MCLK_CNTL__FORCE_YCLKA_MASK                        0x00040000
#define MCLK_CNTL__FORCE_YCLKA                             0x00040000
#define MCLK_CNTL__FORCE_YCLKB_MASK                        0x00080000
#define MCLK_CNTL__FORCE_YCLKB                             0x00080000
#define MCLK_CNTL__FORCE_MC_MASK                           0x00100000
#define MCLK_CNTL__FORCE_MC                                0x00100000
#define MCLK_CNTL__FORCE_AIC_MASK                          0x00200000
#define MCLK_CNTL__FORCE_AIC                               0x00200000
#define MCLK_CNTL__MRDCKA0_SOUTSEL_MASK                    0x03000000
#define MCLK_CNTL__MRDCKA1_SOUTSEL_MASK                    0x0c000000
#define MCLK_CNTL__MRDCKB0_SOUTSEL_MASK                    0x30000000
#define MCLK_CNTL__MRDCKB1_SOUTSEL_MASK                    0xc0000000

// IDCT_RUNS
#define IDCT_RUNS__IDCT_RUNS_3_MASK                        0x000000ff
#define IDCT_RUNS__IDCT_RUNS_2_MASK                        0x0000ff00
#define IDCT_RUNS__IDCT_RUNS_1_MASK                        0x00ff0000
#define IDCT_RUNS__IDCT_RUNS_0_MASK                        0xff000000

// IDCT_LEVELS
#define IDCT_LEVELS__IDCT_LEVEL_HI_MASK                    0x0000ffff
#define IDCT_LEVELS__IDCT_LEVEL_LO_MASK                    0xffff0000

// IDCT_CONTROL
#define IDCT_CONTROL__IDCT_CTL_LUMA_RD_FORMAT_MASK         0x00000003
#define IDCT_CONTROL__IDCT_CTL_CHROMA_RD_FORMAT_MASK       0x0000000c
#define IDCT_CONTROL__IDCT_CTL_SCAN_PATTERN_MASK           0x00000010
#define IDCT_CONTROL__IDCT_CTL_SCAN_PATTERN                0x00000010
#define IDCT_CONTROL__IDCT_CTL_INTRA_MASK                  0x00000020
#define IDCT_CONTROL__IDCT_CTL_INTRA                       0x00000020
#define IDCT_CONTROL__IDCT_CTL_FLUSH_MASK                  0x00000040
#define IDCT_CONTROL__IDCT_CTL_FLUSH                       0x00000040
#define IDCT_CONTROL__IDCT_CTL_PASSTHRU_MASK               0x00000080
#define IDCT_CONTROL__IDCT_CTL_PASSTHRU                    0x00000080
#define IDCT_CONTROL__IDCT_CTL_SW_RESET_MASK               0x00000100
#define IDCT_CONTROL__IDCT_CTL_SW_RESET                    0x00000100
#define IDCT_CONTROL__IDCT_CTL_CONSTREQ_MASK               0x00000200
#define IDCT_CONTROL__IDCT_CTL_CONSTREQ                    0x00000200
#define IDCT_CONTROL__IDCT_CTL_SCRAMBLE_MASK               0x00000400
#define IDCT_CONTROL__IDCT_CTL_SCRAMBLE                    0x00000400

// IDCT_AUTH_CONTROL
#define IDCT_AUTH_CONTROL__CONTROL_BITS_MASK               0xffffffff

// IDCT_AUTH
#define IDCT_AUTH__AUTH_MASK                               0xffffffff

#endif
