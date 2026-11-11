# Ipsofactura Java SDK

Cliente oficial para la API de timbrado CFDI 4.0 de [Ipsofactura](https://ipsofactura.com).  
Java 11+. Sin dependencias externas salvo Gson.

---

## Instalación

### Gradle

```groovy
dependencies {
    implementation 'com.ipsofactura:ipsofactura-java:0.1.0'
}
```

### Maven

```xml
<dependency>
    <groupId>com.ipsofactura</groupId>
    <artifactId>ipsofactura-java</artifactId>
    <version>0.1.0</version>
</dependency>
```

---

## Inicio rápido

```java
import com.ipsofactura.IpsofacturaClient;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Sandbox
IpsofacturaClient client = new IpsofacturaClient("mi_api_key", IpsofacturaClient.SANDBOX_URL);

// Producción (por defecto)
IpsofacturaClient client = new IpsofacturaClient("mi_api_key");
```

---

## Timbrar un CFDI

```java
Map<String, Object> receptor = new HashMap<>();
receptor.put("rfc", "URE180429TM6");
receptor.put("nombre", "UNIVERSIDAD ROBOTICA ESPANOLA");
receptor.put("domicilio_fiscal", "65000");
receptor.put("regimen_fiscal", "601");
receptor.put("uso_cfdi", "G03");

Map<String, Object> traslado = new HashMap<>();
traslado.put("base", 1000);
traslado.put("impuesto", "002");
traslado.put("tipo_factor", "Tasa");
traslado.put("tasa_o_cuota", "0.160000");
traslado.put("importe", 160);

Map<String, Object> impuestosConcepto = new HashMap<>();
impuestosConcepto.put("traslados", List.of(traslado));

Map<String, Object> concepto = new HashMap<>();
concepto.put("clave_prod_serv", "01010101");
concepto.put("cantidad", 1);
concepto.put("clave_unidad", "H87");
concepto.put("descripcion", "Servicio de consultoría");
concepto.put("valor_unitario", 1000);
concepto.put("importe", 1000);
concepto.put("objeto_imp", "02");
concepto.put("impuestos", impuestosConcepto);

Map<String, Object> impuestosGlobal = new HashMap<>();
impuestosGlobal.put("total_impuestos_trasladados", 160);
impuestosGlobal.put("traslados", List.of(traslado));

Map<String, Object> payload = new HashMap<>();
payload.put("rfc_emisor", "EKU9003173C9");
payload.put("serie", "A");
payload.put("folio", "1001");
payload.put("tipo_comprobante", "I");
payload.put("moneda", "MXN");
payload.put("forma_pago", "03");
payload.put("metodo_pago", "PUE");
payload.put("receptor", receptor);
payload.put("conceptos", List.of(concepto));
payload.put("subtotal", 1000);
payload.put("impuestos", impuestosGlobal);
payload.put("total", 1160);

Map<String, Object> result = client.cfdi().timbrar(payload);
String uuid = (String) result.get("uuid");
System.out.println("UUID: " + uuid);
```

---

## Gestión de empresas

```java
// Listar
Map<String, Object> empresas = client.empresas().list();

// Crear
Map<String, Object> empresa = client.empresas().crear(Map.of(
    "rfc", "EKU9003173C9",
    "nombre", "MI EMPRESA SA DE CV"
));

// Subir certificado
Map<String, Object> cert = client.empresas().subirCertificado(
    empresaId,
    Map.of(
        "cer_base64", "...",
        "key_base64", "...",
        "password",   "mi_contraseña"
    )
);
```

---

## Cancelar un CFDI

```java
Map<String, Object> result = client.cfdi().cancelar(Map.of(
    "uuid",        "UUID-DEL-CFDI",
    "rfc_emisor",  "EKU9003173C9",
    "motivo",      "02"
));
```

---

## Descargar XML / PDF

```java
Map<String, Object> xml = client.cfdi().getXml("cfdi_id");
String xmlUrl = (String) xml.get("xml_url");

Map<String, Object> pdf = client.cfdi().getPdf("cfdi_id");
String pdfUrl = (String) pdf.get("pdf_url");
```

---

## Webhooks

```java
// Crear
Map<String, Object> wh = client.webhooks().crear(Map.of(
    "url",     "https://mi-servidor.com/hooks",
    "eventos", List.of("cfdi.timbrado", "cfdi.cancelado")
));
String webhookId = (String) wh.get("id");

// Verificar firma de un evento recibido
boolean valida = WebhookUtils.verifySignature(
    requestBody,          // body crudo del POST
    timestampHeader,      // header x-ipso-timestamp
    signatureHeader,      // header x-ipso-signature
    webhookSecret
);
```

---

## Manejo de errores

```java
import com.ipsofactura.IpsofacturaException;

try {
    Map<String, Object> result = client.cfdi().timbrar(payload);
} catch (IpsofacturaException e) {
    System.err.println("Status: " + e.getStatusCode());
    System.err.println("Code:   " + e.getCode());
    System.err.println("Msg:    " + e.getMessage());
    if (e.getSatCode() != null)   System.err.println("SAT:    " + e.getSatCode());
    if (e.getRetryAfter() != null) System.err.println("Retry:  " + e.getRetryAfter() + "s");
}
```

---

## Tests de integración (sandbox)

```bash
IPSO_API_KEY=tu_api_key ./gradlew test
```

Los tests se omiten automáticamente si `IPSO_API_KEY` no está definida.

---

## Licencia

MIT — ver [LICENSE](../LICENSE).
