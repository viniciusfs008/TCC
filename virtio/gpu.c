#include "gpu.h"
#include "../defs.h"
#include "../libs/numbers.h"
#include "../libs/letters.h"
#include "../libs/math.h"
#include "../libs/algorithms.h"


uint32 BG_COLOR = 0xff000000;
uint32 FILL_COLOR = 0;
uint32 STROKE_COLOR = 0;
uint32 STROKE_WEIGHT = 1;

gpu_device *GPU_DEVICE;


void place_pixel(int x, int y, uint32 color){
	if (x < 0 || x >= DEVICE_WIDTH) return;
	if (y < 0 || y >= DEVICE_HEIGHT) return;
	if (color == TRANSPARENT) return;
	GPU_DEVICE->framebuffer[DEVICE_WIDTH*y + x] = color;
}


void bg_color(uint32 color){
	BG_COLOR = color;
}
void fill_color(uint32 color){
	FILL_COLOR = color;
}
void stroke_color(uint32 color){
	STROKE_COLOR = color;
}
void stroke_weigth(uint32 weight){
	STROKE_WEIGHT = weight;
}



void draw_line(int x1, int y1, int x2, int y2){
	//pegando a distancia entre os 2 pontos
	double distance = distance_points(x1, y1, x2, y2);
	//quanto vai andar no eixo x em cada loop
	double walk_x = (int)(x2-x1)/distance;
	//quanto vai andar no eixo y em cada loop
	double walk_y = (int)(y2-y1)/distance;
	//distancia arrendodado para saber quantas iteracoes
	int distance_trimmed = (int) distance;

	//desenhando o primeiro quadrado centralizado
	int px, py; //novas posicoes onde a linha comeca
	px = x1 - (STROKE_WEIGHT / 2); //coord x canto esquerdo superior
	py = y1 - (STROKE_WEIGHT / 2); //coord y canto esquerdo superior
	for (uint32 row = 0; row < STROKE_WEIGHT; row++){
		for (uint32 col = 0; col < STROKE_WEIGHT; col++){
			place_pixel(px + col, py + row, STROKE_COLOR);
		}
	}
	//como a linha é desenhada em um direcao, mudo px e py de acordo
	// se for <- px tem que ficar a direta
	// se for ↓ py tem que ficar acima
	// e vice-versa
	int dir_x = 1, dir_y = 1;
	if (walk_x < 0) {
		px += (STROKE_WEIGHT - 1);
		dir_x = -1;
	}
	if (walk_y < 0){
		py += (STROKE_WEIGHT - 1);
		dir_y = -1;
	}

	//variaveis para marcar se andou no x ou no y
	int walked_x, walked_y;
	//variaveis para marcar posicao atual
	int cx = px, cy = py;

	for (int i = 1; i <= distance_trimmed; i++){
		walked_x = ( (int) ((px + walk_x*i) - cx) );
		walked_y = ( (int) ((py + walk_y*i) - cy) );
		cx += walked_x; //se andou em x, somo +1 ou -1 no x
		cy += walked_y; //se andou em y, somo +1 ou -1 no y
		if (walked_x){
			for (uint32 p = 0; p < STROKE_WEIGHT; p++){
				place_pixel(
					cx + (STROKE_WEIGHT -1)*dir_x,
					cy + p*dir_y,
					STROKE_COLOR
				);
			}
		}
		if (walked_y){
			for (uint32 p = 0; p < STROKE_WEIGHT; p++){
				place_pixel(
					cx + p*dir_x,
					cy + (STROKE_WEIGHT -1)*dir_y,
					STROKE_COLOR
				);
			}
		}
	}
}


