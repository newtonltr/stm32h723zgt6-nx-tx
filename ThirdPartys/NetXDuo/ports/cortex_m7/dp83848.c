/**
  ******************************************************************************
  * @file    dp83848.c
  * @author  MCD Application Team
  * @brief   This file provides a set of functions needed to manage the DP83848
  *          PHY devices.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "dp83848.h"
#include "main.h"
/** @addtogroup BSP
  * @{
  */

/** @addtogroup Component
  * @{
  */

/** @defgroup DP83848 DP83848
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup DP83848_Private_Defines DP83848 Private Defines
  * @{
  */
#define DP83848_SW_RESET_TO    ((uint32_t)500U)
#define DP83848_INIT_TO        ((uint32_t)2000U)
#define DP83848_MAX_DEV_ADDR   ((uint32_t)31U)
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/** @defgroup DP83848_Private_Functions DP83848 Private Functions
  * @{
  */
/*******************************************************************************
                    PHY IO Functions
*******************************************************************************/
/**
  * @brief  初始化PHY芯片
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_Init(void)
{
  //设置MDIO时钟
  HAL_ETH_SetMDIOClockRange(&heth);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_DeInit (void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if(HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if(HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
int32_t ETH_PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}

/**
  * @brief  注册IO函数
  * @param  pObj: device object  of DP83848_Object_t.
  * @param  ioctx: holds device IO functions.
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_ERROR if missing mandatory function
  */
int32_t  DP83848_RegisterBusIO(dp83848_Object_t *pObj, dp83848_IOCtx_t *ioctx)
{
  if(!pObj || !ioctx->ReadReg || !ioctx->WriteReg || !ioctx->GetTick)
  {
    return DP83848_STATUS_ERROR;
  }

  pObj->IO.Init = ioctx->Init;
  pObj->IO.DeInit = ioctx->DeInit;
  pObj->IO.ReadReg = ioctx->ReadReg;
  pObj->IO.WriteReg = ioctx->WriteReg;
  pObj->IO.GetTick = ioctx->GetTick;

  return DP83848_STATUS_OK;
}

/**
  * @brief  初始化DP83848
  * @param  pObj: device object DP83848_Object_t.
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_ADDRESS_ERROR if cannot find device address
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  *         DP83848_STATUS_RESET_TIMEOUT if cannot perform a software reset
  */
 int32_t DP83848_Init(dp83848_Object_t *pObj)
 {
   uint32_t tickstart = 0, regvalue = 0, addr = 0;
   int32_t status = DP83848_STATUS_OK;

   if(pObj->Is_Initialized == 0)
   {
     if(pObj->IO.Init != 0)
     {
       /* GPIO and Clocks initialization */
       pObj->IO.Init();
     }

     /* for later check */
     pObj->DevAddr = DP83848_MAX_DEV_ADDR + 1;

     /* Get the device address from special mode register */
     for(addr = 0; addr <= DP83848_MAX_DEV_ADDR; addr ++)
     {
       if(pObj->IO.ReadReg(addr, DP83848_SMR, &regvalue) < 0)
       {
         status = DP83848_STATUS_READ_ERROR;
         /* Can't read from this device address
            continue with next address */
         continue;
       }

       if((regvalue & DP83848_SMR_PHY_ADDR) == addr)
       {
         pObj->DevAddr = addr;
         status = DP83848_STATUS_OK;
         break;
       }
     }

     if(pObj->DevAddr > DP83848_MAX_DEV_ADDR)
     {
       status = DP83848_STATUS_ADDRESS_ERROR;
     }

     /* if device address is matched */
     if(status == DP83848_STATUS_OK)
     {
       /* set a software reset  */
       if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_BCR, DP83848_BCR_SOFT_RESET) >= 0)
       {
         /* get software reset status */
         if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &regvalue) >= 0)
         {
           tickstart = pObj->IO.GetTick();

           /* wait until software reset is done or timeout occured  */
           while(regvalue & DP83848_BCR_SOFT_RESET)
           {
             if((pObj->IO.GetTick() - tickstart) <= DP83848_SW_RESET_TO)
             {
               if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &regvalue) < 0)
               {
                 status = DP83848_STATUS_READ_ERROR;
                 break;
               }
             }
             else
             {
               status = DP83848_STATUS_RESET_TIMEOUT;
               break;
             }
           }
         }
         else
         {
           status = DP83848_STATUS_READ_ERROR;
         }
       }
       else
       {
         status = DP83848_STATUS_WRITE_ERROR;
       }
     }
   }

   if(status == DP83848_STATUS_OK)
   {
     tickstart =  pObj->IO.GetTick();

     /* Wait for 2s to perform initialization */
     while((pObj->IO.GetTick() - tickstart) <= DP83848_INIT_TO)
     {
     }
     pObj->Is_Initialized = 1;
   }

   return status;
 }

