#include "keyboard.h"
#include "../defs.h"
#include "../libs/input-event-codes.h"


#define TOTAL_EVENT_BUFFER 0x40

static_event_queue av_events_queue;
keyboard_device KEYBOARD_DEVICE;

void add_q(static_event_queue *q, virtio_input_event e){
    q->events[q->tail] = e;
    q->tail = (q->tail + 1) % EVENT_QUEUE_SIZE;
    if (q->full){
        printf("static_event_queue.add: full queue warning, discarding 1 element...\n");
        q->head = q->tail;
        return;
    }
    if (q->tail == q->head) q->full = 1;
    q->empty = 0;
}

virtio_input_event get_q(static_event_queue *q){
    virtio_input_event e;
    if (q->empty){
        printf("static_event_queue.get_event: cannot get, empty queue error");
        return e;
    }
    e = q->events[q->head];
    q->head = (q->head + 1) % EVENT_QUEUE_SIZE;
    if (q->head == q->tail) q->empty = 1;
    q->full = 0;
    return e;
}

void set_q(static_event_queue *q){
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->add = add_q;
    q->get_event = get_q;
}

void repopulate_event(uint16 idx){
    VirtQDescriptor desc;
    uint16 head;
    set_descriptor(
        &desc,
        (uint64)(&(KEYBOARD_DEVICE.event_buffer[idx])),
        sizeof(virtio_input_event),
        VIRTQ_DESC_F_WRITE,
        0
    );
    head = KEYBOARD_DEVICE.event_buffer_idx;
    KEYBOARD_DEVICE.event_queue->desc[KEYBOARD_DEVICE.event_buffer_idx] = desc;
    KEYBOARD_DEVICE.event_buffer_idx = (KEYBOARD_DEVICE.event_buffer_idx + 1) % QNUM;
    KEYBOARD_DEVICE.event_queue->av.ring[KEYBOARD_DEVICE.event_queue->av.index % QNUM] = head;
    KEYBOARD_DEVICE.event_queue->av.index = KEYBOARD_DEVICE.event_queue->av.index + 1;
}

void setup_input_device(uint64 addr){
    //reset the device
    write_to_reg(addr, STATUS, STATUS_RESET);
    //set ack status bit
    set_bit(addr, STATUS, STATUS_ACKNOWLEDGE); //ack
    //set driver status bit
    set_bit(addr, STATUS, STATUS_DRIVER);
    //read feat bits and write it on the device
    uint32 device_feat = read_from_reg(addr, HOST_FEATURES);
    device_feat &= ~(1 << VIRTIO_F_EVENT_IDX);
    write_to_reg(addr, GUEST_FEATURES, device_feat);
    //setup virtq - populate virtq - read conf - write confg
    uint32 num_pages = 1 + sizeof(VirtQ)/PAGE_SIZE;
    VirtQ *event_queue = (VirtQ*) alloc(num_pages);
    uint32 q_pfn = ((uint64)event_queue)/PAGE_SIZE; //physical page number
    write_to_reg(addr, QUEUE_SEL, 0); //select the queue number 0
    write_to_reg(addr, QUEUE_NUM, QNUM); //writing the ring size of the queue
    write_to_reg(addr, GUEST_PAGE_SIZE, PAGE_SIZE); //writing the page size so the device can calculate the physical address
    write_to_reg(addr, QUEUE_PFN,  q_pfn); //write the physical page number


    num_pages = 1 + sizeof(VirtQ)/PAGE_SIZE;
    VirtQ *status_queue = (VirtQ*) alloc(num_pages);
    q_pfn = ((uint64)status_queue)/PAGE_SIZE; //physical page number
    write_to_reg(addr, QUEUE_SEL, 1); //select the queue number 1
    write_to_reg(addr, QUEUE_NUM, QNUM); //writing the ring size of the queue
    write_to_reg(addr, GUEST_PAGE_SIZE, PAGE_SIZE); //writing the page size so the device can calculate the physical address
    write_to_reg(addr, QUEUE_PFN,  q_pfn); //write the physical page number


    //set DRIVER_OK status bit
    set_bit(addr, STATUS, STATUS_DRIVER_OK);
    //now, our device is alive


    num_pages = 1 + ((sizeof(virtio_input_event) * TOTAL_EVENT_BUFFER) / PAGE_SIZE);

    //filling keyboard struct
    KEYBOARD_DEVICE.address = addr;
    KEYBOARD_DEVICE.event_ack_idx = 0;
    KEYBOARD_DEVICE.event_queue = event_queue;
    KEYBOARD_DEVICE.status_ack_idx = 0;
    KEYBOARD_DEVICE.status_queue = status_queue;
    KEYBOARD_DEVICE.event_buffer = (virtio_input_event*) alloc(num_pages);
    KEYBOARD_DEVICE.event_buffer_idx = 0;


    //we have to always leave some free buffers to the device
    for (int i = 0; i < TOTAL_EVENT_BUFFER; i++){
        repopulate_event(i);
    }
    
    //the queue that will be filled with events and later be cosumed by applications
    set_q(&av_events_queue);
}

void keyboard_int(){
    int i = 0;
    VirtQ *queue;
    VirtQUsedElement element;
    VirtQDescriptor desc;
    virtio_input_event *event;
    //EVENT QUEUE
    queue = KEYBOARD_DEVICE.event_queue;
    while (KEYBOARD_DEVICE.event_ack_idx != queue->used.index){
        i++;
        element = queue->used.ring[KEYBOARD_DEVICE.event_ack_idx % QNUM];
        desc = queue->desc[element.id];
        event = (virtio_input_event*)(desc.address);

        repopulate_event(element.id);
        KEYBOARD_DEVICE.event_ack_idx = KEYBOARD_DEVICE.event_ack_idx + 1;

        switch (event->type){
            case EV_KEY:
                if (event->value == 1){
                    av_events_queue.add(&av_events_queue, *event);
                    printf("Tecla -> %d\n", event->code);
                }
                break;
            default:
                //printf("Tipo de evento nao capturado\n");
                break;
        }
    }



    //STATUS QUEUE
    queue = KEYBOARD_DEVICE.status_queue;
    while (KEYBOARD_DEVICE.status_ack_idx != queue->used.index){
        i++;
        element = queue->used.ring[KEYBOARD_DEVICE.event_ack_idx % QNUM];
        desc = queue->desc[element.id];
        event = (virtio_input_event*)(desc.address);
        KEYBOARD_DEVICE.status_ack_idx += 1;
    }

    if (!i) printf("NENHUM EVENTO CAPTURADO!!!\n");

}


static_event_queue* get_event_queue(){
    return &av_events_queue;
}