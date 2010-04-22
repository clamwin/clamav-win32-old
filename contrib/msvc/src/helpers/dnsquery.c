/*
 * Clamav Native Windows Port: txt dns query for freshclam
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <platform.h>
#include <osdeps.h>
#include <windns.h>
#include <iphlpapi.h>
#include <inttypes.h>

#include "others.h"
#include "shared/output.h"

#define TCPIP_PARAMS "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"

char *txtquery_dnsapi(const char *domain, unsigned int *ttl);
char *txtquery_compat(const char *domain, unsigned int *ttl);

#define NS_INT8SZ 1
#define NS_PACKETSZ 512
#define ns_t_txt 16
#define N_RETRY 5

#define SETUINT16(x, v) *(uint16_t *) x = htons(v)

#define GETUINT16(x) ntohs(*(uint16_t *) x)
#define GETUINT32(x) ntohl(*(uint32_t *) x)

/* Bound checks to avoid buffer overflows */
#define NEED(len) \
    /* printf("DNS Resolver: Need %d bytes - Have %d bytes\n", len, numbytes - (seek - reply)); */ \
    if (((seek + len) - reply) > numbytes) \
    { \
        logg("!DNS Resolver: Bound Check failed - Bad packet\n"); \
        return NULL; \
    }

#ifndef HAVE_ATTRIB_PACKED
#define __attribute__(x)
#endif

#ifdef HAVE_PRAGMA_PACK
#pragma pack(1)
#endif

typedef struct _simple_dns_query
{
    uint16_t transaction_id __attribute__ ((packed));
    uint16_t flags __attribute__ ((packed));
    uint16_t questions __attribute__ ((packed));
    uint16_t ans_rrs __attribute__ ((packed));
    uint16_t auth_rss __attribute__ ((packed));
    uint16_t add_rss __attribute__ ((packed));
    /* queries here */
} simple_dns_query;

#ifdef HAVE_PRAGMA_PACK
#pragma pack()
#endif

char *txtquery_dnsapi(const char *domain, unsigned int *ttl);
char *txtquery_compat(const char *domain, unsigned int *ttl);

/* Switcher */
char *txtquery(const char *domain, unsigned int *ttl)
{
    return ((isWin9x() || isOldOS()) ? txtquery_compat(domain, ttl) : txtquery_dnsapi(domain, ttl));
}

static char *get_dns_fromreg(void)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    unsigned char data[MAX_PATH];
    DWORD datalen = MAX_PATH - 1;
    int i;
    char *keys[] = { "ClamWinNameServer", "NameServer", "DhcpNameServer", NULL };

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TCPIP_PARAMS, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return NULL;

    for (i = 0; keys[i]; i++)
    {
        datalen = MAX_PATH - 1;
        if ((RegQueryValueExA(hKey, keys[i], NULL, &dwType, data, &datalen) == ERROR_SUCCESS) &&
            (datalen > 1) && (dwType == REG_SZ))
        {
            char *space;
            RegCloseKey(hKey);
            if ((space = strchr(data, ' '))) *space = 0;
            if (inet_addr(data) == INADDR_NONE)
            {
                /* printf("DNS Resolver: Found %s key: %s - invalid address\n", keys[i], data); */
                return NULL;
            }
            else
            {
                /* printf("DNS Resolver: Found %s key: %s\n", keys[i], data); */
                return _strdup(data);
            }
        }
    }

    /* printf("DNS Resolver: No nameservers found in registry\n"); */
    RegCloseKey(hKey);
    return NULL;
}

static char *get_dns(void)
{
    FIXED_INFO *FixedInfo;
    ULONG ulOutBufLen;
    DWORD res;
    char *dns_server = NULL;

    FixedInfo = (FIXED_INFO *) GlobalAlloc(GPTR, sizeof(FIXED_INFO));
    ulOutBufLen = sizeof(FIXED_INFO);

    switch (res = GetNetworkParams(FixedInfo, &ulOutBufLen))
    {
        case ERROR_BUFFER_OVERFLOW:
            GlobalFree(FixedInfo);
            FixedInfo = (FIXED_INFO *) GlobalAlloc(GPTR, ulOutBufLen);
        case NO_ERROR:
            break;
        case ERROR_NOT_SUPPORTED:
            /* printf("DNS Resolver: GetNetworkParams() not supported on this OS\n"); */
            GlobalFree(FixedInfo);
            return get_dns_fromreg();
        default:
            GlobalFree(FixedInfo);
            logg("!DNS Resolver: [1] Call to GetNetworkParams() failed %d\n", res);
            return NULL;
    }

    if ((res = GetNetworkParams(FixedInfo, &ulOutBufLen) != NO_ERROR))
    {
        GlobalFree(FixedInfo);
        logg("!DNS Resolver: [2] Call to GetNetworkParams() failed %d\n", res);
        return NULL;
    }

/*
    More than one dns server - we just use the primary
    if (FixedInfo->DnsServerList.Next)
    {
        IP_ADDR_STRING *pIPAddr;
        pIPAddr = FixedInfo->DnsServerList.Next;
        while (pIPAddr)
        {
            //printf("DNS Resolver: Found additional DNS Server: %s\n", pIPAddr->IpAddress.String);
            pIPAddr = pIPAddr->Next;
        }
    }
*/

    dns_server = _strdup(FixedInfo->DnsServerList.IpAddress.String);
    GlobalFree(FixedInfo);
    return dns_server;
}

