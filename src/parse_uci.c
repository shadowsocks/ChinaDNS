

static int parse_uci_args (char * uci_name) {
    struct uci_context * uci_ctx = uci_alloc_context();
    struct uci_package *conf = NULL;
    if (uci_load(uci_ctx, uci_name, &conf) != 0) {
        printf("load error uci: %s \n" , uci_name);
        /* free uci_context ?? */
        return -1;
    }
    struct uci_element *section_elem;
    uci_foreach_element(&conf->sections, section_elem) {
        struct uci_section *s = uci_to_section(section_elem);
        if (strcasecmp(s->type, uci_name) == 0 )
            parse_section(uci_ctx, s);

    }
    uci_unload(uci_ctx, conf);
    uci_free_context(uci_ctx);
    return 0;

}

static int parse_section (struct uci_context *uci_ctx,
                            struct uci_section *uci_sec) {
    char * enabled_str = uci_lookup_option_string(uci_ctx, uci_sec, "enabled");
    char * verbose_str = uci_lookup_option_string(uci_ctx, uci_sec, "verbose");
    char * addr = uci_lookup_option_string(uci_ctx, uci_sec, "addr");
    char * port = uci_lookup_option_string(uci_ctx, uci_sec, "port");

    if (enabled_str)
        enabled = atoi(enabled_str);
    if (verbose_str)
        verbose = atoi(verbose_str);
    if (addr)
        listen_addr=strdup(addr);
    if (port)
        listen_port=strdup(port);

    /* TODO  NotImplemented  */
    struct uci_element *e;

    bool sep = false;
    dns_servers = malloc(BUF_SIZE+1);
    memset(dns_servers, 0, BUF_SIZE+1);
    struct uci_option * servers = uci_lookup_option(uci_ctx, uci_sec, "server");
    uci_foreach_element(&servers->v.list, e){
        if (sep){
            strncat(dns_servers, ",", BUF_SIZE);
        }
        strncat(dns_servers, e->name, BUF_SIZE);
        sep = true;
    }
    DLOG("dns_servers: %s \n", dns_servers);
    struct uci_option * blackips = uci_lookup_option(uci_ctx, uci_sec, "blackip");

    ip_list.entries = 0;
    int i = 0;
    uci_foreach_element(&blackips->v.list, e){
        ip_list.entries++;
    }
    DLOG("ip_list.entries : %d \n", ip_list.entries);
    /* TODO free ???? */
    ip_list.ips = calloc(ip_list.entries, sizeof(struct in_addr));
    uci_foreach_element(&blackips->v.list, e){
        inet_aton(e->name, &ip_list.ips[i]);
        i++;
    }
    qsort(ip_list.ips, ip_list.entries, sizeof(struct in_addr), cmp_in_addr);


#ifdef WITH_CHN_ROUTE
    struct uci_option * chnroutes = uci_lookup_option(uci_ctx, uci_sec, "chnroute");
    chnroute_list.entries = 0;
    uci_foreach_element(&chnroutes->v.list, e){
        chnroute_list.entries++;
    }
    DLOG("chnroute_list.entries : %d \n", chnroute_list.entries);
    /* TODO free ???? */
    chnroute_list.nets = calloc(chnroute_list.entries, sizeof(net_mask_t));

    i = 0;
    uci_foreach_element(&chnroutes->v.list, e){
        char *sp_pos = strchr(e->name, '/');
        *sp_pos = 0;
        chnroute_list.nets[i].mask = (1 << (32 - atoi(sp_pos + 1))) - 1;
        inet_aton(e->name, &chnroute_list.nets[i].net);
        i++;
    }
    qsort(chnroute_list.nets, chnroute_list.entries, sizeof(net_mask_t),
          cmp_net_mask);

#endif

}
