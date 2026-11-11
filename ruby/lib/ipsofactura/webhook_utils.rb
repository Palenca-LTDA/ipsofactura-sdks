require "openssl"

module Ipsofactura
  # Utilidades para verificar firmas de webhooks de Ipsofactura.
  #
  # Ipsofactura firma cada evento con HMAC-SHA256 usando el secreto del webhook.
  # El mensaje firmado tiene la forma: "<timestamp>.<payload>"
  #
  #   valid = Ipsofactura::WebhookUtils.verify_signature(
  #     request.body.read,                    # body crudo del POST
  #     request.env["HTTP_X_IPSO_TIMESTAMP"],
  #     request.env["HTTP_X_IPSO_SIGNATURE"],
  #     webhook_secret
  #   )
  #   raise "Firma inválida" unless valid
  #
  module WebhookUtils
    # Verifica que la firma de un evento webhook sea auténtica.
    #
    # @param payload    [String]  Cuerpo crudo del request HTTP
    # @param timestamp  [String]  Valor del header x-ipso-timestamp
    # @param signature  [String]  Valor del header x-ipso-signature
    # @param secret     [String]  Secreto del webhook
    # @return           [Boolean] true si la firma es válida
    def self.verify_signature(payload, timestamp, signature, secret)
      return false if [payload, timestamp, signature, secret].any? { |v| v.nil? || v.empty? }

      message  = "#{timestamp}.#{payload}"
      expected = OpenSSL::HMAC.hexdigest("sha256", secret, message)
      secure_compare(expected, signature)
    end

    # Constant-time string comparison to prevent timing attacks.
    # Equivalent to Rack::Utils.secure_compare but without the Rack dependency.
    #
    # @param a [String]
    # @param b [String]
    # @return  [Boolean]
    def self.secure_compare(a, b)
      return false unless a.bytesize == b.bytesize

      l = a.unpack("C#{a.bytesize}")
      r = b.unpack("C#{b.bytesize}")
      result = 0
      l.zip(r).each { |x, y| result |= x ^ y }
      result == 0
    end

    private_class_method :secure_compare
  end
end
