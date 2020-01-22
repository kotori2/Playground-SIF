//
//  AESCipher.m
//  AESCipher
//
//  Created by Welkin Xie on 8/13/16.
//  Copyright © 2016 WelkinXie. All rights reserved.
//
//  https://github.com/WelkinXie/AESCipher-iOS
//

#import "AESCipher.h"
#import <CommonCrypto/CommonCryptor.h>
#import <CommonCrypto/CommonRandom.h>

NSString *const kInitVector = @"16-Bytes--String";

size_t const kKeySize = kCCKeySizeAES128;

@implementation AESCipher

+ (NSData *)encryptAES:(NSString *)content key:(const char *)key {
    
    NSData *contentData = [content dataUsingEncoding:NSUTF8StringEncoding];
    NSUInteger dataLength = contentData.length;
    
    unsigned char iv[16] = {'\0'};
    CCRandomGenerateBytes(iv, 16);
    
    char keyPtr[16];
    size_t keySize = (strlen(keyPtr)<=16)?strlen(keyPtr):16;
    memset(keyPtr, '\0', sizeof(keyPtr));
    memcpy(keyPtr, key, sizeof(char) * keySize);
    
    size_t encryptSize = dataLength + kCCBlockSizeAES128;
    char *encryptedBytes = malloc(encryptSize);
    size_t actualOutSize = 0;
    
    
    CCCryptorStatus cryptStatus = CCCrypt(kCCEncrypt,
                                          kCCAlgorithmAES,
                                          kCCOptionPKCS7Padding,
                                          keyPtr,
                                          kKeySize,
                                          iv,
                                          contentData.bytes,
                                          dataLength,
                                          encryptedBytes,
                                          encryptSize,
                                          &actualOutSize);
    
    if (cryptStatus == kCCSuccess) {
        return [NSData dataWithBytesNoCopy:encryptedBytes length:actualOutSize];
    }
    free(encryptedBytes);
    return nil;
}

+ (NSString *)decryptAES:(NSString *)content key:(NSString *)key {

    NSData *contentData = [[NSData alloc] initWithBase64EncodedString:content options:NSDataBase64DecodingIgnoreUnknownCharacters];
    NSUInteger dataLength = contentData.length;
    
    char keyPtr[16];
    memset(keyPtr, '\0', sizeof(keyPtr));
    [key getCString:keyPtr maxLength:sizeof(keyPtr) encoding:NSUTF8StringEncoding];
    
    size_t decryptSize = dataLength + kCCBlockSizeAES128;
    char *decryptedBytes = malloc(decryptSize);
    size_t actualOutSize = 0;
    
    NSData *initVector = [kInitVector dataUsingEncoding:NSUTF8StringEncoding];
    
    CCCryptorStatus cryptStatus = CCCrypt(kCCDecrypt,
                                          kCCAlgorithmAES,
                                          kCCOptionPKCS7Padding,
                                          keyPtr,
                                          kKeySize,
                                          initVector.bytes,
                                          contentData.bytes,
                                          dataLength,
                                          decryptedBytes,
                                          decryptSize,
                                          &actualOutSize);
    
    if (cryptStatus == kCCSuccess) {
        return [[NSString alloc] initWithData:[NSData dataWithBytesNoCopy:decryptedBytes length:actualOutSize] encoding:NSUTF8StringEncoding];
    }
    free(decryptedBytes);
    return nil;
}

@end
