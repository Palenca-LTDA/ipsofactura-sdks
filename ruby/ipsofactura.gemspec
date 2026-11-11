Gem::Specification.new do |s|
  s.name        = "ipsofactura"
  s.version     = "0.1.0"
  s.summary     = "Cliente oficial de Ipsofactura — timbrado CFDI 4.0 para México"
  s.description = "SDK Ruby para la API de Ipsofactura. Permite timbrar, cancelar y " \
                  "gestionar CFDIs 4.0 ante el SAT, además de gestionar empresas, " \
                  "certificados y webhooks. Sin dependencias externas; solo stdlib."
  s.authors     = ["Palenca"]
  s.email       = ["hola@palenca.com"]
  s.homepage    = "https://github.com/Palenca-LTDA/ipsofactura-sdk"
  s.license     = "MIT"

  s.files = [
    "lib/ipsofactura.rb",
    "lib/ipsofactura/client.rb",
    "lib/ipsofactura/resources/cfdi.rb",
    "lib/ipsofactura/resources/empresas.rb",
    "lib/ipsofactura/resources/webhooks.rb",
    "lib/ipsofactura/webhook_utils.rb"
  ]

  s.required_ruby_version = ">= 2.7"

  # Zero runtime gem dependencies — only stdlib
  # (net/http, uri, json, openssl)
end
