/*
 * cJSON - Minimal JSON library for the Ipsofactura C SDK
 * Based on cJSON by Dave Gamble (MIT License)
 */

#include "cJSON.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>

/* ── Memory management ─────────────────────────────────────────────────────── */

static void *(*cjson_malloc)(size_t sz) = malloc;
static void  (*cjson_free)(void *ptr)   = free;

void cJSON_InitHooks(cJSON_Hooks *hooks)
{
    if (!hooks) {
        cjson_malloc = malloc;
        cjson_free   = free;
        return;
    }
    cjson_malloc = hooks->malloc_fn ? hooks->malloc_fn : malloc;
    cjson_free   = hooks->free_fn   ? hooks->free_fn   : free;
}

void cJSON_free(void *object)
{
    cjson_free(object);
}

/* ── Error pointer ─────────────────────────────────────────────────────────── */

static const char *global_error = NULL;

const char *cJSON_GetErrorPtr(void)
{
    return global_error;
}

/* ── Internal helpers ──────────────────────────────────────────────────────── */

static cJSON *cJSON_New_Item(void)
{
    cJSON *node = (cJSON *)cjson_malloc(sizeof(cJSON));
    if (node) memset(node, 0, sizeof(cJSON));
    return node;
}

static char *cJSON_strdup(const char *str)
{
    size_t len;
    char  *copy;
    if (!str) return NULL;
    len  = strlen(str) + 1;
    copy = (char *)cjson_malloc(len);
    if (!copy) return NULL;
    memcpy(copy, str, len);
    return copy;
}

/* ── Deletion ──────────────────────────────────────────────────────────────── */

void cJSON_Delete(cJSON *item)
{
    cJSON *next;
    while (item) {
        next = item->next;
        if (!(item->type & cJSON_IsReference) && item->child) {
            cJSON_Delete(item->child);
        }
        if (!(item->type & cJSON_IsReference) && item->valuestring) {
            cjson_free(item->valuestring);
        }
        if (!(item->type & cJSON_StringIsConst) && item->string) {
            cjson_free(item->string);
        }
        cjson_free(item);
        item = next;
    }
}

/* ── Type checkers ─────────────────────────────────────────────────────────── */

int cJSON_IsInvalid(const cJSON *item) { return item && (item->type & 0xFF) == 0; }
int cJSON_IsFalse  (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_False; }
int cJSON_IsTrue   (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_True; }
int cJSON_IsBool   (const cJSON *item) { return item && ((item->type & 0xFF) == cJSON_True || (item->type & 0xFF) == cJSON_False); }
int cJSON_IsNull   (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_NULL; }
int cJSON_IsNumber (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_Number; }
int cJSON_IsString (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_String; }
int cJSON_IsArray  (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_Array; }
int cJSON_IsObject (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_Object; }
int cJSON_IsRaw    (const cJSON *item) { return item && (item->type & 0xFF) == cJSON_Raw; }

/* ── Creators ──────────────────────────────────────────────────────────────── */

cJSON *cJSON_CreateNull(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_NULL;
    return item;
}

cJSON *cJSON_CreateTrue(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_True;
    return item;
}

cJSON *cJSON_CreateFalse(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_False;
    return item;
}

cJSON *cJSON_CreateBool(int boolean)
{
    return boolean ? cJSON_CreateTrue() : cJSON_CreateFalse();
}

cJSON *cJSON_CreateNumber(double num)
{
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type        = cJSON_Number;
        item->valuedouble = num;
        item->valueint    = (num <= INT_MAX && num >= INT_MIN) ? (int)num : 0;
    }
    return item;
}

cJSON *cJSON_CreateString(const char *string)
{
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type        = cJSON_String;
        item->valuestring = cJSON_strdup(string);
        if (!item->valuestring) { cjson_free(item); return NULL; }
    }
    return item;
}

cJSON *cJSON_CreateRaw(const char *raw)
{
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type        = cJSON_Raw;
        item->valuestring = cJSON_strdup(raw);
        if (!item->valuestring) { cjson_free(item); return NULL; }
    }
    return item;
}

cJSON *cJSON_CreateArray(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_Array;
    return item;
}

cJSON *cJSON_CreateObject(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_Object;
    return item;
}

