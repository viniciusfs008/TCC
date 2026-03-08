#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "virtio.h"

void setup_input_device(uint64 addr);

typedef enum virtio_input_config_select { 
    VIRTIO_INPUT_CFG_UNSET      = 0x00, 
    VIRTIO_INPUT_CFG_ID_NAME    = 0x01, 
    VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02, 
    VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03, 
    VIRTIO_INPUT_CFG_PROP_BITS  = 0x10, 
    VIRTIO_INPUT_CFG_EV_BITS    = 0x11, 
    VIRTIO_INPUT_CFG_ABS_INFO   = 0x12, 
}virtio_input_config_select; 
 
typedef struct virtio_input_absinfo { 
    uint32  min; 
    uint32  max; 
    uint32  fuzz; 
    uint32  flat; 
    uint32  res; 
}virtio_input_absinfo; 
 
typedef struct virtio_input_devids { 
    uint16  bustype; 
    uint16  vendor; 
    uint16  product; 
    uint16  version; 
}virtio_input_devids; 
 
typedef struct virtio_input_config { 
    uint8    select; 
    uint8    subsel; 
    uint8    size; 
    uint8    reserved[5]; 
    union { 
        char string[128]; 
        uint8 bitmap[128]; 
        struct virtio_input_absinfo abs; 
        struct virtio_input_devids ids; 
    } u; 
}virtio_input_config;

typedef struct virtio_input_event { 
    uint16 type; 
    uint16 code; 
    uint32 value;
}virtio_input_event;

typedef struct keyboard_device{
    uint64 address;
    VirtQ *status_queue;
    VirtQ *event_queue;
    uint16 event_ack_idx;
    uint16 status_ack_idx;
    virtio_input_event *event_buffer;
    uint16 event_buffer_idx;
}keyboard_device;

#define EVENT_QUEUE_SIZE 20

typedef struct static_event_queue{
    virtio_input_event events[EVENT_QUEUE_SIZE];
    uint16 head, tail;
    uint8 empty, full;
    void (*add)(struct static_event_queue*, virtio_input_event);
    virtio_input_event (*get_event)(struct static_event_queue*);
}static_event_queue;

void keyboard_int();

static_event_queue* get_event_queue();


#endif
