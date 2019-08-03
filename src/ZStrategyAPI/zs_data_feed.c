
#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_utils.h>

#include "zs_api_object.h"

#include "zs_data_feed.h"


static int default_index_table[128] = { 0 };

static zs_field_item_t field_table[] = {
    { "open",   ZS_FT_Open },
    { "high",   ZS_FT_High },
    { "low",    ZS_FT_Low },
    { "close",  ZS_FT_Close },
    { NULL, ZS_FT_Unknown }
};

static ZSFieldType zs_field_type_find(const char* field_name, int len)
{
    for (int i = 0; field_table[i].field_name; ++i)
    {
        if (strncmp(field_table[i].field_name, field_name, len) == 0)
            return field_table[i].field_type;
    }
    return ZS_FT_Unknown;
}

static int _parse_line_tick(zs_tick_t* tick, zditem_t fields[], int size)
{
    int* dit = default_index_table;

    tick->LastPrice     = atof(fields[dit[ZS_FT_Last]].ptr);
    tick->OpenPrice     = atof(fields[dit[ZS_FT_Open]].ptr);
    tick->HighPrice     = atof(fields[dit[ZS_FT_High]].ptr);
    tick->LowPrice      = atof(fields[dit[ZS_FT_Low]].ptr);
    tick->ClosePrice    = atof(fields[dit[ZS_FT_Close]].ptr);

    return 0;
}

#define conv_double(dst, field) \
    if (dit[field] > 0) { \
        (dst) = atof(fields[dit[field]].ptr); }

static int _parse_line_bar(zs_bar_t* bar, zditem_t fields[], int size)
{
    int* dit = default_index_table;

    conv_double(bar->OpenPrice, ZS_FT_Open);
    conv_double(bar->HighPrice, ZS_FT_High);
    // bar->OpenPrice  = atof(fields[dit[ZS_FT_Open]].ptr);
    // bar->HighPrice  = atof(fields[dit[ZS_FT_High]].ptr);
    bar->LowPrice   = atof(fields[dit[ZS_FT_Low]].ptr);
    bar->ClosePrice = atof(fields[dit[ZS_FT_Close]].ptr);

    return 0;
}

typedef struct  
{
    const char* inner_name;
    const char* column_name;
    ZSFieldType field_type;
}_zs_name_pair_t;

int zs_on_header_line(zs_csv_loader_t* csv_loader, char* line, int len)
{
    int count, name_count;

    if (!csv_loader->field_names_map) {
        return 0;
    }

    name_count = 0;
    _zs_name_pair_t name_pairs[256] = { 0 };
    zditem_t items[256] = { 0 };

    // 解析配置内容
    ZSFieldType field_type;
    zditem_t name_items[256] = { 0 };
    len = (int)strlen(csv_loader->field_names_map);
    count = str_delimiter_ex(csv_loader->field_names_map, len, name_items, 256, csv_loader->sep);
    for (int i = 0; i < count; ++i)
    {
        zditem_t* item = &name_items[i];
        char* pcur = strstr(item->ptr, "=");
        if (!pcur) {
            // error config
            fprintf(stderr, "error config names for %s\n", item->ptr);
            break;
        }

        char* inner_name = item->ptr;
        char* column_name = pcur + 1;
        field_type = zs_field_type_find(inner_name, (int)(pcur - inner_name));
        if (field_type == ZS_FT_Unknown) {
            // warn, we donot define this field yet
            continue;
        }

        name_pairs[name_count].inner_name  = inner_name;
        name_pairs[name_count].column_name = column_name;
        name_pairs[name_count].field_type  = field_type;
        name_count++;
    }

    // 解析头部内容
    count = str_delimiter_ex(line, len, items, 256, csv_loader->sep);
    for (int col = 0; col < count; ++col)
    {
        field_type = ZS_FT_Unknown;
        zditem_t* item = &items[col];

        // get field type by the column name
        for (int k = 0; name_pairs[k].inner_name; ++k)
        {
            if (strncmp(name_pairs[k].column_name, item->ptr, item->len) == 0)
            {
                // keep the column name's index
                field_type = name_pairs[k].field_type;
                csv_loader->index_table[field_type] = col;
                break;
            }
        }
    }
    return 0;
}

