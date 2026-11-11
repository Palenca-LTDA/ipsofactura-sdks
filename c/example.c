/*
 * example.c — Uso básico del SDK de Ipsofactura en C.
 *
 * Compilar:
 *   make
 *
 * Ejecutar:
 *   IPSO_API_KEY=tu_clave ./example
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipsofactura.h"

static void print_error(const char *op, ips_error_t *err)
{
    fprintf(stderr, "[ERROR] %s — HTTP %d [%s]: %s\n",
            op,
            err->status_code,
            err->code    ? err->code    : "?",
            err->message ? err->message : "");
    if (err->sat_code && err->sat_code[0]) {
        fprintf(stderr, "        SAT code: %s\n", err->sat_code);
    }
    if (err->retry_after > 0) {
        fprintf(stderr, "        Retry after: %ds\n", err->retry_after);
    }
}

int main(void)
{
    const char   *api_key;
    ips_client_t *client;
    ips_error_t   err;
    cJSON        *result;
    char         *json_str;

    /* ── 1. Get API key from environment ────────────────────────────────── */
    api_key = getenv("IPSO_API_KEY");
    if (!api_key || api_key[0] == '\0') {
        api_key = "sandbox_test_key";
        fprintf(stdout, "IPSO_API_KEY not set — using placeholder key.\n\n");
    }

    /* ── 2. Create client (sandbox) ─────────────────────────────────────── */
    client = ips_client_new(api_key, IPS_SANDBOX_URL);
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        return 1;
    }

    memset(&err, 0, sizeof(err));

    /* ── 3. List empresas ───────────────────────────────────────────────── */
    fprintf(stdout, "=== Empresas ===\n");
    result = ips_empresas_list(client, "include_certificados=true", &err);
    if (!result) {
        print_error("ips_empresas_list", &err);
        ips_error_clear(&err);
    } else {
        json_str = cJSON_Print(result);
        if (json_str) {
            fprintf(stdout, "%s\n\n", json_str);
            free(json_str);
        }
        ips_response_free(result);
        result = NULL;
    }

    /* ── 4. Timbrar CFDI (folio C-001) ─────────────────────────────────── */
    fprintf(stdout, "=== Timbrar CFDI (folio C-001) ===\n");
    {
        cJSON *payload   = cJSON_CreateObject();
        cJSON *receptor  = cJSON_CreateObject();
        cJSON *conceptos = cJSON_CreateArray();
        cJSON *concepto  = cJSON_CreateObject();
        cJSON *impuestos_concepto = cJSON_CreateObject();
        cJSON *traslados_concepto = cJSON_CreateArray();
        cJSON *traslado_concepto  = cJSON_CreateObject();
        cJSON *impuestos = cJSON_CreateObject();
        cJSON *traslados = cJSON_CreateArray();
        cJSON *traslado  = cJSON_CreateObject();

        /* Root CFDI fields — rfc_emisor va a nivel raíz */
        cJSON_AddStringToObject(payload, "rfc_emisor",       "EKU9003173C9");
        cJSON_AddStringToObject(payload, "tipo_comprobante", "I");
        cJSON_AddStringToObject(payload, "serie",            "SDK");
        cJSON_AddStringToObject(payload, "folio",            "C-001");
        cJSON_AddStringToObject(payload, "idempotency_key",  "sdk-test-c-001");
        cJSON_AddStringToObject(payload, "forma_pago",       "03");
        cJSON_AddStringToObject(payload, "metodo_pago",      "PUE");
        cJSON_AddStringToObject(payload, "moneda",           "MXN");
        cJSON_AddNumberToObject(payload, "subtotal",         1000.0);
        cJSON_AddNumberToObject(payload, "total",            1160.0);

        /* Receptor */
        cJSON_AddStringToObject(receptor, "rfc",              "URE180429TM6");
        cJSON_AddStringToObject(receptor, "nombre",           "UNIVERSIDAD ROBOTICA ESPANOLA");
        cJSON_AddStringToObject(receptor, "domicilio_fiscal", "65000");
        cJSON_AddStringToObject(receptor, "regimen_fiscal",   "601");
        cJSON_AddStringToObject(receptor, "uso_cfdi",         "G03");
        cJSON_AddItemToObject(payload, "receptor", receptor);

        /* Traslado del concepto */
        cJSON_AddStringToObject(traslado_concepto, "impuesto",     "002");
        cJSON_AddStringToObject(traslado_concepto, "tipo_factor",  "Tasa");
        cJSON_AddNumberToObject(traslado_concepto, "base",         1000.0);
        cJSON_AddStringToObject(traslado_concepto, "tasa_o_cuota", "0.160000");
        cJSON_AddNumberToObject(traslado_concepto, "importe",      160.0);
        cJSON_AddItemToArray(traslados_concepto, traslado_concepto);
        cJSON_AddItemToObject(impuestos_concepto, "traslados", traslados_concepto);

        /* Concepto */
        cJSON_AddStringToObject(concepto, "clave_prod_serv", "01010101");
        cJSON_AddNumberToObject(concepto, "cantidad",        1.0);
        cJSON_AddStringToObject(concepto, "clave_unidad",    "H87");
        cJSON_AddStringToObject(concepto, "descripcion",     "Servicio de prueba SDK C");
        cJSON_AddNumberToObject(concepto, "valor_unitario",  1000.0);
        cJSON_AddNumberToObject(concepto, "importe",         1000.0);
        cJSON_AddStringToObject(concepto, "objeto_imp",      "02");
        cJSON_AddItemToObject(concepto, "impuestos", impuestos_concepto);
        cJSON_AddItemToArray(conceptos, concepto);
        cJSON_AddItemToObject(payload, "conceptos", conceptos);

        /* Totales de impuestos */
        cJSON_AddStringToObject(traslado, "impuesto",     "002");
        cJSON_AddStringToObject(traslado, "tipo_factor",  "Tasa");
        cJSON_AddNumberToObject(traslado, "base",         1000.0);
        cJSON_AddStringToObject(traslado, "tasa_o_cuota", "0.160000");
        cJSON_AddNumberToObject(traslado, "importe",      160.0);
        cJSON_AddItemToArray(traslados, traslado);
        cJSON_AddNumberToObject(impuestos, "total_impuestos_trasladados", 160.0);
        cJSON_AddItemToObject(impuestos, "traslados", traslados);
        cJSON_AddItemToObject(payload, "impuestos", impuestos);

        result = ips_cfdi_timbrar(client, payload, &err);
        cJSON_Delete(payload);

        if (!result) {
            print_error("ips_cfdi_timbrar", &err);
            ips_error_clear(&err);
        } else {
            cJSON *uuid_node = cJSON_GetObjectItemCaseSensitive(result, "uuid");
            if (!uuid_node)
                uuid_node = cJSON_GetObjectItemCaseSensitive(result, "folio_fiscal");

            fprintf(stdout, "CFDI timbrado exitosamente.\n");
            if (cJSON_IsString(uuid_node)) {
                fprintf(stdout, "UUID (Folio Fiscal): %s\n\n", uuid_node->valuestring);
            }

            json_str = cJSON_Print(result);
            if (json_str) {
                fprintf(stdout, "%s\n", json_str);
                free(json_str);
            }
            ips_response_free(result);
        }
    }

    /* ── 5. Cleanup ─────────────────────────────────────────────────────── */
    ips_client_free(client);
    return 0;
}
