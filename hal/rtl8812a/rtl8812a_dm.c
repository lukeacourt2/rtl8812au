/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
/* ************************************************************
 * Description:
 *
 * This file is for 92CE/92CU dynamic mechanism only
 *
 *
 * ************************************************************ */
#define _RTL8812A_DM_C_

/* ************************************************************
 * include files
 * ************************************************************
 * #include <drv_types.h> */
#include <rtl8812a_hal.h>

/* ************************************************************
 * Global var
 * ************************************************************ */


static void
dm_CheckProtection(
	IN	PADAPTER	Adapter
)
{
}

static void
dm_CheckStatistics(
	IN	PADAPTER	Adapter
)
{
}
#ifdef CONFIG_SUPPORT_HW_WPS_PBC
static void dm_CheckPbcGPIO(_adapter *padapter)
{
	u8	tmp1byte;
	u8	bPbcPressed = _FALSE;

	if (!padapter->registrypriv.hw_wps_pbc)
		return;

#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI)
	if (IS_HARDWARE_TYPE_8812(padapter)) {
		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
		tmp1byte |= (HAL_8812A_HW_GPIO_WPS_BIT);
		rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	/* enable GPIO[2] as output mode */

		tmp1byte &= ~(HAL_8812A_HW_GPIO_WPS_BIT);
		rtw_write8(padapter,  GPIO_IN, tmp1byte);		/* reset the floating voltage level */

		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
		tmp1byte &= ~(HAL_8812A_HW_GPIO_WPS_BIT);
		rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	/* enable GPIO[2] as input mode */

		tmp1byte = rtw_read8(padapter, GPIO_IN);

		if (tmp1byte == 0xff)
			return ;

		if (tmp1byte & HAL_8812A_HW_GPIO_WPS_BIT)
			bPbcPressed = _TRUE;

	} else if (IS_HARDWARE_TYPE_8821(padapter)) {
		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL_8811A);
		tmp1byte |= (BIT4);
		rtw_write8(padapter, GPIO_IO_SEL_8811A, tmp1byte);	/* enable GPIO[2] as output mode */

		tmp1byte &= ~(BIT4);
		rtw_write8(padapter,  GPIO_IN_8811A, tmp1byte);		/* reset the floating voltage level */

		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL_8811A);
		tmp1byte &= ~(BIT4);
		rtw_write8(padapter, GPIO_IO_SEL_8811A, tmp1byte);	/* enable GPIO[2] as input mode */

		tmp1byte = rtw_read8(padapter, GPIO_IN_8811A);

		if (tmp1byte == 0xff)
			return ;

		if (tmp1byte & BIT4)
			bPbcPressed = _TRUE;
	}
#else

#endif

	if (_TRUE == bPbcPressed) {
		/* Here we only set bPbcPressed to true */
		/* After trigger PBC, the variable will be set to false */
		RTW_INFO("CheckPbcGPIO - PBC is pressed\n");

		rtw_request_wps_pbc_event(padapter);
	}
}
#endif /* #ifdef CONFIG_SUPPORT_HW_WPS_PBC */

#ifdef CONFIG_PCI_HCI
/*
 *	Description:
 *		Perform interrupt migration dynamically to reduce CPU utilization.
 *
 *	Assumption:
 *		1. Do not enable migration under WIFI test.
 *
 *	Created by Roger, 2010.03.05.
 *   */
void
dm_InterruptMigration(
	IN	PADAPTER	Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	BOOLEAN			bCurrentIntMt, bCurrentACIntDisable;
	BOOLEAN			IntMtToSet = _FALSE;
	BOOLEAN			ACIntToSet = _FALSE;


	/* Retrieve current interrupt migration and Tx four ACs IMR settings first. */
	bCurrentIntMt = pHalData->bInterruptMigration;
	bCurrentACIntDisable = pHalData->bDisableTxInt;

	/*  */
	/* <Roger_Notes> Currently we use busy traffic for reference instead of RxIntOK counts to prevent non-linear Rx statistics */
	/* when interrupt migration is set before. 2010.03.05. */
	/*  */
	if (!Adapter->registrypriv.wifi_spec &&
	    (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) &&
	    pmlmepriv->LinkDetectInfo.bHigherBusyTraffic) {
		IntMtToSet = _TRUE;

		/* To check whether we should disable Tx interrupt or not. */
		if (pmlmepriv->LinkDetectInfo.bHigherBusyRxTraffic)
			ACIntToSet = _TRUE;
	}

	/* Update current settings. */
	if (bCurrentIntMt != IntMtToSet) {
		RTW_INFO("%s(): Update interrrupt migration(%d)\n", __FUNCTION__, IntMtToSet);
		if (IntMtToSet) {
			/*  */
			/* <Roger_Notes> Set interrrupt migration timer and corresponging Tx/Rx counter. */
			/* timer 25ns*0xfa0=100us for 0xf packets. */
			/* 2010.03.05. */
			/*  */
			rtw_write32(Adapter, REG_INT_MIG, 0xff000fa0);/* 0x306:Rx, 0x307:Tx */
			pHalData->bInterruptMigration = IntMtToSet;
		} else {
			/* Reset all interrupt migration settings. */
			rtw_write32(Adapter, REG_INT_MIG, 0);
			pHalData->bInterruptMigration = IntMtToSet;
		}
	}

}

