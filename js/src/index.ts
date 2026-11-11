/**
 * Ipsofactura SDK — TypeScript/JavaScript client
 *
 * Uso básico:
 *   import IpsofacturaClient from "ipsofactura";
 *   const client = new IpsofacturaClient("tu-api-key");
 *   const cfdi = await client.cfdi.timbrar({ ... });
 */

import { createHmac, timingSafeEqual } from "node:crypto";

// ---------------------------------------------------------------------------
// Error
// ---------------------------------------------------------------------------

/**
 * Error estructurado que devuelve la API de Ipsofactura.
 * Siempre incluye `status` (HTTP), `code` y `message`.
 * Cuando el SAT rechaza la factura se incluye `satCode`.
 * Si el servidor pide reintentar más tarde se incluye `retryAfter` (segundos).
 */
export class IpsofacturaError extends Error {
  constructor(
    public readonly status: number,
    public readonly code: string,
    message: string,
    public readonly satCode?: string,
    public readonly retryAfter?: number,
  ) {
    super(message);
    this.name = "IpsofacturaError";
    // Mantener el stack trace correcto en V8
    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, IpsofacturaError);
    }
  }
}

// ---------------------------------------------------------------------------
// Tipos base
// ---------------------------------------------------------------------------

export interface RequestOptions {
  /** Sobrescribe la URL base para esta llamada concreta. */
  baseUrl?: string;
}

/** Parámetros de paginación y filtro comunes para listas de CFDI. */
export interface CfdiListParams {
  page?: number;
  per_page?: number;
  tipo_comprobante?: string;
  serie?: string;
  folio?: string;
  rfc_emisor?: string;
  folio_fiscal?: string;
}

/** Parámetros de búsqueda para empresas. */
export interface EmpresasListParams {
  q?: string;
  include_certificados?: boolean;
}

// ---------------------------------------------------------------------------
// Núcleo HTTP
// ---------------------------------------------------------------------------

/**
 * Clase base que maneja fetch, autenticación y deserialización de errores.
 * Los recursos heredan de esta clase y usan `this._request(...)`.
 */
class BaseResource {
  constructor(
    protected readonly _apiKey: string,
    protected readonly _baseUrl: string,
  ) {}

  /**
   * Realiza una petición HTTP autenticada.
   *
   * @param method  Verbo HTTP (GET, POST, PATCH, DELETE)
   * @param path    Path relativo, e.g. "/cfdi/timbrar"
   * @param body    Cuerpo JSON (opcional)
   * @param params  Query params; los valores `undefined` se omiten
   */
  protected async _request<T>(
    method: string,
    path: string,
    body?: unknown,
    params?: Record<string, string | number | boolean | undefined>,
  ): Promise<T> {
    const url = new URL(path, this._baseUrl);

    if (params) {
      for (const [key, value] of Object.entries(params)) {
        if (value !== undefined && value !== null) {
          url.searchParams.set(key, String(value));
        }
      }
    }

    const headers: Record<string, string> = {
      "x-api": this._apiKey,
      "Content-Type": "application/json",
      Accept: "application/json",
      "User-Agent": "ipsofactura-sdk-js/0.1.0",
    };

    const init: RequestInit = {
      method,
      headers,
    };

    if (body !== undefined) {
      init.body = JSON.stringify(body);
    }

    let response: Response;
    try {
      response = await fetch(url.toString(), init);
    } catch (err) {
      // Error de red (sin conexión, DNS, etc.)
      throw new IpsofacturaError(
        0,
        "NETWORK_ERROR",
        `Error de red al conectar con ${url.hostname}: ${(err as Error).message}`,
      );
    }

    // Respuestas sin cuerpo (204, 202 en algunos casos)
    if (response.status === 204) {
      return undefined as unknown as T;
    }

    let data: unknown;
    const contentType = response.headers.get("content-type") ?? "";

    if (contentType.includes("application/json")) {
      data = await response.json();
    } else {
      // Algunos endpoints devuelven texto plano o están vacíos
      data = await response.text();
    }

    if (!response.ok) {
      // La API siempre devuelve JSON de error en este formato:
      // { code, message, sat_code?, retryAfter? }
      if (typeof data === "object" && data !== null) {
        const err = data as Record<string, unknown>;
        throw new IpsofacturaError(
          response.status,
          String(err.code ?? "UNKNOWN_ERROR"),
          String(err.message ?? response.statusText),
          err.sat_code != null ? String(err.sat_code) : undefined,
          err.retryAfter != null ? Number(err.retryAfter) : undefined,
        );
      }
      throw new IpsofacturaError(
        response.status,
        "HTTP_ERROR",
        `HTTP ${response.status}: ${response.statusText}`,
      );
    }

    return data as T;
  }
}

