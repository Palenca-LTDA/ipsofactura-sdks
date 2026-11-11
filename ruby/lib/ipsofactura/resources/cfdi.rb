module Ipsofactura
  # Acceso a todos los endpoints de CFDI: timbrado, cancelación, exports y consultas.
  #
  #   result = client.cfdi.timbrar(payload)
  #   uuid   = result["uuid"]
  #
  class CfdiResource
    def initialize(client)
      @client = client
    end

    # -------------------------------------------------------------------------
    # Timbrado
    # -------------------------------------------------------------------------

    # Timbra un CFDI 4.0 en formato JSON.
    # POST /cfdi/timbrar
    #
    # @param payload [Hash] Datos del comprobante
    # @return [Hash] Respuesta con uuid, xml_url, pdf_url, etc.
    def timbrar(payload)
      @client.request("POST", "/cfdi/timbrar", payload)
    end

    # Timbra un CFDI de nómina (complemento de nómina 1.2).
    # POST /cfdi/timbrar-nomina
    def timbrar_nomina(payload)
      @client.request("POST", "/cfdi/timbrar-nomina", payload)
    end

    # Timbra un CFDI enviando el XML pre-generado.
    # POST /cfdi/timbrar-xml
    def timbrar_xml(payload)
      @client.request("POST", "/cfdi/timbrar-xml", payload)
    end

    # Timbra un CFDI ya sellado (pre-sellado por el emisor).
    # POST /cfdi/timbrar-sellado
    def timbrar_sellado(payload)
      @client.request("POST", "/cfdi/timbrar-sellado", payload)
    end

    # -------------------------------------------------------------------------
    # Cancelación
    # -------------------------------------------------------------------------

    # Cancela un CFDI ante el SAT.
    # POST /cfdi/cancelar
    #
    # @param payload [Hash] uuid, rfc_emisor, motivo, [uuid_sustituto]
    def cancelar(payload)
      @client.request("POST", "/cfdi/cancelar", payload)
    end

    # -------------------------------------------------------------------------
    # Complemento de pago
    # -------------------------------------------------------------------------

    # Genera un complemento de pago (REP).
    # POST /cfdi/complemento-pago
    def complemento_pago(payload)
      @client.request("POST", "/cfdi/complemento-pago", payload)
    end

    # -------------------------------------------------------------------------
    # Exports
    # -------------------------------------------------------------------------

    # Crea una exportación masiva de CFDIs (proceso asíncrono).
    # POST /cfdi/exports
    def crear_export(payload)
      @client.request("POST", "/cfdi/exports", payload)
    end

    # Consulta el estado o resultado de una exportación por su ID.
    # GET /cfdi/exports/{id}
    def get_export(id)
      @client.request("GET", "/cfdi/exports/#{id}")
    end

    # -------------------------------------------------------------------------
    # Consultas
    # -------------------------------------------------------------------------

    # Lista CFDIs con paginación y filtros opcionales.
    # GET /cfdi
    #
    # @param params [Hash, nil] Filtros: page, per_page, tipo_comprobante,
    #                           serie, folio, rfc_emisor, folio_fiscal
    def list(params = nil)
      @client.request("GET", "/cfdi", nil, params)
    end

    # Obtiene un CFDI por su ID interno de Ipsofactura.
    # GET /cfdi/{id}
    def get(id)
      @client.request("GET", "/cfdi/#{id}")
    end

    # Consulta el estatus de un CFDI ante el SAT.
    # GET /cfdi/{id}/estatus
    def get_estatus(id)
      @client.request("GET", "/cfdi/#{id}/estatus")
    end

    # Obtiene la URL de descarga del XML de un CFDI.
    # GET /cfdi/{id}/xml  → { "xml_url" => "..." }
    def get_xml(id)
      @client.request("GET", "/cfdi/#{id}/xml")
    end

    # Obtiene la URL de descarga del PDF de un CFDI.
    # GET /cfdi/{id}/pdf  → { "pdf_url" => "..." }
    def get_pdf(id)
      @client.request("GET", "/cfdi/#{id}/pdf")
    end

    # Obtiene el acuse de cancelación de un CFDI.
    # GET /cfdi/{id}/acuse
    def get_acuse(id)
      @client.request("GET", "/cfdi/#{id}/acuse")
    end
  end
end
