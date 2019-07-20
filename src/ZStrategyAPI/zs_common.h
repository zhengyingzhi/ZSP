/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * some common defines
 */

#pragma warning(disable:4819)

#ifndef _ZS_COMMON_H_INCLUDED_
#define _ZS_COMMON_H_INCLUDED_

#if defined(_MSC_VER) && defined(ZS_ISLIB)
#ifdef ZS_API_EXPORTS
#define ZS_API __declspec(dllexport)
#else
#define ZS_API __declspec(dllimport)
#endif
#define ZS_API_STDCALL  __stdcall       /* ensure stcall calling convention on NT */

#else
#define ZS_API
#define ZS_API_STDCALL                  /* leave blank for other systems */

#endif//_WIN32


/* ZS version */
#define ZS_Version          "0.0.1"
#define ZS_Version_int      1


#endif//_ZS_COMMON_H_INCLUDED_
