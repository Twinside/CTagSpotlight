#ifndef __HACKING_H__
#define __HACKING_H__


static inline void addTotals (
		const unsigned int files, const long unsigned int lines,
		const long unsigned int bytes)
    {}

static inline boolean isDestinationStdout (void)
    { return FALSE; }

#elif defined _ENTRY_H
extern void notifySystemTagEntry (const tagEntryInfo *const tag);
#endif /* __HACKING_H__ */
