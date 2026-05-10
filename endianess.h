#ifndef __ENDIANESS_H__
#define __ENDIANESS_H__

static inline long long BEtoNll(long long val)
{
	return ((val & 0xFFull) << 56) | ((val & 0xFF00ull) << 40) | ((val & 0xFF0000ull) << 24) | ((val & 0xFF000000ull) << 8) |
		((val & 0xFF00000000ull) >> 8) | ((val & 0xFF0000000000ull) >> 24) | ((val & 0xFF000000000000ull) >> 40) | ((val & 0xFF00000000000000ull) >> 56);
}

static inline int BEtoNl(int val)
{
	return ((val & 0xFF) << 24) | ((val & 0xFF00) << 8) | ((val & 0xFF0000) >> 8) | ((val & 0xFF000000) >> 24);
}

#define BEtoNlp(pval) ((*pval) = BEtoNl((*pval)))

static inline short BEtoNs(short val)
{
	return ((val & 0xFF) << 8) | ((val & 0xFF00) >> 8);
}

static inline float BEtoNf(float val)
{
	*((int*)&val) = BEtoNl(*((int*)&val));
	return val;
}

#endif