/**
  * @brief  卸载DP83848
  * @param  pObj: device object DP83848_Object_t.
  * @retval None
  */
int32_t DP83848_DeInit(dp83848_Object_t *pObj)
{
  if(pObj->Is_Initialized)
  {
    if(pObj->IO.DeInit != 0)
    {
      if(pObj->IO.DeInit() < 0)
      {
        return DP83848_STATUS_ERROR;
      }
    }

    pObj->Is_Initialized = 0;
  }

  return DP83848_STATUS_OK;
}

/**
  * @brief  禁用DP83848的PowerDown模式
  * @param  pObj: device object DP83848_Object_t.
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_DisablePowerDownMode(dp83848_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &readval) >= 0)
  {
    readval &= ~DP83848_BCR_POWER_DOWN;

    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_BCR, readval) < 0)
    {
      status =  DP83848_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  启用DP83848的PowerDown模式
  * @param  pObj: device object DP83848_Object_t.
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_EnablePowerDownMode(dp83848_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &readval) >= 0)
  {
    readval |= DP83848_BCR_POWER_DOWN;

    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_BCR, readval) < 0)
    {
      status =  DP83848_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  启用速率自动协商功能
  * @param  pObj: device object DP83848_Object_t.
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_StartAutoNego(dp83848_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &readval) >= 0)
  {
    readval |= DP83848_BCR_AUTONEGO_EN;

    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_BCR, readval) < 0)
    {
      status =  DP83848_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  获取83848连接状态
  * @param  pObj: Pointer to device object.
  * @param  pLinkState: Pointer to link state
  * @retval DP83848_STATUS_LINK_DOWN  if link is down
  *         DP83848_STATUS_AUTONEGO_NOTDONE if Auto nego not completed
  *         DP83848_STATUS_100MBITS_FULLDUPLEX if 100Mb/s FD
  *         DP83848_STATUS_100MBITS_HALFDUPLEX if 100Mb/s HD
  *         DP83848_STATUS_10MBITS_FULLDUPLEX  if 10Mb/s FD
  *         DP83848_STATUS_10MBITS_HALFDUPLEX  if 10Mb/s HD
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_GetLinkState(dp83848_Object_t *pObj)
{
  uint32_t readval = 0;

  /* Read Status register  */
  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BSR, &readval) < 0)
  {
    return DP83848_STATUS_READ_ERROR;
  }

  /* Read Status register again */
  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BSR, &readval) < 0)
  {
    return DP83848_STATUS_READ_ERROR;
  }

  if((readval & DP83848_BSR_LINK_STATUS) == 0)
  {
    /* Return Link Down status */
    return DP83848_STATUS_LINK_DOWN;
  }

  /* Check Auto negotiaition */
  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &readval) < 0)
  {
    return DP83848_STATUS_READ_ERROR;
  }

  if((readval & DP83848_BCR_AUTONEGO_EN) != DP83848_BCR_AUTONEGO_EN)
  {
    if(((readval & DP83848_BCR_SPEED_SELECT) == DP83848_BCR_SPEED_SELECT) && ((readval & DP83848_BCR_DUPLEX_MODE) == DP83848_BCR_DUPLEX_MODE))
    {
      return DP83848_STATUS_100MBITS_FULLDUPLEX;
    }
    else if ((readval & DP83848_BCR_SPEED_SELECT) == DP83848_BCR_SPEED_SELECT)
    {
      return DP83848_STATUS_100MBITS_HALFDUPLEX;
    }
    else if ((readval & DP83848_BCR_DUPLEX_MODE) == DP83848_BCR_DUPLEX_MODE)
    {
      return DP83848_STATUS_10MBITS_FULLDUPLEX;
    }
    else
    {
      return DP83848_STATUS_10MBITS_HALFDUPLEX;
    }
  }
  else /* Auto Nego enabled */
  {
    if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_PHYSCSR, &readval) < 0)
    {
      return DP83848_STATUS_READ_ERROR;
    }

    /* Check if auto nego not done */
    if((readval & DP83848_PHYSCSR_AUTONEGO_DONE) == 0)
    {
      return DP83848_STATUS_AUTONEGO_NOTDONE;
    }

    if((readval & DP83848_PHYSCSR_HCDSPEEDMASK) == DP83848_PHYSCSR_100BTX_FD)
    {
      return DP83848_STATUS_100MBITS_FULLDUPLEX;
    }
    else if ((readval & DP83848_PHYSCSR_HCDSPEEDMASK) == DP83848_PHYSCSR_100BTX_HD)
    {
      return DP83848_STATUS_100MBITS_HALFDUPLEX;
    }
    else if ((readval & DP83848_PHYSCSR_HCDSPEEDMASK) == DP83848_PHYSCSR_10BT_FD)
    {
      return DP83848_STATUS_10MBITS_FULLDUPLEX;
    }
    else
    {
      return DP83848_STATUS_10MBITS_HALFDUPLEX;
    }
  }
}

