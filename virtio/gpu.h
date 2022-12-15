#ifndef GPU_H
#define GPU_H
//5.7.4 Device configuration layout
#include "virtio.h"

//COLOR PRE PROCESSORS

//Toda cor tem que passar por essas macros, pois o tipo de reconhecimento de cor pode mudar
//Reconhecimento atual é BGRA - 8 bits blue, 8 bits green, 8 bits red, 8 bits alpha
//Como a arquitetura é little endian, BGRA vira ARGB, ou seja
//posso definir uma cor verde como 0xff00ff00 -> max alpha e max verde

//uma macro para concatenar uma cor hex ao alpha
//posso definir uma cor vermelha como COLOR_HEX(0xff0000)
#define COLOR_HEX(hex) (0xff << 24 | hex)
//uma macro para definir cores por rgb, 0 <= rgb <= 255
//posso definir uma cor azul como COLOR_RGB(0,0,255)
#define COLOR_RGB(r,g,b) (0xff << 24 | r << 16 | g << 8 | b)

#define BLACK COLOR_HEX(0)
#define WHITE COLOR_HEX(~0U)


#define VIRTIO_GPU_EVENT_DISPLAY (1 << 0)
#define F_VIRGL 0
#define F_EDID 	1
#define FLAG_FENCE (1 << 0)
#define MAX_SCANOUTS 16
#define RESOURCE_ID 1
#define TRANSPARENT 0

typedef struct virtio_gpu_config { 
	uint32 events_read; 
	uint32 events_clear; 
	uint32 num_scanouts;
	uint32 reserved;
}gpu_config;


//5.7.6.7 Device Operation: Request header
typedef enum virtio_gpu_ctrl_type { 
	/* 2d commands */ 
	VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100, 
	VIRTIO_GPU_CMD_RESOURCE_CREATE_2D, 
	VIRTIO_GPU_CMD_RESOURCE_UNREF, 
	VIRTIO_GPU_CMD_SET_SCANOUT, 
	VIRTIO_GPU_CMD_RESOURCE_FLUSH, 
	VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D, 
	VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING, 
	VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING, 
	VIRTIO_GPU_CMD_GET_CAPSET_INFO, 
	VIRTIO_GPU_CMD_GET_CAPSET, 
	VIRTIO_GPU_CMD_GET_EDID, 

	/* cursor commands */ 
	VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300, 
	VIRTIO_GPU_CMD_MOVE_CURSOR, 

	/* success responses */ 
	VIRTIO_GPU_RESP_OK_NODATA = 0x1100, 
	VIRTIO_GPU_RESP_OK_DISPLAY_INFO, 
	VIRTIO_GPU_RESP_OK_CAPSET_INFO, 
	VIRTIO_GPU_RESP_OK_CAPSET, 
	VIRTIO_GPU_RESP_OK_EDID, 

	/* error responses */ 
	VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200, 
	VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY, 
	VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID, 
	VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID, 
	VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID, 
	VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER, 
}gpu_ctrl_type;
 
#define VIRTIO_GPU_FLAG_FENCE (1 << 0) 
 
typedef struct virtio_gpu_ctrl_hdr { 
	gpu_ctrl_type type; 
	uint32 flags; 
	uint64 fence_id; 
	uint32 ctx_id; 
	uint32 padding;
}gpu_ctrl_hdr;

//5.7.6.8 Device Operation: controlq

 
typedef struct virtio_gpu_rect { 
	uint32 x; 
	uint32 y; 
	uint32 width; 
	uint32 height; 
}gpu_rect; 


typedef struct virtio_gpu_display_one { 
	gpu_rect r; 
	uint32 enabled; 
	uint32 flags; 
}gpu_display_one;


typedef struct virtio_gpu_resp_display_info { 
	gpu_ctrl_hdr hdr; 
	gpu_display_one pmodes[MAX_SCANOUTS]; 
}gpu_resp_display_info;

typedef struct get_ed_id{
	gpu_ctrl_hdr hdr;
	uint32 scanout;
	uint32 paddind;
}get_ed_id;

typedef struct resp_ed_id {
	gpu_ctrl_hdr hdr;
	uint32 size;
	uint32 paddind;
	uint8 edid[1024];
}resp_ed_id;

typedef enum virtio_gpu_formats { 
	VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM  = 1, 
	VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM  = 2, 
	VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM  = 3, 
	VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM  = 4, 

	VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM  = 67, 
	VIRTIO_GPU_FORMAT_X8B8G8R8_UNORM  = 68, 

	VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM  = 121, 
	VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM  = 134, 
}virtio_gpu_format;


typedef struct resource_create_2d {
	gpu_ctrl_hdr hdr;
	uint32 resource_id;
	uint32 formats;
	uint32 width;
	uint32 height;
}resource_create_2d;


typedef struct resource_unref {
	gpu_ctrl_hdr hdr;
	uint32 resource_id;
	uint32 padding;
}resource_unref;


typedef struct set_scanout {
	gpu_ctrl_hdr hdr;
	gpu_rect r;
	uint32 scanout_id;
	uint32 resource_id;
}set_scanout;


typedef struct resource_flush {
	gpu_ctrl_hdr hdr;
	gpu_rect r;
	uint32 resource_id;
	uint32 padding;
}resource_flush;



