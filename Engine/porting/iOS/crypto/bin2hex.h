//
// Created by lijun on 2020/1/14.
//

#ifndef _ANDROID_BIN2HEX_H
#define _ANDROID_BIN2HEX_H
char const hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b','c','d','e','f'};

const char* bin2hex(unsigned char* bytes, int size) {
  char *ret = new char[2 * size + 1];
  for (int i = 0; i < size; ++i) {
    const char ch = bytes[i];
    ret[2 * i] = hex[(ch  & 0xF0) >> 4];
    ret[2 * i + 1] = hex[ch & 0xF];
  }
  ret[2 * size] = 0;
  return ret;
}

#endif //GAMEENGINE_ANDROID_BIN2HEX_H
