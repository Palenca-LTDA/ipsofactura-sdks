# @ipsofactura/mcp

Servidor MCP oficial de Ipsofactura. Permite a agentes de IA (Claude Desktop, Cursor, etc.) timbrar, consultar y cancelar CFDIs directamente desde el chat, sin salir del flujo de trabajo.

## Instalación

No requiere instalación previa. Se ejecuta con `npx`:

```bash
npx @ipsofactura/mcp
```

## Configuración en Claude Desktop

Edita el archivo `~/Library/Application Support/Claude/claude_desktop_config.json` (macOS) o `%APPDATA%\Claude\claude_desktop_config.json` (Windows) y agrega la sección `mcpServers`:

```json
{
  "mcpServers": {
    "ipsofactura": {
      "command": "npx",
      "args": ["-y", "@ipsofactura/mcp"],
      "env": {
        "IPS_API_KEY": "tu-api-key-aqui",
        "IPS_API_BASE_URL": "https://api.ipsofactura.com"
      }
    }
  }
}
```

Reinicia Claude Desktop para que reconozca el servidor.

## Configuración en Sandbox

Para pruebas y desarrollo, apunta a la URL del sandbox:

```json
{
  "mcpServers": {
    "ipsofactura-sandbox": {
      "command": "npx",
      "args": ["-y", "@ipsofactura/mcp"],
      "env": {
        "IPS_API_KEY": "tu-api-key-sandbox",
        "IPS_API_BASE_URL": "https://api.sandbox.ipsofactura.com"
      }
    }
  }
}
```

## Variables de entorno

| Variable | Descripción | Default |
|---|---|---|
| `IPS_API_KEY` | **Requerida.** Tu API key de Ipsofactura | — |
| `IPS_API_BASE_URL` | URL base de la API | `https://api.ipsofactura.com` |

## Herramientas disponibles

| Herramienta | Endpoint | Descripción |
|---|---|---|
| `listar_empresas` | `GET /empresas` | Lista los RFCs emisores registrados en tu cuenta |
| `timbrar_cfdi` | `POST /cfdi/timbrar` | Timbra un CFDI 4.0 ante el SAT |
| `cancelar_cfdi` | `POST /cfdi/cancelar` | Cancela un CFDI vigente |
| `listar_cfdis` | `GET /cfdi` | Busca CFDIs con filtros de fecha, RFC, estatus, etc. |
| `consultar_cfdi` | `GET /cfdi/{id}` | Obtiene el detalle completo de un CFDI |
| `obtener_xml` | `GET /cfdi/{id}/xml` | Obtiene la URL del XML firmado (con timbre SAT) |

## Protección contra doble timbrado

La herramienta `timbrar_cfdi` genera automáticamente una clave de idempotencia derivada de `serie + folio + rfc_emisor`. Si el agente reintenta la llamada (por timeout, reconexión, etc.), Ipsofactura reconoce la clave y retorna el CFDI original en lugar de crear un duplicado. No es necesario que el agente calcule o recuerde esta clave.

## Ejemplo de uso

Una vez configurado, puedes hablar con el agente de forma natural:

> "Muéstrame las empresas disponibles en Ipsofactura"
>
> "Timbra una factura por $1,000 MXN de servicios de consultoría al RFC XAXX010101000"
>
> "Lista las facturas del mes de enero de 2025 para el RFC CAFE860101GR6"
>
> "Cancela la factura con ID abc123, motivo 02"

## Documentación

- [Guía de inicio rápido](https://developers.ipsofactura.com)
- [Referencia de la API](https://developers.ipsofactura.com/api)

## Licencia

MIT
