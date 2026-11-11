# ipsofactura-go

Cliente oficial de **Ipsofactura** para Go 1.21+. Solo usa la librería estándar (`net/http`, `encoding/json`, `crypto/hmac`).

## Instalación

```bash
go get github.com/Palenca-LTDA/ipsofactura-go
```

## Quick start

```go
package main

import (
    "context"
    "fmt"
    "log"

    ipsofactura "github.com/Palenca-LTDA/ipsofactura-go"
)

func main() {
    // Producción
    client := ipsofactura.NewClient("tu_api_key")

    // Sandbox
    client = ipsofactura.NewClient("tu_api_key",
        ipsofactura.WithBaseURL(ipsofactura.SandboxURL),
    )

    ctx := context.Background()

    // Listar empresas
    empresas, err := client.Empresas.List(ctx, nil)
    if err != nil {
        log.Fatal(err)
    }
    fmt.Println(empresas)
}
```

## Uso

### CFDI

```go
ctx := context.Background()

// Timbrar
cfdi, err := client.Cfdi.Timbrar(ctx, map[string]any{
    "tipo_comprobante": "I",
    "serie":  "GO",
    "folio":  "GO-001",
    // ... resto del payload
})
if err != nil {
    var apiErr *ipsofactura.IpsofacturaError
    if errors.As(err, &apiErr) {
        fmt.Printf("HTTP %d [%s]: %s\n", apiErr.StatusCode, apiErr.Code, apiErr.Message)
        if apiErr.SatCode != "" {
            fmt.Println("SAT:", apiErr.SatCode)
        }
    }
    log.Fatal(err)
}
fmt.Println("UUID:", cfdi["uuid"])

// Listar con filtros
params := url.Values{}
params.Set("page", "1")
params.Set("per_page", "50")
lista, _ := client.Cfdi.List(ctx, params)

// Cancelar
client.Cfdi.Cancelar(ctx, map[string]any{
    "uuid":   "...",
    "motivo": "02",
})
```

### Empresas

```go
// Crear empresa
empresa, _ := client.Empresas.Crear(ctx, map[string]any{
    "rfc":    "EKU9003173C9",
    "nombre": "Mi Empresa SA de CV",
})

// Subir certificado CSD
client.Empresas.SubirCertificado(ctx, empresa["id"].(string), map[string]any{
    "cer":      "<base64 del .cer>",
    "key":      "<base64 del .key>",
    "password": "mi_contraseña",
})
```

### Webhooks

```go
// Crear webhook
wh, _ := client.Webhooks.Crear(ctx, map[string]any{
    "url":     "https://mi-app.com/webhook",
    "eventos": []string{"cfdi.timbrado", "cfdi.cancelado"},
})

// Verificar firma
valid := ipsofactura.VerifyWebhookSignature(payload, timestamp, signature, secret)
```

## Manejo de errores

```go
var apiErr *ipsofactura.IpsofacturaError
if errors.As(err, &apiErr) {
    fmt.Println(apiErr.StatusCode) // HTTP status
    fmt.Println(apiErr.Code)       // "VALIDATION_ERROR"
    fmt.Println(apiErr.Message)
    fmt.Println(apiErr.SatCode)    // "CFDI40xxx" si aplica
    fmt.Println(apiErr.RetryAfter) // segundos para reintentar
}
```

## Requisitos

- Go >= 1.21
- Sin dependencias externas

## Licencia

MIT
