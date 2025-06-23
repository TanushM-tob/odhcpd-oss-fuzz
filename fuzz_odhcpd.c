#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

extern void dhcpv4_handle_msg(void *addr, void *data, size_t len,
                              void *iface, void *dest_addr,
                              void *send_reply, void *opaque);

extern int config_parse_interface(void *data, size_t len, const char *iname, bool overwrite);

static int mock_send_reply(const void *buf, size_t len,
                          const struct sockaddr *dest, socklen_t dest_len,
                          void *opaque) {
    return len;
}

struct mock_interface {
    int dhcpv4;
    char name[16];
    char ifname[16];
    
    struct in_addr dhcpv4_start_ip;
    struct in_addr dhcpv4_end_ip;
    struct in_addr dhcpv4_local;
    struct in_addr dhcpv4_bcast;
    struct in_addr dhcpv4_mask;
    
    struct {
        struct {
            int fd;
        } uloop;
    } dhcpv4_event;
    
    struct {
        struct mock_dhcp_assignment *next;
        struct mock_dhcp_assignment *prev;
    } dhcpv4_assignments;
    
    bool dhcpv4_forcereconf;
    bool dns_service;
    
    int ifflags;
    
    struct in_addr *dhcpv4_router;
    size_t dhcpv4_router_cnt;
    struct in_addr *dhcpv4_dns;
    size_t dhcpv4_dns_cnt;
    
    uint8_t *search;
    size_t search_len;
    
    char *filter_class;
    
    uint32_t dhcp_leasetime;
    
    struct in_addr *dhcpv4_ntp;
    size_t dhcpv4_ntp_cnt;
    
    void *dnr;
    size_t dnr_cnt;
};

struct mock_dhcp_assignment {
    struct mock_dhcp_assignment *next;
    struct mock_dhcp_assignment *prev;
};

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 8) return 0;
    if (size > 1500) return 0;
    
    struct mock_interface iface;
    memset(&iface, 0, sizeof(iface));
    
    iface.dhcpv4 = 1;
    strcpy(iface.name, "test0");
    strcpy(iface.ifname, "test0");
    
    inet_pton(AF_INET, "192.168.1.100", &iface.dhcpv4_start_ip);
    inet_pton(AF_INET, "192.168.1.200", &iface.dhcpv4_end_ip);
    inet_pton(AF_INET, "192.168.1.1", &iface.dhcpv4_local);
    inet_pton(AF_INET, "192.168.1.255", &iface.dhcpv4_bcast);
    inet_pton(AF_INET, "255.255.255.0", &iface.dhcpv4_mask);
    
    int dummy_fd = 1;
    iface.dhcpv4_event.uloop.fd = dummy_fd;
    
    iface.dhcpv4_assignments.next = (struct mock_dhcp_assignment *)&iface.dhcpv4_assignments;
    iface.dhcpv4_assignments.prev = (struct mock_dhcp_assignment *)&iface.dhcpv4_assignments;
    
    iface.dns_service = true;
    iface.dhcp_leasetime = 3600;
    iface.dhcpv4_forcereconf = false;
    
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(68);
    inet_pton(AF_INET, "192.168.1.50", &client_addr.sin_addr);
    
    dhcpv4_handle_msg(&client_addr, (void*)data, size, &iface, NULL, 
                      mock_send_reply, &dummy_fd);
    
    if (size >= 4 && size <= 1024) {
        config_parse_interface((void*)data, size, "test_iface", true);
    }
    
    return 0;
}