/**
  * @brief  设置dp83848连接状态
  * @param  pObj: Pointer to device object.
  * @param  pLinkState: link state can be one of the following
  *         DP83848_STATUS_100MBITS_FULLDUPLEX if 100Mb/s FD
  *         DP83848_STATUS_100MBITS_HALFDUPLEX if 100Mb/s HD
  *         DP83848_STATUS_10MBITS_FULLDUPLEX  if 10Mb/s FD
  *         DP83848_STATUS_10MBITS_HALFDUPLEX  if 10Mb/s HD
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_ERROR  if parameter error
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_SetLinkState(dp83848_Object_t *pObj, uint32_t LinkState)
{
  uint32_t bcrvalue = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &bcrvalue) >= 0)
  {
    /* Disable link config (Auto nego, speed and duplex) */
    bcrvalue &= ~(DP83848_BCR_AUTONEGO_EN | DP83848_BCR_SPEED_SELECT | DP83848_BCR_DUPLEX_MODE);

    if(LinkState == DP83848_STATUS_100MBITS_FULLDUPLEX)
    {
      bcrvalue |= (DP83848_BCR_SPEED_SELECT | DP83848_BCR_DUPLEX_MODE);
    }
    else if (LinkState == DP83848_STATUS_100MBITS_HALFDUPLEX)
    {
      bcrvalue |= DP83848_BCR_SPEED_SELECT;
    }
    else if (LinkState == DP83848_STATUS_10MBITS_FULLDUPLEX)
    {
      bcrvalue |= DP83848_BCR_DUPLEX_MODE;
    }
    else
    {
      /* Wrong link status parameter */
      status = DP83848_STATUS_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  if(status == DP83848_STATUS_OK)
  {
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_BCR, bcrvalue) < 0)
    {
      status = DP83848_STATUS_WRITE_ERROR;
    }
  }

  return status;
}

/**
  * @brief  启用回环模式
  * @param  pObj: Pointer to device object.
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_EnableLoopbackMode(dp83848_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &readval) >= 0)
  {
    readval |= DP83848_BCR_LOOPBACK;

    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_BCR, readval) < 0)
    {
      status = DP83848_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  禁用回环模式
  * @param  pObj: Pointer to device object.
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_DisableLoopbackMode(dp83848_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_BCR, &readval) >= 0)
  {
    readval &= ~DP83848_BCR_LOOPBACK;

    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_BCR, readval) < 0)
    {
      status =  DP83848_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  启用DP83848的指定中断源
  * @param  pObj: Pointer to device object.
  * @param  Interrupt: IT source to be enabled
  *         should be a value or a combination of the following:
  *         DP83848_WOL_IT
  *         DP83848_ENERGYON_IT
  *         DP83848_AUTONEGO_COMPLETE_IT
  *         DP83848_REMOTE_FAULT_IT
  *         DP83848_LINK_DOWN_IT
  *         DP83848_AUTONEGO_LP_ACK_IT
  *         DP83848_PARALLEL_DETECTION_FAULT_IT
  *         DP83848_AUTONEGO_PAGE_RECEIVED_IT
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_EnableIT(dp83848_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_IMR, &readval) >= 0)
  {
    readval |= Interrupt;

    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_IMR, readval) < 0)
    {
      status =  DP83848_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  禁用DP83848的指定中断源
  * @param  pObj: Pointer to device object.
  * @param  Interrupt: IT source to be disabled
  *         should be a value or a combination of the following:
  *         DP83848_WOL_IT
  *         DP83848_ENERGYON_IT
  *         DP83848_AUTONEGO_COMPLETE_IT
  *         DP83848_REMOTE_FAULT_IT
  *         DP83848_LINK_DOWN_IT
  *         DP83848_AUTONEGO_LP_ACK_IT
  *         DP83848_PARALLEL_DETECTION_FAULT_IT
  *         DP83848_AUTONEGO_PAGE_RECEIVED_IT
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  *         DP83848_STATUS_WRITE_ERROR if connot write to register
  */
