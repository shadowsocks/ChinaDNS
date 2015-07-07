#ifndef LOCAL_NS_PARSER_H
#define LOCAL_NS_PARSER_H

int local_ns_initparse(const unsigned char *msg, int msglen, ns_msg *handle);
int local_ns_parserr(ns_msg *handle, ns_sect section, int rrnum, ns_rr *rr);

#endif
