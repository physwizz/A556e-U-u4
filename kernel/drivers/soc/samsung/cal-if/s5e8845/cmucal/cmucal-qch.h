#ifndef __CMUCAL_QCH_H__
#define __CMUCAL_QCH_H__

#include "../../cmucal.h"

enum qch_id {
    APBIF_GPIO_ALIVE_QCH = QCH_TYPE,
    APBIF_INTCOMB_VGPIO2AP_QCH,
    APBIF_INTCOMB_VGPIO2APM_QCH,
    APBIF_INTCOMB_VGPIO2PMU_QCH,
    APBIF_PMU_ALIVE_QCH,
    APM_DTA_QCH_APB,
    ASM_ISRAMC_QCH,
    ASYNCAHB_MI_ASM_QCH,
    ASYNCINTERRUPT_ASM_QCH_ASYNCINTERRUPT_ASM,
    BAAW_ASM_QCH,
    CLKMON_QCH,
    CMU_ALIVE_QCH,
    DBGCORE_UART_QCH,
    D_TZPC_ALIVE_QCH,
    ECU_ALIVE_QCH,
    GREBEINTEGRATION0_QCH_DBG,
    GREBEINTEGRATION0_QCH_GREBE,
    GREBEINTEGRATION1_QCH_DBG,
    GREBEINTEGRATION1_QCH_GREBE,
    HW_SCANDUMP_CLKSTOP_CTRL_QCH,
    I2C_ALIVE0_QCH,
    INTMEM_QCH,
    LH_AXI_MI_ID_DBGCORE_ALIVE_QCH,
    LH_AXI_MI_IP_ASYNC_ALIVE_QCH,
    LH_AXI_SI_ID_ASYNC_ALIVE_QCH,
    LH_AXI_SI_IP_ALIVE_QCH,
    MAILBOX_APM0_APM1_QCH,
    MAILBOX_APM1_AP_QCH,
    MAILBOX_APM_AP_QCH,
    MAILBOX_APM_ASM_QCH,
    MAILBOX_APM_AUD_QCH,
    MAILBOX_APM_CHUB_QCH,
    MAILBOX_APM_CP_QCH,
    MAILBOX_APM_CP_1_QCH,
    MAILBOX_APM_GNSS_QCH,
    MAILBOX_APM_VTS_QCH,
    MAILBOX_APM_WLBT_QCH,
    MAILBOX_AP_ASM_QCH,
    MAILBOX_AP_CHUB_QCH,
    MAILBOX_AP_CP_QCH,
    MAILBOX_AP_CP_S_QCH,
    MAILBOX_AP_DBGCORE_QCH,
    MAILBOX_AP_GNSS_QCH,
    MAILBOX_AP_WLAN_QCH,
    MAILBOX_AP_WLBT_PMU_QCH,
    MAILBOX_AP_WPAN_QCH,
    MAILBOX_ASM_APM1_QCH,
    MAILBOX_ASM_CP_QCH,
    MAILBOX_CP_CHUB_QCH,
    MAILBOX_CP_GNSS_QCH,
    MAILBOX_CP_WLAN_QCH,
    MAILBOX_CP_WPAN_QCH,
    MAILBOX_GNSS_CHUB_QCH,
    MAILBOX_GNSS_WLBT_QCH,
    MAILBOX_SHARED_SRAM_QCH,
    MAILBOX_WLBT_AUD_QCH,
    MAILBOX_WLBT_CHUB_QCH,
    MCT_ALIVE_QCH,
    PMU_QCH_PMU,
    QCH_ADAPTER_AHB_BUSMATRIX_ALIVE_QCH,
    QCH_ADAPTER_XIU_D_ALIVE_QCH,
    QCH_ADAPTER_XIU_D_ALIVE_DTA_QCH,
    QCH_ADAPTER_XIU_P_ALIVE_QCH,
    QCH_ADAPTER_XIU_P_APM_QCH,
    RSTNSYNC_CLK_ALIVE_APM0_QCH,
    RSTNSYNC_CLK_ALIVE_APM1_QCH,
    RSTNSYNC_CLK_ALIVE_ASM_QCH,
    S2PC_ALIVE_QCH,
    SLH_AXI_MI_LD_CHUBVTS_ALIVE_QCH,
    SLH_AXI_MI_LD_GNSS_ALIVE_QCH,
    SLH_AXI_MI_LP_MODEM_ALIVE_QCH,
    SLH_AXI_MI_LP_WLBT_ALIVE_QCH,
    SLH_AXI_SI_LP_ALIVE_CHUBVTS_QCH,
    SLH_AXI_SI_LP_ALIVE_CMGP_QCH,
    SPC_ALIVE_QCH,
    SYSREG_ALIVE_QCH,
    TIMER_ASM_QCH,
    USI_ALIVE0_QCH,
    VGEN_LITE_ALIVE_QCH,
    WDT_APM0_QCH,
    WDT_APM1_QCH,
    WDT_ASM_QCH,
    YAMIN_MCU_ASM_QCH_CLKIN,
    YAMIN_MCU_ASM_QCH_DBGCLK,
    LH_AXI_MI_ID_ASYNC_ALIVE_QCH,
    LH_AXI_SI_IP_ASYNC_ALIVE_QCH,
    SLH_AXI_MI_P_ALIVE_QCH,
    SLH_AXI_SI_D_ALIVE_QCH,
    ASYNCAHB_MI_APM0_QCH,
    ASYNCAHB_MI_APM1_QCH,
    RTC_0_QCH,
    RTC_1_QCH,
    RTC_S_QCH,
    SPMI_MASTER_PMIC_QCH_P,
    SPMI_MASTER_PMIC_QCH_S,
    BDU_QCH,
    LH_AST_MI_G_NOCL1A_NOCL0_QCH,
    LH_AST_MI_G_RGBP_QCH,
    LH_AXI_MI_D0_CPUCL0_QCH,
    LH_AXI_MI_D0_DNC_QCH,
    LH_AXI_MI_D0_M2M_QCH,
    LH_AXI_MI_D0_MFC_QCH,
    LH_AXI_MI_D1_CPUCL0_QCH,
    LH_AXI_MI_D1_DNC_QCH,
    LH_AXI_MI_D1_M2M_QCH,
    LH_AXI_MI_D1_MFC_QCH,
    LH_AXI_MI_D_AUD_QCH,
    LH_AXI_MI_D_G3D_QCH,
    LH_AXI_MI_D_HSI_QCH,
    LH_TAXI_MI_D0_NOCL1A_NOCL0_QCH,
    LH_TAXI_MI_D0_RGBP_NOCL0_QCH,
    LH_TAXI_MI_D1_NOCL1A_NOCL0_QCH,
    LH_TAXI_MI_D1_RGBP_NOCL0_QCH,
    S2MPU_S0_G3D_QCH_S0,
    S2MPU_S0_PMMU0_G3D_QCH_S0,
    SCI_LITE_D_MIF_QCH,
    SLH_AXI_MI_D0_MODEM_QCH,
    SLH_AXI_MI_D1_MODEM_QCH,
    SLH_AXI_MI_D2_MODEM_QCH,
    SLH_AXI_MI_D_PERIC_QCH,
    SLH_AXI_MI_D_WLBT_QCH,
    SLH_AXI_MI_G_CSSYS_CPUCL0_QCH,
    SYSMMU_S0_MODEM_QCH_S0,
    SYSMMU_S0_PMMU0_MODEM_QCH_S0,
    VGEN_D_NOCL0_QCH,
    VGEN_LITE_D_NOCL0_QCH,
    BAAW_P_DNC_QCH,
    BAAW_P_GNSS_QCH,
    BAAW_P_MODEM_QCH,
    BAAW_P_WLBT_QCH,
    CMU_NOCL0_QCH,
    D_TZPC_NOCL0_QCH,
    ECU_NOCL0_QCH,
    PPMU_D0_AXI_CPUCL0_QCH,
    PPMU_D1_AXI_CPUCL0_QCH,
    PPMU_D_AXI_G3D_QCH,
    QE_D0_CPUCL0_QCH,
    QE_D1_CPUCL0_QCH,
    SFR_APBIF_CMU_TOPC_QCH,
    SLH_AXI_SI_P_ALIVE_QCH,
    SLH_AXI_SI_P_AUD_QCH,
    SLH_AXI_SI_P_CPUCL0_QCH,
    SLH_AXI_SI_P_CSIS_QCH,
    SLH_AXI_SI_P_CSTAT_QCH,
    SLH_AXI_SI_P_DNC_QCH,
    SLH_AXI_SI_P_DPU_QCH,
    SLH_AXI_SI_P_G3D_QCH,
    SLH_AXI_SI_P_GNSS_QCH,
    SLH_AXI_SI_P_HSI_QCH,
    SLH_AXI_SI_P_ICPU_QCH,
    SLH_AXI_SI_P_M2M_QCH,
    SLH_AXI_SI_P_MFC_QCH,
    SLH_AXI_SI_P_MIF0_QCH,
    SLH_AXI_SI_P_MIF1_QCH,
    SLH_AXI_SI_P_MODEM_QCH,
    SLH_AXI_SI_P_NOCL0_NOCL1A_QCH,
    SLH_AXI_SI_P_PERIC_QCH,
    SLH_AXI_SI_P_PERIS_QCH,
    SLH_AXI_SI_P_PSP_QCH,
    SLH_AXI_SI_P_RGBP_QCH,
    SLH_AXI_SI_P_USB_QCH,
    SLH_AXI_SI_P_USI_QCH,
    SLH_AXI_SI_P_WLBT_QCH,
    SLH_AXI_SI_P_YUVP_QCH,
    SYSREG_NOCL0_QCH,
    TREX_D_NOCL0_QCH,
    TREX_PPMU_D0_MODEM_QCH,
    TREX_PPMU_D1_MODEM_QCH,
    TREX_PPMU_D2_MODEM_QCH,
    TREX_PPMU_DP_NOCL0_QCH,
    TREX_PPMU_D_MEM0_QCH,
    TREX_PPMU_D_MEM1_QCH,
    TREX_PPMU_D_WLBT_QCH,
    TREX_PPMU_P_CLUSTER0_QCH,
    TREX_P_NOCL0_QCH,
    WOW_D0_MPACE_QCH,
    WOW_D1_MPACE_QCH,
    LH_AST_SI_G_NOCL1A_NOCL0_QCH,
    LH_AXI_MI_D0_DPU_QCH,
    LH_AXI_MI_D1_DPU_QCH,
    LH_AXI_MI_D_PSP_QCH,
    LH_AXI_MI_D_USB_QCH,
    LH_TAXI_SI_D0_NOCL1A_NOCL0_QCH,
    LH_TAXI_SI_D1_NOCL1A_NOCL0_QCH,
    S2MPU_S0_ALIVE_QCH_S0,
    S2MPU_S0_PMMU0_ALIVE_QCH_S0,
    SLH_AXI_MI_D_ALIVE_QCH,
    CMU_NOCL1A_QCH,
    D_TZPC_NOCL1A_QCH,
    ECU_NOCL1A_QCH,
    SLH_AXI_MI_P_NOCL0_NOCL1A_QCH,
    SYSREG_NOCL1A_QCH,
    TREX_D_NOCL1A_QCH,
    TREX_PPMU_D0_NOCL1A_QCH,
    TREX_PPMU_D1_NOCL1A_QCH,
    TREX_PPMU_D_ALIVE_QCH,
    CMU_MIF_QCH,
    D_TZPC_MIF_QCH,
    ECU_MIF_QCH,
    QCH_ADAPTER_DDRPHY_QCH,
    QCH_ADAPTER_DMC_QCH,
    QCH_ADAPTER_PPC_DEBUG_QCH,
    SLH_AXI_MI_P_MIF_QCH,
    SPC_MIF_QCH,
    SYSREG_MIF_QCH,
    SYSREG_PRIVATE_MIF_QCH,
    CSIS_PDP_QCH_DMA,
    CSIS_PDP_QCH_PDP_TOP,
    CSIS_PDP_QCH_TOP,
    CSIS_PDP_QCH_VOTF,
    CSIS_PDP_QCH_VOTF0,
    LH_AST_SI_OTF0_CSIS_CSTAT_QCH,
    LH_AST_SI_OTF1_CSIS_CSTAT_QCH,
    LH_AST_SI_OTF2_CSIS_CSTAT_QCH,
    LH_AXI_SI_LD0_CSIS_RGBP_QCH,
    LH_AXI_SI_LD1_CSIS_RGBP_QCH,
    SYSMMU_S0_CSIS_QCH_S0,
    SYSMMU_S0_PMMU0_CSIS_QCH_S0,
    SYSMMU_S0_PMMU1_CSIS_QCH_S0,
    VGEN_LITE0_CSIS_QCH,
    CMU_CSIS_QCH,
    D_TZPC_CSIS_QCH,
    ECU_CSIS_QCH,
    LH_AXI_SI_IP_POIS_CSIS_QCH,
    MIPI_PHY_LINK_WRAP_QCH_CSIS0,
    MIPI_PHY_LINK_WRAP_QCH_CSIS1,
    MIPI_PHY_LINK_WRAP_QCH_CSIS2,
    MIPI_PHY_LINK_WRAP_QCH_CSIS3,
    MIPI_PHY_LINK_WRAP_QCH_CSIS4,
    PPMU_D0_CSIS_QCH,
    PPMU_D1_CSIS_QCH,
    QE_CSIS0_QCH,
    QE_CSIS1_QCH,
    QE_CSIS2_QCH,
    QE_CSIS3_QCH,
    QE_PDP_M0_QCH,
    SLH_AXI_MI_P_CSIS_QCH,
    SYSREG_CSIS_QCH,
    LH_AXI_MI_IP_POIS_CSIS_QCH,
    OIS_MCU_TOP_QCH,
    RSTNSYNC_CLK_CSIS_OIS_MCU_CPU_SW_RESET_QCH,
    SLH_AXI_SI_LP_CSIS_PERIC_QCH,
    I3C_CMGP0_QCH_P,
    I3C_CMGP0_QCH_S,
    I3C_CMGP1_QCH_P,
    I3C_CMGP1_QCH_S,
    APBIF_GPIO_CMGP_QCH,
    CMU_CMGP_QCH,
    D_TZPC_CMGP_QCH,
    I2C_CMGP0_QCH,
    I2C_CMGP1_QCH,
    I2C_CMGP2_QCH,
    I2C_CMGP3_QCH,
    I2C_CMGP4_QCH,
    SLH_AXI_MI_LP_ALIVE_CMGP_QCH,
    SYSREG_CMGP_QCH,
    SYSREG_CMGP2APM_QCH,
    SYSREG_CMGP2CHUB_QCH,
    SYSREG_CMGP2CP_QCH,
    SYSREG_CMGP2GNSS_QCH,
    SYSREG_CMGP2PMU_AP_QCH,
    SYSREG_CMGP2WLBT_QCH,
    USI_CMGP0_QCH,
    USI_CMGP1_QCH,
    USI_CMGP2_QCH,
    USI_CMGP3_QCH,
    USI_CMGP4_QCH,
    I3C_CHUB_QCH_P,
    I3C_CHUB_QCH_S,
    APBIF_CHUB_COMBINE_WAKEUP_SRC_QCH,
    APBIF_GPIO_CHUB_QCH,
    APBIF_GPIO_CHUBEINT_QCH,
    CHUB_ALV_QCH,
    CMU_CHUB_QCH,
    I2C_CHUB1_QCH,
    I2C_CHUB2_QCH,
    LH_AXI_MI_IP_MCHUB_CHUBVTS_QCH,
    LH_AXI_SI_IP_SCHUB_CHUBVTS_QCH,
    PWM_CHUB_QCH,
    SYSREG_CHUB_QCH,
    SYSREG_COMBINE_CHUB2AP_QCH,
    SYSREG_COMBINE_CHUB2APM_QCH,
    SYSREG_COMBINE_CHUB2WLBT_QCH,
    TIMER_CHUB_QCH,
    USI_CHUB0_QCH,
    USI_CHUB1_QCH,
    USI_CHUB2_QCH,
    USI_CHUB3_QCH,
    WDT_CHUB_QCH,
    BAAW_CHUB_QCH,
    BAAW_VTS_QCH,
    CMU_CHUBVTS_QCH,
    D_TZPC_CHUBVTS_QCH,
    ECU_CHUBVTS_QCH,
    LH_AXI_MI_IP_SCHUB_CHUBVTS_QCH,
    LH_AXI_MI_IP_SVTS_CHUBVTS_QCH,
    LH_AXI_SI_IP_MCHUB_CHUBVTS_QCH,
    LH_AXI_SI_IP_MVTS_CHUBVTS_QCH,
    MAILBOX_VTS_CHUB_QCH,
    S2PC_CHUBVTS_QCH,
    SLH_AXI_MI_LP_ALIVE_CHUBVTS_QCH,
    SLH_AXI_SI_LD_CHUBVTS_ALIVE_QCH,
    SWEEPER_C_CHUBVTS_QCH,
    SYSREG_CHUBVTS_QCH,
    VGEN_LITE_CHUBVTS_QCH,
    CHUB_RTC_QCH,
    CMU_VTS_QCH,
    DMIC_AHB0_QCH_PCLK,
    DMIC_AHB2_QCH_PCLK,
    DMIC_IF0_QCH_PCLK,
    DMIC_IF1_QCH_PCLK,
    GPIO_VTS_QCH,
    HWACG_SYS_DMIC0_QCH,
    HWACG_SYS_DMIC2_QCH,
    LH_AXI_MI_IP_MVTS_CHUBVTS_QCH,
    LH_AXI_SI_IP_SVTS_CHUBVTS_QCH,
    MAILBOX_ABOX_VTS_QCH,
    MAILBOX_AP_VTS_QCH,
    SERIAL_LIF_DEBUG_QCH_ACLK,
    SERIAL_LIF_DEBUG_QCH_PCLK,
    SYSREG_VTS_QCH,
    TIMER_VTS_QCH,
    WDT_VTS_QCH,
    SERIAL_LIF_DEBUG_QCH_RX_BCLK,
    SERIAL_LIF_DEBUG_QCH_TX_BCLK,
    SERIAL_LIF_DEBUG_QCH_CCLK,
    APBIF_CSSYS_ALIVE_QCH,
    MDIS_DBGCORE_QCH_OSC,
    APBIF_S2D_DBGCORE_QCH,
    CMU_DBGCORE_QCH,
    D_TZPC_DBGCORE_QCH,
    GREBEINTEGRATION_DBGCORE_QCH_S_DBG,
    GREBEINTEGRATION_DBGCORE_QCH_S_GREBE,
    LH_AXI_MI_IP_ALIVE_QCH,
    LH_AXI_SI_ID_DBGCORE_ALIVE_QCH,
    LH_AXI_SI_IG_ASYNCDBGCORECPUCL0_ALIVE_QCH,
    LH_AXI_SI_IG_ASYNCDBGCOREMIF0_ALIVE_QCH,
    MDIS_DBGCORE_QCH,
    RSTNSYNC_CLK_DBGCORE_GREBE_QCH,
    SYSREG_DBGCORE_QCH,
    SYSREG_DBGCORE_CORE_QCH,
    WDT_DBGCORE_QCH,
    LH_AXI_MI_IG_ASYNCDBGCORECPUCL0_ALIVE_QCH,
    LH_AXI_MI_IG_ASYNCDBGCOREMIF0_ALIVE_QCH,
    SLH_AXI_SI_G_DBGCORE_ALIVE_CPUCL0_QCH,
    SLH_AXI_SI_G_SCAN2DRAMMIF_ALIVE_MIF1_QCH,
    DPU_QCH_DPU_DSIM0,
    DPU_QCH_DPU,
    DPU_QCH_DPU_C2SERV,
    DPU_QCH_DPU_DMA,
    DPU_QCH_DPU_DPP,
    LH_AXI_SI_D0_DPU_QCH,
    LH_AXI_SI_D1_DPU_QCH,
    SYSMMU_S0_DPU_QCH_S0,
    SYSMMU_S0_PMMU0_DPU_QCH_S0,
    SYSMMU_S0_PMMU1_DPU_QCH_S0,
    CMU_DPU_QCH,
    D_TZPC_DPU_QCH,
    ECU_DPU_QCH,
    PPMU_D0_DPU_QCH,
    PPMU_D1_DPU_QCH,
    SLH_AXI_MI_P_DPU_QCH,
    SYSREG_DPU_QCH,
    DPU_QCH_DPU_OSDDSIM0,
    HTU_GNPU0_QCH_CLK,
    LH_AST_MI_LD_SRAMCSTFIFO_SDMA_GNPU0_QCH,
    LH_AST_MI_LD_SRAMRDRSP0_SDMA_GNPU0_QCH,
    LH_AST_MI_LD_SRAMRDRSP1_SDMA_GNPU0_QCH,
    LH_AST_MI_LD_SRAMRDRSP2_SDMA_GNPU0_QCH,
    LH_AST_MI_LD_SRAMRDRSP3_SDMA_GNPU0_QCH,
    LH_AST_MI_LD_SRAMRDRSP4_SDMA_GNPU0_QCH,
    LH_AST_MI_LD_SRAMRDRSP5_SDMA_GNPU0_QCH,
    LH_AST_SI_LD_SRAMRDREQ0_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMRDREQ1_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMRDREQ2_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMRDREQ3_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMRDREQ4_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMRDREQ5_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMWR0_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMWR1_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMWR2_GNPU0_SDMA_QCH,
    LH_AXI_MI_LD_CTRL_DNC_GNPU0_QCH,
    LH_AXI_SI_LD_CTRL_GNPU0_DNC_QCH,
    LH_AXI_SI_LD_DRAM_GNPU0_DNC_QCH,
    CMU_GNPU0_QCH,
    D_TZPC_GNPU0_QCH,
    HTU_GNPU0_QCH_PCLK,
    SLH_AXI_MI_LP_DNC_GNPU0_QCH,
    SYSREG_GNPU0_QCH,
    IP_NPUCORE_QCH_CORE,
    IP_SDMA_WRAP_QCH,
    IP_SDMA_WRAP_QCH_2,
    LH_AST_MI_LD_SRAMRDREQ0_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMRDREQ1_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMRDREQ2_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMRDREQ3_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMRDREQ4_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMRDREQ5_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMWR0_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMWR1_GNPU0_SDMA_QCH,
    LH_AST_MI_LD_SRAMWR2_GNPU0_SDMA_QCH,
    LH_AST_SI_LD_SRAMCSTFIFO_SDMA_GNPU0_QCH,
    LH_AST_SI_LD_SRAMRDRSP0_SDMA_GNPU0_QCH,
    LH_AST_SI_LD_SRAMRDRSP1_SDMA_GNPU0_QCH,
    LH_AST_SI_LD_SRAMRDRSP2_SDMA_GNPU0_QCH,
    LH_AST_SI_LD_SRAMRDRSP3_SDMA_GNPU0_QCH,
    LH_AST_SI_LD_SRAMRDRSP4_SDMA_GNPU0_QCH,
    LH_AST_SI_LD_SRAMRDRSP5_SDMA_GNPU0_QCH,
    LH_AXI_MI_LD_CTRL_DNC_SDMA_QCH,
    LH_AXI_MI_LD_SRAM_DNC_SDMA_QCH,
    LH_AXI_SI_LD_MMU0_SDMA_DNC_QCH,
    LH_AXI_SI_LD_MMU1_SDMA_DNC_QCH,
    CMU_SDMA_QCH,
    D_TZPC_SDMA_QCH,
    SLH_AXI_MI_LP_DNC_SDMA_QCH,
    SYSREG_SDMA_QCH,
    HTU_DNC_QCH_CLK,
    IP_DNC_QCH,
    LH_AXI_MI_ID_IPDNC_DNC_QCH,
    LH_AXI_MI_LD_CTRL_GNPU0_DNC_QCH,
    LH_AXI_MI_LD_DRAM_GNPU0_DNC_QCH,
    LH_AXI_MI_LD_MMU0_SDMA_DNC_QCH,
    LH_AXI_MI_LD_MMU1_SDMA_DNC_QCH,
    LH_AXI_SI_D0_DNC_QCH,
    LH_AXI_SI_D1_DNC_QCH,
    LH_AXI_SI_LD_CTRL_DNC_GNPU0_QCH,
    LH_AXI_SI_LD_CTRL_DNC_SDMA_QCH,
    LH_AXI_SI_LD_SRAM_DNC_SDMA_QCH,
    SYSMMU_S0_DNC_QCH_S0,
    SYSMMU_S0_PMMU0_DNC_QCH_S0,
    SYSMMU_S0_PMMU1_DNC_QCH_S0,
    SYSMMU_S1_DNC_QCH_S0,
    SYSMMU_S1_PMMU0_DNC_QCH_S0,
    VGEN_D_DNC_QCH,
    VGEN_LITE_D_DNC_QCH,
    CMU_DNC_QCH,
    D_TZPC_DNC_QCH,
    HTU_DNC_QCH_PCLK,
    LH_AXI_SI_ID_IPDNC_DNC_QCH,
    PPMU_D_DNC0_QCH,
    PPMU_D_DNC1_QCH,
    PPMU_D_IPDNC_QCH,
    SLH_AXI_MI_P_DNC_QCH,
    SLH_AXI_SI_LP_DNC_GNPU0_QCH,
    SLH_AXI_SI_LP_DNC_SDMA_QCH,
    SYSREG_DNC_QCH,
    LH_AXI_MI_ID_PSP_QCH,
    LH_AXI_SI_D_PSP_QCH,
    RSTNSYNC_CLK_PSP_NOCD_CPU_QCH,
    RSTNSYNC_SR_CLK_PSP_PUF_QCH,
    S2MPU_S0_PMMU0_PSP_QCH_S0,
    S2MPU_S0_PSP_QCH_S0,
    SS_PSP_QCH_BAAW,
    SS_PSP_QCH_CM35P,
    SS_PSP_QCH_DBG,
    SS_PSP_QCH_FPU,
    SS_PSP_QCH_LH_D,
    SS_PSP_QCH_LH_P,
    SS_PSP_QCH_MB_AP,
    SS_PSP_QCH_MB_CP,
    SS_PSP_QCH_SC,
    SS_PSP_QCH_SYSREG,
    VGEN_LITE_PSP_QCH,
    CMU_PSP_QCH,
    D_TZPC_PSP_QCH,
    LH_AXI_SI_IP_PSP_QCH,
    PPMU_PSP_QCH,
    RTIC_QCH,
    SLH_AXI_MI_P_PSP_QCH,
    SYSREG_PSP_BLK_QCH,
    LH_AST_MI_OTF_CSTAT_RGBP_QCH,
    LH_AST_MI_OTF_YUVP_RGBP_QCH,
    LH_AST_SI_OTF_RGBP_YUVP_QCH,
    LH_AXI_SI_ID0_RGBP_QCH,
    LH_AXI_SI_ID1_RGBP_QCH,
    LH_AXI_SI_ID2_RGBP_QCH,
    LH_AXI_SI_ID3_RGBP_QCH,
    MCFP_QCH,
    RGBP_QCH,
    RGBP_QCH_VOTF0,
    RGBP_QCH_VOTF1,
    SYSMMU_S0_PMMU0_RGBP_QCH_S0,
    SYSMMU_S0_PMMU1_RGBP_QCH_S0,
    SYSMMU_S0_PMMU2_RGBP_QCH_S0,
    SYSMMU_S0_PMMU3_RGBP_QCH_S0,
    SYSMMU_S0_RGBP_QCH_S0,
    VGEN_LITE_D0_RGBP_QCH,
    VGEN_LITE_D1_RGBP_QCH,
    VGEN_LITE_D2_RGBP_QCH,
    LH_AST_SI_G_RGBP_QCH,
    LH_AXI_MI_ID0_RGBP_QCH,
    LH_AXI_MI_ID1_RGBP_QCH,
    LH_AXI_MI_ID2_RGBP_QCH,
    LH_AXI_MI_ID3_RGBP_QCH,
    LH_AXI_MI_LD0_CSIS_RGBP_QCH,
    LH_AXI_MI_LD0_CSTAT_RGBP_QCH,
    LH_AXI_MI_LD1_CSIS_RGBP_QCH,
    LH_AXI_MI_LD1_CSTAT_RGBP_QCH,
    LH_AXI_MI_LD_ICPU_RGBP_QCH,
    LH_AXI_MI_LD_YUVP_RGBP_QCH,
    LH_TAXI_SI_D0_RGBP_NOCL0_QCH,
    LH_TAXI_SI_D1_RGBP_NOCL0_QCH,
    CMU_RGBP_QCH,
    D_TZPC_RGBP_QCH,
    ECU_RGBP_QCH,
    PPMU_D0_RGBP_QCH,
    PPMU_D1_RGBP_QCH,
    PPMU_D2_RGBP_QCH,
    PPMU_D3_RGBP_QCH,
    SLH_AXI_MI_P_RGBP_QCH,
    SYSREG_RGBP_QCH,
    TREX_D_CAM_QCH,
    TREX_PPMU_D0_CAM_QCH,
    TREX_PPMU_D1_CAM_QCH,
    CMU_G3D_QCH,
    D_TZPC_G3D_QCH,
    ECU_G3D_QCH,
    LH_AXI_SI_IP_G3D_QCH,
    RSTNSYNC_CLK_G3D_NOCP_QCH,
    RSTNSYNC_SR_CLK_G3D_NOCP_QCH,
    SLH_AXI_MI_P_G3D_QCH,
    SYSREG_G3D_QCH,
    RSTNSYNC_CLK_G3D_OSCCLK_QCH,
    RSTNSYNC_SR_CLK_G3D_OSCCLK_QCH,
    ASB_G3D_QCH_LH0,
    ASB_G3D_QCH_LH_P,
    RSTNSYNC_QCH_NOCD,
    RSTNSYNC_QCH_NOCP,
    CMU_G3DCORE_QCH,
    HTU_G3D_QCH_PCLK,
    RSTNSYNC_CLK_G3DCORE_NOCP_QCH,
    RSTNSYNC_SR_CLK_G3DCORE_NOCP_QCH,
    RSTNSYNC_CLK_G3DCORE_OSCCLK_QCH,
    RSTNSYNC_SR_CLK_G3DCORE_OSCCLK_QCH,
    RSTNSYNC_CLK_G3DCORE_PLL_G3D_QCH,
    RSTNSYNC_SR_CLK_G3DCORE_PLL_G3D_QCH,
    HTU_G3D_QCH_CLK,
    RSTNSYNC_CLK_CPUCL0_DBG_PCLKDBG_CSSYS_QCH,
    SECJTAG_SM_QCH,
    SLH_AXI_SI_G_CSSYS_CPUCL0_QCH,
    BPS_CPUCL0_QCH,
    BUSIF_DDC_CPUCL0_QCH,
    CFM_CPUCL0_QCH,
    CMU_CPUCL0_GLB_QCH,
    D_TZPC_CPUCL0_QCH,
    ECU_CPUCL0_QCH,
    SLH_AXI_MI_G_DBGCORE_ALIVE_CPUCL0_QCH,
    SLH_AXI_MI_P_CPUCL0_QCH,
    SYSREG_CPUCL0_QCH,
    CMU_CPUCL0_QCH,
    HTU_CPUCL0_QCH_PCLK,
    HTU_CPUCL0_QCH_CLK,
    DDC_CPUCL1_ATB_QCH,
    DDC_QCH_GCPM_CORE,
    CMU_CPUCL1_QCH,
    HTU_CPUCL1_QCH_PCLK,
    HTU_CPUCL1_QCH_CLK,
    CLUSTER0_QCH_ATCLK,
    CLUSTER0_QCH_GIC,
    CLUSTER0_QCH_PCLK,
    CLUSTER0_QCH_PDBGCLK,
    CLUSTER0_QCH_SCLK,
    CMU_DSU_QCH,
    HTU_DSU_QCH_PCLK,
    LH_AXI_SI_D0_CPUCL0_QCH,
    LH_AXI_SI_D1_CPUCL0_QCH,
    PPC_INSTRRET_CLUSTER0_0_QCH,
    PPC_INSTRRET_CLUSTER0_1_QCH,
    PPC_INSTRRUN_CLUSTER0_0_QCH,
    PPC_INSTRRUN_CLUSTER0_1_QCH,
    HTU_DSU_QCH_CLK,
    FG_QCH,
    LH_AXI_SI_D1_MFC_QCH,
    RSTNSYNC_SR_CLK_FG_SW_RESET_QCH,
    SYSMMU_S1_MFC_QCH_S0,
    SYSMMU_S1_PMMU0_MFC_QCH_S0,
    VGEN_LITE_D_FG_QCH,
    LH_AST_MI_OTF0_M2M_MFC_QCH,
    LH_AST_MI_OTF1_M2M_MFC_QCH,
    LH_AXI_SI_D0_MFC_QCH,
    MFC_QCH,
    MFC_QCH_VOTF,
    RSTNSYNC_CLK_MFC_NOCD_SW_RESET_QCH,
    RSTNSYNC_SR_CLK_MFC_NOCD_SW_RESET_QCH,
    SYSMMU_S0_MFC_QCH_S0,
    SYSMMU_S0_PMMU0_MFC_QCH_S0,
    VGEN_LITE_D_MFC_QCH,
    CMU_MFC_QCH,
    D_TZPC_MFC_QCH,
    ECU_MFC_QCH,
    PPMU_D0_MFC_QCH,
    PPMU_D1_MFC_QCH,
    SLH_AXI_MI_P_MFC_QCH,
    SYSREG_MFC_QCH,
    LH_AST_MI_ID_PMMUSWALKER_CSTAT_QCH,
    LH_AST_SI_ID_SWALKERPMMU_CSTAT_QCH,
    LH_AST_SI_OTF_CSTAT_RGBP_QCH,
    LH_AXI_SI_LD1_CSTAT_RGBP_QCH,
    SIPU_BYRP_QCH,
    SIPU_BYRP_QCH_VOTF1,
    SYSMMU_S0_CSTAT_QCH_S0,
    SYSMMU_S0_PMMU1_CSTAT_QCH_S0,
    VGEN_LITE_D_BYRP_QCH,
    LH_AST_MI_ID_SWALKERPMMU_CSTAT_QCH,
    LH_AST_MI_OTF0_CSIS_CSTAT_QCH,
    LH_AST_MI_OTF1_CSIS_CSTAT_QCH,
    LH_AST_MI_OTF2_CSIS_CSTAT_QCH,
    LH_AST_SI_ID_PMMUSWALKER_CSTAT_QCH,
    LH_AXI_SI_LD0_CSTAT_RGBP_QCH,
    SIPU_CSTAT_QCH,
    SIPU_CSTAT_QCH_C2DS,
    SIPU_CSTAT_QCH_C2RD,
    SYSMMU_S0_PMMU0_CSTAT_QCH_S0,
    VGEN_LITE_D_CSTAT0_QCH,
    VGEN_LITE_D_CSTAT1_QCH,
    CMU_CSTAT_QCH,
    D_TZPC_CSTAT_QCH,
    PPMU_D0_CSTAT_QCH,
    PPMU_D1_CSTAT_QCH,
    SLH_AXI_MI_P_CSTAT_QCH,
    SYSREG_CSTAT_QCH,
    LH_AST_MI_OTF_RGBP_YUVP_QCH,
    LH_AST_SI_OTF_YUVP_RGBP_QCH,
    LH_AXI_SI_LD_YUVP_RGBP_QCH,
    MCSC_QCH,
    MCSC_QCH_C2R,
    MCSC_QCH_C2W,
    SYSMMU_S0_PMMU0_YUVP_QCH_S0,
    SYSMMU_S0_YUVP_QCH_S0,
    VGEN_LITE_D0_YUVP_QCH,
    VGEN_LITE_D1_YUVP_QCH,
    YUVP_QCH,
    YUVP_QCH_VOTF0,
    CMU_YUVP_QCH,
    D_TZPC_YUVP_QCH,
    PPMU_D_YUVP_QCH,
    SLH_AXI_MI_P_YUVP_QCH,
    SYSREG_YUVP_QCH,
    CMU_HSI_QCH,
    D_TZPC_HSI_QCH,
    ECU_HSI_QCH,
    GPIO_HSI_UFS_QCH,
    LH_AXI_SI_D_HSI_QCH,
    PPMU_D_HSI_QCH,
    S2MPU_S0_HSI_QCH_S0,
    S2MPU_S0_PMMU0_HSI_QCH_S0,
    SLH_AXI_MI_P_HSI_QCH,
    SPC_HSI_QCH,
    SYSREG_HSI_QCH,
    UFS_EMBD_QCH,
    UFS_EMBD_QCH_FMP,
    VGEN_LITE_HSI_QCH,
    GDC_L_QCH,
    GDC_O_QCH,
    LH_AST_MI_ID_LM_M2M_QCH,
    LH_AST_SI_ID_ML_M2M_QCH,
    LH_AST_SI_OTF0_M2M_MFC_QCH,
    LH_AST_SI_OTF1_M2M_MFC_QCH,
    LH_AXI_SI_D1_M2M_QCH,
    LH_AXI_SI_ID_GDCL0_M2M_QCH,
    LH_AXI_SI_ID_GDCL1_M2M_QCH,
    LME_QCH,
    SYSMMU_S0_M2M_QCH_S0,
    SYSMMU_S0_PMMU1_M2M_QCH_S0,
    VGEN_LITE_D1_M2M_QCH,
    VGEN_LITE_D2_M2M_QCH,
    JPEG_QCH,
    LH_AXI_SI_ID_JPEG_M2M_QCH,
    LH_AST_MI_ID_ML_M2M_QCH,
    LH_AST_SI_ID_LM_M2M_QCH,
    LH_AXI_MI_ID_GDCL0_M2M_QCH,
    LH_AXI_MI_ID_GDCL1_M2M_QCH,
    LH_AXI_MI_ID_JPEG_M2M_QCH,
    LH_AXI_SI_D0_M2M_QCH,
    M2M_QCH_S1,
    M2M_QCH_S2,
    SYSMMU_S0_PMMU0_M2M_QCH_S0,
    VGEN_LITE_D0_M2M_QCH,
    CMU_M2M_QCH,
    D_TZPC_M2M_QCH,
    PPMU_D0_M2M_QCH,
    PPMU_D1_M2M_QCH,
    SLH_AXI_MI_P_M2M_QCH,
    SYSREG_M2M_QCH,
    MMC_CARD_QCH,
    CMU_PERIC_QCH,
    D_TZPC_PERIC_QCH,
    GPIO_PERIC_QCH_GPIO,
    GPIO_PERICMMC_QCH_GPIO,
    PDMA_PERIC_QCH,
    PPMU_D_PERIC_QCH,
    QE_D_PDMA_QCH,
    QE_D_SPDMA_QCH,
    S2MPU_S0_PERIC_QCH_S0,
    S2MPU_S0_PMMU0_PERIC_QCH_S0,
    SLH_AXI_MI_LP_CSIS_PERIC_QCH,
    SLH_AXI_MI_P_PERIC_QCH,
    SLH_AXI_SI_D_PERIC_QCH,
    SPC_PERIC_QCH,
    SPDMA_PERIC_QCH,
    SYSREG_PERIC_QCH,
    UART_DBG_QCH,
    USI00_I2C_QCH,
    USI00_USI_QCH,
    USI01_I2C_QCH,
    USI01_USI_QCH,
    USI02_I2C_QCH,
    USI02_USI_QCH,
    USI03_I2C_QCH,
    USI03_USI_QCH,
    USI04_I2C_QCH,
    USI04_USI_QCH,
    USI05_I2C_QCH,
    USI09_USI_OIS_QCH,
    USI10_USI_OIS_QCH,
    VGEN_D_PDMA_QCH,
    VGEN_D_SPDMA_QCH,
    VGEN_LITE_D_PERIC_QCH,
    I3C00_OIS_QCH_P,
    I3C00_OIS_QCH_S,
    GIC_QCH,
    LH_AXI_MI_IP_GIC_PERIS_QCH,
    CMU_PERIS_QCH,
    D_TZPC_PERIS_QCH,
    LH_AXI_SI_IP_GIC_PERIS_QCH,
    PWM_QCH,
    SLH_AXI_MI_P_PERIS_QCH,
    SPC_PERIS_QCH,
    SYSREG_PERIS_QCH,
    TMU_QCH,
    WDT0_QCH,
    WDT1_QCH,
    OTP_CON_TOP_QCH,
    ABOX_QCH_CNT,
    ABOX_QCH_CPU0,
    ABOX_QCH_CPU1,
    ABOX_QCH_CPU2,
    ABOX_QCH_L2,
    ABOX_QCH_NEON0,
    ABOX_QCH_NEON1,
    ABOX_QCH_NEON2,
    RSTNSYNC_CLK_AUD_CPU0_SW_RESET_QCH,
    RSTNSYNC_CLK_AUD_CPU1_SW_RESET_QCH,
    RSTNSYNC_CLK_AUD_CPU2_SW_RESET_QCH,
    ABOX_QCH_CCLK_ASB,
    ABOX_QCH_CCLK_ACP,
    RSTNSYNC_CLK_AUD_CPU_PCLKDBG_QCH,
    ABOX_QCH_BCLK_DSIF,
    ABOX_QCH_ACLK,
    ABOX_QCH_ACLK_ACP,
    ABOX_QCH_ACLK_ASB,
    ABOX_QCH_C2A0,
    ABOX_QCH_C2A1,
    ABOX_QCH_XCLK,
    CMU_AUD_QCH,
    DMIC_AUD0_QCH_PCLK,
    DMIC_AUD1_QCH_PCLK,
    LH_AXI_MI_IP_PERIASB_AUD_QCH,
    LH_AXI_SI_D_AUD_QCH,
    MAILBOX_AUD0_QCH,
    MAILBOX_AUD1_QCH,
    PPMU_D_AUD_QCH,
    SERIAL_LIF_AUD_QCH_ACLK,
    SERIAL_LIF_AUD_QCH_PCLK,
    SYSMMU_S0_AUD_QCH_S0,
    SYSMMU_S0_PMMU0_AUD_QCH_S0,
    SYSREG_AUD_QCH,
    VGEN_LITE_D_AUD_QCH,
    WDT_AUD_QCH,
    D_TZPC_AUD_QCH,
    ECU_AUD_QCH,
    LH_AXI_SI_IP_PERIASB_AUD_QCH,
    SLH_AXI_MI_P_AUD_QCH,
    ABOX_QCH_PCMC_CLK,
    SERIAL_LIF_AUD_QCH_BCLK,
    SERIAL_LIF_AUD_QCH_CCLK,
    ABOX_QCH_BCLK0,
    ABOX_QCH_BCLK1,
    ABOX_QCH_BCLK2,
    ABOX_QCH_BCLK3,
    ABOX_QCH_BCLK4,
    ABOX_QCH_BCLK5,
    ABOX_QCH_BCLK6,
    CMU_USB_QCH,
    D_TZPC_USB_QCH,
    LH_AXI_SI_D_USB_QCH,
    PPMU_USB_QCH,
    S2MPU_S0_PMMU0_USB_QCH_S0,
    S2MPU_S0_USB_QCH_S0,
    SLH_AXI_MI_P_USB_QCH,
    SPC_USB_QCH,
    SYSREG_USB_QCH,
    USB20DRD_TOP_QCH_S_EUSBCTL,
    USB20DRD_TOP_QCH_S_SUBCTRL,
    VGEN_LITE_USB_QCH,
    USB20DRD_TOP_QCH_S_EUSBPHY,
    USB20DRD_TOP_QCH_S_LINK,
    ICPU_QCH_CPU0,
    ICPU_QCH_CPU_SI,
    ICPU_QCH_L2,
    ICPU_QCH_NEON,
    RSTNSYNC_CLK_ICPU_CORE_QCH,
    RSTNSYNC_CLK_ICPU_CPUPO_QCH,
    RSTNSYNC_CLK_ICPU_CPU_DBG_QCH,
    ICPU_QCH_PERI,
    ICPU_QCH_PERI_MI,
    LH_AXI_MI_IP_ICPU_QCH,
    LH_AXI_SI_LD_ICPU_RGBP_QCH,
    SYSMMU_S0_ICPU_QCH_S0,
    SYSMMU_S0_PMMU0_ICPU_QCH_S0,
    VGEN_LITE_ICPU_QCH,
    CMU_ICPU_QCH,
    D_TZPC_ICPU_QCH,
    LH_AXI_SI_IP_ICPU_QCH,
    PPMU_D_ICPU_QCH,
    SLH_AXI_MI_P_ICPU_QCH,
    SYSREG_ICPU_QCH,
    CMU_USI_QCH,
    D_TZPC_USI_QCH,
    GPIO_USI_QCH_GPIO,
    SLH_AXI_MI_P_USI_QCH,
    SYSREG_USI_QCH,
    USI06_I2C_QCH,
    USI06_USI_QCH,
    USI07_I2C_QCH,
    USI07_USI_QCH,
    USI08_I2C_QCH,
    CMU_S2D_QCH,
    SLH_AXI_MI_G_SCAN2DRAM_ALIVE_MIF_QCH,
    DFTMUX_CMU_QCH_CIS_CLK0,
    DFTMUX_CMU_QCH_CIS_CLK1,
    DFTMUX_CMU_QCH_CIS_CLK2,
    DFTMUX_CMU_QCH_CIS_CLK3,
    DFTMUX_CMU_QCH_CIS_CLK4,

