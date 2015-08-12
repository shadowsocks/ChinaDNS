#include <errno.h>
#include <resolv.h>
#include <stddef.h>
#include "local_ns_parser.h"

static void local_ns_setsection(ns_msg *msg, ns_sect sect);
static int local_ns_skiprr(const unsigned char *ptr, const unsigned char *eom, ns_sect section, int count);
static int local_ns_dn_skipname(const unsigned char *ptr, const unsigned char *eom);
static int local_ns_name_skip(const unsigned char **ptrptr, const unsigned char *eom);
static int local_ns_labellen(const unsigned char *lp);
#define LOCAL_NS_TYPE_ELT 0x40 /*%< EDNS0 extended label type */
#define LOCAL_DNS_LABELTYPE_BITSTRING 0x41
#ifdef __UCLIBC__
#define LOCAL_NS_MSG_PTR _ptr
#else
#define LOCAL_NS_MSG_PTR _msg_ptr
#endif

int local_ns_initparse(const unsigned char *msg, int msglen, ns_msg *handle)
{
	const unsigned char *eom = msg + msglen;
	int i;

	handle->_msg = msg;
	handle->_eom = eom;
	if (msg + NS_INT16SZ > eom) {
		errno = EMSGSIZE;
		return -1;
	}

	NS_GET16(handle->_id, msg);
	if (msg + NS_INT16SZ > eom) {
		errno = EMSGSIZE;
		return -1;
	}

	NS_GET16(handle->_flags, msg);
	for (i = 0; i < ns_s_max; i++) {
		if (msg + NS_INT16SZ > eom) {
			errno = EMSGSIZE;
			return -1;
		}

		NS_GET16(handle->_counts[i], msg);
	}
	for (i = 0; i < ns_s_max; i++)
		if (handle->_counts[i] == 0)
			handle->_sections[i] = NULL;
		else {
			int b = local_ns_skiprr(msg, eom, (ns_sect)i,
					handle->_counts[i]);

			if (b < 0)
				return -1;
			handle->_sections[i] = msg;
			msg += b;
		}

	if (msg != eom) {
		errno = EMSGSIZE;
		return -1;
	}

	local_ns_setsection(handle, ns_s_max);
	return 0;
}
int local_ns_parserr(ns_msg *handle, ns_sect section, int rrnum, ns_rr *rr)
{
	int b;
	int tmp;

	/* Make section right. */
	tmp = section;
	if (tmp < 0 || section >= ns_s_max) {
		errno = ENODEV;
		return -1;
	}

	if (section != handle->_sect)
		local_ns_setsection(handle, section);

	/* Make rrnum right. */
	if (rrnum == -1)
		rrnum = handle->_rrnum;
	if (rrnum < 0 || rrnum >= handle->_counts[(int)section]) {
		errno = ENODEV;
		return -1;
	}
	if (rrnum < handle->_rrnum)
		local_ns_setsection(handle, section);
	if (rrnum > handle->_rrnum) {
		b = local_ns_skiprr(handle->LOCAL_NS_MSG_PTR, handle->_eom, section,
				rrnum - handle->_rrnum);

		if (b < 0)
			return (-1);
		handle->LOCAL_NS_MSG_PTR += b;
		handle->_rrnum = rrnum;
	}

	/* Do the parse. */
	b = dn_expand(handle->_msg, handle->_eom,
			handle->LOCAL_NS_MSG_PTR, rr->name, NS_MAXDNAME);
	if (b < 0)
		return (-1);
	handle->LOCAL_NS_MSG_PTR += b;
	if (handle->LOCAL_NS_MSG_PTR + NS_INT16SZ + NS_INT16SZ > handle->_eom) {
		errno = EMSGSIZE;
		return -1;
	}
	NS_GET16(rr->type, handle->LOCAL_NS_MSG_PTR);
	NS_GET16(rr->rr_class, handle->LOCAL_NS_MSG_PTR);
	if (section == ns_s_qd) {
		rr->ttl = 0;
		rr->rdlength = 0;
		rr->rdata = NULL;
	} else {
		if (handle->LOCAL_NS_MSG_PTR + NS_INT32SZ + NS_INT16SZ > handle->_eom) {
			errno = EMSGSIZE;
			return -1;
		}
		NS_GET32(rr->ttl, handle->LOCAL_NS_MSG_PTR);
		NS_GET16(rr->rdlength, handle->LOCAL_NS_MSG_PTR);
		if (handle->LOCAL_NS_MSG_PTR + rr->rdlength > handle->_eom) {
			errno = EMSGSIZE;
			return -1;
		}
		rr->rdata = handle->LOCAL_NS_MSG_PTR;
		handle->LOCAL_NS_MSG_PTR += rr->rdlength;
	}
	if (++handle->_rrnum > handle->_counts[(int)section])
		local_ns_setsection(handle, (ns_sect)((int)section + 1));

	return 0;
}
static void local_ns_setsection(ns_msg *msg, ns_sect sect)
{
	msg->_sect = sect;
	if (sect == ns_s_max) {
		msg->_rrnum = -1;
		msg->LOCAL_NS_MSG_PTR = NULL;
	} else {
		msg->_rrnum = 0;
		msg->LOCAL_NS_MSG_PTR = msg->_sections[(int)sect];
	}
}
static int local_ns_skiprr(const unsigned char *ptr, const unsigned char *eom, ns_sect section, int count)
{
	const unsigned char *optr = ptr;

	for (; count > 0; count--) {
		int b, rdlength;

		b = local_ns_dn_skipname(ptr, eom);
		if (b < 0) {
			errno = EMSGSIZE;
			return -1;
		}
		ptr += b/*Name*/ + NS_INT16SZ/*Type*/ + NS_INT16SZ/*Class*/;
		if (section != ns_s_qd) {
			if (ptr + NS_INT32SZ + NS_INT16SZ > eom) {
				errno = EMSGSIZE;
				return -1;
			}

			ptr += NS_INT32SZ/*TTL*/;
			NS_GET16(rdlength, ptr);
			ptr += rdlength/*RData*/;
		}
	}

	if (ptr > eom) {
		errno = EMSGSIZE;
		return -1;
	}

	return ptr - optr;
}
static int local_ns_dn_skipname(const unsigned char *ptr, const unsigned char *eom)
{
	const unsigned char *saveptr = ptr;

	if (local_ns_name_skip(&ptr, eom) == -1)
		return -1;

	return ptr - saveptr;
}
static int local_ns_name_skip(const unsigned char **ptrptr, const unsigned char *eom)
{
	const unsigned char *cp;
	unsigned int n;
	int l;

	cp = *ptrptr;
	while (cp < eom && (n = *cp++) != 0) {
		/* Check for indirection. */
		switch (n & NS_CMPRSFLGS) {
			case 0: /*%< normal case, n == len */
				cp += n;
				continue;
			case LOCAL_NS_TYPE_ELT: /*%< EDNS0 extended label */
				if ((l = local_ns_labellen(cp - 1)) < 0) {
					errno = EMSGSIZE; /*%< XXX */
					return -1;
				}
				cp += l;
				continue;
			case NS_CMPRSFLGS:      /*%< indirection */
				cp++;
				break;
			default: /*%< illegal type */
				errno = EMSGSIZE;
				return -1;
		}

		break;
	}

	if (cp > eom) {
		errno = EMSGSIZE;
		return -1;
	}

	*ptrptr = cp;

	return 0;
}
static int local_ns_labellen(const unsigned char *lp)
{
	int bitlen;
	unsigned char l = *lp;

	if ((l & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
		/* should be avoided by the caller */
		return -1;
	}

	if ((l & NS_CMPRSFLGS) == LOCAL_NS_TYPE_ELT) {
		if (l == LOCAL_DNS_LABELTYPE_BITSTRING) {
			if ((bitlen = *(lp + 1)) == 0)
				bitlen = 256;
			return ((bitlen + 7 ) / 8 + 1);
		}

		return -1;    /*%< unknwon ELT */
	}

	return l;
}