// ---------------------------------------------------------------------------
// Recurso: CFDI
// ---------------------------------------------------------------------------

export class CfdiResource extends BaseResource {
  /**
   * Timbra una factura CFDI 4.0 (tipo I, E, T, N, P).
   * El payload debe incluir todos los campos requeridos por el SAT.
   */
  timbrar<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/cfdi/timbrar", payload);
  }

  /**
   * Timbra un CFDI de Nómina (tipo N, complemento nómina 1.2).
   */
  timbrarNomina<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/cfdi/timbrar-nomina", payload);
  }

  /**
   * Timbra un XML ya generado externamente.
   * El payload debe contener `{ xml: "<string base64 o XML en texto>" }`.
   */
  timbrarXml<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/cfdi/timbrar-xml", payload);
  }

  /**
   * Timbra un XML ya sellado (con cadena original y sello del emisor).
   */
  timbrarSellado<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/cfdi/timbrar-sellado", payload);
  }

  /**
   * Solicita la cancelación de un CFDI ante el SAT.
   * El payload debe incluir `{ id, motivo, folio_sustitucion? }`.
   */
  cancelar<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/cfdi/cancelar", payload);
  }

  /**
   * Genera un complemento de pago (tipo P) para una factura con PPD.
   */
  complementoPago<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/cfdi/complemento-pago", payload);
  }

  /**
   * Crea un trabajo de exportación masiva de CFDIs.
   * @returns El objeto del export con `id` y `status`.
   */
  crearExport<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/cfdi/exports", payload);
  }

  /**
   * Consulta el estado de un export previamente creado.
   * Cuando `status === "ready"` el campo `download_url` contiene la URL del ZIP.
   */
  getExport<T = unknown>(id: string): Promise<T> {
    return this._request<T>("GET", `/cfdi/exports/${encodeURIComponent(id)}`);
  }

  /**
   * Lista CFDIs con paginación y filtros opcionales.
   */
  list<T = unknown>(params?: CfdiListParams): Promise<T> {
    return this._request<T>(
      "GET",
      "/cfdi",
      undefined,
      params as Record<string, string | number | boolean | undefined>,
    );
  }

  /**
   * Obtiene un CFDI por su ID interno de Ipsofactura.
   */
  get<T = unknown>(id: string): Promise<T> {
    return this._request<T>("GET", `/cfdi/${encodeURIComponent(id)}`);
  }

  /**
   * Consulta el estatus del CFDI ante el SAT (vigente, cancelado, etc.).
   */
  getEstatus<T = unknown>(id: string): Promise<T> {
    return this._request<T>(
      "GET",
      `/cfdi/${encodeURIComponent(id)}/estatus`,
    );
  }

  /**
   * Obtiene la URL firmada del XML del CFDI.
   * @returns `{ xml_url: string }`
   */
  getXml<T = { xml_url: string }>(id: string): Promise<T> {
    return this._request<T>("GET", `/cfdi/${encodeURIComponent(id)}/xml`);
  }

  /**
   * Obtiene la URL firmada del PDF de la representación impresa.
   * @returns `{ pdf_url: string }`
   */
  getPdf<T = { pdf_url: string }>(id: string): Promise<T> {
    return this._request<T>("GET", `/cfdi/${encodeURIComponent(id)}/pdf`);
  }

  /**
   * Obtiene el acuse de cancelación de un CFDI cancelado.
   */
  getAcuse<T = unknown>(id: string): Promise<T> {
    return this._request<T>(
      "GET",
      `/cfdi/${encodeURIComponent(id)}/acuse`,
    );
  }
}

// ---------------------------------------------------------------------------
// Recurso: Empresas
// ---------------------------------------------------------------------------

export class EmpresasResource extends BaseResource {
  /**
   * Lista las empresas (emisores) registradas en la cuenta.
   * Usa `q` para buscar por RFC o nombre; `include_certificados` para incluir
   * los certificados CSD activos.
   */
  list<T = unknown>(params?: EmpresasListParams): Promise<T> {
    return this._request<T>(
      "GET",
      "/empresas",
      undefined,
      params as Record<string, string | number | boolean | undefined>,
    );
  }