void fill_triangle_flat_base(
	int x1, int y1,
	int x2, int y2,
	int x3, int y3)
{
	int range_y, range_x;
	double left_slope, right_slope;
	double x;
	//y2 tem que ser igual a
	//y3 ou y1
	//printf("%d %d\t\t%d %d\t\t%d %d\n",x1,y1,x2,y2,x3,y3);
	if (y1 < y2){
		//flat bottom
		range_y = y2 - y1;
		if (x2 > x3){
			left_slope = (double)(x3-x1)/(y2-y1);
			right_slope = (double)(x2-x1)/(y2-y1);
		}else{
			left_slope = (double)(x2-x1)/(y2-y1);
			right_slope = (double)(x3-x1)/(y2-y1);
		}
		for (int scan = 0; scan <= range_y; scan++){
			x = scan * left_slope;
			range_x = (scan * right_slope) - x;
			for (int px = 0; px < range_x; px++){
				place_pixel((int)x1+ x + px, y1 + scan, FILL_COLOR);
			}
		}
	}else if (y3 > y2){
		//flat top
		range_y = y2 - y3;
		if (x2 > x1){
			left_slope = (double)(x1-x3)/(y3-y2);
			right_slope = (double)(x2-x3)/(y3-y2);
		}else{
			left_slope = (double)(x2-x3)/(y3-y2);
			right_slope = (double)(x1-x3)/(y3-y2);
		}
		for (int scan = range_y; scan <= 0; scan++){
			x = scan * left_slope * -1;
			range_x = (scan * right_slope * -1) - x;
			for (int px = 0; px < range_x; px++){
				place_pixel((int)x3+ x + px, y3 + scan, FILL_COLOR);
			}
		}

	}

}


void draw_triangle(
	int x1, int y1,
	int x2, int y2,
	int x3, int y3)
{
	//ordenar de acordo com y
	//para que y1 <= y2 <= y3
	if (y1 > y2){ swap(&x1, &x2); swap(&y1, &y2); }
	if (y2 > y3){ swap(&x3, &x2); swap(&y3, &y2); }
	if (y1 > y2){ swap(&x1, &x2); swap(&y1, &y2); }
	if (y2 == y3 || y1 == y2){
		//base reta
		fill_triangle_flat_base(
			x1, y1,
			x2, y2,
			x3, y3
		);
	}else{
		int y4 = y2;
		int x4 = x1 + ((y2-y1) * ((double)(x3-x1)/(y3-y1)));
		fill_triangle_flat_base(
			x1, y1,
			x2, y2,
			x4, y4
		);
		fill_triangle_flat_base(
			x4, y4,
			x2, y2,
			x3, y3
		);
	}
	//se tem y igual
	//entao tem base reta
	//se nao, separa em 2 trianglu
	//-----------------------------
	//
	draw_line(x1, y1, x2, y2);
	draw_line(x2, y2, x3, y3);
	draw_line(x3, y3, x1, y1);
}


void draw_circle(int x, int y, int r){
	//preenche o circulo
	int rt2 = r*r;
	int rowt2, colt2;
	int init;
	if (STROKE_COLOR != TRANSPARENT){
		init = -r - (STROKE_WEIGHT/2);
	}else{
		init = -r;
	}
	int half_sw_ceil = ceil((double)STROKE_WEIGHT/2);
	int bord_init = pow(init ,2);
	int bord_end = pow(r- half_sw_ceil, 2);
	//como a circunferencia eh simetrica, so preciso fazer
	//o loop em 1/4 da circunferencia
	//entao esses 2 loops passam pelo quadrante esquerdo superior
	uint32 color;
	for (int row = init+1; row <= 0; row++){
		rowt2 = row * row;
		for (int col = init+1; col <=0 ; col++){
			colt2 = col * col;
			if (rowt2 + colt2 <= bord_init){
				if (rowt2 + colt2 > bord_end){
					if ( STROKE_COLOR == TRANSPARENT && rowt2 + colt2 <= rt2){
						color = FILL_COLOR;
					}else{
						color = STROKE_COLOR;
					}
				}else{
					color = FILL_COLOR;
				}
				if (color != TRANSPARENT){
					place_pixel(col+x, row+y, color);
					place_pixel(-col+x, row+y, color);
					place_pixel(col+x, -row+y, color);
					place_pixel(-col+x, -row+y, color);
				}
			}
		}
	}
	//desenhar os pixels quem faltam na borda para ficar mais bonito
	if (STROKE_COLOR != TRANSPARENT && STROKE_WEIGHT > 0){
		place_pixel(x, (y-r)+half_sw_ceil, STROKE_COLOR);
		place_pixel(x, (y+r)-half_sw_ceil, STROKE_COLOR);
		place_pixel(x-r+half_sw_ceil, y, STROKE_COLOR);
		place_pixel(x+r-half_sw_ceil, y, STROKE_COLOR);
	}
}

