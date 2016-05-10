#ifndef _LZCOMMON_H
#define _LZCOMMON_H



//#define HASH6(a) (((*(u32*)(&a)^(*(u32*)((&a)+4))^(*((&a)+8)<<13))*2654435761u)>>(32-m_Hash6Bits))
//#define HASH6(a) (((*(u32*)(&a)^(*(u32*)((&a)+4)))*2654435761u)>>(32-m_Hash6Bits))
//#define HASH6(a) (((*(u32*)(&a)^(*(u32*)((&a)+4)>>8))*2654435761u)>>(32-m_Hash6Bits))
#define HASH6(a) (((*(u32*)(&a)^(*(u16*)((&a)+4)<<13))*2654435761u)>>(32-m_Hash6Bits))
//#define HASH6(a) (((*(u32*)(&a)^(*((&a)+4)<<15))*2654435761u)>>(32-m_Hash6Bits))
//#define HASH6(a) (1)
//#define HASH4(a) (((*(u32*)(&a)^(*(u16*)((&a)+4)<<15))*2654435761u)>>(32-20))
//#define HASH6(a) (((*(u32*)(&a))*2654435761u)>>(32-m_Hash6Bits))
#define HASH3(a) (((a)<<8)^(*((&a)+1)<<5)^(*((&a)+2)))
#define HASH2(a) (*(u16*)&a)

#define CURPOS (m_WndCurrPos|((m_Wnd[m_WndCurrPos]&0x0E)<<28))



#define MIN_LEN_HT2 2
#define MIN_LEN_REP 2
#define MIN_LEN_HT6 6
#define MIN_LEN_HT3 3

#define GOOD_LEN_REP 32
#define GOOD_LEN_HT6 34


#endif