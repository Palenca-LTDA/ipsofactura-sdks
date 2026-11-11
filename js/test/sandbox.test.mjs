/**
 * Sandbox integration test para el SDK de Ipsofactura (JS/TS).
 *
 * Uso:
 *   IPSO_API_KEY=tu-key node test/sandbox.test.mjs
 *
 * El script usa el RFC de prueba del SAT (EKU9003173C9) que siempre
 * está disponible en el ambiente sandbox.
 *
 * No requiere framework de testing — usa Node.js puro para facilitar
 * la ejecución sin dependencias adicionales.
 */

// Importar desde el source compilado (dist/) o directamente si se usa tsx/ts-node.
// Para ejecutar sin compilar, usar: node --import tsx test/sandbox.test.mjs
// CJS → ESM interop: usar named imports
import { IpsofacturaClient, IpsofacturaError } from "../dist/index.js";

// ---------------------------------------------------------------------------
// Configuración
// ---------------------------------------------------------------------------

const API_KEY = process.env.IPSO_API_KEY;
if (!API_KEY) {
  console.error("ERROR: La variable de entorno IPSO_API_KEY no está definida.");
  process.exit(1);
}

const SANDBOX_URL = "https://sandbox.api.ipsofactura.com";
const client = new IpsofacturaClient(API_KEY, { baseUrl: SANDBOX_URL });

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

let passed = 0;
let failed = 0;

function section(title) {
  console.log(`\n${"─".repeat(60)}`);
  console.log(`  ${title}`);
  console.log("─".repeat(60));
}

function ok(label, value) {
  console.log(`  ✓  ${label}`);
  if (value !== undefined) {
    console.log(`     ${JSON.stringify(value, null, 2).split("\n").join("\n     ")}`);
  }
  passed++;
}

function fail(label, err) {
  console.error(`  ✗  ${label}`);
  if (err instanceof IpsofacturaError) {
    console.error(`     IpsofacturaError [${err.status}] ${err.code}: ${err.message}`);
    if (err.satCode) console.error(`     SAT code: ${err.satCode}`);
    if (err.retryAfter) console.error(`     Retry after: ${err.retryAfter}s`);
  } else {
    console.error(`     ${err?.message ?? err}`);
  }
  failed++;
}

// ---------------------------------------------------------------------------
// Payload de prueba — RFC de prueba oficial del SAT
// ---------------------------------------------------------------------------

const facturaPayload = {
  rfc_emisor: "EKU9003173C9",
  serie: "SDK",
  folio: "JS-001",
  idempotency_key: "sdk-test-js-001",
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
      descripcion: "Servicio de prueba SDK",
      valor_unitario: 1000,
      importe: 1000,
      objeto_imp: "02",
      impuestos: {
        traslados: [
          {
            base: 1000,
            impuesto: "002",
            tipo_factor: "Tasa",
            tasa_o_cuota: "0.160000",
            importe: 160,
          },
        ],
      },
    },
  ],
  subtotal: 1000,
  impuestos: {
    total_impuestos_trasladados: 160,
    traslados: [
      {
        base: 1000,
        impuesto: "002",
        tipo_factor: "Tasa",
        tasa_o_cuota: "0.160000",
        importe: 160,
      },
    ],
  },
  total: 1160,
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

section("1. Empresas — list()");

let empresas;
try {
  empresas = await client.empresas.list();
  ok("empresas.list() respondió correctamente", {
    total: empresas?.data?.length ?? empresas?.length ?? "(sin paginación)",
  });
} catch (err) {
  fail("empresas.list() falló", err);
}

// ---------------------------------------------------------------------------

section("2. CFDI — timbrar()");

let cfdiCreado;
try {
  cfdiCreado = await client.cfdi.timbrar(facturaPayload);
  ok("cfdi.timbrar() exitoso", {
    id: cfdiCreado?.id,
    folio_fiscal: cfdiCreado?.folio_fiscal ?? cfdiCreado?.uuid,
    serie_folio: `${cfdiCreado?.serie ?? ""}-${cfdiCreado?.folio ?? ""}`,
    total: cfdiCreado?.total,
  });
} catch (err) {
  fail("cfdi.timbrar() falló", err);
}

