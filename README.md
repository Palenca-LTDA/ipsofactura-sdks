# Ipsofactura SDK

SDKs y clientes HTTP oficiales para la [API de Ipsofactura](https://developers.ipsofactura.com) — timbrado CFDI 4.0 para México.

## Lenguajes disponibles

| Lenguaje | Directorio | Publicado en | Versión |
|---|---|---|---|
| JavaScript / TypeScript | [`js/`](./js/) | npm: `ipsofactura` | 0.1.0 |
| Python | [`python/`](./python/) | PyPI: `ipsofactura` | 0.1.0 |
| Java | [`java/`](./java/) | Maven Central: `com.ipsofactura:ipsofactura-java` | 0.1.0 |
| PHP | [`php/`](./php/) | Packagist: `ipsofactura/ipsofactura-php` | 0.1.0 |
| Go | [`go/`](./go/) | `github.com/Palenca-LTDA/ipsofactura-go` | 0.1.0 |
| C# / .NET | [`csharp/`](./csharp/) | NuGet: `Ipsofactura` | 0.1.0 |
| Ruby | [`ruby/`](./ruby/) | RubyGems: `ipsofactura` | 0.1.0 |
| C | [`c/`](./c/) | — (fuente, compilar con `make`) | 0.1.0 |

## Instalación rápida

```bash
# Node.js / TypeScript
npm install ipsofactura

# Python
pip install ipsofactura

# PHP
composer require ipsofactura/ipsofactura-php

# Ruby
gem install ipsofactura

# Go
go get github.com/Palenca-LTDA/ipsofactura-go

# C# / .NET
dotnet add package Ipsofactura

# Java (Gradle)
implementation 'com.ipsofactura:ipsofactura-java:0.1.0'

# C
git clone https://github.com/Palenca-LTDA/ipsofactura-sdk
cd ipsofactura-sdk/c && make
```

## Inicio rápido

Todos los SDKs siguen la misma estructura:

```
cliente
  .cfdi       — timbrar, cancelar, complemento-pago, consultar, descargar
  .empresas   — CRUD de emisores y certificados CSD
  .webhooks   — CRUD de webhooks, entregas, reenvíos
```

### JavaScript / TypeScript

```typescript
import IpsofacturaClient from "ipsofactura";

const ipso = new IpsofacturaClient(process.env.IPSO_API_KEY!);

const cfdi = await ipso.cfdi.timbrar({
  rfc_emisor: "TU_RFC",
  tipo_comprobante: "I",
  // ...
});
console.log(cfdi.uuid);
```

### Python

```python
from ipsofactura import IpsofacturaClient

ipso = IpsofacturaClient(api_key=os.environ["IPSO_API_KEY"])

cfdi = ipso.cfdi.timbrar({
    "rfc_emisor": "TU_RFC",
    "tipo_comprobante": "I",
    # ...
})
print(cfdi["uuid"])
```

### Java

```java
import com.ipsofactura.IpsofacturaClient;
import java.util.Map;

IpsofacturaClient ipso = new IpsofacturaClient(System.getenv("IPSO_API_KEY"));
Map<String, Object> cfdi = ipso.cfdi().timbrar(payload);
System.out.println(cfdi.get("uuid"));
```

### PHP

```php
use Ipsofactura\IpsofacturaClient;

$ipso = new IpsofacturaClient($_ENV['IPSO_API_KEY']);
$cfdi = $ipso->cfdi->timbrar([...]);
echo $cfdi['uuid'];
```

### Go

```go
import "github.com/Palenca-LTDA/ipsofactura-go"

client := ipsofactura.NewClient(os.Getenv("IPSO_API_KEY"))
cfdi, err := client.Cfdi.Timbrar(ctx, payload)
```

### C# / .NET

```csharp
using Ipsofactura;

var ipso = new IpsofacturaClient(Environment.GetEnvironmentVariable("IPSO_API_KEY")!);
var cfdi = await ipso.Cfdi.TimbrarAsync(payload);
```

### Ruby

```ruby
require "ipsofactura"

ipso = Ipsofactura::Client.new(ENV["IPSO_API_KEY"])
cfdi = ipso.cfdi.timbrar(payload)
puts cfdi["uuid"]
```

## Sandbox

Para pruebas, usa la URL de sandbox y una API key de sandbox:

```bash
export IPSO_API_KEY="ifk_test_..."
```

Y pasa `base_url: "https://sandbox.api.ipsofactura.com"` al crear el cliente (o usa la constante `SANDBOX_URL` que exporta cada SDK).

Los CFDIs del sandbox llevan `"sandbox": true` en la respuesta y no tienen validez fiscal ante el SAT.

## Verificación de webhooks

Todos los SDKs incluyen una utilidad para verificar la firma HMAC-SHA256 de los webhooks:

```typescript
import { verifyWebhookSignature } from "ipsofactura";
const valid = verifyWebhookSignature(rawBody, timestamp, signature, secret);
```

```python
from ipsofactura import verify_webhook_signature
valid = verify_webhook_signature(raw_body, timestamp, signature, secret)
```

## Documentación

- [Referencia de la API](https://developers.ipsofactura.com/api-reference)
- [Guía de timbrado](https://developers.ipsofactura.com/guia-timbrado)
- [Sandbox](https://developers.ipsofactura.com/sandbox)
- [Webhooks](https://developers.ipsofactura.com/webhooks)
- [Errores](https://developers.ipsofactura.com/errores)

## Licencia

MIT
