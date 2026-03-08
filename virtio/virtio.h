#ifndef VIRTIO_H
#define VIRTIO_H


#include "../types.h"
#include "../memlayout.h"



#define VIRTIO_MAGIC 0x74726976 //"triv" em little endian fica virt
#define VIRTIO_F_EVENT_IDX        29

#define DEVICE_WIDTH 900
#define DEVICE_HEIGHT 832

//status register bits
#define STATUS_RESET				(0)
#define STATUS_ACKNOWLEDGE 			(1 << 0)
#define STATUS_DRIVER				(1 << 1)
#define STATUS_DRIVER_OK 			(1 << 2)
#define STATUS_FEATURES_OK 			(1 << 3)
#define STATUS_DEVICE_NEEDS_RESET 	(1 << 6)
#define STATUS_FAILED				(1 << 7)


#define VIRTQ_DESC_F_NEXT 			1
#define VIRTQ_DESC_F_WRITE 			2
#define VIRTQ_DESC_F_INDIRECT 		4
#define VIRTQ_AVAIL_F_NO_INTERRUPT 	1
#define VIRTQ_USED_F_NO_NOTIFY 		1

#define QNUM 0x40

//Regitradores de controle virtio mapeados em memória
typedef enum MMIO_Regs {
	MAGIC_VALUE = 		0x000,// R	|| Magic value 
	VERSION = 			0X004,// R	|| Legacy device returns value 0x1.
	DEVICE_ID = 		0X008,// R	|| Virtio Subsystem Device ID 
	VENDOR_ID = 		0X00C,// R	|| Virtio Subsystem Vendor ID 
	HOST_FEATURES = 	0X010,// R 	|| Flags representing features the device supports
	HOST_FEATURES_SEL = 0X014,// W	|| Device (host) features word selection.
	GUEST_FEATURES = 	0X020,// W	|| Flags representing device features understood and activated by the driver 
	GUEST_FEATURES_SEL= 0X024,// W	|| Activated (guest) features word selection 
	GUEST_PAGE_SIZE = 	0X028,// W	|| The driver writes the guest page size in bytes to the register during initialization, before any queues are used. This value should be a power of 2 and is used by the device to calculate the Guest address of the first queue page (see QueuePFN).
	QUEUE_SEL  = 		0X030,// W	|| Writing to this register selects the virtual queue that the following operations on the QueueNumMax, QueueNum, QueueAlign and QueuePFN registers apply to. The index number of the first queue is zero (0x0). . 
	QUEUE_NUM_MAX = 	0X034,// R	|| Reading from the register returns the maximum size of the queue the device is ready to process or zero (0x0) if the queue is not available. This applies to the queue selected by writing to QueueSel and is allowed only when QueuePFN is set to zero (0x0), so when the queue is not actively used.
	QUEUE_NUM = 		0X038,// W	|| Queue size is the number of elements in the queue. Writing to this register notifies the device what size of the queue the driver will use. This applies to the queue selected by writing to QueueSel. 
	QUEUE_ALIGN = 		0X03C,// W	|| Writing to this register notifies the device about alignment boundary of the Used Ring in bytes. This value should be a power of 2 and applies to the queue selected by writing to QueueSel. 
	QUEUE_PFN = 		0X040,// RW	|| Writing to this register notifies the device about location of the virtual queue in the Guest’s physical address space. This value is the index number of a page starting with the queue Descriptor Table. Value zero (0x0) means physical address zero (0x00000000) and is illegal. When the driver stops using the queue it writes zero (0x0) to this register. Reading from this register returns the currently used page number of the queue, therefore a value other than zero (0x0) means that the queue is in use. Both read and write accesses apply to the queue selected by writing to QueueSel.
	QUEUE_NOTIFY = 		0X050,// W	|| Queue notifier 
	INTERRUPT_STATUS = 	0X060,// R	|| Interrupt status 
	INTERRUPT_ACK = 	0X064,// W	|| Interrupt acknowledge 
	STATUS = 			0X070,// RW	|| Reading from this register returns the current device status flags. Writing non-zero values to this register sets the status flags, indicating the OS/driver progress. Writing zero (0x0) to this register triggers a device reset. The device sets QueuePFN to zero (0x0) for all queues in the device.
	CONFIG = 			0X100 // RW	|| Configuration space
}MMIOReg;


typedef enum Virtio_Devices {
	NO_DEVICE =					0X00,
	NETWORKD_CARD_DEVICE_ID = 	0X01,
	BLOCK_DEVICE_ID = 			0X02,
	CONSOLE_DEVICE_ID = 		0X03,
	ENTROPY_DEVICE_ID = 		0X04,
	GPU_DEVICE_ID = 			0X10,
	INPUT_DEVICE_ID = 			0X12
}Device_id;


typedef struct virtq_descriptor{
	uint64 address;
	uint32 length;
	uint16 flags;
	uint16 next;
} VirtQDescriptor;

typedef struct virtq_available {
	uint16 flags;
	uint16 index;
	uint16 ring[QNUM];
	uint16 used_event;
} VirtQAvailable;

typedef struct virtq_used_element {
	uint32 id;
	uint32 length;
}VirtQUsedElement;


typedef struct virtq_used {
	uint16 flags;
	uint16 index;
	VirtQUsedElement ring[QNUM];
	uint16 available_event;
}VirtQUsed;


typedef struct virtq {
	VirtQDescriptor desc[QNUM];
	VirtQAvailable av;
	uint8 padding0[ //padding para que virtqused tenha um alinhamento de 4 bytes
		PAGE_SIZE*(((sizeof(VirtQDescriptor) * QNUM + sizeof(VirtQAvailable)) / PAGE_SIZE) +1)
		- sizeof(VirtQDescriptor) * QNUM
		- sizeof(VirtQAvailable)
	];
	VirtQUsed used;
} VirtQ;



uint32 read_from_reg(uint64 base, MMIOReg reg);
void write_to_reg(uint64 base, MMIOReg reg, uint32 value);
void set_bit(uint64 base, MMIOReg reg, uint32 mask);
void set_descriptor(VirtQDescriptor *desc, uint64 address, uint32 length, uint16 flags, uint16 next);
void handle_virt_int(int id);
void virtio_probe();

#endif