static char *do_query(struct hostent *he, const char *domain, unsigned int *ttl)
{
    struct sockaddr_in dns;
    char *packet, *seek, *txtreply;
    char reply[NS_PACKETSZ];
    simple_dns_query query, *res;
    int numbytes, addr_len, i;
    int start, rev, len, off;
    uint16_t tid;
    int sockfd = -1;
    struct timeval tv;

/* win32 random functions are enough here */
#undef rand
#undef srand
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec + clock() + rand());
    tid = rand();

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        logg("!DNS Resolver: socket() failed, %s\n", strerror(errno));
        return NULL;
    }

    memset(&dns, 0, sizeof(dns));
    dns.sin_family = AF_INET;
    dns.sin_port = htons(53);
    dns.sin_addr = *((struct in_addr *) he->h_addr);

    /* Generate the packet */
    memset(&query, 0, sizeof(query));
    query.transaction_id = htons(tid);
    query.flags = htons(0x0100); /* Request + Recursion */
    query.questions = htons(1); /* 1 query */

    len = sizeof(query) + NS_INT8SZ + (int) strlen(domain) + NS_INT8SZ + (2 * sizeof(uint16_t)); /* \0 + Type + Class */
    packet = (char *) cli_malloc(len);

    memcpy(packet, &query, sizeof(query));
    off = sizeof(query) + NS_INT8SZ;

    start = 0;
    off += (int) strlen(domain);
    rev = start = off;

    /* String-ize */
    for (i = (int) strlen(domain); i >= 0; i--)
    {
        if (domain[i] != '.')
            packet[rev] = domain[i];
        else
        {
            packet[rev] = (uint8_t) (start - rev - 1);
            start = rev;
        }
        rev--;
    }

    /* First string length */
    packet[rev] = (uint8_t) (start - rev - 1);
    packet[off++] = 0;

    /* Type TXT */
    SETUINT16(&packet[off], ns_t_txt);
    off += sizeof(uint16_t);

    /* Class */
    SETUINT16(&packet[off], NS_INT8SZ);
    off += sizeof(uint16_t);

    if ((numbytes = sendto(sockfd, packet, (int) len, 0, (struct sockaddr *) &dns, sizeof(struct sockaddr))) == -1)
    {
        logg("!DNS Resolver: sendto() failed, %s\n", strerror(errno));
        free(packet);
        return NULL;
    }
    free(packet);

    addr_len = sizeof(struct sockaddr);
    if ((numbytes = recvfrom(sockfd, reply, NS_PACKETSZ, 0, (struct sockaddr *) &dns, &addr_len)) == -1)
    {
        logg("!DNS Resolver: recvfrom() failed, %s\n", strerror(errno));
        return NULL;
    }

    /* printf("DNS Resolver: Received %d bytes from the DNS\n", numbytes); */

    if (numbytes <= sizeof(simple_dns_query))
    {
        logg("!DNS Resolver: Short Reply\n");
        return NULL;
    }

    seek = reply;
    res = (simple_dns_query *) seek;

    /* All your replies are belong to us? */
    if (ntohs(res->transaction_id) != tid)
    {
        logg("!DNS Resolver: Bad TID expected 0x%04x got 0x%04x\n", tid, ntohs(res->transaction_id));
        return NULL;
    }

    /* Is a reply and result is ok */
    if (!(ntohs(res->flags) >> 15) || (ntohs(res->flags) & 0x000f))
    {
        logg("!DNS Resolver: Bad Reply\n");
        return NULL;
    }

    if (ntohs(res->ans_rrs) < 1)
    {
        logg("!DNS Resolver: No replies :(\n");
        return NULL;
    }

    seek += sizeof(simple_dns_query) + NS_INT8SZ;
    while (((seek - reply) < numbytes) && *seek) seek++; /* my request, don't care*/
    seek++;

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != ns_t_txt)
    {
        logg("!DNS Resolver: Dns query in reply is not TXT\n");
        return NULL;
    }
    seek += sizeof(uint16_t);

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != NS_INT8SZ)
    {
        logg("!DNS Resolver: Dns query in reply has a different Class\n");
        return NULL;
    }
    seek += sizeof(uint16_t);
    seek += sizeof(uint16_t); /* Answer c0 0c ?? - Wireshark says Name */

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != ns_t_txt)
    {
        logg("!DNS Resolver: Dns reply Type is not TXT\n");
        return NULL;
    }
    seek += sizeof(uint16_t);

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != NS_INT8SZ)
    {
        logg("!DNS Resolver: Dns reply has a different Class\n");
        return NULL;
    }
    seek += sizeof(uint16_t);

    NEED(sizeof(uint32_t));
    *ttl = (unsigned int) GETUINT32(seek);
    seek += sizeof(uint32_t);

    NEED(sizeof(uint16_t));
    len = GETUINT16(seek);

    if (len > NS_PACKETSZ)
    {
        logg("!DNS Resolver: Oversized reply\n");
        return NULL;
    }
    seek += sizeof(uint16_t); /* Len */

    NEED(len);

    seek++;
    if (!*seek)
    {
        logg("!DNS Resolver: NULL txt reply, very weird\n");
        return NULL;
    }

    txtreply = (char *) cli_malloc(len);
    memcpy(txtreply, seek, len);
    txtreply[len - 1] = 0;
    return txtreply;
}

