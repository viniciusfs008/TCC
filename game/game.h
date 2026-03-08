#ifndef GAME_H
#define GAME_H

#define FPS 30
#define FRAME_TIME 1/FPS

#define TETROMINO_SIZE 4 //4x4 blocks
#define BLOCK_SIZE 32 //32x32 pixels
#define BLOCKS_PER_TETROMINO 16 //4x4 blocks = 16 blocks
#define NUM_OF_TETROMINOS 7 //7 variants of tetrominoes
#define START_OFFSET 3

#define FIELD_WIDTH 10 //10 blocks width
#define FIELD_HEIGHT (20 + START_OFFSET) //20 blocks height + start offset

#define FIELD_START_X ((DEVICE_WIDTH/2) - (FIELD_WIDTH*(BLOCK_SIZE/2))) //where in width of the device the field starts
#define FIELD_START_Y (DEVICE_HEIGHT - ((FIELD_HEIGHT + 1)*BLOCK_SIZE)) //where in height of the device the field starts
#define PIECE_START ((FIELD_WIDTH/2) - (TETROMINO_SIZE/2)) //where in width of the field, the piece starts

#define NEXT_WIDTH 5
#define NEXT_HEIGHT 12

#define HOLD_WIDTH NEXT_WIDTH
#define HOLD_HEIGHT 4

//defines how many next pieces we have
#define NEXT_MAX 3


typedef enum pieces{ I, Z, S, O, T, L, J, SHADOW, UNAVAILABLE, EMPTY}piece_shape;

typedef struct piece {
    uint32 x, y, rotation;
    piece_shape shape;
} piece;




void draw_tetris_block(uint32 x, uint32 y, piece_shape index);
void draw_piece(piece p, uint8 shadow);
uint32 rotate(uint32 x, uint32 y, uint32 rotation, piece_shape shape);
void draw_field(uint8 *field);
void draw_score(uint16 score);
void draw_shadow(piece p);
void draw_next(piece *next, uint32 next_idx);
void draw_hold(piece hold, uint8 has_switched);
void init_game();
#endif