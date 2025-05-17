/*
// @(#) lu/dirent.h
*/
# if	!defined(LU_DIRENT_H)
# define	LU_DIRENT_H	1

# include	<stdint.h>

enum	{
	NAMELEN	= 8,
	EXTLEN	= 3,
	SECTOR	= 128,
	NAMEREC_LEN	= NAMELEN + 1 + EXTLEN + 1,
};
enum	status	{
	ST_ACTIVE	= 0,
	ST_DELETED	= 0xfe,
	ST_UNUSED	= 0xff,
};

struct	ludirent_t	{
	uint8_t	status;
	char	name [NAMELEN];
	char	ext [EXTLEN];
	uint16_t	first_sector;
	uint16_t 	sectors_used;
	uint16_t	crc;
//	--
	uint16_t	ctim_day;
	uint16_t	mtim_day;
	uint16_t	ctim_hms;
	uint16_t	mtim_hms;
//	--
	uint8_t	pad;
	uint8_t filler [5];
};

typedef	struct	ludirent_t	ludirent_t;

#endif