/* ── Add to array/object ───────────────────────────────────────────────────── */

static void suffix_object(cJSON *prev, cJSON *item)
{
    prev->next = item;
    item->prev = prev;
}

static cJSON *get_last(cJSON *first)
{
    while (first && first->next) first = first->next;
    return first;
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    cJSON *last;
    if (!array || !item || array == item) return;
    last = get_last(array->child);
    if (!last) {
        array->child = item;
        item->prev   = NULL;
        item->next   = NULL;
    } else {
        suffix_object(last, item);
    }
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    char *key;
    if (!object || !string || !item) return;

    /* free old key if replacing */
    if (item->string && !(item->type & cJSON_StringIsConst)) {
        cjson_free(item->string);
    }
    key = cJSON_strdup(string);
    if (!key) return;
    item->string = key;
    item->type  &= ~cJSON_StringIsConst;

    cJSON_AddItemToArray(object, item);
}

/* ── Object sugar helpers ──────────────────────────────────────────────────── */

cJSON *cJSON_AddNullToObject(cJSON *object, const char *name)
{
    cJSON *n = cJSON_CreateNull();
    if (cJSON_AddItemToObject(object, name, n), n) return n;
    cJSON_Delete(n); return NULL;
}

cJSON *cJSON_AddTrueToObject(cJSON *object, const char *name)
{
    cJSON *n = cJSON_CreateTrue();
    cJSON_AddItemToObject(object, name, n); return n;
}

cJSON *cJSON_AddFalseToObject(cJSON *object, const char *name)
{
    cJSON *n = cJSON_CreateFalse();
    cJSON_AddItemToObject(object, name, n); return n;
}

cJSON *cJSON_AddBoolToObject(cJSON *object, const char *name, int boolean)
{
    cJSON *n = cJSON_CreateBool(boolean);
    cJSON_AddItemToObject(object, name, n); return n;
}

cJSON *cJSON_AddNumberToObject(cJSON *object, const char *name, double number)
{
    cJSON *n = cJSON_CreateNumber(number);
    cJSON_AddItemToObject(object, name, n); return n;
}

cJSON *cJSON_AddStringToObject(cJSON *object, const char *name, const char *string)
{
    cJSON *n = cJSON_CreateString(string);
    cJSON_AddItemToObject(object, name, n); return n;
}

cJSON *cJSON_AddRawToObject(cJSON *object, const char *name, const char *raw)
{
    cJSON *n = cJSON_CreateRaw(raw);
    cJSON_AddItemToObject(object, name, n); return n;
}

cJSON *cJSON_AddObjectToObject(cJSON *object, const char *name)
{
    cJSON *n = cJSON_CreateObject();
    cJSON_AddItemToObject(object, name, n); return n;
}

cJSON *cJSON_AddArrayToObject(cJSON *object, const char *name)
{
    cJSON *n = cJSON_CreateArray();
    cJSON_AddItemToObject(object, name, n); return n;
}

/* ── Array access ──────────────────────────────────────────────────────────── */

int cJSON_GetArraySize(const cJSON *array)
{
    cJSON *c = array ? array->child : NULL;
    int i = 0;
    while (c) { i++; c = c->next; }
    return i;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index)
{
    cJSON *c;
    if (!array || index < 0) return NULL;
    c = array->child;
    while (c && index-- > 0) c = c->next;
    return c;
}

/* ── Object access ─────────────────────────────────────────────────────────── */

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string)
{
    cJSON *c;
    if (!object || !string) return NULL;
    c = object->child;
    while (c && c->string && strcasecmp(c->string, string) != 0) c = c->next;
    return c;
}

cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string)
{
    cJSON *c;
    if (!object || !string) return NULL;
    c = object->child;
    while (c && c->string && strcmp(c->string, string) != 0) c = c->next;
    return c;
}

int cJSON_HasObjectItem(const cJSON *object, const char *string)
{
    return cJSON_GetObjectItem(object, string) ? 1 : 0;
}

/* ── Value accessors ───────────────────────────────────────────────────────── */

char *cJSON_GetStringValue(const cJSON *item)
{
    if (!cJSON_IsString(item)) return NULL;
    return item->valuestring;
}