  /**
   * Registra una nueva empresa emisora.
   * El payload debe incluir RFC, nombre, régimen fiscal y domicilio fiscal.
   */
  crear<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/empresas", payload);
  }

  /**
   * Obtiene el detalle de una empresa por su ID.
   */
  get<T = unknown>(id: string): Promise<T> {
    return this._request<T>("GET", `/empresas/${encodeURIComponent(id)}`);
  }

  /**
   * Actualiza campos de una empresa existente (razón social, dirección, etc.).
   * Solo envía los campos que quieres modificar.
   */
  actualizar<T = unknown>(
    id: string,
    payload: Record<string, unknown>,
  ): Promise<T> {
    return this._request<T>(
      "PATCH",
      `/empresas/${encodeURIComponent(id)}`,
      payload,
    );
  }

  /**
   * Sube un certificado CSD (.cer + .key + contraseña) a una empresa.
   * El payload debe incluir `{ cer: "<base64>", key: "<base64>", password: "..." }`.
   */
  subirCertificado<T = unknown>(
    empresaId: string,
    payload: Record<string, unknown>,
  ): Promise<T> {
    return this._request<T>(
      "POST",
      `/empresas/${encodeURIComponent(empresaId)}/certificados`,
      payload,
    );
  }

  /**
   * Actualiza la contraseña del certificado CSD sin tener que resubirlo.
   */
  refreshPassword<T = unknown>(
    empresaId: string,
    payload: Record<string, unknown>,
  ): Promise<T> {
    return this._request<T>(
      "POST",
      `/empresas/${encodeURIComponent(empresaId)}/certificados/refresh-password`,
      payload,
    );
  }

  /**
   * Elimina un certificado CSD de la empresa por su número de serie.
   * @param empresaId  ID de la empresa en Ipsofactura
   * @param numero     Número de serie del certificado (20 dígitos)
   */
  eliminarCertificado<T = unknown>(
    empresaId: string,
    numero: string,
  ): Promise<T> {
    return this._request<T>(
      "DELETE",
      `/empresas/${encodeURIComponent(empresaId)}/certificados/${encodeURIComponent(numero)}`,
    );
  }
}

// ---------------------------------------------------------------------------
// Recurso: Webhooks
// ---------------------------------------------------------------------------

export class WebhooksResource extends BaseResource {
  /**
   * Lista todos los webhooks configurados en la cuenta.
   */
  list<T = unknown>(): Promise<T> {
    return this._request<T>("GET", "/webhooks");
  }

  /**
   * Crea un nuevo webhook.
   * El payload debe incluir `{ url, events: string[], secret? }`.
   */
  crear<T = unknown>(payload: Record<string, unknown>): Promise<T> {
    return this._request<T>("POST", "/webhooks", payload);
  }

  /**
   * Actualiza un webhook existente (URL, eventos activos, habilitado/deshabilitado).
   */
  actualizar<T = unknown>(
    id: string,
    payload: Record<string, unknown>,
  ): Promise<T> {
    return this._request<T>(
      "PATCH",
      `/webhooks/${encodeURIComponent(id)}`,
      payload,
    );
  }

  /**
   * Elimina un webhook permanentemente.
   */
  eliminar<T = unknown>(id: string): Promise<T> {
    return this._request<T>(
      "DELETE",
      `/webhooks/${encodeURIComponent(id)}`,
    );
  }

  /**
   * Lista el historial de entregas (deliveries) de un webhook.
   * Útil para diagnosticar fallos de entrega.
   */
  listDeliveries<T = unknown>(webhookId: string): Promise<T> {
    return this._request<T>(
      "GET",
      `/webhooks/${encodeURIComponent(webhookId)}/deliveries`,
    );
  }

  /**
   * Obtiene el detalle de una entrega específica (request, response, status code).
   */
  getDelivery<T = unknown>(
    webhookId: string,
    deliveryId: string,
  ): Promise<T> {
    return this._request<T>(
      "GET",
      `/webhooks/${encodeURIComponent(webhookId)}/deliveries/${encodeURIComponent(deliveryId)}`,
    );
  }