#endif

/*
 * Initialize GPIO setting registers
 *   */
static void
dm_InitGPIOSetting(
	IN	PADAPTER	Adapter
)
{
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(Adapter);

	u8	tmp1byte;

	tmp1byte = rtw_read8(Adapter, REG_GPIO_MUXCFG);
	tmp1byte &= (GPIOSEL_GPIO | ~GPIOSEL_ENBT);

	rtw_write8(Adapter, REG_GPIO_MUXCFG, tmp1byte);
}

/* ************************************************************
 * functions
 * ************************************************************ */
static void Init_ODM_ComInfo_8812(PADAPTER	Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct PHY_DM_STRUCT		*pDM_Odm = &(pHalData->odmpriv);
	u32 SupportAbility = 0;
	u8	cut_ver, fab_ver;

	Init_ODM_ComInfo(Adapter);

	fab_ver = ODM_TSMC;
	if (IS_A_CUT(pHalData->version_id))
		cut_ver = ODM_CUT_A;
	else if (IS_B_CUT(pHalData->version_id))
		cut_ver = ODM_CUT_B;
	else if (IS_C_CUT(pHalData->version_id))
		cut_ver = ODM_CUT_C;
	else if (IS_D_CUT(pHalData->version_id))
		cut_ver = ODM_CUT_D;
	else if (IS_E_CUT(pHalData->version_id))
		cut_ver = ODM_CUT_E;
	else
		cut_ver = ODM_CUT_A;

	odm_cmn_info_init(pDM_Odm, ODM_CMNINFO_FAB_VER, fab_ver);
	odm_cmn_info_init(pDM_Odm, ODM_CMNINFO_CUT_VER, cut_ver);

#ifdef CONFIG_DISABLE_ODM
	SupportAbility = 0;
#else
	SupportAbility =	ODM_RF_CALIBRATION	|
				ODM_RF_TX_PWR_TRACK
				;
#endif

	odm_cmn_info_update(pDM_Odm, ODM_CMNINFO_ABILITY, SupportAbility);

}
static void Update_ODM_ComInfo_8812(PADAPTER	Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct PHY_DM_STRUCT		*pDM_Odm = &(pHalData->odmpriv);
	u32 SupportAbility = 0;

	SupportAbility = 0
			 | ODM_BB_DIG
			 | ODM_BB_RA_MASK
			 | ODM_BB_FA_CNT
			 | ODM_BB_RSSI_MONITOR
			 | ODM_BB_CFO_TRACKING
			 | ODM_RF_TX_PWR_TRACK
			 | ODM_MAC_EDCA_TURBO
			 | ODM_BB_NHM_CNT
			 /*		| ODM_BB_PWR_TRAIN */
			 ;

	if (rtw_odm_adaptivity_needed(Adapter) == _TRUE) {
		rtw_odm_adaptivity_config_msg(RTW_DBGDUMP, Adapter);
		SupportAbility |= ODM_BB_ADAPTIVITY;
	}

#ifdef CONFIG_ANTENNA_DIVERSITY
	if (pHalData->AntDivCfg)
		SupportAbility |= ODM_BB_ANT_DIV;
#endif

#if (MP_DRIVER == 1)
	if (Adapter->registrypriv.mp_mode == 1) {
		SupportAbility = 0
				 | ODM_RF_CALIBRATION
				 | ODM_RF_TX_PWR_TRACK
				 ;
	}
#endif/* (MP_DRIVER==1) */

#ifdef CONFIG_DISABLE_ODM
	SupportAbility = 0;
#endif/* CONFIG_DISABLE_ODM */

	odm_cmn_info_update(pDM_Odm, ODM_CMNINFO_ABILITY, SupportAbility);
}

void
rtl8812_InitHalDm(
	IN	PADAPTER	Adapter
)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct PHY_DM_STRUCT		*pDM_Odm = &(pHalData->odmpriv);
	u8	i;

#ifdef CONFIG_USB_HCI
	dm_InitGPIOSetting(Adapter);
