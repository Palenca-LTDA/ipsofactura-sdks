# Ipsofactura Ruby SDK

Cliente oficial para la API de timbrado CFDI 4.0 de [Ipsofactura](https://ipsofactura.com).  
Ruby 2.7+. Sin dependencias externas; solo stdlib (`net/http`, `uri`, `json`, `openssl`).

---

## Instalación

Agrega a tu `Gemfile`:

```ruby
gem "ipsofactura", "~> 0.1"
```

O instala directamente:

```bash
gem install ipsofactura
```

---

## Inicio rápido

```ruby
require "ipsofactura"

# Sandbox
client = Ipsofactura::Client.new("mi_api_key", base_url: Ipsofactura::SANDBOX_URL)

# Producción (por defecto)
client = Ipsofactura::Client.new("mi_api_key")
```

---

## Timbrar un CFDI

```ruby
result = client.cfdi.timbrar(
  rfc_emisor:       "EKU9003173C9",
  serie:            "A",
  folio:            "1001",
  tipo_comprobante: "I",
  moneda:           "MXN",
  forma_pago:       "03",
  metodo_pago:      "PUE",
  receptor: {
    rfc:              "URE180429TM6",
    nombre:           "UNIVERSIDAD ROBOTICA ESPANOLA",
    domicilio_fiscal: "65000",
    regimen_fiscal:   "601",
    uso_cfdi:         "G03"
  },
  conceptos: [
    {
      clave_prod_serv: "01010101",
      cantidad:        1,
      clave_unidad:    "H87",
      descripcion:     "Servicio de consultoría",
      valor_unitario:  1000,
      importe:         1000,
      objeto_imp:      "02",
      impuestos: {
        traslados: [
          { base: 1000, impuesto: "002", tipo_factor: "Tasa", tasa_o_cuota: "0.160000", importe: 160 }
        ]
      }
    }
  ],
  subtotal:   1000,
  impuestos: {
    total_impuestos_trasladados: 160,
    traslados: [
      { base: 1000, impuesto: "002", tipo_factor: "Tasa", tasa_o_cuota: "0.160000", importe: 160 }
    ]
  },
  total: 1160
)

puts result["uuid"]
```

---

## Gestión de empresas

```ruby
# Listar
empresas = client.empresas.list

# Crear
empresa = client.empresas.crear(
  rfc:    "EKU9003173C9",
  nombre: "MI EMPRESA SA DE CV"
)
empresa_id = empresa["id"]

# Subir certificado (CSD)
client.empresas.subir_certificado(empresa_id,
  cer_base64: "...",
  key_base64: "...",
  password:   "mi_contraseña"
)

# Eliminar certificado
client.empresas.eliminar_certificado(empresa_id, numero_serie)
```

---

## Cancelar un CFDI

```ruby
result = client.cfdi.cancelar(
  uuid:       "UUID-DEL-CFDI",
  rfc_emisor: "EKU9003173C9",
  motivo:     "02"
)
```

---

## Descargar XML / PDF

```ruby
xml = client.cfdi.get_xml("cfdi_id")
puts xml["xml_url"]

pdf = client.cfdi.get_pdf("cfdi_id")
puts pdf["pdf_url"]
```

---

## Webhooks

```ruby
# Crear
wh = client.webhooks.crear(
  url:     "https://mi-servidor.com/hooks",
  eventos: ["cfdi.timbrado", "cfdi.cancelado"]
)
webhook_id = wh["id"]

# Verificar firma de un evento recibido
valid = Ipsofactura::WebhookUtils.verify_signature(
  request.body.read,                    # body crudo
  request.env["HTTP_X_IPSO_TIMESTAMP"],
  request.env["HTTP_X_IPSO_SIGNATURE"],
  webhook_secret
)
raise "Firma inválida" unless valid
```

---

## Manejo de errores

```ruby
begin
  result = client.cfdi.timbrar(payload)
rescue Ipsofactura::Error => e
  puts "Status:    #{e.status}"
  puts "Code:      #{e.code}"
  puts "Mensaje:   #{e.message}"
  puts "SAT code:  #{e.sat_code}"    if e.sat_code
  puts "Retry en:  #{e.retry_after}s" if e.retry_after
end
```

---

## Test de integración (sandbox)

```bash
IPSO_API_KEY=tu_api_key ruby test_sandbox.rb
```

El script se omite automáticamente si `IPSO_API_KEY` no está definida.

---

## Licencia

MIT — ver [LICENSE](../LICENSE).
