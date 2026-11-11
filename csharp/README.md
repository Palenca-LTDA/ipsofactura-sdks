# ipsofactura-csharp

Cliente oficial de **Ipsofactura** para .NET 6+. Usa `System.Net.Http.HttpClient` y `System.Text.Json` — sin dependencias NuGet adicionales.

## Instalación

Agrega el proyecto como referencia o copia los archivos `.cs` a tu solución.

```bash
# Como proyecto referenciado:
dotnet add reference ../ipsofactura-sdk/csharp/Ipsofactura.csproj
```

## Quick start

```csharp
using Ipsofactura;

// Producción
using var client = new IpsofacturaClient("tu_api_key");

// Sandbox
using var client = new IpsofacturaClient("tu_api_key", IpsofacturaClient.SandboxUrl);
```

## Uso

### CFDI

```csharp
// Timbrar
var cfdi = await client.Cfdi.TimbrarAsync(new Dictionary<string, object?>
{
    ["tipo_comprobante"] = "I",
    ["serie"]            = "CS",
    ["folio"]            = "CS-001",
    // ... resto del payload
});
Console.WriteLine(cfdi["uuid"]);

// Listar con filtros
var lista = await client.Cfdi.ListAsync(new Dictionary<string, string>
{
    ["page"]     = "1",
    ["per_page"] = "50",
});

// Cancelar
await client.Cfdi.CancelarAsync(new Dictionary<string, object?>
{
    ["uuid"]   = "...",
    ["motivo"] = "02",
});
```

### Empresas

```csharp
// Crear
var empresa = await client.Empresas.CrearAsync(new Dictionary<string, object?>
{
    ["rfc"]    = "EKU9003173C9",
    ["nombre"] = "Mi Empresa SA de CV",
});

// Subir certificado
await client.Empresas.SubirCertificadoAsync(empresa["id"]!.ToString()!, new Dictionary<string, object?>
{
    ["cer"]      = Convert.ToBase64String(File.ReadAllBytes("cert.cer")),
    ["key"]      = Convert.ToBase64String(File.ReadAllBytes("cert.key")),
    ["password"] = "mi_contraseña",
});
```

### Webhooks

```csharp
// Crear webhook
var wh = await client.Webhooks.CrearAsync(new Dictionary<string, object?>
{
    ["url"]     = "https://mi-app.com/webhook",
    ["eventos"] = new[] { "cfdi.timbrado", "cfdi.cancelado" },
});

// Verificar firma
bool valid = WebhookUtils.VerifySignature(payload, timestamp, signature, secret);
```

## Manejo de errores

```csharp
try
{
    var cfdi = await client.Cfdi.TimbrarAsync(payload);
}
catch (IpsofacturaException ex)
{
    Console.WriteLine(ex.StatusCode);  // HTTP status
    Console.WriteLine(ex.Code);        // "VALIDATION_ERROR"
    Console.WriteLine(ex.Message);
    Console.WriteLine(ex.SatCode);     // "CFDI40xxx" si aplica
    Console.WriteLine(ex.RetryAfter);  // segundos para reintentar
}
```

## Requisitos

- .NET 6 o superior
- Sin dependencias NuGet externas

## Licencia

MIT
