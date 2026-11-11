#ifndef IPSOFACTURA_H
#define IPSOFACTURA_H

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPS_SANDBOX_URL "https://sandbox.api.ipsofactura.com"
#define IPS_PROD_URL    "https://api.ipsofactura.com"

/** Opaque client handle. */
typedef struct ips_client_s ips_client_t;

/** Error information populated when an API call fails. */
typedef struct {
    int   status_code;   /**< HTTP status code (e.g. 400, 422, 500) */
    char *code;          /**< Machine-readable code, e.g. "VALIDATION_ERROR" */
    char *message;       /**< Human-readable error message */
    char *sat_code;      /**< SAT CFDI code, may be NULL */
    int   retry_after;   /**< Seconds to wait before retrying; 0 if not set */
} ips_error_t;

/* ── Client lifecycle ───────────────────────────────────────────────────────── */

/**
 * Create a new Ipsofactura client.
 * @param api_key   Your API key.
 * @param base_url  IPS_PROD_URL or IPS_SANDBOX_URL (or custom URL).
 * @return Heap-allocated client. Free with ips_client_free().
 */
ips_client_t *ips_client_new(const char *api_key, const char *base_url);

/** Free all resources associated with the client. */
void ips_client_free(ips_client_t *client);

/* ── CFDI operations ────────────────────────────────────────────────────────── */

/** Timbra un CFDI 4.0. */
cJSON *ips_cfdi_timbrar(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Timbra un CFDI de nómina. */
cJSON *ips_cfdi_timbrar_nomina(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Timbra desde XML ya generado. */
cJSON *ips_cfdi_timbrar_xml(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Timbra un comprobante pre-sellado. */
cJSON *ips_cfdi_timbrar_sellado(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Cancela un CFDI timbrado. */
cJSON *ips_cfdi_cancelar(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Agrega un complemento de pago. */
cJSON *ips_cfdi_complemento_pago(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Crea un job de exportación masiva. */
cJSON *ips_cfdi_crear_export(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Consulta el estado de un export por ID. */
cJSON *ips_cfdi_get_export(ips_client_t *c, const char *id, ips_error_t *err);

/**
 * Lista CFDIs.
 * @param query_string  URL query string, e.g. "page=1&per_page=50" (may be NULL).
 */
cJSON *ips_cfdi_list(ips_client_t *c, const char *query_string, ips_error_t *err);

/** Obtiene un CFDI por ID. */
cJSON *ips_cfdi_get(ips_client_t *c, const char *id, ips_error_t *err);

/** Consulta el estatus SAT de un CFDI. */
cJSON *ips_cfdi_get_estatus(ips_client_t *c, const char *id, ips_error_t *err);

/** Descarga el XML de un CFDI. */
cJSON *ips_cfdi_get_xml(ips_client_t *c, const char *id, ips_error_t *err);

/** Descarga el PDF de un CFDI. */
cJSON *ips_cfdi_get_pdf(ips_client_t *c, const char *id, ips_error_t *err);

/** Obtiene el acuse de cancelación. */
cJSON *ips_cfdi_get_acuse(ips_client_t *c, const char *id, ips_error_t *err);

/* ── Empresas operations ────────────────────────────────────────────────────── */

/**
 * Lista empresas.
 * @param query_string  e.g. "include_certificados=true" (may be NULL).
 */
cJSON *ips_empresas_list(ips_client_t *c, const char *query_string, ips_error_t *err);

/** Crea una nueva empresa. */
cJSON *ips_empresas_crear(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Obtiene una empresa por ID. */
cJSON *ips_empresas_get(ips_client_t *c, const char *id, ips_error_t *err);

/** Actualiza una empresa (PATCH). */
cJSON *ips_empresas_actualizar(ips_client_t *c, const char *id, cJSON *payload, ips_error_t *err);

/** Sube un certificado CSD a una empresa. */
cJSON *ips_empresas_subir_certificado(ips_client_t *c, const char *empresa_id, cJSON *payload, ips_error_t *err);

/** Refresca la contraseña del certificado. */
cJSON *ips_empresas_refresh_password(ips_client_t *c, const char *empresa_id, cJSON *payload, ips_error_t *err);

/** Elimina un certificado por número de serie. */
cJSON *ips_empresas_eliminar_certificado(ips_client_t *c, const char *empresa_id, const char *numero, ips_error_t *err);

/* ── Webhooks operations ────────────────────────────────────────────────────── */

/** Lista todos los webhooks. */
cJSON *ips_webhooks_list(ips_client_t *c, ips_error_t *err);

/** Crea un webhook. */
cJSON *ips_webhooks_crear(ips_client_t *c, cJSON *payload, ips_error_t *err);

/** Actualiza un webhook (PATCH). */
cJSON *ips_webhooks_actualizar(ips_client_t *c, const char *id, cJSON *payload, ips_error_t *err);

/** Elimina un webhook. */
cJSON *ips_webhooks_eliminar(ips_client_t *c, const char *id, ips_error_t *err);

/** Lista el historial de deliveries. */
cJSON *ips_webhooks_list_deliveries(ips_client_t *c, const char *webhook_id, ips_error_t *err);

/** Obtiene un delivery específico. */
cJSON *ips_webhooks_get_delivery(ips_client_t *c, const char *webhook_id, const char *delivery_id, ips_error_t *err);

/** Re-envía un delivery fallido. */
cJSON *ips_webhooks_resend_delivery(ips_client_t *c, const char *webhook_id, const char *delivery_id, ips_error_t *err);

/** Envía un evento de prueba al webhook. */
cJSON *ips_webhooks_test(ips_client_t *c, const char *webhook_id, ips_error_t *err);

/* ── Webhook signature verification ────────────────────────────────────────── */

/**
 * Verifies the HMAC-SHA256 signature of an incoming webhook event.
 *
 * The signed message is: timestamp + "." + payload.
 * The comparison uses a constant-time approach to prevent timing attacks.
 *
 * @param payload    Raw request body.
 * @param timestamp  X-Timestamp header value.
 * @param signature  X-Signature header value (hex-encoded).
 * @param secret     The webhook secret.
 * @return 1 if valid, 0 otherwise.
 */
int ips_verify_webhook_signature(const char *payload, const char *timestamp,
                                  const char *signature, const char *secret);

/* ── Memory helpers ─────────────────────────────────────────────────────────── */

/** Free a cJSON response returned by any API function. */
void ips_response_free(cJSON *response);

/** Free strings inside an ips_error_t and zero it out. */
void ips_error_clear(ips_error_t *err);

#ifdef __cplusplus
}
#endif

#endif /* IPSOFACTURA_H */