int zs_on_line_default(zs_csv_loader_t* csv_loader, int num, char* line, int len)
{
    int count;
    zditem_t items[256] = { 0 };
    count = str_delimiter_ex(line, len, items, 256, csv_loader->sep);

    if (csv_loader->parse_line_fields) {
        csv_loader->parse_line_fields(csv_loader, num, items, count);
    }

#if 0
    int* table = csv_loader->index_table;
    zs_bar_t    bar = { 0 };
    zs_bar_t*   pbar = &bar;

    // FIXE: we no need use for(...)， since we mannuly retrieve each field
    for (int i = 0; i < count; ++i)
    {
        zditem_t* item = &items[i];

        if (csv_loader->is_tick_data)
        {
            // 
        }
        else 
        {
            if (csv_loader->alloc_ptr)
            {
                pbar = csv_loader->alloc_ptr(csv_loader, sizeof(zs_bar_t));
                if (!pbar) {
                    break;
                }
            }

            strcpy(pbar->Symbol, table[ZS_FT_Open]);
            pbar->OpenPrice = atof(table[ZS_FT_Open]);
            pbar->HighPrice = atof(table[ZS_FT_High]);
            pbar->LowPrice  = atof( table[ZS_FT_Low]);
            pbar->ClosePrice = atof(table[ZS_FT_Close]);

            // todo: develop a smart time parser ?

            if (csv_loader->on_bar)
                csv_loader->on_bar(csv_loader, pbar);
        }
    }
#endif

    return ZS_OK;
}

/// core impl
int zs_data_load_csv(zs_csv_loader_t* csv_loader)
{
    // read file content
    FILE*    fp;
    char*    buf;
    uint32_t filesz, readb, len;

    // some faults
    if (csv_loader->sep[0] == '\0')
        csv_loader->sep = ",";
    if (!csv_loader->on_line)
        csv_loader->on_line = zs_on_line_default;

    fp = fopen(csv_loader->filename, "rb");
    if (!fp) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    filesz = ftell(fp);
    buf = malloc(ztl_align(filesz + 1, 64));
    if (!buf)
    {
        printf("could not allocate memory %d bytes\n", filesz);
        fclose(fp);
        return -2;
    }

    fseek(fp, 0, SEEK_SET);
    readb = (uint32_t)fread(buf, 1, filesz, fp);
    fclose(fp);

    // iterate each line
    char* pcur = buf;
    char* pend = NULL;

    // if have header line
    if (csv_loader->have_header)
    {
        // process header line
        pend = strchr(pcur, '\n');
        if (!pend) {
            // error content
            return -1;
        }

        len = (uint32_t)(pend - pcur);
        if (*(pend - 1) == '\r')
            len -= 1;

        zs_on_header_line(csv_loader, pcur, len);
        pcur = pend + 1;
    }

    // content lines
    int num = 0;
    do 
    {
        pend = strchr(pcur, '\n');
        if (!pend) {
            break;
        }

        len = (uint32_t)(pend - pcur);
        if (*(pend - 1) == '\r')
            len -= 1;

        // got line pcur
        csv_loader->on_line(csv_loader, num, pcur, len);

        pcur = pend + 1;
        num += 1;
    } while (pcur);

#if 0
    // demo init a table
    default_index_table[ZS_FT_Open]     = 0;
    default_index_table[ZS_FT_High]     = 2;
    default_index_table[ZS_FT_Low]      = 1;
    default_index_table[ZS_FT_Close]    = 3;

    memcpy(csv_loader->index_table, default_index_table, sizeof(default_index_table));

    // 读取文件内容:
    char* header[] = { "OpenPrice", "HighPrice", "LowPrice", "ClosePrice", NULL };
    char* buffer[] = { "2.0", "4.0", "3.0", "2.5" };

    // TODO: 根据header信息解析头部的索引

    // 解析内容时，取各个字段

    // 对象内部分配
    zs_bar_t bar = { 0 };
    zs_bar_t* pbar = NULL;
    if (csv_loader->alloc_ptr)
        pbar = (zs_bar_t*)csv_loader->alloc_ptr(csv_loader->userdata, sizeof(zs_bar_t));
    else
        pbar = &bar;

    int32_t* pit = csv_loader->index_table;

    // 内容获取
    if (csv_loader->parse_line_bar)
    {
        // 由上层的回调函数来解析文件内容
        zditem_t fields[256] = { 0 };
        // fields pointed to the result by str_delemeter
        csv_loader->parse_line_bar(pbar, fields, 0);
    }
    else
    {
        // 由平台自己根据index_table解析文件内容
        strcpy(bar.Symbol, buffer[pit[ZS_FT_Name]]);
        pbar->OpenPrice = atof(buffer[pit[ZS_FT_Open]]);
        pbar->HighPrice = atof(buffer[pit[ZS_FT_High]]);
        pbar->LowPrice = atof(buffer[pit[ZS_FT_Low]]);
        pbar->ClosePrice = atof(buffer[pit[ZS_FT_Close]]);
    }

    // 解析完成，通知上层
    if (csv_loader->on_bar)
        csv_loader->on_bar(csv_loader, pbar);
#endif//0

    return 0;
}