void draw_rect(int x, int y, int w, int h){
	if (FILL_COLOR != TRANSPARENT){
		for (int row = y; row < y + h; row++){
			for (int col = x; col < x + w; col++){
				place_pixel(col, row, FILL_COLOR);
			}
		}
	}
	if (STROKE_COLOR != TRANSPARENT){
		draw_line(x,y,x+w,y);
		draw_line(x+w,y, x+w, y+h);
		draw_line(x+w,y+h, x, y+h);
		draw_line(x,y+h,x,y);
	}
}



void draw_char(char c, int x, int y, uint32 size){
	char mapped_letter;
	uint32 new_height, new_width, color;
	new_height = LETTER_HEIGHT * size;
	new_width = LETTER_WIDTH * size;
	mapped_letter = map_ascii[c];
	uint8 marked_as_fg;
	for (uint32 line = 0; line < new_height; line++){
		for (uint32 col = 0; col < new_width; col++){
			marked_as_fg = characters[mapped_letter][(line/size)*LETTER_WIDTH + (col/size)];
			if (marked_as_fg){
				color = FILL_COLOR;
			}else{
				color = BG_COLOR;
			}
			place_pixel(col + x,line + y, color);
		}
	}
}


void draw_string(char *string, int x, int y, uint32 size, double cursor_x, double cursor_y){
	int new_x, new_y;
	double espacamento_linha = 1.09; //9%
	if (cursor_x < 0) cursor_x = 0;
	if (cursor_x > 1) cursor_x = 1;
	if (cursor_y < 0) cursor_y = 0;
	if (cursor_y > 1) cursor_y = 1;
	if (size == 0) size = 1;
	uint32 string_size;
	uint32 line = 0;
	uint32 char_iterator = 0;
	uint32 c;
	uint32 qtd_linhas = str_number_of_lines(string);
	while(true){
		string_size = 0;
		while(string[char_iterator] != '\n' && string[char_iterator] != '\0'){
			string_size++;
			char_iterator++;
		}
		new_x = x - ((string_size*LETTER_WIDTH*size) * cursor_x);
		new_y = y - ((qtd_linhas*LETTER_HEIGHT*size) * cursor_y);
		for (int i = string_size; i > 0; i--){
			draw_char(string[char_iterator-i], new_x + (string_size - i)*LETTER_WIDTH*size, new_y, size);
		}
		if (string[char_iterator] == '\0') break;
		line++;
		char_iterator++;
		y += LETTER_HEIGHT*size*espacamento_linha;
	}
}


void draw_styled_number(uint32 number, int x, int y, double cursor_x, double cursor_y){
	uint8 color_number;
	uint32 color;
	uint8 num_array[64];
	uint16 number_len = 0;
	while (number > 0){
		num_array[number_len++] = number % 10;
		number /= 10;
	}
	if (number_len == 0){
		number_len++;
		num_array[0] = 0;
	}

	x -= (number_len*NUMBER_WIDTH) * cursor_x;
	y -= NUMBER_HEIGHT * cursor_y;

	while (number_len){
		number = num_array[--number_len];
		for (uint32 line = 0; line < NUMBER_HEIGHT; line++){
			for (uint32 col = 0; col < NUMBER_WIDTH; col++){
				color_number = numbers[number][line*NUMBER_WIDTH + col];
				switch (color_number) {
					case 0:
						color = BG_COLOR;
						break;
					case 1:
						color = STROKE_COLOR;
						break;
					case 2:
						color = FILL_COLOR;
						break;
				}
				place_pixel(col + x, line + y, color);
			}
		}
		x += NUMBER_WIDTH;
	}
}




void clear(){
	uint32 size = DEVICE_HEIGHT*DEVICE_WIDTH;
	for (uint32 pos = 0; pos < size; pos++){
		GPU_DEVICE->framebuffer[pos] = BG_COLOR; 
	}
}


void print_hdr(gpu_ctrl_hdr *hdr){
	printf("HEADER: \n");
	printf("\ttype: %d\n", hdr->type);
	printf("\tflags: %d\n", hdr->flags);
	printf("\tfence: %d\n", hdr->fence_id);
	printf("\tctx_id: %d\n", hdr->ctx_id);
	printf("\tpadding: %d\n", hdr->padding);
}

void print_rect(gpu_rect rect){
	printf("RECTANGLE:\n");
	printf("\tx: %d\n", rect.x);
	printf("\ty: %d\n", rect.y);
	printf("\th: %d\n", rect.height);
	printf("\tw: %d\n", rect.width);
}

