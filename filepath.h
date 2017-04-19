//
// Created by Arnold Balliu on 4/18/17.
//

#ifndef A4_CLION_FILEPATH_H
#define A4_CLION_FILEPATH_H

#define FILE_NUM 256
struct filepath_s {
    char* names[FILE_NUM]; // 4096 (pathname length max in unix) / 16 (our set filename length)
    int len;
    char* curr_name;
    int curr_index;
};

void filepath_init(struct filepath_s*, char* path);
void filepath_deinit(struct filepath_s* filepath);

void filepath_traverse(struct filepath_s* filepath);

int filepath_has_next(struct filepath_s* filepath);

char* filepath_next(struct filepath_s* filepath);


#endif //A4_CLION_FILEPATH_H
