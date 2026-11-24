#!/usr/bin/env node

/**
 * Servidor MCP oficial de Ipsofactura
 * Expone 6 herramientas para timbrar, consultar y cancelar CFDIs vía stdio.
 * Compatible con Claude Desktop, Cursor y cualquier cliente MCP.
 */

import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} from "@modelcontextprotocol/sdk/types.js";

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

const IPS_API_KEY = process.env.IPS_API_KEY;
const IPS_API_BASE_URL =
  process.env.IPS_API_BASE_URL ?? "https://api.ipsofactura.com";

if (!IPS_API_KEY) {
  process.stderr.write(
    "[ipsofactura-mcp] Error: La variable de entorno IPS_API_KEY es requerida.\n" +
      "  Configúrala en tu cliente MCP o exporta IPS_API_KEY=<tu-api-key>.\n",
  );
  process.exit(1);
}

// Remove from process.env so it's not accessible to other code in the process
delete process.env.IPS_API_KEY;

function redact(msg: string): string {
  if (IPS_API_KEY && msg.includes(IPS_API_KEY)) {
    return msg.replaceAll(IPS_API_KEY, "[REDACTED]");
  }
  return msg;
}

// ---------------------------------------------------------------------------
// HTTP helper
// ---------------------------------------------------------------------------

async function apiRequest<T>(
  method: string,
  path: string,
  body?: unknown,
  params?: Record<string, string | number | boolean | undefined>,
): Promise<T> {
  const url = new URL(path, IPS_API_BASE_URL);
  if (params) {
    for (const [k, v] of Object.entries(params)) {
      if (v !== undefined && v !== null) url.searchParams.set(k, String(v));
    }
  }
  const headers: Record<string, string> = {
    "x-api": IPS_API_KEY as string,
    "Content-Type": "application/json",
    Accept: "application/json",
    "User-Agent": "ipsofactura-mcp/0.1.0",
  };
  const res = await fetch(url.toString(), {
    method,
    headers,
    body: body !== undefined ? JSON.stringify(body) : undefined,
  });
  if (res.status === 204) return undefined as unknown as T;
  const data = await res.json().catch(() => ({}));
  if (!res.ok) {
    const err = data as Record<string, unknown>;
    const msg = `${err.code ?? "ERROR"}: ${err.message ?? res.statusText}${
      err.sat_code ? ` (SAT: ${err.sat_code})` : ""
    }`;
    throw new Error(msg);
  }
  return data as T;
}

// ---------------------------------------------------------------------------
// Tool helpers
// ---------------------------------------------------------------------------

type ToolResult = {
  content: Array<{ type: "text"; text: string }>;
  isError?: boolean;
};

function ok(result: unknown): ToolResult {
  return {
    content: [{ type: "text", text: JSON.stringify(result, null, 2) }],
  };
}

function err(e: unknown): ToolResult {
  return {
    content: [{ type: "text", text: `Error: ${redact((e as Error).message)}` }],
    isError: true,
  };
}

// ---------------------------------------------------------------------------
// MCP Server
// ---------------------------------------------------------------------------

const server = new Server(
  { name: "ipsofactura-mcp", version: "0.1.0" },
  {
    capabilities: {
      tools: {},
    },
  },
);

// ---------------------------------------------------------------------------
// Tool definitions
// ---------------------------------------------------------------------------

