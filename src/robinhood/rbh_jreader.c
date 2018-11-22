

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <jansson.h>
#include <lustre/lustreapi.h>
#include "list_mgr.h"
#include "rbh_misc.h"
#include "rbh_types.h"
#include "rbh_logs.h"
#include "rbh_basename.h"
#include "cmd_helpers.h"
#include "lustre_extended_types.h"


#define JSONREADFLAGS        JSON_DISABLE_EOF_CHECK
#define DEFAULTNBREAD        1000
#define MODEMASK             0777
#define MAX_OPT_LEN          1024

typedef struct {
    long nbread;
    FILE *f;
} targs;

typedef size_t ext2_inode_t;


json_t *read_one(FILE *);
int read_n(FILE *, long, json_t **);
stripe_items_t build_stripes_tab(json_t *, json_t *);
entry_id_t build_fid(json_t *);
attr_set_t *make_one_pfid(json_t *, int n);
int make_pfid_entry(json_t *, attr_set_t **, entry_id_t **);
int extract_name(json_t*, char*);
stripe_info_t getstripeinfo(json_t *);
attr_set_t *make_rbh_entry(json_t *);
void make_Nrbh_entries(json_t **, int, attr_set_t **, entry_id_t **);
void process_pfid(lmgr_t *, int, json_t **, entry_id_t **);
void process(lmgr_t*, int, attr_set_t**, entry_id_t**);
void rbh_init(char *);
FILE **process_filenames(char *, int *, FILE **);
void reader(targs *);


json_t *read_one(FILE *f)
{
    return json_loadf(f, JSONREADFLAGS, NULL);
}

int read_n(FILE *f, long n, json_t **tab)
{
    int count;

    if (n < 1 || tab == NULL) {
        fprintf(stderr, "NULL argument for read_n");
        return -1;
    }

    count = 0;

    while (count < n) {
        tab[count] = read_one(f);
        if (tab[count] == NULL)
            return count;
        count++;
    }

    return count;

}

stripe_items_t build_stripes_tab(json_t *lov, json_t *lma)
{
    int n;
    uint64_t seq, temp;
    json_t *elem, *tab;
    stripe_items_t res = {0};

    if (lov == NULL || json_is_null(lov))
        return res;

    res.count = json_integer_value(json_object_get(lov, "stripecount"));
    seq = json_integer_value(json_object_get(lma, "sequence"));
    res.stripe = malloc(sizeof(stripe_item_t) * res.count);
    tab = json_object_get(lov, "stripeparts");
    for (n = 0; n < res.count; n++) {
        elem = json_array_get(tab, n);
        temp = json_integer_value(json_object_get(elem, "ostID"));
        res.stripe[n].ost_idx = temp;
        temp = json_integer_value(json_object_get(elem, "objectID"));
        res.stripe[n].obj_id = temp;
        temp = json_integer_value(json_object_get(elem, "ostgen"));
        res.stripe[n].ost_gen = temp;
        res.stripe[n].obj_seq = seq;//ici c'est n'est pas la bonne sequence
        //voir lustre_tools.c:l135
    }

    return res;
}

entry_id_t build_fid(json_t *j)
{
    unsigned long seq;
    unsigned int oid, ver;
    entry_id_t res = {0};

    sscanf(json_string_value(j), "0x%lx:0x%x:0x%x", &seq, &oid, &ver);
    res.f_seq = seq;
    res.f_oid = oid;
    res.f_ver = ver;

    return res;
}

int extract_name(json_t *j, char *name)
{
    json_t *temp;

    temp = json_object_get(json_object_get(j, "ea"), "linkea");
    if (temp == NULL || json_is_null(temp) || json_array_size(temp) < 1) {
        return 0;/*no linkea it's a stripe from another MDT*/
    } else {
        temp = json_object_get(json_array_get(temp, 0), "name");
        strncpy(name, json_string_value(temp), RBH_NAME_MAX);
    }

    return 1;
}

stripe_info_t getstripeinfo(json_t *lov)
{
    stripe_info_t res = {0};

    if (lov == NULL || json_is_null(lov))
        return res;

    res.stripe_count = json_integer_value(json_object_get(lov, "stripecount"));
    res.stripe_size = json_integer_value(json_object_get(lov, "stripesize"));
    //poolname
    return res;
}

