/*
 * ipsofactura.c — C implementation of the Ipsofactura API client.
 *
 * Dependencies: libcurl, OpenSSL (for HMAC-SHA256), cJSON (bundled).
 * Compile: cc -Wall -Wextra -O2 ipsofactura.c cJSON.c -lcurl -lssl -lcrypto -o ipsofactura.o
 *
 * On macOS, libcurl is bundled with Xcode Command Line Tools; link with -lcurl.
 * OpenSSL can be installed via Homebrew: brew install openssl
 * and linked with -I$(brew --prefix openssl)/include -L$(brew --prefix openssl)/lib -lssl -lcrypto
 */

#include "ipsofactura.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

/* ══════════════════════════════════════════════════════════════════════════════
 * Internal types
 * ══════════════════════════════════════════════════════════════════════════════ */

struct ips_client_s {
    char *api_key;
    char *base_url;
};

/* Growing string buffer for cURL response body. */
typedef struct {
    char  *data;
    size_t size;
} response_buf_t;

/* ══════════════════════════════════════════════════════════════════════════════
 * Client lifecycle
 * ══════════════════════════════════════════════════════════════════════════════ */

ips_client_t *ips_client_new(const char *api_key, const char *base_url)
{
    ips_client_t *c;
    if (!api_key || !base_url) return NULL;

    /* One-time global curl init (safe to call multiple times) */
    curl_global_init(CURL_GLOBAL_DEFAULT);

    c = (ips_client_t *)malloc(sizeof(ips_client_t));
    if (!c) return NULL;

    c->api_key  = strdup(api_key);
    c->base_url = strdup(base_url);

    if (!c->api_key || !c->base_url) {
        free(c->api_key);
        free(c->base_url);
        free(c);
        return NULL;
    }

    /* Strip trailing slash */
    {
        size_t n = strlen(c->base_url);
        while (n > 0 && c->base_url[n - 1] == '/') { c->base_url[--n] = '\0'; }
    }

    return c;
}

void ips_client_free(ips_client_t *client)
{
    if (!client) return;
    free(client->api_key);
    free(client->base_url);
    free(client);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * Error helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

void ips_error_clear(ips_error_t *err)
{
    if (!err) return;
    free(err->code);
    free(err->message);
    free(err->sat_code);
    memset(err, 0, sizeof(ips_error_t));
}

static void set_error(ips_error_t *err, int status_code, cJSON *body, const char *fallback_message)
{
    if (!err) return;
    ips_error_clear(err);
    err->status_code = status_code;

    if (body) {
        cJSON *code_node = cJSON_GetObjectItemCaseSensitive(body, "code");
        cJSON *msg_node  = cJSON_GetObjectItemCaseSensitive(body, "message");
        cJSON *sat_node  = cJSON_GetObjectItemCaseSensitive(body, "sat_code");
        cJSON *ra_node   = cJSON_GetObjectItemCaseSensitive(body, "retryAfter");

        err->code    = strdup(cJSON_IsString(code_node) ? code_node->valuestring : "UNKNOWN_ERROR");
        err->message = strdup(cJSON_IsString(msg_node)  ? msg_node->valuestring  : (fallback_message ? fallback_message : ""));
        err->sat_code    = cJSON_IsString(sat_node) ? strdup(sat_node->valuestring) : NULL;
        err->retry_after = cJSON_IsNumber(ra_node)  ? (int)ra_node->valuedouble : 0;
    } else {
        err->code    = strdup("UNKNOWN_ERROR");
        err->message = strdup(fallback_message ? fallback_message : "");
        err->sat_code    = NULL;
        err->retry_after = 0;
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 * cURL helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    response_buf_t *buf = (response_buf_t *)userp;
    char *ptr = (char *)realloc(buf->data, buf->size + realsize + 1);
    if (!ptr) return 0; /* OOM: abort */
    buf->data = ptr;
    memcpy(buf->data + buf->size, contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';
    return realsize;
}

typedef enum { HTTP_GET, HTTP_POST, HTTP_PATCH, HTTP_DELETE } http_method_t;

/*
 * Internal dispatcher: build URL, set headers, run request, parse JSON.
 * Returns parsed JSON on success, NULL on error (populates *err if non-NULL).
 */
static cJSON *do_request(
    ips_client_t   *c,
    http_method_t   method,
    const char     *path,            /* e.g. "/cfdi/timbrar" */
    const char     *query_string,    /* optional, e.g. "page=1" (no leading '?') */
    cJSON          *body,            /* optional JSON body */
    ips_error_t    *err)
{
    CURL             *curl;
    CURLcode          res;
    response_buf_t    rbuf;
    struct curl_slist *headers = NULL;
    char              url[2048];
    char              auth_header[512];
    char             *body_str = NULL;
    long              http_code = 0;
    cJSON            *parsed   = NULL;

    if (!c) return NULL;

    /* Build URL */
    if (query_string && query_string[0]) {
        snprintf(url, sizeof(url), "%s%s?%s", c->base_url, path, query_string);
    } else {
        snprintf(url, sizeof(url), "%s%s", c->base_url, path);
    }

    /* Init response buffer */
    rbuf.data = (char *)malloc(1);
    rbuf.size = 0;
    if (rbuf.data) rbuf.data[0] = '\0';

    curl = curl_easy_init();
    if (!curl) {
        free(rbuf.data);
        if (err) { ips_error_clear(err); err->status_code = 0; err->code = strdup("CURL_INIT_FAILED"); err->message = strdup("curl_easy_init failed"); }
        return NULL;
    }

    /* Headers */
    snprintf(auth_header, sizeof(auth_header), "x-api: %s", c->api_key);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    /* Body */
    if (body) {
        body_str = cJSON_PrintUnformatted(body);
    }

    /* cURL options */
    curl_easy_setopt(curl, CURLOPT_URL,            url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &rbuf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    switch (method) {
        case HTTP_GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
        case HTTP_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if (body_str) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body_str);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body_str));
            } else {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    "");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
            }
            break;
        case HTTP_PATCH:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            if (body_str) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body_str);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body_str));
            }
            break;
        case HTTP_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
    }

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        if (err) {
            ips_error_clear(err);
            err->status_code = 0;
            err->code        = strdup("CURL_ERROR");
            err->message     = strdup(curl_easy_strerror(res));
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(body_str);
        free(rbuf.data);
        return NULL;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(body_str);

    /* Parse JSON response */
    if (rbuf.data && rbuf.size > 0) {
        parsed = cJSON_Parse(rbuf.data);
    }
    free(rbuf.data);

    if (http_code >= 400) {
        set_error(err, (int)http_code, parsed, "API error");
        cJSON_Delete(parsed);
        return NULL;
    }

    return parsed ? parsed : cJSON_CreateObject();
}

