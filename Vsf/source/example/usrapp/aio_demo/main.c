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

/*============================ INCLUDES ======================================*/

#include "vsf.h"
#include <stdio.h>
#include <stdarg.h>

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

struct usrapp_t {
    bool is_usbd_connected;
    vsf_callback_timer_t poll_timer;
};
typedef struct usrapp_t usrapp_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/

static usrapp_t usrapp;

/*============================ PROTOTYPES ====================================*/

extern void usbh_demo_start(void);
extern void usbd_demo_start(void);
extern void usbd_demo_connect(void);
extern void tcpip_demo_start(void);
extern void input_demo_start(void);
extern void eda_sub_demo_start(void);

/*============================ IMPLEMENTATION ================================*/

void usrapp_on_timer(vsf_callback_timer_t *timer)
{
    if (!usrapp.is_usbd_connected) {
        usrapp.is_usbd_connected = true;
        usbd_demo_connect();
    } else {
        vsf_trace(VSF_TRACE_INFO, "heartbeat: [%lld]" VSF_TRACE_CFG_LINEEND, vsf_timer_get_tick());
    }
    vsf_callback_timer_add_ms(timer, 1000);
}

int main(void)
{
    vsf_trace_init(NULL);

    usbh_demo_start();
    tcpip_demo_start();
    usbd_demo_start();
    input_demo_start();
    eda_sub_demo_start();

    usrapp.poll_timer.on_timer = usrapp_on_timer;
    vsf_callback_timer_add_ms(&usrapp.poll_timer, 200);
    return 0;
}

/* EOF */