server.setRequestHandler(ListToolsRequestSchema, async () => ({
  tools: [
    {
      name: "listar_empresas",
      description:
        "Lista las empresas (emisores) registradas en la cuenta de Ipsofactura. " +
        "Úsalo primero para conocer qué RFCs están disponibles antes de timbrar. " +
        "Puedes filtrar por RFC o nombre.",
      inputSchema: {
        type: "object",
        properties: {
          q: {
            type: "string",
            description: "Buscar por RFC o nombre de empresa (parcial o exacto)",
          },
          include_certificados: {
            type: "boolean",
            description:
              "Si es true, incluye los certificados CSD activos de cada empresa",
          },
        },
      },
    },
    {
      name: "timbrar_cfdi",
      description:
        "Timbra un CFDI 4.0 con el SAT a través de Ipsofactura. " +
        "Genera automáticamente una clave de idempotencia a partir de serie+folio+rfc_emisor " +
        "para evitar doble timbrado si el agente reintenta la llamada. " +
        "El campo receptor debe incluir rfc, nombre, uso_cfdi y domicilio_fiscal_receptor. " +
        "Cada concepto requiere clave_prod_serv, cantidad, clave_unidad, descripcion, valor_unitario y objeto_imp.",
      inputSchema: {
        type: "object",
        properties: {
          rfc_emisor: {
            type: "string",
            description: "RFC del emisor del CFDI (debe estar registrado en Ipsofactura)",
          },
          serie: {
            type: "string",
            description: "Serie del comprobante (p.ej. 'A'). Opcional.",
          },
          folio: {
            type: "string",
            description: "Folio del comprobante (p.ej. '1001'). Opcional.",
          },
          tipo_comprobante: {
            type: "string",
            enum: ["I", "E", "T", "N", "P"],
            description:
              "Tipo: I=Ingreso, E=Egreso, T=Traslado, N=Nómina, P=Pago",
          },
          metodo_pago: {
            type: "string",
            enum: ["PUE", "PPD"],
            description: "PUE=Pago en una sola exhibición, PPD=Pago en parcialidades o diferido",
          },
          forma_pago: {
            type: "string",
            description:
              "Clave SAT de forma de pago (p.ej. '01'=Efectivo, '03'=Transferencia, '04'=Tarjeta de crédito)",
          },
          moneda: {
            type: "string",
            description: "Código ISO de moneda (p.ej. 'MXN', 'USD'). Default: MXN.",
          },
          tipo_cambio: {
            type: "number",
            description: "Tipo de cambio respecto a MXN. Requerido si moneda != MXN.",
          },
          lugar_expedicion: {
            type: "string",
            description: "Código postal del lugar de expedición (5 dígitos)",
          },
          receptor: {
            type: "object",
            description: "Datos del receptor del CFDI",
            properties: {
              rfc: { type: "string", description: "RFC del receptor" },
              nombre: { type: "string", description: "Nombre o razón social del receptor" },
              uso_cfdi: {
                type: "string",
                description: "Clave SAT de uso CFDI (p.ej. 'G01'=Adquisición de mercancias, 'S01'=Sin efectos fiscales)",
              },
              domicilio_fiscal_receptor: {
                type: "string",
                description: "Código postal del domicilio fiscal del receptor",
              },
              regimen_fiscal_receptor: {
                type: "string",
                description: "Régimen fiscal del receptor (p.ej. '601', '612', '626')",
              },
            },
            required: ["rfc", "nombre", "uso_cfdi", "domicilio_fiscal_receptor"],
          },
          conceptos: {
            type: "array",
            description: "Lista de conceptos (líneas de la factura)",
            items: {
              type: "object",
              properties: {
                clave_prod_serv: {
                  type: "string",
                  description: "Clave del producto/servicio del catálogo SAT",
                },
                cantidad: { type: "number", description: "Cantidad de unidades" },
                clave_unidad: {
                  type: "string",
                  description: "Clave de unidad SAT (p.ej. 'H87'=Pieza, 'E48'=Unidad de servicio)",
                },
                unidad: { type: "string", description: "Descripción de la unidad. Opcional." },
                descripcion: { type: "string", description: "Descripción del concepto" },
                valor_unitario: { type: "number", description: "Precio unitario sin impuestos" },
                descuento: { type: "number", description: "Descuento en monto (opcional)" },
                objeto_imp: {
                  type: "string",
                  enum: ["01", "02", "03"],
                  description: "Objeto de impuesto: 01=No objeto, 02=Sí objeto, 03=Sí objeto y no obligado",
                },
                impuestos: {
                  type: "object",
                  description: "Impuestos del concepto (traslados y retenciones)",
                },
              },
              required: [
                "clave_prod_serv",
                "cantidad",
                "clave_unidad",
                "descripcion",
                "valor_unitario",
                "objeto_imp",
              ],
            },
          },
          cfdi_relacionados: {
            type: "array",
            description: "CFDIs relacionados (UUID y tipo de relación). Opcional.",
            items: {
              type: "object",
              properties: {
                tipo_relacion: { type: "string", description: "Clave de tipo de relación SAT" },
                uuids: {
                  type: "array",
                  items: { type: "string" },
                  description: "Lista de UUIDs relacionados",
                },
              },
            },
          },
          complementos: {
            type: "object",
            description: "Complementos adicionales del CFDI (p.ej. carta porte). Opcional.",
          },
        },
        required: [
          "rfc_emisor",
          "tipo_comprobante",
          "lugar_expedicion",
          "receptor",
          "conceptos",
        ],
      },
    },
    {
      name: "cancelar_cfdi",
      description:
        "Cancela un CFDI previamente timbrado ante el SAT. " +
        "El motivo determina si se requiere folio_sustitucion: " +
        "motivo 01 (error con relación) siempre requiere el UUID del CFDI sustituto; " +
        "motivos 02, 03 y 04 no lo requieren. " +
        "La cancelación puede requerir aceptación del receptor según las reglas del SAT.",
      inputSchema: {
        type: "object",
        properties: {
          id: {
            type: "string",
            description: "ID interno del CFDI en Ipsofactura (obtenido al timbrar o con listar_cfdis)",
          },
          motivo: {
            type: "string",
            enum: ["01", "02", "03", "04"],
            description:
              "Motivo de cancelación SAT: " +
              "01=Comprobante emitido con errores con relación, " +
              "02=Comprobante emitido con errores sin relación, " +
              "03=No se llevó a cabo la operación, " +
              "04=Operación nominativa relacionada en la factura global",
          },
          folio_sustitucion: {
            type: "string",
            description:
              "UUID del CFDI que sustituye a este (requerido cuando motivo=01)",
          },
        },
        required: ["id", "motivo"],
      },
    },
    {
      name: "listar_cfdis",
      description:
        "Lista los CFDIs registrados en Ipsofactura con soporte de paginación y filtros. " +
        "Útil para buscar facturas por rango de fechas, receptor, estatus o folio fiscal.",
      inputSchema: {
        type: "object",
        properties: {
          page: {
            type: "integer",
            description: "Número de página (inicia en 1). Default: 1.",
          },
          per_page: {
            type: "integer",
            description: "Registros por página (máx 100). Default: 20.",
          },
          tipo_comprobante: {
            type: "string",
            enum: ["I", "E", "T", "N", "P"],
            description: "Filtrar por tipo: I=Ingreso, E=Egreso, T=Traslado, N=Nómina, P=Pago",
          },
          serie: {
            type: "string",
            description: "Filtrar por serie del comprobante",
          },
          folio: {
            type: "string",
            description: "Filtrar por folio del comprobante",
          },
          rfc_emisor: {
            type: "string",
            description: "Filtrar por RFC del emisor",
          },
          folio_fiscal: {
            type: "string",
            description: "Filtrar por UUID/folio fiscal asignado por el SAT",
          },
          fecha_desde: {
            type: "string",
            description: "Fecha de inicio del rango (formato ISO 8601, p.ej. '2024-01-01')",
          },
          fecha_hasta: {
            type: "string",
            description: "Fecha de fin del rango (formato ISO 8601, p.ej. '2024-12-31')",
          },
          rfc_receptor: {
            type: "string",
            description: "Filtrar por RFC del receptor",
          },
          estatus: {
            type: "string",
            enum: ["vigente", "cancelado", "cancelacion_en_proceso"],
            description: "Filtrar por estatus del CFDI",
          },
        },
      },
    },
    {
      name: "consultar_cfdi",
      description:
        "Obtiene el detalle completo de un CFDI por su ID interno en Ipsofactura. " +
        "Incluye datos del emisor, receptor, conceptos, impuestos, folio fiscal (UUID) y estatus.",
      inputSchema: {
        type: "object",
        properties: {
          id: {
            type: "string",
            description: "ID interno del CFDI en Ipsofactura",
          },
        },
        required: ["id"],
      },
    },
    {
      name: "obtener_xml",
      description:
        "Obtiene la URL de descarga del XML firmado de un CFDI. " +
        "El XML contiene el timbre fiscal digital (TFD) del SAT y es el documento oficial. " +
        "Retorna la url_xml para descargar el archivo.",
      inputSchema: {
        type: "object",
        properties: {
          id: {
            type: "string",
            description: "ID interno del CFDI en Ipsofactura",
          },
        },
        required: ["id"],
      },
    },
  ],
}));

