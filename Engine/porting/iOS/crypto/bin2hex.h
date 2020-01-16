//
// Created by lijun on 2020/1/14.
//

#ifndef _ANDROID_BIN2HEX_H
#define _ANDROID_BIN2HEX_H
char const hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b','c','d','e','f'};

const char* bin2hex(unsigned char* bytes, int size) {
  std::string str;
  for (int i = 0; i < size; ++i) {
    const char ch = bytes[i];
    str.append(&hex[(ch  & 0xF0) >> 4], 1);
    str.append(&hex[ch & 0xF], 1);
  }
  return str.c_str();
}

#endif //GAMEENGINE_ANDROID_BIN2HEX_H