double cJSON_GetNumberValue(const cJSON *item)
{
    if (!cJSON_IsNumber(item)) return (double)NAN;
    return item->valuedouble;
}

/* ── Duplicate ─────────────────────────────────────────────────────────────── */

cJSON *cJSON_Duplicate(const cJSON *item, int recurse)
{
    cJSON *newitem, *cptr, *nptr, *newchild;
    if (!item) return NULL;
    newitem = cJSON_New_Item();
    if (!newitem) return NULL;
    newitem->type        = item->type & ~cJSON_IsReference;
    newitem->valueint    = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring) {
        newitem->valuestring = cJSON_strdup(item->valuestring);
        if (!newitem->valuestring) { cJSON_Delete(newitem); return NULL; }
    }
    if (item->string) {
        newitem->string = cJSON_strdup(item->string);
        if (!newitem->string) { cJSON_Delete(newitem); return NULL; }
    }
    if (!recurse) return newitem;
    cptr = item->child;
    nptr = NULL;
    while (cptr) {
        newchild = cJSON_Duplicate(cptr, 1);
        if (!newchild) { cJSON_Delete(newitem); return NULL; }
        if (!nptr) {
            newitem->child = newchild;
        } else {
            suffix_object(nptr, newchild);
        }
        nptr = newchild;
        cptr = cptr->next;
    }
    return newitem;
}

/* ══════════════════════════════════════════════════════════════════════════════
 * PARSER
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *content;
    size_t      length;
    size_t      offset;
    size_t      depth;
} parse_buffer;

#define can_read(b, l) (((b)->offset + (l)) <= (b)->length)
#define peek(b)        ((b)->content[(b)->offset])
#define advance(b)     ((b)->offset++)

static void skip_whitespace(parse_buffer *buf)
{
    while (can_read(buf, 1) && isspace((unsigned char)peek(buf))) advance(buf);
}

static cJSON *parse_value(parse_buffer *buf);

static cJSON *parse_string(parse_buffer *buf)
{
    cJSON *item;
    char  *out, *o;
    size_t alloc_len = 0;
    size_t start;

    if (!can_read(buf, 1) || peek(buf) != '"') {
        global_error = buf->content + buf->offset;
        return NULL;
    }
    advance(buf); /* skip opening " */
    start = buf->offset;

    /* first pass: measure */
    while (buf->offset < buf->length) {
        char c = peek(buf);
        if (c == '"') break;
        if (c == '\\') {
            advance(buf);
            if (!can_read(buf, 1)) { global_error = buf->content + buf->offset; return NULL; }
            if (peek(buf) == 'u') {
                advance(buf);
                /* skip 4 hex digits — simplified: just count them */
                if (!can_read(buf, 4)) { global_error = buf->content + buf->offset; return NULL; }
                buf->offset += 4;
                alloc_len += 4; /* over-allocate for multi-byte */
                continue;
            }
        }
        advance(buf);
        alloc_len++;
    }
    if (!can_read(buf, 1) || peek(buf) != '"') {
        global_error = buf->content + buf->offset;
        return NULL;
    }

    alloc_len += 4; /* safety margin */

    /* second pass: build output */
    out = (char *)cjson_malloc(alloc_len + 1);
    if (!out) return NULL;
    o   = out;

    buf->offset = start;
    while (buf->offset < buf->length) {
        char c = peek(buf);
        if (c == '"') break;
        if (c != '\\') {
            *o++ = c;
            advance(buf);
        } else {
            advance(buf); /* skip backslash */
            if (!can_read(buf, 1)) { cjson_free(out); return NULL; }
            switch (peek(buf)) {
                case '"' : *o++ = '"';  break;
                case '\\': *o++ = '\\'; break;
                case '/' : *o++ = '/';  break;
                case 'b' : *o++ = '\b'; break;
                case 'f' : *o++ = '\f'; break;
                case 'n' : *o++ = '\n'; break;
                case 'r' : *o++ = '\r'; break;
                case 't' : *o++ = '\t'; break;
                case 'u' : {
                    /* basic BMP Unicode: read 4 hex digits */
                    unsigned int codepoint = 0;
                    int i;
                    advance(buf); /* skip 'u' */
                    for (i = 0; i < 4; i++) {
                        char hc = peek(buf);
                        codepoint <<= 4;
                        if (hc >= '0' && hc <= '9') codepoint += hc - '0';
                        else if (hc >= 'a' && hc <= 'f') codepoint += hc - 'a' + 10;
                        else if (hc >= 'A' && hc <= 'F') codepoint += hc - 'A' + 10;
                        advance(buf);
                    }
                    /* encode as UTF-8 */
                    if (codepoint < 0x80) {
                        *o++ = (char)codepoint;
                    } else if (codepoint < 0x800) {
                        *o++ = (char)(0xC0 | (codepoint >> 6));
                        *o++ = (char)(0x80 | (codepoint & 0x3F));
                    } else {
                        *o++ = (char)(0xE0 | (codepoint >> 12));
                        *o++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                        *o++ = (char)(0x80 | (codepoint & 0x3F));
                    }
                    continue; /* already advanced */
                }
                default: *o++ = peek(buf); break;
            }
            advance(buf);
        }
    }
    *o = '\0';
    advance(buf); /* skip closing " */

    item = cJSON_New_Item();
    if (!item) { cjson_free(out); return NULL; }
    item->type        = cJSON_String;
    item->valuestring = out;
    return item;
}

