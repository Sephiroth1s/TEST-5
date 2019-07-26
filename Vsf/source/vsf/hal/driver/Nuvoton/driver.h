/*****************************************************************************
 *   Copyright(C)2009-2019 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

#ifndef __HAL_DRIVER_NUVOTON_H__
#define __HAL_DRIVER_NUVOTON_H__

/*============================ INCLUDES ======================================*/
#include "hal/vsf_hal_cfg.h"

#if VSF_USE_SERVICE_VSFSTREAM == ENABLED || VSF_USE_SERVICE_STREAM == ENABLED
#include "service/vsf_service.h"
#endif

#undef VSF_DRIVER_HEADER

#if     defined(__M484__)
#   define  VSF_DRIVER_HEADER       "./M480/M484/driver.h"
#else
#   error No supported device found.
#endif

/* include specified device driver header file */
#include VSF_DRIVER_HEADER

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/

#if     VSF_USE_SERVICE_VSFSTREAM == ENABLED
#define VSF_DEBUG_STREAM_TX     VSF_DEBUG_STREAM
#define VSF_DEBUG_STREAM_RX     VSF_DEBUG_STREAM

extern vsf_stream_t  VSF_DEBUG_STREAM;
#elif   VSF_USE_SERVICE_STREAM == ENABLED
extern const vsf_stream_tx_t VSF_DEBUG_STREAM_TX;
#endif

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/


#endif
/* EOF */
