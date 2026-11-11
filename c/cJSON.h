/*
 * cJSON - Minimal JSON library for the Ipsofactura C SDK
 * Based on cJSON by Dave Gamble (MIT License)
 *
 * Provides enough functionality for the SDK:
 *   create, add, parse, print, free, query.
 */

#ifndef CJSON_H
#define CJSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* cJSON Types */
#define cJSON_False   0
#define cJSON_True    1
#define cJSON_NULL    2
#define cJSON_Number  3
#define cJSON_String  4
#define cJSON_Array   5
#define cJSON_Object  6
#define cJSON_Raw     7  /* raw JSON string */

#define cJSON_IsReference    256
#define cJSON_StringIsConst  512

/* The cJSON structure */
typedef struct cJSON {
    struct cJSON *next;    /* next/prev allow you to walk array/object chains */
    struct cJSON *prev;
    struct cJSON *child;   /* An array or object item will have a child pointer */
    int    type;           /* The type of the item, as above */
    char  *valuestring;    /* The item's string, if type==cJSON_String */
    int    valueint;       /* The item's number, if type==cJSON_Number (deprecated, use valuedouble) */
    double valuedouble;    /* The item's number, if type==cJSON_Number */
    char  *string;         /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} cJSON;

/* ── Memory allocation hooks ── */
typedef struct cJSON_Hooks {
    void *(*malloc_fn)(size_t sz);
    void  (*free_fn)(void *ptr);
} cJSON_Hooks;

void cJSON_InitHooks(cJSON_Hooks *hooks);

/* ── Parsing ── */
cJSON *cJSON_Parse(const char *value);
cJSON *cJSON_ParseWithLength(const char *value, size_t buffer_length);

/* ── Printing ── */
char  *cJSON_Print(const cJSON *item);
char  *cJSON_PrintUnformatted(const cJSON *item);
void   cJSON_free(void *object);

/* ── Deletion ── */
void   cJSON_Delete(cJSON *item);

/* ── Type checkers (return non-zero for true) ── */
int cJSON_IsInvalid (const cJSON *item);
int cJSON_IsFalse   (const cJSON *item);
int cJSON_IsTrue    (const cJSON *item);
int cJSON_IsBool    (const cJSON *item);
int cJSON_IsNull    (const cJSON *item);
int cJSON_IsNumber  (const cJSON *item);
int cJSON_IsString  (const cJSON *item);
int cJSON_IsArray   (const cJSON *item);
int cJSON_IsObject  (const cJSON *item);
int cJSON_IsRaw     (const cJSON *item);

/* ── Creators ── */
cJSON *cJSON_CreateNull   (void);
cJSON *cJSON_CreateTrue   (void);
cJSON *cJSON_CreateFalse  (void);
cJSON *cJSON_CreateBool   (int boolean);
cJSON *cJSON_CreateNumber (double num);
cJSON *cJSON_CreateString (const char *string);
cJSON *cJSON_CreateRaw    (const char *raw);
cJSON *cJSON_CreateArray  (void);
cJSON *cJSON_CreateObject (void);

/* ── Array helpers ── */
int    cJSON_GetArraySize (const cJSON *array);
cJSON *cJSON_GetArrayItem (const cJSON *array, int index);

/* ── Object query ── */
cJSON *cJSON_GetObjectItem             (const cJSON *object, const char *string);
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string);
int    cJSON_HasObjectItem             (const cJSON *object, const char *string);

/* ── String value accessor ── */
char  *cJSON_GetStringValue(const cJSON *item);
double cJSON_GetNumberValue(const cJSON *item);

/* ── Mutators: add to object ── */
cJSON *cJSON_AddNullToObject    (cJSON *object, const char *name);
cJSON *cJSON_AddTrueToObject    (cJSON *object, const char *name);
cJSON *cJSON_AddFalseToObject   (cJSON *object, const char *name);
cJSON *cJSON_AddBoolToObject    (cJSON *object, const char *name, int boolean);
cJSON *cJSON_AddNumberToObject  (cJSON *object, const char *name, double number);
cJSON *cJSON_AddStringToObject  (cJSON *object, const char *name, const char *string);
cJSON *cJSON_AddRawToObject     (cJSON *object, const char *name, const char *raw);
cJSON *cJSON_AddObjectToObject  (cJSON *object, const char *name);
cJSON *cJSON_AddArrayToObject   (cJSON *object, const char *name);

/* ── Mutators: add to array ── */
void cJSON_AddItemToArray (cJSON *array,  cJSON *item);
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);

/* ── Duplicate ── */
cJSON *cJSON_Duplicate(const cJSON *item, int recurse);

/* ── Utility ── */
const char *cJSON_GetErrorPtr(void);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_H */
