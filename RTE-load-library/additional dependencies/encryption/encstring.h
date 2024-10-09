#pragma once
#include <string>
#include <stdio.h>
#include <string.h>
#include <array>

#ifndef ENCSTRING_H
#define ENCSTRING_H

constexpr wchar_t __wkeys[] = {
	0x71B9, 0xB256, 0xCF22, 0xDB52, 0xA9A8, 0x5E31, 0x3A49, 0x0465,
	0x5399, 0x16A2, 0x6134, 0xF989, 0x976A, 0xB6E9, 0xF29C, 0xA8B7,
	0x520C, 0xBEA6, 0x1205, 0x99C4, 0x4F89, 0x9417, 0xF6F9, 0xE0B2,
	0xCD08, 0x6CC3, 0x53A5, 0x666F, 0x42B2, 0x8953, 0x0220, 0xD533,
	0x18F3, 0x3A5F, 0xD2EF, 0x4017, 0xC2E9, 0x4147, 0x143F, 0xCB56,
	0x33A3, 0x9F4A, 0x7B3C, 0xFDBB, 0xBABA, 0x8BC8, 0xC683, 0x13B6,
	0xEE0A, 0x4987, 0x6DF9, 0x1C29, 0x5530, 0x7B15, 0x3803, 0x521F,
	0xA05E, 0x9A7A, 0xCC99, 0xEC9F, 0x8E09, 0x3E75, 0x960F, 0x1CC5
};

constexpr size_t __wkeysSz = sizeof(__wkeys) / sizeof(wchar_t);

constexpr char GetKey(size_t i)
{
	i = i % __wkeysSz;
	char b1 = HIBYTE(__wkeys[i]);
	char b2 = LOBYTE(__wkeys[i]);
	return b1 & b2;
}

template <size_t N>
constexpr std::array<char, N> str_encrypt(const char(&str)[N]) 
{
	std::array<char, N> output{};
	unsigned char x = 0, c = 0;

	for (int i = 0; i < N; ++i) {
		c = str[i];
		char k = GetKey(i);
		char t1 = ( c ^ k );
		output[i] = t1;
	}
	return output;
}

template <size_t N>
constexpr std::string str_decrypt(const std::array<char, N>& str)
{
	std::string rc;
	unsigned char x = 0;
	for (size_t i = 0; i < str.size(); i++)
	{
		char c = str[i];
		char k = GetKey(i);
		char v = c ^ k;
		rc.push_back(v);
	}
		
	return rc;
}

#ifndef _DEBUG
/// <summary>
/// ѕри использовании следует передавать туда значени€ строк, которые определены до компил€ции, в ином случае можно использовать
/// такие методы, как: 
/// <para>str_encrypt()</para>
/// <para>str_decrypt()</para>
/// </summary>
/// <param name="str"> - массив элементов char, которые должны быть зашифрованы во врем€ компил€ции</param>
/// <returns><para>const char*</para> в расшифрованном виде</returns>
#define ENCSTR(str)										\
    ([]() {												\
        constexpr auto encrypted = str_encrypt(str);	\
        return str_decrypt(encrypted);					\
    }().c_str())

#define ENCSTR_STR(str)									\
    ([]() {												\
        constexpr auto encrypted = str_encrypt(str);	\
        return str_decrypt(encrypted);					\
    }())

#else
#define ENCSTR(str) (std::string(str).c_str())
#endif
/// <summary>
/// <para>≈сли нужен тип данных std::string - то можно использовать ENCSTR()</para><para>-</para>
/// ѕри использовании следует передавать туда значени€ строк, которые определены до компил€ции, в ином случае можно использовать
/// такие методы, как: 
/// <para>str_encrypt()</para>
/// <para>str_decrypt()</para>
/// </summary>
/// <param name="sz"> - массив элементов char, которые должны быть зашифрованы во врем€ компил€ции</param>
/// <returns><para>const char*</para> в расшифрованном виде</returns>
//#define ENCCCH(sz) (ENCSTR(sz).c_str())

#endif