  /**
   * Reintenta una entrega fallida.
   */
  resendDelivery<T = unknown>(
    webhookId: string,
    deliveryId: string,
  ): Promise<T> {
    return this._request<T>(
      "POST",
      `/webhooks/${encodeURIComponent(webhookId)}/deliveries/${encodeURIComponent(deliveryId)}/resend`,
    );
  }

  /**
   * Envía un payload de prueba al webhook para verificar que está funcionando.
   */
  test<T = unknown>(webhookId: string): Promise<T> {
    return this._request<T>(
      "POST",
      `/webhooks/${encodeURIComponent(webhookId)}/test`,
    );
  }
}

// ---------------------------------------------------------------------------
// Utilidades de webhook
// ---------------------------------------------------------------------------

/**
 * Verifica la firma HMAC-SHA256 de un evento de webhook entrante.
 *
 * Ipsofactura incluye dos headers en cada entrega:
 *   - `X-Ipso-Timestamp`: unix timestamp (segundos) del momento del envío
 *   - `X-Ipso-Signature`: HMAC-SHA256 de `"${timestamp}.${rawBody}"`
 *
 * @param payload    El cuerpo crudo de la petición (string, sin parsear)
 * @param timestamp  Valor del header `X-Ipso-Timestamp`
 * @param signature  Valor del header `X-Ipso-Signature`
 * @param secret     El secreto del webhook configurado en Ipsofactura
 * @returns `true` si la firma es válida, `false` en caso contrario
 *
 * @example
 * ```ts
 * import { verifyWebhookSignature } from "ipsofactura";
 *
 * app.post("/webhook", (req, res) => {
 *   const valid = verifyWebhookSignature(
 *     req.rawBody,
 *     req.headers["x-ipso-timestamp"],
 *     req.headers["x-ipso-signature"],
 *     process.env.WEBHOOK_SECRET,
 *   );
 *   if (!valid) return res.status(401).send("Invalid signature");
 *   // procesar evento...
 * });
 * ```
 */
export function verifyWebhookSignature(
  payload: string,
  timestamp: string,
  signature: string,
  secret: string,
): boolean {
  const expected = createHmac("sha256", secret)
    .update(`${timestamp}.${payload}`)
    .digest("hex");

  // Comparación en tiempo constante para prevenir timing attacks
  try {
    return timingSafeEqual(
      Buffer.from(expected, "utf8"),
      Buffer.from(signature, "utf8"),
    );
  } catch {
    // timingSafeEqual lanza si los buffers tienen distinto tamaño
    return false;
  }
}

// ---------------------------------------------------------------------------
// Cliente principal
// ---------------------------------------------------------------------------

export interface IpsofacturaClientOptions {
  /**
   * URL base de la API.
   * - Sandbox:    `https://sandbox.api.ipsofactura.com`  (default en test)
   * - Producción: `https://api.ipsofactura.com`          (default)
   */
  baseUrl?: string;
}

/**
 * Cliente principal de Ipsofactura.
 *
 * @example
 * ```ts
 * import IpsofacturaClient from "ipsofactura";
 *
 * const client = new IpsofacturaClient(process.env.IPSO_API_KEY!);
 *
 * // Timbrar una factura
 * const result = await client.cfdi.timbrar({ ... });
 * console.log(result.folio_fiscal);
 *
 * // Usar sandbox
 * const sandboxClient = new IpsofacturaClient(apiKey, {
 *   baseUrl: "https://sandbox.api.ipsofactura.com",
 * });
 * ```
 */
export class IpsofacturaClient {
  readonly cfdi: CfdiResource;
  readonly empresas: EmpresasResource;
  readonly webhooks: WebhooksResource;

  private readonly _apiKey: string;
  private readonly _baseUrl: string;

  constructor(apiKey: string, options: IpsofacturaClientOptions = {}) {
    if (!apiKey || typeof apiKey !== "string") {
      throw new Error(
        "IpsofacturaClient: se requiere una API key válida como primer argumento.",
      );
    }

    this._apiKey = apiKey;
    this._baseUrl = (options.baseUrl ?? "https://api.ipsofactura.com").replace(
      /\/$/,
      "",
    );

    this.cfdi = new CfdiResource(this._apiKey, this._baseUrl);
    this.empresas = new EmpresasResource(this._apiKey, this._baseUrl);
    this.webhooks = new WebhooksResource(this._apiKey, this._baseUrl);
  }
}

export default IpsofacturaClient;
