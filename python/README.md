# ipsofactura (Python)

Cliente oficial de **Ipsofactura** para Python — timbrado CFDI 4.0 para México.

Incluye clientes síncrono y asíncrono.

---

## Instalación

```bash
pip install ipsofactura
```

Requiere Python 3.9+ y `httpx>=0.27` (se instala automáticamente).

---

## Inicio rápido

```python
from ipsofactura import IpsofacturaClient

with IpsofacturaClient("tu-api-key") as client:
    cfdi = client.cfdi.timbrar({ ... })  # payload CFDI 4.0
    print(cfdi["folio_fiscal"])
```

---

## Autenticación

La API usa el header `x-api` con tu API key. El cliente lo agrega automáticamente.

```python
client = IpsofacturaClient("tu-api-key")
```

---

## Sandbox vs Producción

Por defecto el cliente apunta a producción (`https://api.ipsofactura.com`).
Para usar el sandbox:

```python
from ipsofactura import IpsofacturaClient, SANDBOX_BASE_URL

client = IpsofacturaClient("tu-api-key", base_url=SANDBOX_BASE_URL)
```

> El sandbox usa el RFC de prueba del SAT `EKU9003173C9`. Los CFDIs timbrados ahí no tienen validez fiscal.

---

## Ejemplos — cliente síncrono

### Timbrar una factura (tipo I)

```python
from ipsofactura import IpsofacturaClient

with IpsofacturaClient("tu-api-key") as client:
    result = client.cfdi.timbrar({
        "rfc_emisor": "MI_RFC",
        "serie": "A",
        "folio": "1",
        "tipo_comprobante": "I",
        "moneda": "MXN",
        "forma_pago": "03",
        "metodo_pago": "PUE",
        "receptor": {
            "rfc": "URE180429TM6",
            "nombre": "UNIVERSIDAD ROBOTICA ESPANOLA",
            "domicilio_fiscal": "65000",
            "regimen_fiscal": "601",
            "uso_cfdi": "G03",
        },
        "conceptos": [
            {
                "clave_prod_serv": "01010101",
                "cantidad": 1,
                "clave_unidad": "H87",
                "descripcion": "Servicio de desarrollo",
                "valor_unitario": 5000,
                "importe": 5000,
                "objeto_imp": "02",
                "impuestos": {
                    "traslados": [{
                        "base": 5000,
                        "impuesto": "002",
                        "tipo_factor": "Tasa",
                        "tasa_o_cuota": "0.160000",
                        "importe": 800,
                    }]
                },
            }
        ],
        "subtotal": 5000,
        "impuestos": {
            "total_impuestos_trasladados": 800,
            "traslados": [{
                "base": 5000,
                "impuesto": "002",
                "tipo_factor": "Tasa",
                "tasa_o_cuota": "0.160000",
                "importe": 800,
            }],
        },
        "total": 5800,
    })

    print(result["folio_fiscal"])  # UUID del SAT
    print(result["id"])            # ID interno de Ipsofactura
```

### Timbrar nómina

```python
nomina = client.cfdi.timbrar_nomina({
    "rfc_emisor": "MI_RFC",
    # ... campos del complemento de nómina 1.2
})
```

### Cancelar un CFDI

```python
cancelacion = client.cfdi.cancelar({
    "id": "cfdi_abc123",
    "motivo": "02",
    "folio_sustitucion": "cfdi_xyz456",  # solo si motivo es "01"
})
```

### Listar CFDIs con filtros

```python
lista = client.cfdi.list(
    page=1,
    per_page=50,
    rfc_emisor="MI_RFC",
    tipo_comprobante="I",
)
```

### Obtener XML y PDF

```python
xml = client.cfdi.get_xml("cfdi_abc123")   # {"xml_url": "https://..."}
pdf = client.cfdi.get_pdf("cfdi_abc123")   # {"pdf_url": "https://..."}
```

### Gestionar empresas emisoras

```python
# Listar todas las empresas
empresas = client.empresas.list()

# Crear empresa
empresa = client.empresas.crear({
    "rfc": "MI_RFC",
    "nombre": "Mi Empresa S.A. de C.V.",
    "regimen_fiscal": "601",
    "domicilio_fiscal": "06600",
})

# Subir certificado CSD
import base64

with open("cert.cer", "rb") as f:
    cer_b64 = base64.b64encode(f.read()).decode()

with open("cert.key", "rb") as f:
    key_b64 = base64.b64encode(f.read()).decode()

client.empresas.subir_certificado(empresa["id"], {
    "cer": cer_b64,
    "key": key_b64,
    "password": "contraseña_del_certificado",
})
```

### Webhooks

```python
# Crear webhook
webhook = client.webhooks.crear({
    "url": "https://mi-sistema.com/hooks/ipsofactura",
    "events": ["cfdi.timbrado", "cfdi.cancelado"],
    "secret": "mi-secreto-seguro",
})

# Ver historial de entregas
deliveries = client.webhooks.list_deliveries(webhook["id"])

# Reenviar una entrega fallida
client.webhooks.resend_delivery(webhook["id"], deliveries[0]["id"])

# Enviar evento de prueba
client.webhooks.test(webhook["id"])
```

---

## Ejemplos — cliente asíncrono

```python
import asyncio
from ipsofactura import AsyncIpsofacturaClient

async def main():
    async with AsyncIpsofacturaClient("tu-api-key") as client:
        # Timbrar
        cfdi = await client.cfdi.timbrar({ ... })
        print(cfdi["folio_fiscal"])

        # Consultar empresas y webhooks en paralelo
        import asyncio
        empresas, webhooks = await asyncio.gather(
            client.empresas.list(),
            client.webhooks.list(),
        )
        print(f"{len(empresas)} empresas, {len(webhooks)} webhooks")

asyncio.run(main())
```

---

## Verificar firma de webhook

```python
from ipsofactura import verify_webhook_signature

# En tu endpoint receptor (Flask, FastAPI, etc.)
# FastAPI:
from fastapi import FastAPI, Request, HTTPException

app = FastAPI()

@app.post("/webhook")
async def webhook(request: Request):
    raw_body = (await request.body()).decode("utf-8")
    timestamp = request.headers.get("x-ipso-timestamp", "")
    signature = request.headers.get("x-ipso-signature", "")

    if not verify_webhook_signature(raw_body, timestamp, signature, WEBHOOK_SECRET):
        raise HTTPException(status_code=401, detail="Firma inválida")

    evento = await request.json()
    print(f"Evento recibido: {evento['type']}")
    return {"ok": True}
```

---

## Manejo de errores

```python
from ipsofactura import IpsofacturaClient, IpsofacturaError

try:
    client.cfdi.timbrar({ ... })
except IpsofacturaError as err:
    print(f"Error {err.status_code}: [{err.code}] {err.message}")
    if err.sat_code:
        print(f"Código SAT: {err.sat_code}")
    if err.retry_after:
        print(f"Reintentar en {err.retry_after} segundos")
```

---

## Requisitos

- Python >= 3.9
- httpx >= 0.27

---

## Licencia

MIT — Palenca LTDA