void print_gpu_display_one(gpu_display_one display){
	printf("DISPLAY ONE:\n");
	printf("\tenabled: %d\n", display.enabled);
	printf("\tflags: %d\n",display.flags);
	print_rect(display.r);
}

void print_display_info(gpu_resp_display_info *info){
	printf("DISPLAY INFO: \n");
	print_hdr(&(info->hdr));
	for (int i = 0; i < MAX_SCANOUTS; i++){
		print_gpu_display_one(info->pmodes[i]);
	}
}

void print_gpu_cfg(gpu_config *cfg){
	printf("GPU CONFIGS: \n");
	printf(
		"\tevents_read: %d\n"
		"\tevents_clear: %d\n"
		"\tnum_scanouts: %d\n"
		"\treserved: %d\n"
		,
		cfg->events_read,
		cfg->events_clear,
		cfg->num_scanouts,
		cfg->reserved
	);
}


void set_resource_flush(resource_flush *flush, gpu_ctrl_hdr hdr, gpu_rect rect, uint32 res_id, uint32 padding){
	flush->hdr = hdr;
	flush->r = rect;
	flush->resource_id = res_id;
	flush->padding = padding;
}

void set_transfer_to_host(transfer_to_host_2d *t, gpu_ctrl_hdr hdr, gpu_rect rect, uint64 offset, uint32 res_id, uint32 padding){
	t->hdr = hdr;
	t->r = rect;
	t->offset = offset;
	t->resource_id = res_id;
	t->padding = padding;
}

void set_setscanout(set_scanout *scanout, gpu_ctrl_hdr hdr, gpu_rect rect, uint32 scanout_id, uint32 res_id){
	scanout->hdr = hdr;
	scanout->r = rect;
	scanout->scanout_id = scanout_id;
	scanout->resource_id = res_id;
}

void set_mem_entry(mem_entry *mem, uint64 addr, uint32 length, uint32 padding){
	mem->addr = addr;
	mem->length = length;
	mem->padding = padding;
}

void set_header(gpu_ctrl_hdr *hdr, gpu_ctrl_type type, uint32 flags, uint64 fence_id, uint32 ctx_id, uint32 pad){
	hdr->type = type;
	hdr->flags = flags;
	hdr->fence_id = fence_id;
	hdr->ctx_id = ctx_id;
	hdr->padding = pad;
}

void set_res_create_2d(resource_create_2d *res, gpu_ctrl_hdr hdr, uint32 res_id, uint32 format, uint32 width, uint32 height){
	res->hdr = hdr;
	res->resource_id = res_id;
	res->formats = format;
	res->width = width;
	res->height = height;
}

void set_attach_backing(attach_backing *at, gpu_ctrl_hdr hdr, uint32 res_id, uint32 n_entries){
	at->hdr = hdr;
	at->resource_id = res_id;
	at->nr_entries = n_entries;
}


void set_rect(gpu_rect *rect, uint32 x, uint32 y, uint32 w, uint32 h){
	rect->x = x;
	rect->y = y;
	rect->height = h;
	rect->width = w;
}




/*
*	funcao que inicializa o dispositivo
*	@param dev: ponteiro para o dispositivo
*/