int32_t DP83848_DisableIT(dp83848_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_IMR, &readval) >= 0)
  {
    readval &= ~Interrupt;

    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, DP83848_IMR, readval) < 0)
    {
      status = DP83848_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  清除DP83848的指定中断标志
  * @param  pObj: Pointer to device object.
  * @param  Interrupt: IT flag to be cleared
  *         should be a value or a combination of the following:
  *         DP83848_WOL_IT
  *         DP83848_ENERGYON_IT
  *         DP83848_AUTONEGO_COMPLETE_IT
  *         DP83848_REMOTE_FAULT_IT
  *         DP83848_LINK_DOWN_IT
  *         DP83848_AUTONEGO_LP_ACK_IT
  *         DP83848_PARALLEL_DETECTION_FAULT_IT
  *         DP83848_AUTONEGO_PAGE_RECEIVED_IT
  * @retval DP83848_STATUS_OK  if OK
  *         DP83848_STATUS_READ_ERROR if connot read register
  */
int32_t  DP83848_ClearIT(dp83848_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = DP83848_STATUS_OK;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_ISFR, &readval) < 0)
  {
    status =  DP83848_STATUS_READ_ERROR;
  }

  return status;
}

/**
  * @brief  获取DP83848的中断状态标志
  * @param  pObj: Pointer to device object.
  * @param  Interrupt: IT Flag to be checked,
  *         should be a value or a combination of the following:
  *         DP83848_WOL_IT
  *         DP83848_ENERGYON_IT
  *         DP83848_AUTONEGO_COMPLETE_IT
  *         DP83848_REMOTE_FAULT_IT
  *         DP83848_LINK_DOWN_IT
  *         DP83848_AUTONEGO_LP_ACK_IT
  *         DP83848_PARALLEL_DETECTION_FAULT_IT
  *         DP83848_AUTONEGO_PAGE_RECEIVED_IT
  * @retval 1 IT flag is SET
  *         0 IT flag is RESET
  *         DP83848_STATUS_READ_ERROR if connot read register
  */
int32_t DP83848_GetITStatus(dp83848_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = 0;

  if(pObj->IO.ReadReg(pObj->DevAddr, DP83848_ISFR, &readval) >= 0)
  {
    status = ((readval & Interrupt) == Interrupt);
  }
  else
  {
    status = DP83848_STATUS_READ_ERROR;
  }

  return status;
}

//DP83848组件
dp83848_Object_t DP83848;
//DP83848 IO操作函数
dp83848_IOCtx_t  DP83848_IOCtx = {ETH_PHY_IO_Init,
                                  ETH_PHY_IO_DeInit,
                                  ETH_PHY_IO_WriteReg,
                                  ETH_PHY_IO_ReadReg,
                                  ETH_PHY_IO_GetTick};

/*******************************************************************************
                      NX和PHY接口函数
*******************************************************************************/

//初始化PHY
int32_t nx_eth_phy_init(void)
{
    int32_t ret = DP83848_STATUS_ERROR;
    /* Set PHY IO functions */

    DP83848_RegisterBusIO(&DP83848, &DP83848_IOCtx);
    /* Initialize the dp83848 ETH PHY */

    if (DP83848_Init(&DP83848) == DP83848_STATUS_OK)
    {
        ret = DP83848_STATUS_OK;
    }

    return ret;
}

/**
  * @brief  设置PHY连接状态
  * @param  LinkState
  * @retval the link status.
  */

int32_t nx_eth_phy_set_link_state(int32_t LinkState)
{
    return (DP83848_SetLinkState(&DP83848, LinkState));
}

/**
  * @brief  获取PHY连接状态
  * @param  none
  * @retval the link status.
  */

int32_t nx_eth_phy_get_link_state(void)
{
    int32_t  linkstate = DP83848_GetLinkState(&DP83848);

    return linkstate;
}

/**
  * @brief  获取PHY驱动句柄
  * @param  none
  * @retval pointer to the LAN8742 main object
  */

nx_eth_phy_handle_t nx_eth_phy_get_handle(void)
{
    return (nx_eth_phy_handle_t)&DP83848;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