static cJSON *parse_number(parse_buffer *buf)
{
    cJSON *item;
    double number;
    char  tmp[64];
    size_t i = 0;

    while (can_read(buf, 1) && i < 63) {
        char c = peek(buf);
        if (c == '-' || c == '+' || isdigit((unsigned char)c) || c == '.' || c == 'e' || c == 'E') {
            tmp[i++] = c;
            advance(buf);
        } else {
            break;
        }
    }
    tmp[i] = '\0';
    number = strtod(tmp, NULL);

    item = cJSON_New_Item();
    if (!item) return NULL;
    item->type        = cJSON_Number;
    item->valuedouble = number;
    item->valueint    = (number <= INT_MAX && number >= INT_MIN) ? (int)number : 0;
    return item;
}

static cJSON *parse_array(parse_buffer *buf)
{
    cJSON *array, *child, *last_child = NULL;

    if (!can_read(buf, 1) || peek(buf) != '[') {
        global_error = buf->content + buf->offset;
        return NULL;
    }
    advance(buf); /* skip '[' */

    array = cJSON_CreateArray();
    if (!array) return NULL;

    skip_whitespace(buf);
    if (can_read(buf, 1) && peek(buf) == ']') {
        advance(buf);
        return array;
    }

    while (can_read(buf, 1)) {
        skip_whitespace(buf);
        child = parse_value(buf);
        if (!child) { cJSON_Delete(array); return NULL; }
        if (!last_child) {
            array->child = child;
        } else {
            suffix_object(last_child, child);
        }
        last_child = child;

        skip_whitespace(buf);
        if (!can_read(buf, 1)) break;
        if (peek(buf) == ']') { advance(buf); return array; }
        if (peek(buf) != ',') { global_error = buf->content + buf->offset; cJSON_Delete(array); return NULL; }
        advance(buf); /* skip ',' */
    }

    cJSON_Delete(array);
    return NULL;
}

static cJSON *parse_object(parse_buffer *buf)
{
    cJSON *object, *child, *last_child = NULL;
    cJSON *key;

    if (!can_read(buf, 1) || peek(buf) != '{') {
        global_error = buf->content + buf->offset;
        return NULL;
    }
    advance(buf); /* skip '{' */

    object = cJSON_CreateObject();
    if (!object) return NULL;

    skip_whitespace(buf);
    if (can_read(buf, 1) && peek(buf) == '}') {
        advance(buf);
        return object;
    }

    while (can_read(buf, 1)) {
        skip_whitespace(buf);

        /* key */
        key = parse_string(buf);
        if (!key) { cJSON_Delete(object); return NULL; }

        skip_whitespace(buf);
        if (!can_read(buf, 1) || peek(buf) != ':') {
            cJSON_Delete(key); cJSON_Delete(object); return NULL;
        }
        advance(buf); /* skip ':' */
        skip_whitespace(buf);

        /* value */
        child = parse_value(buf);
        if (!child) { cJSON_Delete(key); cJSON_Delete(object); return NULL; }

        /* attach key as child->string */
        child->string = key->valuestring;
        key->valuestring = NULL;
        cJSON_Delete(key);

        if (!last_child) {
            object->child = child;
        } else {
            suffix_object(last_child, child);
        }
        last_child = child;

        skip_whitespace(buf);
        if (!can_read(buf, 1)) break;
        if (peek(buf) == '}') { advance(buf); return object; }
        if (peek(buf) != ',') { global_error = buf->content + buf->offset; cJSON_Delete(object); return NULL; }
        advance(buf); /* skip ',' */
    }

    cJSON_Delete(object);
    return NULL;
}

