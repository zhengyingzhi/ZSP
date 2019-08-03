/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define the errors
 */

#ifndef _ZS_ERROR_H_INCLUDED_
#define _ZS_ERROR_H_INCLUDED_


#define ZS_EXISTED               1
#define ZS_OK                    0
#define ZS_ERROR                -1
#define ZS_ERR_NotImpl          -2

#define ZS_ERR_NoAccount        -11
#define ZS_ERR_NoBroker         -12
#define ZS_ERR_NoBrokerInfo     -13
#define ZS_ERR_NoContract       -14
#define ZS_ERR_NoAsset          -15
#define ZS_ERR_NoStrategy       -16
#define ZS_ERR_NoEntryFunc      -17
#define ZS_ERR_NoEntryObject    -18
#define ZS_ERR_NoOrder          -19
#define ZS_ERR_NoPosEngine      -20
#define ZS_ERR_AvailCash        -21
#define ZS_ERR_AvailClose       -22
#define ZS_ERR_InvalidField     -23
#define ZS_ERR_JsonData         -24
#define ZS_ERR_NoConfItem       -25
#define ZS_ERR_OrderFinished    -26

#define ZS_ERR_NotStarted       -27
#define ZS_ERR_PlacePaused      -28
#define ZS_ERR_PlaceOpenPaused  -29
#define ZS_ERR_NoMemory         -30


#endif//_ZS_ERROR_H_INCLUDED_
