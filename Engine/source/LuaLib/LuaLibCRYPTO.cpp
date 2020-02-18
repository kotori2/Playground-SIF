
#include "LuaLibCRYPTO.h"
#include "CKLBUtility.h"

static LuaLibCRYPTO libdef(0);

LuaLibCRYPTO::LuaLibCRYPTO(DEFCONST* arrConstDef) : ILuaFuncLib(arrConstDef) {}
LuaLibCRYPTO::~LuaLibCRYPTO() {}

void
LuaLibCRYPTO::addLibrary()
{
	addFunction("CRYPTO_decrypt_aes_128_cbc",		LuaLibCRYPTO::luaDecryptAES128CBC);

	// 27.01.2020 not used functions in lua:
	// CRYPTO_encrypt_aes_128_cbc
	// CRYPTO_public_encrypt
	// CRYPTO_random_bytes
	// CRYPTO_sha1
	// CRYPTO_hmac_sha1
	// CRYPTO_key
}

int
LuaLibCRYPTO::luaDecryptAES128CBC(lua_State* L)
{
	CLuaState lua(L);
	if (lua.numArgs() != 2 || !lua.isString(1) || !lua.isString(2)) {
		klb_assertAlways("invalid args");
		lua.retBool(false);
		return 0;
	}
	size_t ciphertextLen = 0;
	size_t keyLen = 0;
	const char* ciphertext = luaL_checklstring(L, 1, &ciphertextLen);
	const char* key = luaL_checklstring(L, 2, &keyLen);
	char* plaintext = KLBNEWA(char, ciphertextLen + 1);

	int plaintextLen = CPFInterface::getInstance().platform().decryptAES128CBC((u8*)ciphertext, ciphertextLen, key, plaintext, ciphertextLen + 1);
	if (plaintextLen == 0) {
		klb_assertAlways("platform.decryptAES128CBC failed");
		lua.retBool(false);
	}
	else {
		lua_pushlstring(L, plaintext, plaintextLen);
	}

	KLBDELETEA(plaintext);
	return 1;
}