void init_gpu_device(gpu_device *device){
	uint16 head;
	uint32 color = 0xffeeeeee;
	gpu_rect rect;
	gpu_ctrl_hdr hdr;

	clear();

	set_header(&hdr, VIRTIO_GPU_CMD_GET_DISPLAY_INFO, 0,0,0,0);
	gpu_resp_display_info *resposta_display = (gpu_resp_display_info*) alloc(1);
	VirtQDescriptor info, info_resp;
	set_descriptor(
		&info, (uint64) &hdr, sizeof(gpu_ctrl_hdr), VIRTQ_DESC_F_NEXT, device->idx + 1
	);
	set_descriptor(
		&info_resp,
		(uint64) resposta_display,
		sizeof(gpu_resp_display_info),
		VIRTQ_DESC_F_WRITE,
		0
	);

	head = device->idx;
	device->queue->desc[device->idx] = info;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->desc[device->idx] = info_resp;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->av.ring[device->queue->av.index % QNUM] = head;
	device->queue->av.index = (device->queue->av.index + 1); //wrapping_add(1) ???

	write_to_reg(device->dev, QUEUE_NOTIFY, 0);
	while(device->queue->av.index != device->queue->used.index);

	// print_display_info(resposta_display);

	set_header(&hdr, VIRTIO_GPU_CMD_RESOURCE_CREATE_2D, 0, 0, 0, 0);
	resource_create_2d *res = (resource_create_2d*) alloc(1);
	set_res_create_2d(
		res,
		hdr,
		RESOURCE_ID,
		VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM,
		device->width,
		device->height
	);
	VirtQDescriptor c2d, c2d_response;
	set_descriptor(
		&c2d,
		(uint64)res,
		sizeof(resource_create_2d),
		VIRTQ_DESC_F_NEXT,
		(device->idx + 1) % QNUM
	);
	gpu_ctrl_hdr *response1 = (gpu_ctrl_hdr*) alloc(1);
	set_descriptor(
		&c2d_response,
		(uint64)response1,
		sizeof(gpu_ctrl_hdr),
		VIRTQ_DESC_F_WRITE,
		0
	);


	head = device->idx;
	device->queue->desc[device->idx] = c2d;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->desc[device->idx] = c2d_response;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->av.ring[device->queue->av.index % QNUM] = head;
	device->queue->av.index = (device->queue->av.index + 1); //wrapping_add(1) ???



	//--------------------------------------------------------------//
	//passo 2
	//attach backing

	attach_backing *attach  = (attach_backing*) alloc(1);
	mem_entry *memory_entry = (mem_entry*) alloc(1);
	set_header(
		&hdr,
		VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
		0,
		0,
		0,
		0
	);
	set_attach_backing(attach, hdr, RESOURCE_ID, 1);
	set_mem_entry(
		memory_entry,
		(uint64)device->framebuffer,
		device->height * device->width * sizeof(uint32),
		0
	);
	VirtQDescriptor desc_ab, desc_ab_mem, desc_ab_resp;
	set_descriptor(
		&desc_ab,
		(uint64)attach,
		sizeof(attach_backing),
		VIRTQ_DESC_F_NEXT,
		(device->idx + 1) % QNUM
	);
	set_descriptor(
		&desc_ab_mem,
		(uint64)memory_entry,
		sizeof(mem_entry),
		VIRTQ_DESC_F_NEXT,
		(device->idx + 2) % QNUM
	);
	gpu_ctrl_hdr *response2 = (gpu_ctrl_hdr*)alloc(1);
	set_descriptor(
		&desc_ab_resp,
		(uint64)response2,
		sizeof(gpu_ctrl_hdr),
		VIRTQ_DESC_F_WRITE,
		0
	);
	head = device->idx;
	device->queue->desc[device->idx] = desc_ab;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->desc[device->idx] = desc_ab_mem;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->desc[device->idx] = desc_ab_resp;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->av.ring[device->queue->av.index % QNUM] = head;
	device->queue->av.index = (device->queue->av.index + 1) % QNUM; //wrapping_add(1) ???

	//*****************************************************************************************//
	//step 3
	set_scanout *scanout = (set_scanout*) alloc(1);
	gpu_ctrl_hdr *response3 = (gpu_ctrl_hdr*) alloc(1);
	set_header(
		&hdr,
		VIRTIO_GPU_CMD_SET_SCANOUT,
		0,
		0,
		0,
		0
	);
	set_rect(&rect, 0, 0, device->width, device->height);
	set_setscanout(scanout, hdr, rect, 0, RESOURCE_ID);
	VirtQDescriptor desc_sso, desc_sso_resp;
	set_descriptor(
		&desc_sso,
		(uint64)scanout,
		sizeof(set_scanout),
		VIRTQ_DESC_F_NEXT,
		(device->idx + 1) % QNUM
	);
	set_descriptor(
		&desc_sso_resp,
		(uint64)response3,
		sizeof(gpu_ctrl_hdr),
		VIRTQ_DESC_F_WRITE,
		0
	);
	head = device->idx;
	device->queue->desc[device->idx] = desc_sso;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->desc[device->idx] = desc_sso_resp;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->av.ring[device->queue->av.index % QNUM] = head;
	device->queue->av.index = (device->queue->av.index + 1) % QNUM; //wrapping_add(1) ???

	//RUN THE QUEUE
	write_to_reg(device->dev, QUEUE_NOTIFY, 0);
	while(device->queue->av.index != device->queue->used.index);

	set_header(
		&hdr,
		VIRTIO_GPU_CMD_GET_EDID,
		0,0,0,0
	);

	//getting the edid and printing on the screen
	get_ed_id *edid = (get_ed_id*) alloc(1);
	edid->hdr = hdr;
	edid->paddind = 0;
	edid->scanout = 0;

	resp_ed_id *resp_edid = (resp_ed_id*) alloc(1);
	
	VirtQDescriptor desc_edid, desc_resp_edid;
	set_descriptor(
		&desc_edid,
		(uint64) edid,
		sizeof(get_ed_id),
		VIRTQ_DESC_F_NEXT,
		(device->idx + 1) % QNUM
	);
	set_descriptor(
		&desc_resp_edid,
		(uint64) resp_edid,
		sizeof(resp_ed_id),
		VIRTQ_DESC_F_WRITE,
		0
	);
	head = device->idx;
	device->queue->desc[device->idx] = desc_edid;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->desc[device->idx] = desc_resp_edid;
	device->idx = (device->idx + 1) % QNUM;
	device->queue->av.ring[device->queue->av.index % QNUM] = head;
	device->queue->av.index = (device->queue->av.index + 1) % QNUM; //wrapping_add(1) ???
	write_to_reg(device->dev, QUEUE_NOTIFY, 0);
	while(device->queue->av.index != device->queue->used.index);

	// for (int i = 0; i < 1024; i++){
	// 	if (i % 8 == 0){
	// 		printf("\n%x -- %x\t", i, i + 7);
	// 	}
	// 	printf("%x ", resp_edid->edid[i]);
	// }
	// printf("\n\n");


}