attr_set_t *make_rbh_entry(json_t *j)
{
    unsigned int itemp;
    attr_set_t *res;
    json_t *temp;

    res = NULL;
    temp = NULL;

    res = malloc(sizeof(attr_set_t));
    memset(res, 0, sizeof(attr_set_t));
    ATTR_MASK_INIT(res);

    /*name*/
    ATTR_MASK_SET(res, name);
    if (!extract_name(j, ATTR(res, name))) {
        free(res);
        return NULL;
    }

    /*ctime & creation_time*/
    ATTR_MASK_SET(res, last_mdchange);
    ATTR_MASK_SET(res, creation_time);
    temp = json_object_get(j, "ctime");
    itemp = json_integer_value(temp);
    ATTR(res, last_mdchange) = itemp;
    ATTR(res, creation_time) = itemp;

    /*atime*/
    ATTR_MASK_SET(res, last_access);
    temp = json_object_get(j, "atime");
    itemp = json_integer_value(temp);
    ATTR(res, last_access) = itemp;

    /*mtime*/
    ATTR_MASK_SET(res, last_mod);
    temp = json_object_get(j, "mtime");
    itemp = json_integer_value(temp);
    ATTR(res, last_mod) = itemp;

    /*uid*/
    ATTR_MASK_SET(res, uid);
    temp = json_object_get(j, "uid");
    itemp = json_integer_value(temp);
    uid2str(itemp, ATTR(res, uid).txt);

    /*gid*/
    ATTR_MASK_SET(res, gid);
    temp = json_object_get(j, "gid");
    itemp = json_integer_value(temp);
    gid2str(itemp, ATTR(res, gid).txt);

    /*mode & type*/
    ATTR_MASK_SET(res, mode);
    ATTR_MASK_SET(res, type);
    temp = json_object_get(j, "mode");
    itemp = json_integer_value(temp);
    ATTR(res, mode) = MODEMASK & itemp;
    strncpy(ATTR(res, type), mode2type(itemp & ~MODEMASK), strlen(mode2type(itemp & ~MODEMASK)));

    /*nlink*/
    ATTR_MASK_SET(res, nlink);
    temp = json_object_get(j, "nlink");
    itemp = json_integer_value(temp);
    ATTR(res, nlink) = itemp;

    /*stripe*/
    temp = json_object_get(j, "ea");
    ATTR_MASK_SET(res, stripe_info);
    ATTR(res, stripe_info) = getstripeinfo(json_object_get(temp, "lov"));
    ATTR_MASK_SET(res, stripe_items);
    ATTR(res, stripe_items) = build_stripes_tab(
                json_object_get(temp, "lov"),
                json_object_get(temp, "lma"));

    /*updates*/
    ATTR_MASK_SET(res, path_update);
    ATTR_MASK_SET(res, md_update);
    ATTR(res, path_update) = time(NULL);
    ATTR(res, md_update) = time(NULL);

    ATTR_MASK_SET(res, invalid);
    ATTR(res, invalid) = 0;

    /*parentfid*/
    ATTR_MASK_SET(res, parent_id);
    temp = json_array_get(json_object_get(json_object_get(j, "ea"), "linkea"),
            0);
    ATTR(res, parent_id) = build_fid(json_object_get(temp, "pfid"));

    return res;
}

void make_Nrbh_entries(json_t **j, int n, attr_set_t **res, entry_id_t **idtab)
{
    int count;
    json_t *temp;

    if (j == NULL || res == NULL)
        return;

    for (count = 0; count < n; count++) {
        res[count] = make_rbh_entry(j[count]);
        temp = json_object_get(j[count], "ea");
        idtab[count][0] = build_fid(json_object_get(temp, "lma"));
    }
}

void process(lmgr_t *l, int nb, attr_set_t **tab, entry_id_t **idtab)
{
    int n, count;
    attr_set_t **process;
    entry_id_t **processid;

    process = malloc(sizeof(attr_set_t *) * nb);
    processid = malloc(sizeof(entry_id_t) * nb);

    for (n = count = 0; n < nb; n++)
        if (tab[n] != NULL) {
            process[count] = tab[n];
            processid[count] = idtab[n];
            count++;
        }

    ListMgr_BatchInsert(l, processid, process, count, 1);
    free(process);
    free(processid);
}

attr_set_t *make_one_pfid(json_t *linkea, int n)
{
    json_t *temp;
    attr_set_t *res;

    res = malloc(sizeof(attr_set_t));
    temp = json_array_get(linkea, n);
    ATTR_MASK_INIT(res);

    ATTR_MASK_SET(res, name);
    strncpy(ATTR(res, name), json_string_value(json_object_get(temp, "name")),
           json_string_length(json_object_get(temp, "name")));

    ATTR_MASK_SET(res, parent_id);
    ATTR(res, parent_id) = build_fid(json_object_get(temp, "pfid"));

    return res;
}

void process_pfid(lmgr_t *l, int nb, json_t **jtab, entry_id_t **idtab)
{
    int n, m, nblink, count;
    json_t *temp;
    attr_set_t **ptab;
    entry_id_t **idptab;

    for (n = count = 0; n < nb; n++) {
        temp = json_object_get(json_object_get(jtab[n], "ea"), "linkea");
        if (temp == NULL || json_is_null(temp))
            continue;
        count += json_array_size(temp) - 1;
    }

    ptab = malloc(sizeof(attr_set_t *) * count);
    idptab = malloc(sizeof(entry_id_t *) * count);

    for (n = count = 0; n < nb; n++) {
        temp = json_object_get(json_object_get(jtab[n], "ea"), "linkea");
        if (temp == NULL || json_is_null(temp))
            continue;
        nblink = json_array_size(temp) - 1;
        for (m = 0; m < nblink; m++) {
            idptab[count] = idtab[n];
            ptab[count] = make_one_pfid(temp, m + 1);
            count++;
        }
    }

    ListMgr_BatchInsert(l, idptab, ptab, count, 1);

    for(n = 0; n < count; n++)
        free(ptab[n]);
    free(ptab);
    free(idptab);

}

