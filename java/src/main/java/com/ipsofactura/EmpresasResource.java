package com.ipsofactura;

import java.util.Map;

/**
 * Acceso a la gestión de empresas y certificados (CSD/FIEL).
 *
 * <pre>{@code
 * // Listar empresas
 * Map<String,Object> result = client.empresas().list();
 *
 * // Crear empresa
 * Map<String,Object> payload = Map.of(
 *     "rfc", "EKU9003173C9",
 *     "nombre", "EMPRESA DE PRUEBA SA DE CV"
 * );
 * Map<String,Object> empresa = client.empresas().crear(payload);
 * String empresaId = (String) empresa.get("id");
 * }</pre>
 */
public class EmpresasResource {

    private final IpsofacturaClient client;

    EmpresasResource(IpsofacturaClient client) {
        this.client = client;
    }

    // -------------------------------------------------------------------------
    // CRUD de empresas
    // -------------------------------------------------------------------------

    /**
     * Lista todas las empresas registradas en la cuenta.
     * GET /empresas
     *
     * @param params Parámetros de consulta opcionales (page, per_page, etc.). Puede ser null.
     */
    public Map<String, Object> list(Map<String, String> params) {
        return client.get("/empresas", params);
    }

    /** Lista empresas sin parámetros adicionales. */
    public Map<String, Object> list() {
        return list(null);
    }

    /**
     * Registra una nueva empresa emisora.
     * POST /empresas
     *
     * @param payload Datos de la empresa (rfc, nombre, regimen_fiscal, domicilio_fiscal, etc.)
     */
    public Map<String, Object> crear(Map<String, Object> payload) {
        return client.request("POST", "/empresas", payload);
    }

    /**
     * Obtiene los datos de una empresa por su ID.
     * GET /empresas/{id}
     */
    public Map<String, Object> get(String id) {
        return client.request("GET", "/empresas/" + id, null);
    }

    /**
     * Actualiza parcialmente los datos de una empresa.
     * PATCH /empresas/{id}
     *
     * @param id      ID de la empresa
     * @param payload Campos a actualizar
     */
    public Map<String, Object> actualizar(String id, Map<String, Object> payload) {
        return client.request("PATCH", "/empresas/" + id, payload);
    }

    // -------------------------------------------------------------------------
    // Certificados (CSD)
    // -------------------------------------------------------------------------

    /**
     * Sube o actualiza el certificado de sello digital (CSD) de una empresa.
     * POST /empresas/{id}/certificados
     *
     * @param empresaId ID de la empresa
     * @param payload   Payload con certificado (.cer en base64), llave (.key en base64) y contraseña
     */
    public Map<String, Object> subirCertificado(String empresaId, Map<String, Object> payload) {
        return client.request("POST", "/empresas/" + empresaId + "/certificados", payload);
    }

    /**
     * Actualiza la contraseña de un certificado existente.
     * POST /empresas/{id}/certificados/refresh-password
     *
     * @param empresaId ID de la empresa
     * @param payload   Payload con numero (número de serie del certificado) y nueva contraseña
     */
    public Map<String, Object> refreshPassword(String empresaId, Map<String, Object> payload) {
        return client.request("POST", "/empresas/" + empresaId + "/certificados/refresh-password", payload);
    }

    /**
     * Elimina un certificado por su número de serie.
     * DELETE /empresas/{id}/certificados/{numero}
     *
     * @param empresaId ID de la empresa
     * @param numero    Número de serie del certificado a eliminar
     */
    public Map<String, Object> eliminarCertificado(String empresaId, String numero) {
        return client.request("DELETE", "/empresas/" + empresaId + "/certificados/" + numero, null);
    }
}
