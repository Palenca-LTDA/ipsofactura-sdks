# @ipsofactura/sdk

Cliente oficial de **Ipsofactura** para JavaScript y TypeScript — timbrado CFDI 4.0 para México.

Requiere Node.js 18 o superior (usa `fetch` nativo).

---

## Instalación

```bash
npm install @ipsofactura/sdk
```

---

## Inicio rápido

```ts
import IpsofacturaClient from "@ipsofactura/sdk";

const client = new IpsofacturaClient(process.env.IPSO_API_KEY!);
const cfdi = await client.cfdi.timbrar({ /* payload CFDI 4.0 */ });
console.log(cfdi.folio_fiscal);
```

---

## Autenticación

La API usa el header `x-api` con tu API key. El cliente lo agrega automáticamente a todas las peticiones.

```ts
const client = new IpsofacturaClient("tu-api-key");
```

---

## Sandbox vs Producción

Por defecto el cliente apunta a producción (`https://api.ipsofactura.com`).
Para usar el sandbox:

```ts
const client = new IpsofacturaClient(apiKey, {
  baseUrl: "https://sandbox.api.ipsofactura.com",
});
```

> El sandbox usa el RFC de prueba del SAT `EKU9003173C9`. Los CFDIs timbrados ahí no tienen validez fiscal.

---

## Ejemplos

### Timbrar una factura (tipo I)

```ts
const result = await client.cfdi.timbrar({
  rfc_emisor: "EKU9003173C9",
  serie: "A",
  folio: "1",
  tipo_comprobante: "I",
  moneda: "MXN",
  forma_pago: "03",
  metodo_pago: "PUE",
  receptor: {
    rfc: "URE180429TM6",
    nombre: "UNIVERSIDAD ROBOTICA ESPANOLA",
    domicilio_fiscal: "65000",
    regimen_fiscal: "601",
    uso_cfdi: "G03",
  },
  conceptos: [
    {
      clave_prod_serv: "01010101",
      cantidad: 1,
      clave_unidad: "H87",
      descripcion: "Servicio de desarrollo",
      valor_unitario: 5000,
      importe: 5000,
      objeto_imp: "02",
      impuestos: {
        traslados: [
          {
            base: 5000,
            impuesto: "002",
            tipo_factor: "Tasa",
            tasa_o_cuota: "0.160000",
            importe: 800,
          },
        ],
      },
    },
  ],
  subtotal: 5000,
  impuestos: {
    total_impuestos_trasladados: 800,
    traslados: [
      {
        base: 5000,
        impuesto: "002",
        tipo_factor: "Tasa",
        tasa_o_cuota: "0.160000",
        importe: 800,
      },
    ],
  },
  total: 5800,
});

console.log(result.folio_fiscal); // UUID del SAT
console.log(result.id);           // ID interno de Ipsofactura
```

### Timbrar nómina

```ts
const nomina = await client.cfdi.timbrarNomina({
  rfc_emisor: "MI_RFC",
  // ... campos del complemento de nómina 1.2
});
```

### Cancelar un CFDI

```ts
const cancelacion = await client.cfdi.cancelar({
  id: "cfdi_abc123",
  motivo: "02", // 01=comprobante emitido por error, 02=comprobante con errores con relación
  folio_sustitucion: "cfdi_xyz456", // requerido si motivo es "01"
});
```

### Listar CFDIs con filtros

```ts
const lista = await client.cfdi.list({
  page: 1,
  per_page: 50,
  rfc_emisor: "MI_RFC",
  tipo_comprobante: "I",
  serie: "A",
});
```

### Obtener XML y PDF

```ts
const { xml_url } = await client.cfdi.getXml("cfdi_abc123");
const { pdf_url } = await client.cfdi.getPdf("cfdi_abc123");
```

### Gestionar empresas emisoras

```ts
// Listar todas las empresas
const empresas = await client.empresas.list();

// Crear empresa
const empresa = await client.empresas.crear({
  rfc: "MI_RFC",
  nombre: "Mi Empresa S.A. de C.V.",
  regimen_fiscal: "601",
  domicilio_fiscal: "06600",
});

// Subir certificado CSD
await client.empresas.subirCertificado(empresa.id, {
  cer: "<base64 del archivo .cer>",
  key: "<base64 del archivo .key>",
  password: "contraseña_del_certificado",
});
```

### Crear y gestionar webhooks

```ts
// Crear webhook
const webhook = await client.webhooks.crear({
  url: "https://mi-sistema.com/hooks/ipsofactura",
  events: ["cfdi.timbrado", "cfdi.cancelado"],
  secret: "mi-secreto-seguro",
});

// Ver historial de entregas
const deliveries = await client.webhooks.listDeliveries(webhook.id);

// Reenviar una entrega fallida
await client.webhooks.resendDelivery(webhook.id, deliveries[0].id);

// Enviar evento de prueba
await client.webhooks.test(webhook.id);
```

### Verificar firma de webhook

```ts
import { verifyWebhookSignature } from "@ipsofactura/sdk";

// En tu endpoint receptor (Express, Fastify, etc.)
app.post("/webhook", express.raw({ type: "application/json" }), (req, res) => {
  const isValid = verifyWebhookSignature(
    req.body.toString(),
    req.headers["x-ipso-timestamp"] as string,
    req.headers["x-ipso-signature"] as string,
    process.env.WEBHOOK_SECRET!,
  );

  if (!isValid) {
    return res.status(401).json({ error: "Firma inválida" });
  }

  const evento = JSON.parse(req.body.toString());
  console.log("Evento recibido:", evento.type);
  res.sendStatus(200);
});
```

---

## Manejo de errores

```ts
import IpsofacturaClient, { IpsofacturaError } from "@ipsofactura/sdk";

try {
  await client.cfdi.timbrar({ /* ... */ });
} catch (err) {
  if (err instanceof IpsofacturaError) {
    console.error(`Error ${err.status}: [${err.code}] ${err.message}`);
    if (err.satCode) {
      console.error(`Código SAT: ${err.satCode}`);
    }
    if (err.retryAfter) {
      console.error(`Reintentar en ${err.retryAfter} segundos`);
    }
  }
}
```

---

## TypeScript

El SDK está escrito en TypeScript. Todos los métodos devuelven `Promise<T>` con `T = unknown` por defecto, lo que permite tipar las respuestas según tus propias interfaces:

```ts
interface CfdiTimbrado {
  id: string;
  folio_fiscal: string;
  total: number;
  status: string;
}

const result = await client.cfdi.timbrar<CfdiTimbrado>({ /* ... */ });
console.log(result.folio_fiscal); // string
```

---

## Requisitos

- Node.js >= 18 (usa `fetch` nativo)
- TypeScript >= 5 (si usas tipos)

---

## Licencia

MIT — Palenca LTDA
