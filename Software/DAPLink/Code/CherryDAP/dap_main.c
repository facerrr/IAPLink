#include "dap_main.h"
#include "user_param.h"
#include "mbcrc.h"

static volatile uint16_t USB_RequestIndexI; // Request  Index In
static volatile uint16_t USB_RequestIndexO; // Request  Index Out
static volatile uint16_t USB_RequestCountI; // Request  Count In
static volatile uint16_t USB_RequestCountO; // Request  Count Out
static volatile uint8_t USB_RequestIdle;    // Request  Idle  Flag

static volatile uint16_t USB_ResponseIndexI; // Response Index In
static volatile uint16_t USB_ResponseIndexO; // Response Index Out
static volatile uint16_t USB_ResponseCountI; // Response Count In
static volatile uint16_t USB_ResponseCountO; // Response Count Out
static volatile uint8_t USB_ResponseIdle;    // Response Idle  Flag

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t USB_Request[DAP_PACKET_COUNT][DAP_PACKET_SIZE];  // Request  Buffer
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t USB_Response[DAP_PACKET_COUNT][DAP_PACKET_SIZE]; // Response Buffer
static uint16_t USB_RespSize[DAP_PACKET_COUNT];                                                        // Response Size

uint32_t IC_UID;

void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            usart1_state.usbrx_idle_flag = 0;
            usart1_state.usbtx_idle_flag = 0;
            usart1_state.uarttx_idle_flag = 0;
            usart1_state.config_uart_transfer = 0;
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            /* setup first out ep read transfer */
            USB_RequestIdle = 0U;

            usbd_ep_start_read(DAP_BUS_ID, DAP_OUT_EP, USB_Request[0], DAP_PACKET_SIZE);
            usbd_ep_start_read(DAP_BUS_ID, CDC1_OUT_EP,usart1_state.usbrx_tmpbuffer, DAP_PACKET_SIZE);
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void dap_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if (USB_Request[USB_RequestIndexI][0] == ID_DAP_TransferAbort) {
        DAP_TransferAbort = 1U;
    } else {
        USB_RequestIndexI++;
        if (USB_RequestIndexI == DAP_PACKET_COUNT) {
            USB_RequestIndexI = 0U;
        }
        USB_RequestCountI++;
    }

    // Start reception of next request packet
    if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) != DAP_PACKET_COUNT) {
        usbd_ep_start_read(DAP_BUS_ID, DAP_OUT_EP, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE);
    } else {
        USB_RequestIdle = 1U;
    }
}

void dap_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if (USB_ResponseCountI != USB_ResponseCountO) {
        // Load data from response buffer to be sent back
        usbd_ep_start_write(DAP_BUS_ID, DAP_IN_EP, USB_Response[USB_ResponseIndexO], USB_RespSize[USB_ResponseIndexO]);
        USB_ResponseIndexO++;
        if (USB_ResponseIndexO == DAP_PACKET_COUNT) {
            USB_ResponseIndexO = 0U;
        }
        USB_ResponseCountO++;
    } else {
        USB_ResponseIdle = 1U;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    usb2uart_state_t *state = &usart1_state;
    chry_ringbuffer_write(state->g_usbrx, state->usbrx_tmpbuffer, nbytes);
    if (chry_ringbuffer_get_free(state->g_usbrx) >= DAP_PACKET_SIZE) {
        usbd_ep_start_read(DAP_BUS_ID, ep, state->usbrx_tmpbuffer, DAP_PACKET_SIZE);
    } else {
        state->usbrx_idle_flag = 1;
    }
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    usb2uart_state_t *state = &usart1_state;
    
    // 发完之后处理
    /* Calculate the variables of interest */
    state->Rx_RemainLen -= nbytes;
    state->Rx_DealPtr += nbytes;
    if( state->Rx_DealPtr >= USART_BUFFER_SIZE )
    {
        state->Rx_DealPtr = 0x00;
    }
    
    if ((nbytes % DAP_PACKET_SIZE) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(DAP_BUS_ID, ep, NULL, 0);
    } else {
        state->usbtx_idle_flag = 1;
    }
}

static struct usbd_endpoint dap_out_ep = {
    .ep_addr = DAP_OUT_EP,
    .ep_cb = dap_out_callback
};