static cJSON *parse_value(parse_buffer *buf)
{
    skip_whitespace(buf);
    if (!can_read(buf, 1)) return NULL;

    switch (peek(buf)) {
        case 'n':
            if (can_read(buf, 4) && strncmp(buf->content + buf->offset, "null", 4) == 0) {
                buf->offset += 4;
                return cJSON_CreateNull();
            }
            break;
        case 't':
            if (can_read(buf, 4) && strncmp(buf->content + buf->offset, "true", 4) == 0) {
                buf->offset += 4;
                return cJSON_CreateTrue();
            }
            break;
        case 'f':
            if (can_read(buf, 5) && strncmp(buf->content + buf->offset, "false", 5) == 0) {
                buf->offset += 5;
                return cJSON_CreateFalse();
            }
            break;
        case '"':
            return parse_string(buf);
        case '[':
            return parse_array(buf);
        case '{':
            return parse_object(buf);
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number(buf);
        default:
            break;
    }
    global_error = buf->content + buf->offset;
    return NULL;
}

cJSON *cJSON_ParseWithLength(const char *value, size_t buffer_length)
{
    parse_buffer buf;
    cJSON *item;

    if (!value) return NULL;
    global_error = NULL;

    buf.content = value;
    buf.length  = buffer_length;
    buf.offset  = 0;
    buf.depth   = 0;

    item = parse_value(&buf);
    return item;
}

cJSON *cJSON_Parse(const char *value)
{
    if (!value) return NULL;
    return cJSON_ParseWithLength(value, strlen(value));
}

