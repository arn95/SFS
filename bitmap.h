//
// Created by Arnold Balliu on 4/19/17.
//

#ifndef A4_CLION_BITMAP_H
#define A4_CLION_BITMAP_H

#define CHAR_BITS 8
#define MAX_CHARS 128
#define MAX_BITS MAX_CHARS*CHAR_BITS //I assumed a char contains 8 bits.
// I know were supposed to use some macro that has a different value depending on the architecture.


#define GET_CHAR_INDEX(i) ((i/CHAR_BITS))
#define GET_BIT_INDEX(i) ((i%CHAR_BITS))

struct bitmap_t {
    unsigned char array[MAX_CHARS];
};

void bitmap_set_bit(unsigned char* c, int bit_val, int pos);

int bitmap_check_bit(unsigned char* c, int pos);

int bitmap_set(struct bitmap_t* bitmap, int value, int index);

int bitmap_check(struct bitmap_t* bitmap, int index);

void bitmap_init(struct bitmap_t* bitmap);

void char_to_bits(unsigned char* c);

#endif //A4_CLION_BITMAP_H