// ---------------------------------------------------------------------------

section("3. CFDI — get(id)");

if (cfdiCreado?.id) {
  try {
    const cfdiDetalle = await client.cfdi.get(cfdiCreado.id);
    ok("cfdi.get(id) devolvió el CFDI timbrado", {
      id: cfdiDetalle?.id,
      status: cfdiDetalle?.status ?? cfdiDetalle?.estatus,
      folio_fiscal: cfdiDetalle?.folio_fiscal ?? cfdiDetalle?.uuid,
    });
  } catch (err) {
    fail(`cfdi.get(${cfdiCreado.id}) falló`, err);
  }
} else {
  console.log("  -  Skipped (no se obtuvo id del paso anterior)");
}

// ---------------------------------------------------------------------------

section("4. CFDI — list() con filtros");

try {
  const lista = await client.cfdi.list({
    per_page: 5,
    rfc_emisor: "EKU9003173C9",
  });
  ok("cfdi.list() con filtros respondió", {
    items: lista?.data?.length ?? lista?.length ?? "(sin datos)",
  });
} catch (err) {
  fail("cfdi.list() falló", err);
}

// ---------------------------------------------------------------------------

section("5. CFDI — getXml(id) y getPdf(id)");

if (cfdiCreado?.id) {
  try {
    const xmlResult = await client.cfdi.getXml(cfdiCreado.id);
    ok("cfdi.getXml() devolvió URL del XML", {
      xml_url: xmlResult?.xml_url ? xmlResult.xml_url.substring(0, 60) + "..." : "(vacío)",
    });
  } catch (err) {
    fail("cfdi.getXml() falló", err);
  }

  try {
    const pdfResult = await client.cfdi.getPdf(cfdiCreado.id);
    ok("cfdi.getPdf() devolvió URL del PDF", {
      pdf_url: pdfResult?.pdf_url ? pdfResult.pdf_url.substring(0, 60) + "..." : "(vacío)",
    });
  } catch (err) {
    fail("cfdi.getPdf() falló", err);
  }
} else {
  console.log("  -  Skipped (no se obtuvo id en paso 2)");
}

// ---------------------------------------------------------------------------

section("6. CFDI — getEstatus(id)");

if (cfdiCreado?.id) {
  try {
    const estatus = await client.cfdi.getEstatus(cfdiCreado.id);
    ok("cfdi.getEstatus() respondió", {
      estatus: estatus?.estatus ?? estatus?.status ?? estatus,
    });
  } catch (err) {
    fail("cfdi.getEstatus() falló", err);
  }
}

// ---------------------------------------------------------------------------

section("7. Webhooks — list()");

try {
  const webhooks = await client.webhooks.list();
  ok("webhooks.list() respondió", {
    total: webhooks?.data?.length ?? webhooks?.length ?? "(sin datos)",
  });
} catch (err) {
  fail("webhooks.list() falló", err);
}

// ---------------------------------------------------------------------------

section("8. Error handling — API key inválida");

try {
  const badClient = new IpsofacturaClient("key-invalida-000", {
    baseUrl: SANDBOX_URL,
  });
  await badClient.cfdi.list();
  fail("Debería haber lanzado IpsofacturaError", new Error("No se lanzó error"));
} catch (err) {
  if (err instanceof IpsofacturaError) {
    ok(`IpsofacturaError lanzado correctamente [status=${err.status}]`, {
      code: err.code,
      message: err.message,
    });
  } else {
    fail("Error inesperado (esperaba IpsofacturaError)", err);
  }
}

// ---------------------------------------------------------------------------
// Resumen
// ---------------------------------------------------------------------------

console.log(`\n${"═".repeat(60)}`);
console.log(`  Resultado: ${passed} pasados, ${failed} fallados`);
console.log("═".repeat(60));

if (failed > 0) {
  process.exit(1);
}