/* Bare Version */
char *txtquery_compat(const char *domain, unsigned int *ttl)
{
    struct hostent *he;
    char *txt = NULL, *nameserver = NULL;
    int i;

    if ((nameserver = get_dns()) == NULL)
    {
        logg("!DNS Resolver: Cannot find a dns server\n");
        return NULL;
    }

    if ((he = gethostbyname(nameserver)) == NULL)
    {
        logg("!DNS Resolver: gethostbyname() failed\n");
        return NULL;
    }

    for (i = 0; i < N_RETRY; i++)
    {
        if ((txt = do_query(he, domain, ttl)) != NULL)
            break;
    }

    if (nameserver) free(nameserver);
    /* printf("DNS Resolver: Query Done using compatibility Method\n"); */
    /* printf("DNS Resolver: Result [%s]\n", txt); */
    return txt;
}

/* DNS API Version */
typedef DNS_STATUS (WINAPI *fnDnsQuery)(
    PCSTR           pszName,
    WORD            wType,
    DWORD           Options,
    PIP4_ARRAY      aipServers,
    PDNS_RECORD     *ppQueryResults,
    PVOID           *pReserved
);

typedef VOID (WINAPI *fnDnsRecordListFree)(
    PDNS_RECORD     pRecordList,
    DNS_FREE_TYPE   FreeType
);

char *txtquery_dnsapi(const char *domain, unsigned int *ttl)
{
    PDNS_RECORD pRec = NULL, pRecOrig = NULL;
    HMODULE hDnsApi = NULL;
    fnDnsQuery pDnsQuery = NULL;
    fnDnsRecordListFree pDnsRecordListFree = NULL;
    char *txt = NULL;

    if (!(hDnsApi = LoadLibraryA("dnsapi.dll")))
    {
        logg("!DNS Resolver: Cannot load dnsapi.dll: %lu\n", GetLastError());
        return txtquery_compat(domain, ttl);
    }

    if (!(pDnsQuery = (fnDnsQuery) GetProcAddress(hDnsApi, "DnsQuery_A")) ||
        !(pDnsRecordListFree = (fnDnsRecordListFree) GetProcAddress(hDnsApi, "DnsRecordListFree")))
    {
        logg("!DNS Resolver: Cannot find needed exports in dnsapi.dll\n");
        FreeLibrary(hDnsApi);
        return txtquery_compat(domain, ttl);
    }

    if (pDnsQuery(domain, DNS_TYPE_TEXT, DNS_QUERY_STANDARD | DNS_QUERY_BYPASS_CACHE, NULL, &pRec, NULL) != ERROR_SUCCESS)
    {
        logg("!DNS Resolver: Can't query %s\n", domain);
        FreeLibrary(hDnsApi);
        return NULL;
    }

    pRecOrig = pRec;

    while (pRec)
    {
        if ((pRec->wType == DNS_TYPE_TEXT) && pRec->wDataLength && pRec->Data.TXT.dwStringCount)
        {
            /* int len = DNS_TEXT_RECORD_LENGTH(pRec->Data.TXT.dwStringCount); */ /* returns 12 instead of 22? */
            /* yes strlen is unsafe but win32 doesn't tell me the right length */
            size_t len = strlen(pRec->Data.TXT.pStringArray[0]);
            txt = (char *) cli_malloc(len + 1);
            strncpy(txt, (char *) pRec->Data.TXT.pStringArray[0], len);
            txt[len] = 0;
            *ttl = pRec->dwTtl;
            break;
        }
        pRec = pRec->pNext;
    }
    if (!txt) logg("!DNS Resolver: Broken DNS reply.\n");
    pDnsRecordListFree(pRecOrig, DnsFreeRecordList);
    FreeLibrary(hDnsApi);
    /* printf("DNS Resolver: Query Done using DnsApi Method\n"); */
    /* printf("DNS Resolver: Result [%s]\n", txt); */
    return txt;
}
