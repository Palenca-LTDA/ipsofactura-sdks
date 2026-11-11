package com.ipsofactura;

import java.util.HashMap;
import java.util.Map;

/**
 * Acceso a todos los endpoints de CFDI: timbrado, cancelación, exports y consultas.
 *
 * <pre>{@code
 * Map<String,Object> payload = new HashMap<>();
 * payload.put("rfc_emisor", "EKU9003173C9");
 * // ... resto del payload
 * Map<String,Object> result = client.cfdi().timbrar(payload);
 * String uuid = (String) result.get("uuid");
 * }</pre>
 */
public class CfdiResource {

    private final IpsofacturaClient client;

    CfdiResource(IpsofacturaClient client) {
        this.client = client;
    }

    // -------------------------------------------------------------------------
    // Timbrado
    // -------------------------------------------------------------------------

    /**
     * Timbra un CFDI 4.0 en formato JSON.
     * POST /cfdi/timbrar
     */
    public Map<String, Object> timbrar(Map<String, Object> payload) {
        return client.request("POST", "/cfdi/timbrar", payload);
    }

    /**
     * Timbra un CFDI de nómina (complemento de nómina 1.2).
     * POST /cfdi/timbrar-nomina
     */
    public Map<String, Object> timbrarNomina(Map<String, Object> payload) {
        return client.request("POST", "/cfdi/timbrar-nomina", payload);
    }

    /**
     * Timbra un CFDI enviando el XML pre-generado (base64 o texto plano).
     * POST /cfdi/timbrar-xml
     */
    public Map<String, Object> timbrarXml(Map<String, Object> payload) {
        return client.request("POST", "/cfdi/timbrar-xml", payload);
    }

    /**
     * Timbra un CFDI ya sellado (pre-sellado por el emisor).
     * POST /cfdi/timbrar-sellado
     */
    public Map<String, Object> timbrarSellado(Map<String, Object> payload) {
        return client.request("POST", "/cfdi/timbrar-sellado", payload);
    }

    // -------------------------------------------------------------------------
    // Cancelación
    // -------------------------------------------------------------------------

    /**
     * Cancela un CFDI ante el SAT.
     * POST /cfdi/cancelar
     */
    public Map<String, Object> cancelar(Map<String, Object> payload) {
        return client.request("POST", "/cfdi/cancelar", payload);
    }

    // -------------------------------------------------------------------------
    // Complemento de pago
    // -------------------------------------------------------------------------

    /**
     * Genera un complemento de pago (REP).
     * POST /cfdi/complemento-pago
     */
    public Map<String, Object> complementoPago(Map<String, Object> payload) {
        return client.request("POST", "/cfdi/complemento-pago", payload);
    }

    // -------------------------------------------------------------------------
    // Exports
    // -------------------------------------------------------------------------

    /**
     * Crea una exportación masiva de CFDIs (proceso asíncrono).
     * POST /cfdi/exports
     */
    public Map<String, Object> crearExport(Map<String, Object> payload) {
        return client.request("POST", "/cfdi/exports", payload);
    }

    /**
     * Consulta el estado o resultado de una exportación por su ID.
     * GET /cfdi/exports/{id}
     */
    public Map<String, Object> getExport(String id) {
        return client.request("GET", "/cfdi/exports/" + id, null);
    }

    // -------------------------------------------------------------------------
    // Consultas
    // -------------------------------------------------------------------------

    /**
     * Lista CFDIs con paginación y filtros opcionales.
     * GET /cfdi
     *
     * @param params Filtros opcionales: page, per_page, tipo_comprobante,
     *               serie, folio, rfc_emisor, folio_fiscal. Puede ser null.
     */
    public Map<String, Object> list(Map<String, String> params) {
        return client.get("/cfdi", params);
    }

    /** Lista CFDIs sin filtros adicionales. */
    public Map<String, Object> list() {
        return list(null);
    }

    /**
     * Obtiene un CFDI por su ID interno de Ipsofactura.
     * GET /cfdi/{id}
     */
    public Map<String, Object> get(String id) {
        return client.request("GET", "/cfdi/" + id, null);
    }

    /**
     * Consulta el estatus de un CFDI ante el SAT.
     * GET /cfdi/{id}/estatus
     */
    public Map<String, Object> getEstatus(String id) {
        return client.request("GET", "/cfdi/" + id + "/estatus", null);
    }

    /**
     * Obtiene la URL de descarga del XML de un CFDI.
     * GET /cfdi/{id}/xml  → { xml_url: "..." }
     */
    public Map<String, Object> getXml(String id) {
        return client.request("GET", "/cfdi/" + id + "/xml", null);
    }

    /**
     * Obtiene la URL de descarga del PDF de un CFDI.
     * GET /cfdi/{id}/pdf  → { pdf_url: "..." }
     */
    public Map<String, Object> getPdf(String id) {
        return client.request("GET", "/cfdi/" + id + "/pdf", null);
    }

    /**
     * Obtiene el acuse de cancelación de un CFDI.
     * GET /cfdi/{id}/acuse
     */
    public Map<String, Object> getAcuse(String id) {
        return client.request("GET", "/cfdi/" + id + "/acuse", null);
    }
}
