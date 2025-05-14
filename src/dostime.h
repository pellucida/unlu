/*
// @(#) unlu/dostime.h
*/
# if	!defined(DOSTIME_H)
# define	DOSTIME_H	1

# include	<stdint.h>
# include	<time.h>

// date/time stamps - only .lbr create on MSDOS?

enum	time_stamp	{
	EPOCH_YEAR	= 1977,
	EPOCH_MONTH	= 12,
	EPOCH_DAY	= 31,
};
static	inline	time_t __attribute__((const)) epoch(void) {
	static	time_t	epoch_	= 0;
	if (epoch_==0) {
		struct	tm	EPOCH	= {
			.tm_year	= EPOCH_YEAR - 1900,
			.tm_mon		= EPOCH_MONTH-1, 
			.tm_mday	= EPOCH_DAY,
		};
		epoch_	= mktime (&EPOCH);
	}
	return	epoch_;
}
		
// times dos encoded uint16_t	hour:5, minutes:6, seconds_2sec_resolution:5;
enum	{
	BITS_HOURS	= 5,
	BITS_MINS	= 6,
	BITS_2SEC	= 5,
	SHIFT_HOURS	= BITS_MINS + BITS_2SEC,
	SHIFT_MINS	= BITS_2SEC,
	MASK_HOURS	= ~((~0u) << BITS_HOURS),
	MASK_MINS	= ~((~0u) << BITS_MINS),
	MASK_2SEC	= ~((~0u) << BITS_2SEC),
};
static	inline	unsigned	dos_hour (const uint16_t t){
	return	 (t >> SHIFT_HOURS) & MASK_HOURS;
}
static	inline	unsigned	dos_minutes (const uint16_t t){
	return	 (t >> SHIFT_MINS) & MASK_MINS;
}
static	inline	unsigned	dos_seconds (const uint16_t t){
	return	 (t & MASK_2SEC)*2;
}

// 24 hours/day x 60 minutes/hour x 60 secs/minute good upto leap seconds.

static	time_t	inline dos_mktime (uint16_t day, uint16_t time) {
	time_t	hour	= dos_hour (time);
	time_t	min	= dos_minutes (time);
	time_t	sec	= dos_seconds (time);
	time_t	t	= epoch() + (((day*24+hour)*60+min)*60+sec);
	return	t;
}
# endif
