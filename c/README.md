# ipsofactura-c

Cliente oficial de **Ipsofactura** en C. Funciona en macOS y Linux.

**Dependencias:**
- `libcurl` — para las llamadas HTTP
- `OpenSSL` (`libssl` + `libcrypto`) — para verificación de firma HMAC-SHA256
- `cJSON` — bundled en este repo (Dave Gamble, MIT License)

## Requisitos

### macOS (Homebrew)
```bash
# Si no tienes OpenSSL:
brew install openssl

# libcurl ya viene con Xcode Command Line Tools
xcode-select --install
```

### Linux (Debian/Ubuntu)
```bash
apt-get install libcurl4-openssl-dev libssl-dev
```

## Compilación

```bash
make
```

Esto genera el ejecutable `example` y los objetos `.o` para usar en tu propio proyecto.

Para integrar en tu proyecto, incluye `ipsofactura.c`, `cJSON.c`, `ipsofactura.h` y `cJSON.h`, y linka con `-lcurl -lssl -lcrypto`.

## Uso rápido

```bash
export IPSO_API_KEY=tu_clave_aqui
./example
```

## Integración

```c
#include "ipsofactura.h"

int main(void) {
    ips_client_t *client = ips_client_new("tu_api_key", IPS_SANDBOX_URL);
    ips_error_t   err;
    memset(&err, 0, sizeof(err));

    // Listar empresas
    cJSON *empresas = ips_empresas_list(client, NULL, &err);
    if (!empresas) {
        fprintf(stderr, "Error [%s]: %s\n", err.code, err.message);
        ips_error_clear(&err);
    } else {
        char *s = cJSON_Print(empresas);
        printf("%s\n", s);
        free(s);
        ips_response_free(empresas);
    }

    // Timbrar un CFDI
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "tipo_comprobante", "I");
    cJSON_AddStringToObject(payload, "serie", "A");
    cJSON_AddStringToObject(payload, "folio", "A-001");
    // ... (completar campos requeridos)

    cJSON *cfdi = ips_cfdi_timbrar(client, payload, &err);
    cJSON_Delete(payload);

    if (!cfdi) {
        fprintf(stderr, "Error HTTP %d [%s]: %s\n",
                err.status_code, err.code, err.message);
        ips_error_clear(&err);
    } else {
        cJSON *uuid = cJSON_GetObjectItemCaseSensitive(cfdi, "uuid");
        if (cJSON_IsString(uuid)) printf("UUID: %s\n", uuid->valuestring);
        ips_response_free(cfdi);
    }

    ips_client_free(client);
    return 0;
}
```

## Verificar firma de webhook

```c
int valid = ips_verify_webhook_signature(
    payload_body,   // cuerpo crudo del request
    x_timestamp,    // header X-Timestamp
    x_signature,    // header X-Signature (hex)
    webhook_secret  // secreto configurado en el dashboard
);
if (!valid) {
    // rechazar el evento
}
```

## Manejo de errores

Todas las funciones `ips_*` retornan `NULL` en caso de error y rellenan la estructura `ips_error_t`:

```c
ips_error_t err;
memset(&err, 0, sizeof(err));

cJSON *result = ips_cfdi_get(client, "algún-id", &err);
if (!result) {
    printf("HTTP %d\n",   err.status_code);
    printf("Code: %s\n",  err.code);
    printf("Msg:  %s\n",  err.message);
    if (err.sat_code)    printf("SAT:  %s\n",  err.sat_code);
    if (err.retry_after) printf("Wait: %ds\n", err.retry_after);
    ips_error_clear(&err);  // libera las cadenas
}
```

Siempre llama `ips_error_clear(&err)` después de manejar el error para liberar la memoria.

## Licencia

MIT