static struct usbd_endpoint dap_in_ep = {
    .ep_addr = DAP_IN_EP,
    .ep_cb = dap_in_callback
};

static struct usbd_endpoint cdc1_out_ep = {
    .ep_addr = CDC1_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc1_in_ep = {
    .ep_addr = CDC1_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_endpoint cdc2_out_ep = {
    .ep_addr = CDC2_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc2_in_ep = {
    .ep_addr = CDC2_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

struct usbd_interface dap_intf;
struct usbd_interface intf1;
struct usbd_interface intf2;
struct usbd_interface intf3;
struct usbd_interface intf4;

static void chry_dap_state_init(void)
{
    // Initialize variables
    USB_RequestIndexI = 0U;
    USB_RequestIndexO = 0U;
    USB_RequestCountI = 0U;
    USB_RequestCountO = 0U;
    USB_RequestIdle = 1U;
    USB_ResponseIndexI = 0U;
    USB_ResponseIndexO = 0U;
    USB_ResponseCountI = 0U;
    USB_ResponseCountO = 0U;
    USB_ResponseIdle = 1U;
}

void chry_dap_init(void)
{
    DAP_Setup();
    chry_dap_state_init();

    IC_UID = *(uint32_t *)0x1FFFF7E8 & 0x0001FFFF;                                    //取前面0~16位,17位，对应唯一识别码0~16位
    IC_UID |= (((*(uint32_t *)(0x1FFFF7E8)) & (0x0000000F << 28)) >> 28 << 17);       //取28到31位，4位 ，对应17-20
    IC_UID |= (((*(uint32_t *)(0x1FFFF7E8 + 4)) & 0x00000003) << 21);                 //取32~33，2位，对应21~22
    IC_UID |= ((((*(uint32_t *)(0x1FFFF7E8 + 8)) & (0x000001FF << 15)) >> 15) << 23); //取最后9位,对应唯一识别码23~31

//    // Fix UID
//    IC_UID = *(uint32_t*)(0x1FFFF7E8);
//    IC_UID ^= *(uint32_t*)(0x1FFFF7E8 + 4);
//    IC_UID ^= *(uint32_t*)(0x1FFFF7E8 + 8);
    
#ifdef CONFIG_USBDEV_ADVANCE_DESC
    char UID_BUF[9] = { 0 };
    sprintf(UID_BUF, "%08X", IC_UID);
    string3_descriptor[0] = UID_BUF[0];
    string3_descriptor[1] = UID_BUF[1];
    string3_descriptor[2] = UID_BUF[2];
    string3_descriptor[3] = UID_BUF[3];
    string3_descriptor[4] = UID_BUF[4];
    string3_descriptor[5] = UID_BUF[5];
    string3_descriptor[6] = UID_BUF[6];
    string3_descriptor[7] = UID_BUF[7];
    
    char ID_BUF[4] = { 0 };
    sprintf(ID_BUF, "%03d", (uint8_t)user_param.dap_id);
    string2_descriptor[1] = ID_BUF[0];
    string2_descriptor[2] = ID_BUF[1];
    string2_descriptor[3] = ID_BUF[2];
    usbd_desc_register(DAP_BUS_ID, &cmsisdap_descriptor);
#else
    char UID_BUF[9] = { 0 };
    sprintf(UID_BUF, "%08X", IC_UID);
    cmsisdap_descriptor[182] = UID_BUF[0];
    cmsisdap_descriptor[184] = UID_BUF[1];
    cmsisdap_descriptor[186] = UID_BUF[2];
    cmsisdap_descriptor[188] = UID_BUF[3];
    cmsisdap_descriptor[190] = UID_BUF[4];
    cmsisdap_descriptor[192] = UID_BUF[5];
    cmsisdap_descriptor[194] = UID_BUF[6];
    cmsisdap_descriptor[196] = UID_BUF[7];
    
    usbd_desc_register(DAP_BUS_ID, cmsisdap_descriptor);
    usbd_bos_desc_register(DAP_BUS_ID, &bos_desc);
    usbd_msosv2_desc_register(DAP_BUS_ID, &msosv2_desc);
#endif

    /*!< winusb */
    usbd_add_interface(DAP_BUS_ID, &dap_intf);
    usbd_add_endpoint(DAP_BUS_ID, &dap_out_ep);
    usbd_add_endpoint(DAP_BUS_ID, &dap_in_ep);

    /*!< cdc acm */
    usbd_add_interface(DAP_BUS_ID, usbd_cdc_acm_init_intf(DAP_BUS_ID, &intf1));
    usbd_add_interface(DAP_BUS_ID, usbd_cdc_acm_init_intf(DAP_BUS_ID, &intf2));
    usbd_add_endpoint(DAP_BUS_ID, &cdc1_out_ep);
    usbd_add_endpoint(DAP_BUS_ID, &cdc1_in_ep);
    
    /*!< cdc acm2 */
    usbd_add_interface(DAP_BUS_ID, usbd_cdc_acm_init_intf(DAP_BUS_ID, &intf3));
    usbd_add_interface(DAP_BUS_ID, usbd_cdc_acm_init_intf(DAP_BUS_ID, &intf4));
    usbd_add_endpoint(DAP_BUS_ID, &cdc2_out_ep);
    usbd_add_endpoint(DAP_BUS_ID, &cdc2_in_ep);

#if 0
    usbd_add_interface(DAP_BUS_ID, usbd_hid_init_intf(DAP_BUS_ID, &intf3, hid_custom_report_desc, HID_CUSTOM_REPORT_DESC_SIZE));
    usbd_add_endpoint(DAP_BUS_ID, &custom_in_ep);
    usbd_add_endpoint(DAP_BUS_ID, &custom_out_ep);
#endif

    usbd_initialize(DAP_BUS_ID, USBFS_BASE, &usbd_event_handler);
}

void chry_dap_handle(void)
{
    uint32_t n;

    // Process pending requests
    while (USB_RequestCountI != USB_RequestCountO) {
        // Handle Queue Commands
        n = USB_RequestIndexO;
        while (USB_Request[n][0] == ID_DAP_QueueCommands) {
            USB_Request[n][0] = ID_DAP_ExecuteCommands;
            n++;
            if (n == DAP_PACKET_COUNT) {
                n = 0U;
            }
            if (n == USB_RequestIndexI) {
                // flags = osThreadFlagsWait(0x81U, osFlagsWaitAny, osWaitForever);
                // if (flags & 0x80U) {
                //     break;
                // }
            }
        }

        // Execute DAP Command (process request and prepare response)
        USB_RespSize[USB_ResponseIndexI] =
            (uint16_t)DAP_ExecuteCommand(USB_Request[USB_RequestIndexO], USB_Response[USB_ResponseIndexI]);

        // Update Request Index and Count
        USB_RequestIndexO++;
        if (USB_RequestIndexO == DAP_PACKET_COUNT) {
            USB_RequestIndexO = 0U;
        }
        USB_RequestCountO++;

        if (USB_RequestIdle) {
            if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) != DAP_PACKET_COUNT) {
                USB_RequestIdle = 0U;
                usbd_ep_start_read(DAP_BUS_ID, DAP_OUT_EP, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE);
            }
        }

        // Update Response Index and Count
        USB_ResponseIndexI++;
        if (USB_ResponseIndexI == DAP_PACKET_COUNT) {
            USB_ResponseIndexI = 0U;
        }
        USB_ResponseCountI++;

        if (USB_ResponseIdle) {
            if (USB_ResponseCountI != USB_ResponseCountO) {
                // Load data from response buffer to be sent back
                n = USB_ResponseIndexO++;
                if (USB_ResponseIndexO == DAP_PACKET_COUNT) {
                    USB_ResponseIndexO = 0U;
                }
                USB_ResponseCountO++;
                USB_ResponseIdle = 0U;
                usbd_ep_start_write(DAP_BUS_ID, DAP_IN_EP, USB_Response[n], USB_RespSize[n]);
            }
        }
    }
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    if (memcmp(line_coding, (uint8_t *)&usart1_state.g_cdc_lincoding, sizeof(struct cdc_line_coding)) != 0) {
        memcpy((uint8_t *)&usart1_state.g_cdc_lincoding, line_coding, sizeof(struct cdc_line_coding));
        usart1_state.config_uart = 1;
        usart1_state.config_uart_transfer = 0;
    }
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    memcpy(line_coding, (uint8_t *)&usart1_state.g_cdc_lincoding, sizeof(struct cdc_line_coding));
}

void chry_dap_usb2uart_handle(usb2uart_state_t *usart_state)
{
    uint32_t size;
    uint8_t *buffer;

    if (usart_state->config_uart) {
        /* disable irq here */
        usart_state->config_uart = 0;
        /* config uart here */
        chry_dap_usb2uart_uart_config_callback(usart_state);
        usart_state->usbtx_idle_flag = 1;
        usart_state->uarttx_idle_flag = 1;
        usart_state->config_uart_transfer = 1;
        //chry_ringbuffer_reset_read(&g_uartrx);
        /* enable irq here */
    }

    if (usart_state->config_uart_transfer == 0) {
        return;
    }

    /* why we use chry_ringbuffer_linear_read_setup?
     * becase we use dma and we do not want to use temp buffer to memcpy from ringbuffer
     * 
    */
    /* Serial port 1 data processing via USB upload and reception */
    if(usart_state->usbtx_idle_flag)
    {
        if(usart_state->Rx_RemainLen)
        {
            usart_state->usbtx_idle_flag = 0;

            //uint32_t remain_len;
            uint16_t packlen;
            /* Calculate the length of this upload */
            if (usart_state->Rx_RemainLen > 512) {
                packlen = 512;
            } else {
                packlen = usart_state->Rx_RemainLen;
            }
            // 这里防止一次性发太多直接越界了
            if( packlen > ( USART_BUFFER_SIZE - usart_state->Rx_DealPtr ) )
            {
                packlen = ( USART_BUFFER_SIZE - usart_state->Rx_DealPtr );
            }

            /* Upload serial data via usb */
            if( packlen )
            {
                memcpy(usart_state->usbtx_tmpbuffer,&usart_state->usart_rx_buffer[ usart_state->Rx_DealPtr ], packlen);
                usbd_ep_start_write(DAP_BUS_ID, CDC1_IN_EP, usart_state->usbtx_tmpbuffer, packlen);
            }
        }
    }

    /* usbrx to uart tx */
    if (usart_state->uarttx_idle_flag) {
        if (chry_ringbuffer_get_used(usart_state->g_usbrx)) {
            usart_state->uarttx_idle_flag = 0;
            /* start first transfer */
            buffer = chry_ringbuffer_linear_read_setup(usart_state->g_usbrx, &size);
            chry_dap_usb2uart_uart_send_bydma(usart_state, buffer, size);
        }
    }

    /* check whether usb rx ringbuffer have space to store */
    if (usart_state->usbrx_idle_flag) {
        if (chry_ringbuffer_get_free(usart_state->g_usbrx) >= DAP_PACKET_SIZE) {
            usart_state->usbrx_idle_flag = 0;
            usbd_ep_start_read(DAP_BUS_ID, CDC1_OUT_EP, usart_state->usbrx_tmpbuffer, DAP_PACKET_SIZE);
        }
    }
}

#if 0
void chry_dap_hid_handle(void)
{
    if(!hid_received)
    {
        return;
    }
    /**
    report id | Front         |  CMD    |  Length |  Data  |  Check
    0x01      | 0x55 0xaa     |  0x01   |  0x00   |  0x0   |  0x11  0x22
    0         | 1             |  3      |  4      |  5     |  6
    */
    hid_received = 0;
    // check header
    if(hid_read_buffer[1] != HID_PACK_HEADER_0 || hid_read_buffer[2] != HID_PACK_HEADER_1)
    {
        return;
    }
    uint8_t data_len = hid_read_buffer[4];
    // check len, remove header+len+crc+hid report id
    if(data_len > (HIDRAW_OUT_EP_SIZE - (HID_PACK_REPORT_ID_LEN + HID_PACK_FRONT_LEN + HID_PACK_CMD_LEN + HID_PACK_LENGTH_LEN)))
    {
        return;
    }
    // CRC
    if(usMBCRC16( &hid_read_buffer[1], data_len + 6 ) != 0 )
    {
        return;
    }
    
    chry_dap_hid_exec(&hid_read_buffer[3] ,data_len);
}
#endif