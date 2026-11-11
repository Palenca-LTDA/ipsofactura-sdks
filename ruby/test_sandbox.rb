#!/usr/bin/env ruby
# test_sandbox.rb — Script de integración contra el sandbox de Ipsofactura.
#
# Uso:
#   IPSO_API_KEY=tu_api_key ruby test_sandbox.rb
#
# Si IPSO_API_KEY no está definida, el script termina con un aviso.

$LOAD_PATH.unshift(File.join(__dir__, "lib"))
require "ipsofactura"

api_key = ENV["IPSO_API_KEY"]
if api_key.nil? || api_key.strip.empty?
  puts "IPSO_API_KEY no definida — omitiendo tests de sandbox."
  exit 0
end

client = Ipsofactura::Client.new(api_key, base_url: Ipsofactura::SANDBOX_URL)

passed = 0
failed = 0

def run_test(name)
  print "  #{name} ... "
  yield
  puts "OK"
  return true
rescue => e
  puts "FALLO: #{e}"
  return false
end

puts "\n=== Ipsofactura Sandbox Tests ===\n\n"

# ---------------------------------------------------------------------------
# 1. Listar empresas
# ---------------------------------------------------------------------------
puts "[ Empresas ]"
ok = run_test("empresas.list devuelve un resultado") do
  result = client.empresas.list
  raise "Respuesta vacía" if result.nil?
  raise "No contiene claves esperadas" if result.empty? && !result.is_a?(Hash)
  puts "(keys: #{result.keys.join(", ")})" unless result.keys.empty?
end
ok ? passed += 1 : failed += 1

# ---------------------------------------------------------------------------
# 2. Timbrar CFDI
# ---------------------------------------------------------------------------
puts "\n[ CFDI ]"
ok = run_test("cfdi.timbrar devuelve uuid") do
  payload = {
    "rfc_emisor"       => "EKU9003173C9",
    "serie"            => "SDK",
    "folio"            => "RUBY-001",
    "idempotency_key"  => "sdk-test-ruby-001",
    "tipo_comprobante" => "I",
    "moneda"           => "MXN",
    "forma_pago"       => "03",
    "metodo_pago"      => "PUE",
    "receptor"         => {
      "rfc"              => "URE180429TM6",
      "nombre"           => "UNIVERSIDAD ROBOTICA ESPANOLA",
      "domicilio_fiscal" => "65000",
      "regimen_fiscal"   => "601",
      "uso_cfdi"         => "G03"
    },
    "conceptos" => [
      {
        "clave_prod_serv" => "01010101",
        "cantidad"        => 1,
        "clave_unidad"    => "H87",
        "descripcion"     => "Servicio prueba SDK Ruby",
        "valor_unitario"  => 1000,
        "importe"         => 1000,
        "objeto_imp"      => "02",
        "impuestos"       => {
          "traslados" => [
            {
              "base"         => 1000,
              "impuesto"     => "002",
              "tipo_factor"  => "Tasa",
              "tasa_o_cuota" => "0.160000",
              "importe"      => 160
            }
          ]
        }
      }
    ],
    "subtotal"   => 1000,
    "impuestos"  => {
      "total_impuestos_trasladados" => 160,
      "traslados" => [
        {
          "base"         => 1000,
          "impuesto"     => "002",
          "tipo_factor"  => "Tasa",
          "tasa_o_cuota" => "0.160000",
          "importe"      => 160
        }
      ]
    },
    "total" => 1160
  }

  result = client.cfdi.timbrar(payload)
  uuid = result["uuid"] || result["folio_fiscal"]
  raise "uuid ausente en la respuesta. Keys: #{result.keys.inspect}" unless uuid
  raise "uuid vacío" if uuid.strip.empty?
  puts "(UUID: #{uuid})"
end
ok ? passed += 1 : failed += 1

# ---------------------------------------------------------------------------
# Resumen
# ---------------------------------------------------------------------------
puts "\n================================"
puts "Resultado: #{passed} OK, #{failed} fallidos"
puts "================================\n"
exit(failed > 0 ? 1 : 0)