// ---------------------------------------------------------------------------
// Tool handlers
// ---------------------------------------------------------------------------

server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  switch (name) {
    // -----------------------------------------------------------------------
    case "listar_empresas": {
      try {
        const { q, include_certificados } = (args ?? {}) as {
          q?: string;
          include_certificados?: boolean;
        };
        const result = await apiRequest<unknown>(
          "GET",
          "/empresas",
          undefined,
          { q, include_certificados },
        );
        return ok(result);
      } catch (e) {
        return err(e);
      }
    }

    // -----------------------------------------------------------------------
    case "timbrar_cfdi": {
      try {
        const payload = (args ?? {}) as Record<string, unknown>;

        // Auto-generate idempotency key from (serie, folio, rfc_emisor)
        const serie = typeof payload.serie === "string" ? payload.serie : "";
        const folio = typeof payload.folio === "string" ? payload.folio : "";
        const rfcEmisor =
          typeof payload.rfc_emisor === "string" ? payload.rfc_emisor : "";
        const idempotencyKey = `${serie}-${folio}-${rfcEmisor}`;

        const body = { ...payload, idempotency_key: idempotencyKey };
        const result = await apiRequest<unknown>("POST", "/cfdi/timbrar", body);
        return ok(result);
      } catch (e) {
        return err(e);
      }
    }

    // -----------------------------------------------------------------------
    case "cancelar_cfdi": {
      try {
        const { id, motivo, folio_sustitucion } = (args ?? {}) as {
          id: string;
          motivo: string;
          folio_sustitucion?: string;
        };
        if (!id || !/^[a-zA-Z0-9_-]{1,100}$/.test(id)) {
          return err(new Error("ID inválido: debe ser alfanumérico (máx 100 caracteres)"));
        }
        const body: Record<string, unknown> = { id, motivo };
        if (folio_sustitucion) body.folio_sustitucion = folio_sustitucion;
        const result = await apiRequest<unknown>("POST", "/cfdi/cancelar", body);
        return ok(result);
      } catch (e) {
        return err(e);
      }
    }

    // -----------------------------------------------------------------------
    case "listar_cfdis": {
      try {
        const {
          page,
          per_page,
          tipo_comprobante,
          serie,
          folio,
          rfc_emisor,
          folio_fiscal,
          fecha_desde,
          fecha_hasta,
          rfc_receptor,
          estatus,
        } = (args ?? {}) as {
          page?: number;
          per_page?: number;
          tipo_comprobante?: string;
          serie?: string;
          folio?: string;
          rfc_emisor?: string;
          folio_fiscal?: string;
          fecha_desde?: string;
          fecha_hasta?: string;
          rfc_receptor?: string;
          estatus?: string;
        };
        const result = await apiRequest<unknown>(
          "GET",
          "/cfdi",
          undefined,
          {
            page,
            per_page,
            tipo_comprobante,
            serie,
            folio,
            rfc_emisor,
            folio_fiscal,
            fecha_desde,
            fecha_hasta,
            rfc_receptor,
            estatus,
          },
        );
        return ok(result);
      } catch (e) {
        return err(e);
      }
    }

    // -----------------------------------------------------------------------
    case "consultar_cfdi": {
      try {
        const { id } = (args ?? {}) as { id: string };
        if (!id || !/^[a-zA-Z0-9_-]{1,100}$/.test(id)) {
          return err(new Error("ID inválido: debe ser alfanumérico (máx 100 caracteres)"));
        }
        const result = await apiRequest<unknown>("GET", `/cfdi/${encodeURIComponent(id)}`);
        return ok(result);
      } catch (e) {
        return err(e);
      }
    }

    // -----------------------------------------------------------------------
    case "obtener_xml": {
      try {
        const { id } = (args ?? {}) as { id: string };
        if (!id || !/^[a-zA-Z0-9_-]{1,100}$/.test(id)) {
          return err(new Error("ID inválido: debe ser alfanumérico (máx 100 caracteres)"));
        }
        const result = await apiRequest<unknown>("GET", `/cfdi/${encodeURIComponent(id)}/xml`);
        return ok(result);
      } catch (e) {
        return err(e);
      }
    }

    // -----------------------------------------------------------------------
    default:
      return {
        content: [
          {
            type: "text" as const,
            text: `Error: Herramienta desconocida: ${name}`,
          },
        ],
        isError: true,
      };
  }
});

// ---------------------------------------------------------------------------
// Start server
// ---------------------------------------------------------------------------

async function main(): Promise<void> {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  process.stderr.write(
    `[ipsofactura-mcp] Servidor iniciado. Base URL: ${IPS_API_BASE_URL}\n`,
  );
}

main().catch((e: unknown) => {
  process.stderr.write(`[ipsofactura-mcp] Error fatal: ${(e as Error).message}\n`);
  process.exit(1);
});