/*
*	funcao que seta o dispositivo
*	@param ptr: ponteiro para o dispositivo na memoria
*	@return 0 ou o endereco do device para caso o dispositivo falhe ou não na inicialização
*/
gpu_device* setup_gpu_device(uint64 ptr){
	//uint32 idx = (ptr - VIRTIO_START) >> 12;

	//reset device -- writing 0 to status register
	write_to_reg(ptr, STATUS, 0);
	
	set_bit(ptr, STATUS, STATUS_ACKNOWLEDGE);//set ack bit
	
	set_bit(ptr, STATUS, STATUS_DRIVER);//set driver status bit

	write_to_reg(ptr, HOST_FEATURES_SEL, 0); //selecionando palavra 0 do hostfeat
	uint32 host_feat = read_from_reg(ptr, HOST_FEATURES); //lendo as hostfeat
	write_to_reg(ptr, GUEST_FEATURES_SEL, 0); //selecionando palavra 0 da guest feat
	write_to_reg(ptr, GUEST_FEATURES, host_feat & ~(1 << VIRTIO_F_EVENT_IDX)); //escrevendo as host feat na guest feat, com a flag de nao notificaçao

	set_bit(ptr, STATUS, STATUS_FEATURES_OK); //set feat ok
	uint32 feat_ok = read_from_reg(ptr, STATUS) & ~(STATUS_FEATURES_OK);
	if (!feat_ok){
		printf("Features negociation failed\n");
		set_bit(ptr, STATUS, STATUS_FAILED);
		return 0;
	}

	//setup device specific
	write_to_reg(ptr, QUEUE_NUM, QNUM);
	uint32 num_pages = 1+ ((sizeof(VirtQ))/ PAGE_SIZE);

	//SELECT THE QUEUE
	write_to_reg(ptr, QUEUE_SEL, 0X0);
	VirtQ *queue = alloc(num_pages);
	queue->av.flags = 1;
	queue->av.used_event = 1;
	uint32 q_pfn = ((uint64)queue)/PAGE_SIZE;

	//INFORM PAGE_SIZE
	write_to_reg(ptr, GUEST_PAGE_SIZE, PAGE_SIZE);
	//INFORM THE PAGE WHERE THE FIRST QUEUE IS
	write_to_reg(ptr, QUEUE_PFN, q_pfn);
	//INFORM THAT EVERYTHING IS OK
	set_bit(ptr, STATUS, STATUS_DRIVER_OK);

	num_pages = 1 + ((DEVICE_WIDTH*DEVICE_HEIGHT*sizeof(uint32))/PAGE_SIZE);
	uint32 *fb = alloc(num_pages*2);
	gpu_device *device = (gpu_device*)alloc(1);
	device->queue = queue;
	device->dev = ptr;
	device->idx = 0;
	device->ack_used_idx = 0;
	device->framebuffer = fb ;//(pixel*)((uint64)fb + num_pages*PAGE_SIZE);
	device->width = DEVICE_WIDTH;
	device->height = DEVICE_HEIGHT;
	


	GPU_DEVICE = device;
	return device;
}

