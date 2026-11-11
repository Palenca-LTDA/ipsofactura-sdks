module Ipsofactura
  # Gestión de empresas emisoras y sus certificados (CSD).
  #
  #   empresa = client.empresas.crear(rfc: "EKU9003173C9", nombre: "MI EMPRESA")
  #   id      = empresa["id"]
  #
  class EmpresasResource
    def initialize(client)
      @client = client
    end

    # -------------------------------------------------------------------------
    # CRUD de empresas
    # -------------------------------------------------------------------------

    # Lista todas las empresas registradas en la cuenta.
    # GET /empresas
    #
    # @param params [Hash, nil] Parámetros de consulta opcionales (page, per_page, etc.)
    def list(params = nil)
      @client.request("GET", "/empresas", nil, params)
    end

    # Registra una nueva empresa emisora.
    # POST /empresas
    #
    # @param payload [Hash] Datos de la empresa (rfc, nombre, regimen_fiscal, domicilio_fiscal, etc.)
    def crear(payload)
      @client.request("POST", "/empresas", payload)
    end

    # Obtiene los datos de una empresa por su ID.
    # GET /empresas/{id}
    def get(id)
      @client.request("GET", "/empresas/#{id}")
    end

    # Actualiza parcialmente los datos de una empresa.
    # PATCH /empresas/{id}
    #
    # @param id      [String] ID de la empresa
    # @param payload [Hash]   Campos a actualizar
    def actualizar(id, payload)
      @client.request("PATCH", "/empresas/#{id}", payload)
    end

    # -------------------------------------------------------------------------
    # Certificados (CSD)
    # -------------------------------------------------------------------------

    # Sube o actualiza el certificado de sello digital (CSD) de una empresa.
    # POST /empresas/{id}/certificados
    #
    # @param empresa_id [String] ID de la empresa
    # @param payload    [Hash]   cer_base64, key_base64, password
    def subir_certificado(empresa_id, payload)
      @client.request("POST", "/empresas/#{empresa_id}/certificados", payload)
    end

    # Actualiza la contraseña de un certificado existente.
    # POST /empresas/{id}/certificados/refresh-password
    #
    # @param empresa_id [String] ID de la empresa
    # @param payload    [Hash]   numero (serie del certificado), password (nueva contraseña)
    def refresh_password(empresa_id, payload)
      @client.request("POST", "/empresas/#{empresa_id}/certificados/refresh-password", payload)
    end

    # Elimina un certificado por su número de serie.
    # DELETE /empresas/{id}/certificados/{numero}
    #
    # @param empresa_id [String] ID de la empresa
    # @param numero     [String] Número de serie del certificado
    def eliminar_certificado(empresa_id, numero)
      @client.request("DELETE", "/empresas/#{empresa_id}/certificados/#{numero}")
    end
  end
end
