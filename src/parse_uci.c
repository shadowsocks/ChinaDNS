

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
    struct uci_option * server = uci_lookup_option(uci_ctx, uci_sec, "server");
    struct uci_option * blackip = uci_lookup_option(uci_ctx, uci_sec, "blackip");
    struct uci_option * chnroute = uci_lookup_option(uci_ctx, uci_sec, "chnroute");

}