/* ══════════════════════════════════════════════════════════════════════════════
 * Path builder helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

static void build_path(char *out, size_t out_size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, out_size, fmt, ap);
    va_end(ap);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * CFDI
 * ══════════════════════════════════════════════════════════════════════════════ */

cJSON *ips_cfdi_timbrar(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/cfdi/timbrar", NULL, payload, err);
}

cJSON *ips_cfdi_timbrar_nomina(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/cfdi/timbrar-nomina", NULL, payload, err);
}

cJSON *ips_cfdi_timbrar_xml(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/cfdi/timbrar-xml", NULL, payload, err);
}

cJSON *ips_cfdi_timbrar_sellado(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/cfdi/timbrar-sellado", NULL, payload, err);
}

cJSON *ips_cfdi_cancelar(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/cfdi/cancelar", NULL, payload, err);
}

cJSON *ips_cfdi_complemento_pago(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/cfdi/complemento-pago", NULL, payload, err);
}

cJSON *ips_cfdi_crear_export(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/cfdi/exports", NULL, payload, err);
}

cJSON *ips_cfdi_get_export(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/cfdi/exports/%s", id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_cfdi_list(ips_client_t *c, const char *query_string, ips_error_t *err)
{
    return do_request(c, HTTP_GET, "/cfdi", query_string, NULL, err);
}

cJSON *ips_cfdi_get(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/cfdi/%s", id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_cfdi_get_estatus(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/cfdi/%s/estatus", id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_cfdi_get_xml(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/cfdi/%s/xml", id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_cfdi_get_pdf(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/cfdi/%s/pdf", id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_cfdi_get_acuse(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/cfdi/%s/acuse", id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * EMPRESAS
 * ══════════════════════════════════════════════════════════════════════════════ */

cJSON *ips_empresas_list(ips_client_t *c, const char *query_string, ips_error_t *err)
{
    return do_request(c, HTTP_GET, "/empresas", query_string, NULL, err);
}

cJSON *ips_empresas_crear(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/empresas", NULL, payload, err);
}

cJSON *ips_empresas_get(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/empresas/%s", id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_empresas_actualizar(ips_client_t *c, const char *id, cJSON *payload, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/empresas/%s", id);
    return do_request(c, HTTP_PATCH, path, NULL, payload, err);
}

cJSON *ips_empresas_subir_certificado(ips_client_t *c, const char *empresa_id, cJSON *payload, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/empresas/%s/certificados", empresa_id);
    return do_request(c, HTTP_POST, path, NULL, payload, err);
}

cJSON *ips_empresas_refresh_password(ips_client_t *c, const char *empresa_id, cJSON *payload, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/empresas/%s/certificados/refresh-password", empresa_id);
    return do_request(c, HTTP_POST, path, NULL, payload, err);
}

cJSON *ips_empresas_eliminar_certificado(ips_client_t *c, const char *empresa_id, const char *numero, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/empresas/%s/certificados/%s", empresa_id, numero);
    return do_request(c, HTTP_DELETE, path, NULL, NULL, err);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * WEBHOOKS
 * ══════════════════════════════════════════════════════════════════════════════ */

cJSON *ips_webhooks_list(ips_client_t *c, ips_error_t *err)
{
    return do_request(c, HTTP_GET, "/webhooks", NULL, NULL, err);
}

cJSON *ips_webhooks_crear(ips_client_t *c, cJSON *payload, ips_error_t *err)
{
    return do_request(c, HTTP_POST, "/webhooks", NULL, payload, err);
}

cJSON *ips_webhooks_actualizar(ips_client_t *c, const char *id, cJSON *payload, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/webhooks/%s", id);
    return do_request(c, HTTP_PATCH, path, NULL, payload, err);
}

cJSON *ips_webhooks_eliminar(ips_client_t *c, const char *id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/webhooks/%s", id);
    return do_request(c, HTTP_DELETE, path, NULL, NULL, err);
}

cJSON *ips_webhooks_list_deliveries(ips_client_t *c, const char *webhook_id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/webhooks/%s/deliveries", webhook_id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_webhooks_get_delivery(ips_client_t *c, const char *webhook_id, const char *delivery_id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/webhooks/%s/deliveries/%s", webhook_id, delivery_id);
    return do_request(c, HTTP_GET, path, NULL, NULL, err);
}

cJSON *ips_webhooks_resend_delivery(ips_client_t *c, const char *webhook_id, const char *delivery_id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/webhooks/%s/deliveries/%s/resend", webhook_id, delivery_id);
    return do_request(c, HTTP_POST, path, NULL, NULL, err);
}

cJSON *ips_webhooks_test(ips_client_t *c, const char *webhook_id, ips_error_t *err)
{
    char path[512];
    build_path(path, sizeof(path), "/webhooks/%s/test", webhook_id);
    return do_request(c, HTTP_POST, path, NULL, NULL, err);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * Webhook signature verification
 * ══════════════════════════════════════════════════════════════════════════════ */

static int hex_to_bytes(const char *hex, unsigned char *out, size_t out_len)
{
    size_t hex_len = strlen(hex);
    size_t i;
    if (hex_len != out_len * 2) return 0;
    for (i = 0; i < out_len; i++) {
        unsigned int byte;
        if (sscanf(hex + 2 * i, "%02x", &byte) != 1) return 0;
        out[i] = (unsigned char)byte;
    }
    return 1;
}

/* Constant-time compare (avoids timing attacks) */
static int ct_memcmp(const unsigned char *a, const unsigned char *b, size_t len)
{
    unsigned char diff = 0;
    size_t i;
    for (i = 0; i < len; i++) diff |= a[i] ^ b[i];
    return diff == 0;
}

int ips_verify_webhook_signature(const char *payload, const char *timestamp,
                                  const char *signature, const char *secret)
{
    unsigned char computed[EVP_MAX_MD_SIZE];
    unsigned int  computed_len = 0;
    unsigned char expected[32]; /* SHA-256 produces 32 bytes */
    char          message[8192];
    size_t        sig_len;

    if (!payload || !timestamp || !signature || !secret) return 0;

    /* message = timestamp + "." + payload */
    snprintf(message, sizeof(message), "%s.%s", timestamp, payload);

    /* Compute HMAC-SHA256 */
    if (!HMAC(EVP_sha256(),
              secret,  (int)strlen(secret),
              (const unsigned char *)message, strlen(message),
              computed, &computed_len)) {
        return 0;
    }
    if (computed_len != 32) return 0;

    /* Decode the hex signature */
    sig_len = strlen(signature);
    if (sig_len != 64) return 0; /* SHA-256 hex is always 64 chars */
    if (!hex_to_bytes(signature, expected, 32)) return 0;

    return ct_memcmp(computed, expected, 32);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * Memory helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

void ips_response_free(cJSON *response)
{
    cJSON_Delete(response);
}