typedef struct transfer_to_host_2d {
	gpu_ctrl_hdr hdr;
	gpu_rect r;
	uint64 offset;
	uint32 resource_id;
	uint32 padding;
}transfer_to_host_2d;


typedef struct attach_backing {
	gpu_ctrl_hdr hdr;
	uint32 resource_id;
	uint32 nr_entries;
}attach_backing;



typedef struct mem_entry {
	uint64 addr;
	uint32 length;
	uint32 padding;
}mem_entry;



typedef struct detach_backing {
	gpu_ctrl_hdr hdr;
	uint32 resource_id;
	uint32 padding;
}detach_backing;


typedef struct cursor_pos {
	uint32 scanout_id;
	uint32 x;
	uint32 y;
	uint32 padding;
}cursor_pos;



typedef struct update_cursor {
	gpu_ctrl_hdr hdr;
	cursor_pos pos;
	uint32 resource_id;
	uint32 hot_x;
	uint32 hot_y;
	uint32 padding;
}update_cursor;





typedef struct gpu_device{
	VirtQ *queue;
	uint64 dev;
	uint16 idx;
	uint16 ack_used_idx;
	uint32 *framebuffer;
	uint32 width;
	uint32 height;
}gpu_device;

/*
*	funcao que seta o dispositivo
*	@param ptr: ponteiro para o dispositivo na memoria
*	@return 0 ou o endereco do device para caso o dispositivo falhe ou não na inicialização
*/
gpu_device *setup_gpu_device(uint64 ptr);
void init_gpu_device(gpu_device *device);
void gpu_update();
void set_rect(gpu_rect *rect, uint32 x, uint32 y, uint32 width, uint32 height);

/*******************FUNCOES PARA SETAR PARAMETROS EM GPU.C ******************************/

/*
* Funcao para setar a cor de fundo
* @param color: Cor do fundo
*/
void bg_color(uint32 color);

/*
* Funcao para setar a cor de preenchimento
* @param color: Cor do preenchimento
*/
void fill_color(uint32 color);

/*
* Funcao para setar a cor da borda
* @param color: Cor da borda
*/
void stroke_color(uint32 color);

/*
* Funcao para setar a largura da borda
* @param weight: Largura da borda
*/
void stroke_weigth(uint32 weight);

/*
* Funcao que limpa a tela. Cores aceitas: BACKGROUND
*/
void clear();


/************************** FUNCOES PARA DESENHAR PRIMITIVAS *******************************/

/*
* Funcao que dado 2 pontos desenha uma linha entre eles. Cores aceitas: STROKE. Esta função aceita STROKE_WEIGHT
* @param x1: Coordenada x do primeiro ponto
* @param y1: Coordenada y do primeiro ponto
* @param x2: Coordenada x do segundo ponto
* @param y2: Coordenada y do segundo ponto
*/
void draw_line(int x1, int y1, int x2, int y2);


/*
* Funcao que dado um ponto, uma largura e uma altura desenha um retangulo na tela. Cores aceitas: FILL, STROKE. Esta função aceita STROKE_WEIGHT
*/
void draw_rect(int x, int y, int w, int h);


/*
* Funcao que dado um ponto de um raio desenha um Circulo na tela. Cores aceitas: FILL, STROKE. Esta funçao aceita STROKE_WEIGHT
* @param x: Coordenada x do centro da circunferencia
* @param y: Coordenada y do centro da circunferencia
* @param r: Raio da circunferencia
*/
void draw_circle(int x, int y, int r);


/*
* Funcao que dado 3 pontos, desenha um triangulo na tela. Cores aceitas: FILL, STROKE. Esta função aceita STROKE_WEIGHT
* @param x1: Coordenada x do primeiro ponto
* @param y1: Coordenada y do primeiro ponto
* @param x2: Coordenada x do segundo ponto
* @param y2: Coordenada y do segundo ponto
* @param x3: Coordenada x do terceiro ponto
* @param y3: Coordenada y do terceiro ponto
*/
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3);


/*
* Funcao que usando lookup tables consegue desenhar uma string na tela. Cores aceitas: FILL, BACKGROUND.
* @param *string: A string que deseja desenhar
* @param x: Coordenada x da string
* @param y: Coordenada y da string
* @param size: Tamanho da string, tamanho minimo = 1
* @param cursor_x: numero flutuante que define a porcentagem do offset em que o cursor desenha em x... 0 <= cursor_x <= 1
* @param cursor_y: numero flutuante que define a porcentagem do offset em que o cursor desenha em y... 0 <= cursor_y <= 1
*/
void draw_string(char *string, int x, int y, uint32 size, double cursor_x, double cursor_y);


/*
* Funcao que usando lookup tables consegue desenhar numeros mais "estilizados". Cores aceitas: FILL, BACKGROUND, STROKE. Esta função nao aceita STROKE_WEIGHT
* @param number: O numero que deseja escrever na tela
* @param x: Coordenada x do numero
* @param y: Coordenada y do numero
* @param cursor_x: numero flutuante que define a porcentagem do offset em que o cursor desenha em x... 0 <= cursor_x <= 1
* @param cursor_y: numero flutuante que define a porcentagem do offset em que o cursor desenha em y... 0 <= cursor_y <= 1
*/
void draw_styled_number(uint32 number, int x, int y, double cursor_x, double cursor_y);




#endif