    end_of_qch,
    num_of_qch = (end_of_qch - QCH_TYPE) & MASK_OF_ID,
};

enum option_id {
    CTRL_OPTION_S5E8845 = OPTION_TYPE,
    CTRL_OPTION_TOP,
    CTRL_OPTION_ALIVE,
    CTRL_OPTION_PMU,
    CTRL_OPTION_NOCL0,
    CTRL_OPTION_NOCL1A,
    CTRL_OPTION_MIF,
    CTRL_OPTION_CSIS,
    CTRL_OPTION_CMGP,
    CTRL_OPTION_CHUB,
    CTRL_OPTION_CHUBVTS,
    CTRL_OPTION_VTS,
    CTRL_OPTION_DBGCORE,
    CTRL_OPTION_DPU,
    CTRL_OPTION_GNPU0,
    CTRL_OPTION_SDMA,
    CTRL_OPTION_DNC,
    CTRL_OPTION_PSP,
    CTRL_OPTION_RGBP,
    CTRL_OPTION_G3D,
    CTRL_OPTION_G3DCORE,
    CTRL_OPTION_CPUCL0_GLB,
    CTRL_OPTION_CPUCL0,
    CTRL_OPTION_CPUCL1,
    CTRL_OPTION_DSU,
    CTRL_OPTION_MFC,
    CTRL_OPTION_CSTAT,
    CTRL_OPTION_YUVP,
    CTRL_OPTION_HSI,
    CTRL_OPTION_M2M,
    CTRL_OPTION_PERIC,
    CTRL_OPTION_PERIS,
    CTRL_OPTION_AUD,
    CTRL_OPTION_USB,
    CTRL_OPTION_ICPU,
    CTRL_OPTION_USI,
    CTRL_OPTION_S2D,
    end_of_option,
    num_of_option = (end_of_option - OPTION_TYPE) & MASK_OF_ID,
};
#endif