#endif

	pHalData->DM_Type = dm_type_by_driver;

	Update_ODM_ComInfo_8812(Adapter);
	odm_dm_init(pDM_Odm);

	/* Adapter->fix_rate = 0xFF; */

}


void
rtl8812_HalDmWatchDog(
	IN	PADAPTER	Adapter
)
{
	BOOLEAN		bFwCurrentInPSMode = _FALSE;
	BOOLEAN		bFwPSAwake = _TRUE;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct PHY_DM_STRUCT		*pDM_Odm = &(pHalData->odmpriv);


	if (!rtw_is_hw_init_completed(Adapter))
		goto skip_dm;

#ifdef CONFIG_LPS
	bFwCurrentInPSMode = adapter_to_pwrctl(Adapter)->bFwCurrentInPSMode;
	rtw_hal_get_hwreg(Adapter, HW_VAR_FWLPS_RF_ON, (u8 *)(&bFwPSAwake));
#endif

#ifdef CONFIG_P2P_PS
	/* Fw is under p2p powersaving mode, driver should stop dynamic mechanism. */
	/* modifed by thomas. 2011.06.11. */
	if (Adapter->wdinfo.p2p_ps_mode)
		bFwPSAwake = _FALSE;
#endif /* CONFIG_P2P_PS */

	if ((rtw_is_hw_init_completed(Adapter))
	    && ((!bFwCurrentInPSMode) && bFwPSAwake)) {
		/*  */
		/* Calculate Tx/Rx statistics. */
		/*  */
		dm_CheckStatistics(Adapter);
		rtw_hal_check_rxfifo_full(Adapter);
		/*  */
		/* Dynamically switch RTS/CTS protection. */
		/*  */
		/* dm_CheckProtection(Adapter); */

#ifdef CONFIG_PCI_HCI
		/* 20100630 Joseph: Disable Interrupt Migration mechanism temporarily because it degrades Rx throughput. */
		/* Tx Migration settings. */
		/* dm_InterruptMigration(Adapter); */

		/* if(Adapter->HalFunc.TxCheckStuckHandler(Adapter)) */
		/*	PlatformScheduleWorkItem(&(GET_HAL_DATA(Adapter)->HalResetWorkItem)); */
#endif

	}

	/* ODM */
	if (rtw_is_hw_init_completed(Adapter)) {
		u8	bLinked = _FALSE;
		u8	bsta_state = _FALSE;
		u8	bBtDisabled = _TRUE;

#ifdef CONFIG_DISABLE_ODM
		pHalData->odmpriv.support_ability = 0;
#endif

		if (rtw_mi_check_status(Adapter, MI_ASSOC)) {
			bLinked = _TRUE;
			if (rtw_mi_check_status(Adapter, MI_STA_LINKED))
				bsta_state = _TRUE;
		}

		odm_cmn_info_update(&pHalData->odmpriv , ODM_CMNINFO_LINK, bLinked);
		odm_cmn_info_update(&pHalData->odmpriv , ODM_CMNINFO_STATION_STATE, bsta_state);

#ifdef CONFIG_BT_COEXIST
		bBtDisabled = rtw_btcoex_IsBtDisabled(Adapter);
#endif /* CONFIG_BT_COEXIST */
		odm_cmn_info_update(&pHalData->odmpriv, ODM_CMNINFO_BT_ENABLED, ((bBtDisabled == _TRUE) ? _FALSE : _TRUE));

		odm_dm_watchdog(&pHalData->odmpriv);

	}

skip_dm:

#ifdef CONFIG_SUPPORT_HW_WPS_PBC
	/* Check GPIO to determine current Pbc status. */
	dm_CheckPbcGPIO(Adapter);
#endif

	return;
}

void rtl8812_init_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct PHY_DM_STRUCT		*podmpriv = &pHalData->odmpriv;

	/* _rtw_spinlock_init(&(pHalData->odm_stainfo_lock)); */

#ifndef CONFIG_IQK_PA_OFF /* FW has no IQK PA OFF option yet, don't offload */
	#ifdef CONFIG_BT_COEXIST
	/* firmware size issue, btcoex fw doesn't support IQK offload */
	if (pHalData->EEPROMBluetoothCoexist == _FALSE)
	#endif
	{
		pHalData->RegIQKFWOffload = 1;
		rtw_sctx_init(&pHalData->iqk_sctx, 0);
	}
#endif

	Init_ODM_ComInfo_8812(Adapter);
	odm_init_all_timers(podmpriv);
}

void rtl8812_deinit_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct PHY_DM_STRUCT		*podmpriv = &pHalData->odmpriv;
	/* _rtw_spinlock_free(&pHalData->odm_stainfo_lock); */
	odm_cancel_all_timers(podmpriv);
}
