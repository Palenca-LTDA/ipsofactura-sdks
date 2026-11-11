module Ipsofactura
  # Gestión de webhooks: configuración, deliveries y reenvíos.
  #
  #   wh = client.webhooks.crear(url: "https://mi-servidor.com/hooks", eventos: ["cfdi.timbrado"])
  #   id = wh["id"]
  #
  class WebhooksResource
    def initialize(client)
      @client = client
    end

    # -------------------------------------------------------------------------
    # CRUD de webhooks
    # -------------------------------------------------------------------------

    # Lista todos los webhooks configurados en la cuenta.
    # GET /webhooks
    def list
      @client.request("GET", "/webhooks")
    end

    # Crea un nuevo webhook.
    # POST /webhooks
    #
    # @param payload [Hash] url, eventos, activo, etc.
    def crear(payload)
      @client.request("POST", "/webhooks", payload)
    end

    # Actualiza parcialmente un webhook existente.
    # PATCH /webhooks/{id}
    #
    # @param id      [String] ID del webhook
    # @param payload [Hash]   Campos a actualizar
    def actualizar(id, payload)
      @client.request("PATCH", "/webhooks/#{id}", payload)
    end

    # Elimina un webhook.
    # DELETE /webhooks/{id}
    #
    # @param id [String] ID del webhook
    def eliminar(id)
      @client.request("DELETE", "/webhooks/#{id}")
    end

    # -------------------------------------------------------------------------
    # Deliveries
    # -------------------------------------------------------------------------

    # Lista los intentos de entrega de un webhook.
    # GET /webhooks/{id}/deliveries
    #
    # @param webhook_id [String] ID del webhook
    def list_deliveries(webhook_id)
      @client.request("GET", "/webhooks/#{webhook_id}/deliveries")
    end

    # Obtiene el detalle de un intento de entrega específico.
    # GET /webhooks/{id}/deliveries/{delivery_id}
    #
    # @param webhook_id  [String] ID del webhook
    # @param delivery_id [String] ID del delivery
    def get_delivery(webhook_id, delivery_id)
      @client.request("GET", "/webhooks/#{webhook_id}/deliveries/#{delivery_id}")
    end

    # Reenvía un delivery fallido.
    # POST /webhooks/{id}/deliveries/{delivery_id}/resend
    #
    # @param webhook_id  [String] ID del webhook
    # @param delivery_id [String] ID del delivery a reenviar
    def resend_delivery(webhook_id, delivery_id)
      @client.request("POST", "/webhooks/#{webhook_id}/deliveries/#{delivery_id}/resend")
    end

    # Envía un evento de prueba al endpoint del webhook para verificar conectividad.
    # POST /webhooks/{id}/test
    #
    # @param webhook_id [String] ID del webhook
    def test(webhook_id)
      @client.request("POST", "/webhooks/#{webhook_id}/test")
    end
  end
end