void reader(targs *arg)
{
    long readed, n;
    lmgr_t l;
    json_t **jtab;
    attr_set_t **maintab;
    entry_id_t **idtab;

    readed = arg->nbread;
    jtab = malloc(sizeof(json_t) * arg->nbread);
    idtab = malloc(sizeof(entry_id_t) * arg->nbread);
    for (n = 0; n < arg->nbread; n++)
        idtab[n] = malloc(sizeof(entry_id_t));
    maintab = malloc(sizeof(attr_set_t) * arg->nbread);
    ListMgr_Init(0);
    ListMgr_InitAccess(&l);

    while (readed == arg->nbread) {
        readed = read_n(arg->f, arg->nbread, jtab);
        fprintf(stdout, "%ld entries read\n", readed);
        make_Nrbh_entries(jtab, readed, maintab, idtab);
        process(&l, readed, maintab, idtab);
        for (n = 0; n < readed; n++)
            json_decref(jtab[n]);

        for (n = 0; n < readed; n++) {
            if (maintab[n] != NULL &&
                    maintab[n]->attr_values.stripe_items.count != 0)
                free(maintab[n]->attr_values.stripe_items.stripe);
                free(maintab[n]);
        }
    }
    for (n = 0; n < arg->nbread; n++)
        free(idtab[n]);
    free(idtab);
    free(maintab);
    free(jtab);
    fclose(arg->f);
    free(arg);
    ListMgr_CloseAccess(&l);
}

void rbh_init(char *arg0)
{
    char config_file[MAX_OPT_LEN] = "";
    char badcfg[RBH_PATH_MAX];
    char err_msg[4096];
    bool dummy;
    int rc;
    const char *bin;

    bin = rh_basename(arg0);
    rc = rbh_init_internals();
    if (rc != 0)
        exit(rc);

    /* get default config file, if not specified */
    if (SearchConfig(config_file, config_file, &dummy, badcfg,
                     MAX_OPT_LEN) != 0) {
        fprintf(stderr, "No config file (or too many) found matching %s\n",
                badcfg);
        exit(2);
    } else if (dummy) {
        fprintf(stderr, "Using config file '%s'.\n", config_file);
    }

    /* only read common config (listmgr, ...) (mask=0) */
    if (rbh_cfg_load(0, config_file, err_msg)) {
        fprintf(stderr, "Error reading configuration file '%s': %s\n",
                config_file, err_msg);
        exit(1);
    }

    if (!log_config.force_debug_level)
        log_config.debug_level = LVL_MAJOR; /* no event message */

    /* Initialize logging */
    rc = InitializeLogs(bin);
    if (rc) {
        fprintf(stderr, "Error opening log files: rc=%d, errno=%d: %s\n",
                rc, errno, strerror(errno));
        exit(rc);
    }
}

FILE **process_filenames(char *arg, int *nb, FILE **res)
{
    int n, nbfile;
    char *file_list ,*end, *start;
    FILE *temp;

    nbfile = 0;
    file_list = strdup(arg);
    end = file_list;

    do {
        end = strchr(end, ',');
        if (end != NULL)
            end++;
        nbfile++;
    } while (end != NULL);
    res = realloc(res, sizeof(FILE*) * nbfile);
    end = start = file_list;

    for (n = 0; n < nbfile; n++) {
        end = strchr(start, ',');
        if (end != NULL){
            *end = '\0';
            end++;
        }
        temp = fopen(start, "r");
        if (temp == NULL) {
            fprintf(stderr, "can't open file %s, the file is ignored\n", start);
            n--;
            nbfile--;
        } else
            res[n] = temp;
        start = end;
    }

    free(file_list);
    if(nbfile == 0) {
        nbfile = 1;
        res[0] = stdin;
    }

    *nb = nbfile;
    return res;
}

int main(int argc, char *argv[])
{
    int n;
    int nbfile;
    long nbread;
    char arg;
    targs *argt;
    FILE **f;

    nbread = DEFAULTNBREAD;
    f = malloc(sizeof(FILE *));

    while ((arg = getopt(argc, argv, "i:n:")) != -1) {
        switch (arg) {
        case 'i':
            f = process_filenames(optarg, &nbfile, f);
            break;
        case 'n':
            nbread = strtol(optarg, NULL, 10);
            if (nbread < 1)
                nbread = DEFAULTNBREAD;
            break;
        }
    }
    rbh_init(argv[0]);
    /*calling the main program*/
    for (n = 0; n < nbfile; n++) {
        argt = malloc(sizeof(targs));
        argt->f = f[n];
        argt->nbread = nbread;
        reader(argt);
    }
//rendre reader "threadble"
    return EXIT_SUCCESS;
}