void gpu_update(){
	//*******************************************************************************************//
	//step 4
	//transfer to host
	uint16 head;
	gpu_ctrl_hdr hdr;
	gpu_rect rect;
	set_rect(&rect, 0, 0, DEVICE_WIDTH, DEVICE_HEIGHT);
	transfer_to_host_2d *transfer = (transfer_to_host_2d*)alloc(1);
	set_header(
		&hdr,
		VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
		0,
		0,
		0,
		0
	);
	set_transfer_to_host(transfer, hdr, rect, 0, 1, 0);
	VirtQDescriptor desc_t2h, desc_t2h_response;
	gpu_ctrl_hdr *response4 = (gpu_ctrl_hdr*)alloc(1);
	set_descriptor(
		&desc_t2h,
		(uint64)transfer,
		sizeof(transfer_to_host_2d),
		VIRTQ_DESC_F_NEXT,
		(GPU_DEVICE->idx + 1)%QNUM
	);
	set_descriptor(
		&desc_t2h_response,
		(uint64)response4,
		sizeof(gpu_ctrl_hdr),
		VIRTQ_DESC_F_WRITE,
		0
	);
	head = GPU_DEVICE->idx;
	GPU_DEVICE->queue->desc[GPU_DEVICE->idx] = desc_t2h;
	GPU_DEVICE->idx = (GPU_DEVICE->idx + 1) % QNUM;
	GPU_DEVICE->queue->desc[GPU_DEVICE->idx] = desc_t2h_response;
	GPU_DEVICE->idx = (GPU_DEVICE->idx + 1) % QNUM;
	GPU_DEVICE->queue->av.ring[GPU_DEVICE->queue->av.index % QNUM] = head;
	GPU_DEVICE->queue->av.index = (GPU_DEVICE->queue->av.index + 1) ; 

	//****************************************************************************************//
	//STEP 5 
	//FLUSH
	resource_flush *flush = (resource_flush*) alloc(1);
	set_header(
		&hdr,
		VIRTIO_GPU_CMD_RESOURCE_FLUSH,
		0,
		0,
		0,
		0
	);
	set_resource_flush(flush, hdr, rect, 1, 0);

	VirtQDescriptor desc_rf, desc_rf_resp;
	gpu_ctrl_hdr *response5 = (gpu_ctrl_hdr*)alloc(1);
	set_descriptor(
		&desc_rf,
		(uint64)flush,
		sizeof(resource_flush),
		VIRTQ_DESC_F_NEXT,
		(GPU_DEVICE->idx + 1) % QNUM
	);
	set_descriptor(
		&desc_rf_resp,
		(uint64)response5,
		sizeof(gpu_ctrl_hdr),
		VIRTQ_DESC_F_WRITE,
		0
	);
	head = GPU_DEVICE->idx;
	GPU_DEVICE->queue->desc[GPU_DEVICE->idx] = desc_rf;
	GPU_DEVICE->idx = (GPU_DEVICE->idx + 1) % QNUM;
	GPU_DEVICE->queue->desc[GPU_DEVICE->idx] = desc_rf_resp;
	GPU_DEVICE->idx = (GPU_DEVICE->idx + 1) % QNUM;
	GPU_DEVICE->queue->av.ring[GPU_DEVICE->queue->av.index % QNUM] = head;
	GPU_DEVICE->queue->av.index = (GPU_DEVICE->queue->av.index + 1);

	//RUN THE QUEUE
	write_to_reg(GPU_DEVICE->dev, QUEUE_NOTIFY, 0);
	while (GPU_DEVICE->queue->av.index != GPU_DEVICE->queue->used.index);
	dealloc(transfer);
	dealloc(response4);
	dealloc(response5);
	dealloc(flush);
}