/* ══════════════════════════════════════════════════════════════════════════════
 * PRINTER
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char  *buffer;
    size_t length;
    size_t offset;
    int    format;
} print_buffer;

static int ensure_capacity(print_buffer *p, size_t needed)
{
    char  *newbuf;
    size_t newlen;
    if (p->offset + needed < p->length) return 1;
    newlen = p->length + needed + 256;
    newbuf = (char *)cjson_malloc(newlen);
    if (!newbuf) return 0;
    memcpy(newbuf, p->buffer, p->offset);
    cjson_free(p->buffer);
    p->buffer = newbuf;
    p->length = newlen;
    return 1;
}

#define APPEND(pb, str, len) do { \
    if (!ensure_capacity((pb), (len))) return 0; \
    memcpy((pb)->buffer + (pb)->offset, (str), (len)); \
    (pb)->offset += (len); \
} while (0)

#define APPENDC(pb, c) do { \
    if (!ensure_capacity((pb), 1)) return 0; \
    (pb)->buffer[(pb)->offset++] = (c); \
} while (0)

static int print_item(const cJSON *item, print_buffer *p, int depth);

static int print_string_ptr(const char *str, print_buffer *p)
{
    const char *c;
    char  buf[8];
    size_t len;

    APPENDC(p, '"');
    if (!str) { APPENDC(p, '"'); return 1; }

    for (c = str; *c; c++) {
        unsigned char uc = (unsigned char)*c;
        if (uc == '"')       { APPEND(p, "\\\"", 2); }
        else if (uc == '\\') { APPEND(p, "\\\\", 2); }
        else if (uc == '\b') { APPEND(p, "\\b",  2); }
        else if (uc == '\f') { APPEND(p, "\\f",  2); }
        else if (uc == '\n') { APPEND(p, "\\n",  2); }
        else if (uc == '\r') { APPEND(p, "\\r",  2); }
        else if (uc == '\t') { APPEND(p, "\\t",  2); }
        else if (uc < 0x20) {
            len = (size_t)snprintf(buf, sizeof(buf), "\\u%04x", uc);
            APPEND(p, buf, len);
        } else {
            APPENDC(p, (char)uc);
        }
    }
    APPENDC(p, '"');
    return 1;
}

static int print_number(const cJSON *item, print_buffer *p)
{
    char buf[64];
    double d = item->valuedouble;
    size_t len;

    if (d != d) {
        len = (size_t)snprintf(buf, sizeof(buf), "null");
    } else if (d == (double)(int)d && fabs(d) < 1.0e15) {
        len = (size_t)snprintf(buf, sizeof(buf), "%.0f", d);
    } else {
        len = (size_t)snprintf(buf, sizeof(buf), "%g", d);
    }
    APPEND(p, buf, len);
    return 1;
}

static int print_item(const cJSON *item, print_buffer *p, int depth)
{
    cJSON *child;
    int    first = 1;
    char   indent[64];
    int    indent_len;

    if (!item) return 0;

    switch (item->type & 0xFF) {
        case cJSON_NULL:
            APPEND(p, "null", 4); break;
        case cJSON_False:
            APPEND(p, "false", 5); break;
        case cJSON_True:
            APPEND(p, "true", 4); break;
        case cJSON_Number:
            if (!print_number(item, p)) return 0; break;
        case cJSON_String:
        case cJSON_Raw:
            if (!print_string_ptr(item->valuestring, p)) return 0; break;
        case cJSON_Array:
            APPENDC(p, '[');
            if (p->format && item->child) {
                APPENDC(p, '\n');
                indent_len = (depth + 1) * 2;
                if (indent_len > 62) indent_len = 62;
                memset(indent, ' ', (size_t)indent_len);
                indent[indent_len] = '\0';
            }
            child = item->child;
            while (child) {
                if (!first) {
                    APPENDC(p, ',');
                    if (p->format) APPENDC(p, '\n');
                }
                first = 0;
                if (p->format) APPEND(p, indent, (size_t)indent_len);
                if (!print_item(child, p, depth + 1)) return 0;
                child = child->next;
            }
            if (p->format && item->child) {
                APPENDC(p, '\n');
                indent_len = depth * 2;
                if (indent_len > 62) indent_len = 62;
                memset(indent, ' ', (size_t)indent_len);
                indent[indent_len] = '\0';
                APPEND(p, indent, (size_t)indent_len);
            }
            APPENDC(p, ']');
            break;
        case cJSON_Object:
            APPENDC(p, '{');
            if (p->format && item->child) {
                APPENDC(p, '\n');
                indent_len = (depth + 1) * 2;
                if (indent_len > 62) indent_len = 62;
                memset(indent, ' ', (size_t)indent_len);
                indent[indent_len] = '\0';
            }
            child = item->child;
            while (child) {
                if (!first) {
                    APPENDC(p, ',');
                    if (p->format) APPENDC(p, '\n');
                }
                first = 0;
                if (p->format) APPEND(p, indent, (size_t)indent_len);
                if (!print_string_ptr(child->string ? child->string : "", p)) return 0;
                APPENDC(p, ':');
                if (p->format) APPENDC(p, ' ');
                if (!print_item(child, p, depth + 1)) return 0;
                child = child->next;
            }
            if (p->format && item->child) {
                APPENDC(p, '\n');
                indent_len = depth * 2;
                if (indent_len > 62) indent_len = 62;
                memset(indent, ' ', (size_t)indent_len);
                indent[indent_len] = '\0';
                APPEND(p, indent, (size_t)indent_len);
            }
            APPENDC(p, '}');
            break;
        default:
            return 0;
    }
    return 1;
}

static char *print_internal(const cJSON *item, int format)
{
    print_buffer pb;
    pb.length = 256;
    pb.offset = 0;
    pb.format = format;
    pb.buffer = (char *)cjson_malloc(pb.length);
    if (!pb.buffer) return NULL;
    if (!print_item(item, &pb, 0)) {
        cjson_free(pb.buffer);
        return NULL;
    }
    APPENDC(&pb, '\0');
    return pb.buffer;
}

char *cJSON_Print(const cJSON *item)
{
    return print_internal(item, 1);
}

char *cJSON_PrintUnformatted(const cJSON *item)
{
    return print_internal(item, 0);
}
