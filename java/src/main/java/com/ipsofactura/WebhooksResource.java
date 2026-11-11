package com.ipsofactura;

import java.util.Map;

/**
 * Gestión de webhooks: configuración, deliveries y reenvíos.
 *
 * <pre>{@code
 * // Crear webhook
 * Map<String,Object> payload = Map.of(
 *     "url", "https://mi-servidor.com/hooks/ipso",
 *     "eventos", List.of("cfdi.timbrado", "cfdi.cancelado")
 * );
 * Map<String,Object> wh = client.webhooks().crear(payload);
 * String webhookId = (String) wh.get("id");
 * }</pre>
 */
public class WebhooksResource {

    private final IpsofacturaClient client;

    WebhooksResource(IpsofacturaClient client) {
        this.client = client;
    }

    // -------------------------------------------------------------------------
    // CRUD de webhooks
    // -------------------------------------------------------------------------

    /**
     * Lista todos los webhooks configurados en la cuenta.
     * GET /webhooks
     */
    public Map<String, Object> list() {
        return client.request("GET", "/webhooks", null);
    }

    /**
     * Crea un nuevo webhook.
     * POST /webhooks
     *
     * @param payload Datos del webhook (url, eventos, activo, etc.)
     */
    public Map<String, Object> crear(Map<String, Object> payload) {
        return client.request("POST", "/webhooks", payload);
    }

    /**
     * Actualiza parcialmente un webhook existente.
     * PATCH /webhooks/{id}
     *
     * @param id      ID del webhook
     * @param payload Campos a actualizar
     */
    public Map<String, Object> actualizar(String id, Map<String, Object> payload) {
        return client.request("PATCH", "/webhooks/" + id, payload);
    }

    /**
     * Elimina un webhook.
     * DELETE /webhooks/{id}
     */
    public Map<String, Object> eliminar(String id) {
        return client.request("DELETE", "/webhooks/" + id, null);
    }

    // -------------------------------------------------------------------------
    // Deliveries
    // -------------------------------------------------------------------------

    /**
     * Lista los intentos de entrega de un webhook.
     * GET /webhooks/{id}/deliveries
     *
     * @param webhookId ID del webhook
     */
    public Map<String, Object> listDeliveries(String webhookId) {
        return client.request("GET", "/webhooks/" + webhookId + "/deliveries", null);
    }

    /**
     * Obtiene el detalle de un intento de entrega específico.
     * GET /webhooks/{id}/deliveries/{deliveryId}
     *
     * @param webhookId  ID del webhook
     * @param deliveryId ID del delivery
     */
    public Map<String, Object> getDelivery(String webhookId, String deliveryId) {
        return client.request("GET", "/webhooks/" + webhookId + "/deliveries/" + deliveryId, null);
    }

    /**
     * Reenvía un delivery fallido.
     * POST /webhooks/{id}/deliveries/{deliveryId}/resend
     *
     * @param webhookId  ID del webhook
     * @param deliveryId ID del delivery a reenviar
     */
    public Map<String, Object> resendDelivery(String webhookId, String deliveryId) {
        return client.request("POST", "/webhooks/" + webhookId + "/deliveries/" + deliveryId + "/resend", null);
    }

    /**
     * Envía un evento de prueba al endpoint del webhook para verificar conectividad.
     * POST /webhooks/{id}/test
     *
     * @param webhookId ID del webhook
     */
    public Map<String, Object> test(String webhookId) {
        return client.request("POST", "/webhooks/" + webhookId + "/test", null);
    }
}
