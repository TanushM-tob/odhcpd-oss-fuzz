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


#include "src/odhcpd.h"

static int mock_send_reply(const void *buf, size_t len,
                          const struct sockaddr *dest, socklen_t dest_len,
                          void *opaque) {
    return len;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 8) return 0;
    if (size > 1500) return 0;
    
    uint8_t *copy = malloc(size);
    if (!copy) return 0;
    memcpy(copy, data, size);
    
    struct interface iface;
    memset(&iface, 0, sizeof(iface));
    
    iface.ifflags = 0;
    iface.ifindex = 1;
    iface.ifname = "test0";
    iface.name = "test0";
    
    iface.dhcpv4 = MODE_SERVER;
    
    inet_pton(AF_INET, "192.168.1.100", &iface.dhcpv4_start_ip);
    inet_pton(AF_INET, "192.168.1.200", &iface.dhcpv4_end_ip);
    inet_pton(AF_INET, "192.168.1.1", &iface.dhcpv4_local);
    inet_pton(AF_INET, "192.168.1.255", &iface.dhcpv4_bcast);
    inet_pton(AF_INET, "255.255.255.0", &iface.dhcpv4_mask);
    
    int dummy_fd = 1;  
    iface.dhcpv4_event.uloop.fd = dummy_fd;
    
    INIT_LIST_HEAD(&iface.dhcpv4_assignments);
    INIT_LIST_HEAD(&iface.dhcpv4_fr_ips);
    
    iface.dns_service = true;
    iface.dhcp_leasetime = 3600; 
    iface.dhcpv4_forcereconf = false;
    
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(68); 
    inet_pton(AF_INET, "192.168.1.50", &client_addr.sin_addr);
    
    dhcpv4_handle_msg(&client_addr, copy, size, &iface, NULL, 
                      mock_send_reply, &dummy_fd);

    if (size >= 4 && size <= 1024) {
        uint8_t *config_copy = malloc(size);
        if (config_copy) {
            memcpy(config_copy, data, size);
            config_parse_interface(config_copy, size, "test_iface", true);
            free(config_copy);
        }
    }
    
    free(copy);
    return 0;
}