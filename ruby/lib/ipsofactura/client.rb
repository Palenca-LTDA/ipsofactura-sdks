require "net/http"
require "uri"
require "json"

module Ipsofactura
  SANDBOX_URL = "https://sandbox.api.ipsofactura.com"
  PROD_URL    = "https://api.ipsofactura.com"

  # Raised when the Ipsofactura API returns an HTTP error (4xx / 5xx).
  class Error < StandardError
    attr_reader :status, :code, :sat_code, :retry_after

    # @param status      [Integer]        HTTP status code
    # @param code        [String]         Machine-readable error code (e.g. "VALIDATION_ERROR")
    # @param message     [String]         Human-readable error description
    # @param sat_code    [String, nil]    SAT CFDI error code when applicable
    # @param retry_after [Integer, nil]   Seconds to wait before retrying
    def initialize(status, code, message, sat_code: nil, retry_after: nil)
      super(message)
      @status      = status
      @code        = code
      @sat_code    = sat_code
      @retry_after = retry_after
    end

    def to_s
      parts = ["Ipsofactura::Error(#{@status}) #{@code}: #{message}"]
      parts << "sat_code=#{@sat_code}" if @sat_code
      parts << "retry_after=#{@retry_after}s" if @retry_after
      parts.join(" | ")
    end
  end

  # Main client. Instantiate once and reuse.
  #
  #   client = Ipsofactura::Client.new("mi_api_key")                         # producción
  #   client = Ipsofactura::Client.new("mi_api_key", base_url: SANDBOX_URL)  # sandbox
  #
  class Client
    attr_reader :cfdi, :empresas, :webhooks

    def initialize(api_key, base_url: PROD_URL)
      raise ArgumentError, "api_key must not be blank" if api_key.nil? || api_key.strip.empty?

      @api_key  = api_key
      @base_url = base_url.chomp("/")
      @cfdi     = CfdiResource.new(self)
      @empresas = EmpresasResource.new(self)
      @webhooks = WebhooksResource.new(self)
    end

    # -------------------------------------------------------------------
    # Internal HTTP helpers (used by resource classes)
    # -------------------------------------------------------------------

    # @api private
    def request(method, path, body = nil, params = nil)
      uri = build_uri(path, params)

      http = Net::HTTP.new(uri.host, uri.port)
      http.use_ssl     = uri.scheme == "https"
      http.read_timeout = 30
      http.open_timeout = 10

      req_class = {
        "GET"    => Net::HTTP::Get,
        "POST"   => Net::HTTP::Post,
        "PATCH"  => Net::HTTP::Patch,
        "DELETE" => Net::HTTP::Delete
      }.fetch(method.upcase) { raise ArgumentError, "Unsupported HTTP method: #{method}" }

      req = req_class.new(uri)
      req["x-api"]        = @api_key
      req["Content-Type"] = "application/json"
      req["Accept"]       = "application/json"

      if body
        req.body = body.to_json
      elsif %w[POST PATCH].include?(method.upcase)
        req.body = "{}"
      end

      response = http.request(req)
      handle_response(response)
    end

    private

    def build_uri(path, params)
      url = @base_url + path
      if params && !params.empty?
        query = params.map { |k, v| "#{URI.encode_www_form_component(k.to_s)}=#{URI.encode_www_form_component(v.to_s)}" }.join("&")
        url = "#{url}?#{query}"
      end
      URI.parse(url)
    end

    def handle_response(response)
      status = response.code.to_i
      body   = response.body

      if status >= 200 && status < 300
        return {} if body.nil? || body.strip.empty?
        parsed = JSON.parse(body)
        return parsed.is_a?(Hash) ? parsed : { "data" => parsed }
      end

      # Parse error body
      code        = "UNKNOWN_ERROR"
      message     = "HTTP #{status}"
      sat_code    = nil
      retry_after = nil

      if body && !body.strip.empty?
        begin
          err         = JSON.parse(body)
          code        = err["code"]        if err["code"]
          message     = err["message"]     if err["message"]
          sat_code    = err["sat_code"]    if err["sat_code"]
          retry_after = err["retryAfter"].to_i if err["retryAfter"]
        rescue JSON::ParserError
          message = body
        end
      end

      raise Ipsofactura::Error.new(status, code, message, sat_code: sat_code, retry_after: retry_after)
    end
  end
end
