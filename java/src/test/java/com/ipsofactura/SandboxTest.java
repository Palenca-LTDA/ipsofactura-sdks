package com.ipsofactura;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Integration tests that run against the Ipsofactura sandbox.
 * They are skipped automatically when IPSO_API_KEY is not set, so they
 * never break CI builds without credentials.
 *
 * To run locally:
 *   IPSO_API_KEY=your_sandbox_key ./gradlew test
 */
class SandboxTest {

    private static IpsofacturaClient client;

    @BeforeAll
    static void setUp() {
        String apiKey = System.getenv("IPSO_API_KEY");
        Assumptions.assumeTrue(
            apiKey != null && !apiKey.isBlank(),
            "IPSO_API_KEY not set — skipping sandbox tests"
        );
        client = new IpsofacturaClient(apiKey, IpsofacturaClient.SANDBOX_URL);
    }

    // -------------------------------------------------------------------------
    // Empresas
    // -------------------------------------------------------------------------

    @Test
    void empresasList_returnsNonEmptyResult() {
        Map<String, Object> result = client.empresas().list();
        assertNotNull(result, "Response must not be null");

        // The API returns either a top-level list under "data" or directly an array-like map
        assertTrue(
            result.containsKey("data") || result.containsKey("empresas") || !result.isEmpty(),
            "Response should contain empresa data"
        );
    }

    // -------------------------------------------------------------------------
    // CFDI — timbrar
    // -------------------------------------------------------------------------

    @Test
    void cfdiTimbrar_returnsUuid() {
        Map<String, Object> payload = buildTestCfdiPayload();
        Map<String, Object> result  = client.cfdi().timbrar(payload);

        assertNotNull(result, "Timbrar response must not be null");
        assertTrue(
            result.containsKey("uuid") || result.containsKey("folio_fiscal"),
            "Response must contain 'uuid' or 'folio_fiscal'"
        );

        String uuid = result.containsKey("uuid")
                ? (String) result.get("uuid")
                : (String) result.get("folio_fiscal");

        assertNotNull(uuid, "UUID must not be null");
        assertFalse(uuid.isBlank(), "UUID must not be blank");
        System.out.println("[SandboxTest] CFDI timbrado — UUID: " + uuid);
    }

    // -------------------------------------------------------------------------
    // Helper: test payload
    // -------------------------------------------------------------------------

    @SuppressWarnings("unchecked")
    private static Map<String, Object> buildTestCfdiPayload() {
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
        concepto.put("descripcion", "Servicio prueba SDK Java");
        concepto.put("valor_unitario", 1000);
        concepto.put("importe", 1000);
        concepto.put("objeto_imp", "02");
        concepto.put("impuestos", impuestosConcepto);

        Map<String, Object> impuestosGlobal = new HashMap<>();
        impuestosGlobal.put("total_impuestos_trasladados", 160);
        impuestosGlobal.put("traslados", List.of(traslado));

        Map<String, Object> payload = new HashMap<>();
        payload.put("rfc_emisor", "EKU9003173C9");
        payload.put("serie", "SDK");
        payload.put("folio", "JAVA-001");
        payload.put("idempotency_key", "sdk-test-java-001");
        payload.put("tipo_comprobante", "I");
        payload.put("moneda", "MXN");
        payload.put("forma_pago", "03");
        payload.put("metodo_pago", "PUE");
        payload.put("receptor", receptor);
        payload.put("conceptos", List.of(concepto));
        payload.put("subtotal", 1000);
        payload.put("impuestos", impuestosGlobal);
        payload.put("total", 1160);

        return payload;
    }
}
