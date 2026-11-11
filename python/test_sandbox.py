"""
Sandbox integration test para el SDK de Ipsofactura (Python).

Uso:
    IPSO_API_KEY=tu-key python test_sandbox.py

El script usa el RFC de prueba del SAT (EKU9003173C9) que siempre
está disponible en el ambiente sandbox.

No requiere pytest — es un script independiente.
"""

import json
import os
import sys

# Permitir ejecutar desde el directorio python/ sin instalar el paquete
sys.path.insert(0, os.path.dirname(__file__))

from ipsofactura import IpsofacturaClient, IpsofacturaError, SANDBOX_BASE_URL

# ---------------------------------------------------------------------------
# Configuración
# ---------------------------------------------------------------------------

API_KEY = os.environ.get("IPSO_API_KEY")
if not API_KEY:
    print("ERROR: La variable de entorno IPSO_API_KEY no está definida.", file=sys.stderr)
    sys.exit(1)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

passed = 0
failed = 0


def section(title: str) -> None:
    print(f"\n{'─' * 60}")
    print(f"  {title}")
    print("─" * 60)


def ok(label: str, value=None) -> None:
    global passed
    print(f"  ✓  {label}")
    if value is not None:
        formatted = json.dumps(value, ensure_ascii=False, indent=2)
        for line in formatted.splitlines():
            print(f"     {line}")
    passed += 1


def fail(label: str, err=None) -> None:
    global failed
    print(f"  ✗  {label}", file=sys.stderr)
    if isinstance(err, IpsofacturaError):
        print(
            f"     IpsofacturaError [{err.status_code}] {err.code}: {err.message}",
            file=sys.stderr,
        )
        if err.sat_code:
            print(f"     SAT code: {err.sat_code}", file=sys.stderr)
        if err.retry_after is not None:
            print(f"     Retry after: {err.retry_after}s", file=sys.stderr)
    elif err is not None:
        print(f"     {type(err).__name__}: {err}", file=sys.stderr)
    failed += 1


# ---------------------------------------------------------------------------
# Payload de prueba — RFC de prueba oficial del SAT
# ---------------------------------------------------------------------------

FACTURA_PAYLOAD = {
    "rfc_emisor": "EKU9003173C9",
    "serie": "SDK",
    "folio": "PY-001",
    "idempotency_key": "sdk-test-py-001",
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
            "descripcion": "Servicio de prueba SDK Python",
            "valor_unitario": 1000,
            "importe": 1000,
            "objeto_imp": "02",
            "impuestos": {
                "traslados": [
                    {
                        "base": 1000,
                        "impuesto": "002",
                        "tipo_factor": "Tasa",
                        "tasa_o_cuota": "0.160000",
                        "importe": 160,
                    }
                ]
            },
        }
    ],
    "subtotal": 1000,
    "impuestos": {
        "total_impuestos_trasladados": 160,
        "traslados": [
            {
                "base": 1000,
                "impuesto": "002",
                "tipo_factor": "Tasa",
                "tasa_o_cuota": "0.160000",
                "importe": 160,
            }
        ],
    },
    "total": 1160,
}

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

with IpsofacturaClient(API_KEY, base_url=SANDBOX_BASE_URL) as client:

    # -------------------------------------------------------------------------
    section("1. Empresas — list()")
    # -------------------------------------------------------------------------
    try:
        empresas = client.empresas.list()
        data = empresas.get("data", empresas) if isinstance(empresas, dict) else empresas
        ok("empresas.list() respondió correctamente", {
            "total": len(data) if isinstance(data, list) else "(sin datos)",
        })
    except Exception as err:
        fail("empresas.list() falló", err)

    # -------------------------------------------------------------------------
    section("2. CFDI — timbrar()")
    # -------------------------------------------------------------------------
    cfdi_creado = None
    try:
        cfdi_creado = client.cfdi.timbrar(FACTURA_PAYLOAD)
        ok("cfdi.timbrar() exitoso", {
            "id": cfdi_creado.get("id"),
            "folio_fiscal": cfdi_creado.get("folio_fiscal") or cfdi_creado.get("uuid"),
            "total": cfdi_creado.get("total"),
        })
    except Exception as err:
        fail("cfdi.timbrar() falló", err)

    # -------------------------------------------------------------------------
    section("3. CFDI — get(id)")
    # -------------------------------------------------------------------------
    if cfdi_creado and cfdi_creado.get("id"):
        cfdi_id = cfdi_creado["id"]
        try:
            detalle = client.cfdi.get(cfdi_id)
            ok("cfdi.get(id) devolvió el CFDI timbrado", {
                "id": detalle.get("id"),
                "status": detalle.get("status") or detalle.get("estatus"),
                "folio_fiscal": detalle.get("folio_fiscal") or detalle.get("uuid"),
            })
        except Exception as err:
            fail(f"cfdi.get({cfdi_id}) falló", err)
    else:
        print("  -  Skipped (no se obtuvo id del paso anterior)")

    # -------------------------------------------------------------------------
    section("4. CFDI — list() con filtros")
    # -------------------------------------------------------------------------
    try:
        lista = client.cfdi.list(per_page=5, rfc_emisor="EKU9003173C9")
        data = lista.get("data", lista) if isinstance(lista, dict) else lista
        ok("cfdi.list() con filtros respondió", {
            "items": len(data) if isinstance(data, list) else "(sin datos)",
        })
    except Exception as err:
        fail("cfdi.list() falló", err)

    # -------------------------------------------------------------------------
    section("5. CFDI — get_xml() y get_pdf()")
    # -------------------------------------------------------------------------
    if cfdi_creado and cfdi_creado.get("id"):
        cfdi_id = cfdi_creado["id"]
        try:
            xml_result = client.cfdi.get_xml(cfdi_id)
            xml_url = xml_result.get("xml_url", "") if isinstance(xml_result, dict) else ""
            ok("cfdi.get_xml() devolvió URL del XML", {
                "xml_url": xml_url[:60] + "..." if xml_url else "(vacío)",
            })
        except Exception as err:
            fail("cfdi.get_xml() falló", err)

        try:
            pdf_result = client.cfdi.get_pdf(cfdi_id)
            pdf_url = pdf_result.get("pdf_url", "") if isinstance(pdf_result, dict) else ""
            ok("cfdi.get_pdf() devolvió URL del PDF", {
                "pdf_url": pdf_url[:60] + "..." if pdf_url else "(vacío)",
            })
        except Exception as err:
            fail("cfdi.get_pdf() falló", err)
    else:
        print("  -  Skipped (no se obtuvo id en paso 2)")

    # -------------------------------------------------------------------------
    section("6. CFDI — get_estatus()")
    # -------------------------------------------------------------------------
    if cfdi_creado and cfdi_creado.get("id"):
        try:
            estatus = client.cfdi.get_estatus(cfdi_creado["id"])
            ok("cfdi.get_estatus() respondió", {
                "estatus": estatus.get("estatus") or estatus.get("status") if isinstance(estatus, dict) else estatus,
            })
        except Exception as err:
            fail("cfdi.get_estatus() falló", err)

    # -------------------------------------------------------------------------
    section("7. Webhooks — list()")
    # -------------------------------------------------------------------------
    try:
        webhooks = client.webhooks.list()
        data = webhooks.get("data", webhooks) if isinstance(webhooks, dict) else webhooks
        ok("webhooks.list() respondió", {
            "total": len(data) if isinstance(data, list) else "(sin datos)",
        })
    except Exception as err:
        fail("webhooks.list() falló", err)

    # -------------------------------------------------------------------------
    section("8. Error handling — API key inválida")
    # -------------------------------------------------------------------------
    try:
        bad_client = IpsofacturaClient("key-invalida-000", base_url=SANDBOX_BASE_URL)
        bad_client.cfdi.list()
        fail("Debería haber lanzado IpsofacturaError")
    except IpsofacturaError as err:
        ok(f"IpsofacturaError lanzado correctamente [status={err.status_code}]", {
            "code": err.code,
            "message": err.message,
        })
    except Exception as err:
        fail("Error inesperado (esperaba IpsofacturaError)", err)

# ---------------------------------------------------------------------------
# Resumen
# ---------------------------------------------------------------------------

print(f"\n{'═' * 60}")
print(f"  Resultado: {passed} pasados, {failed} fallados")
print("═" * 60)

if failed > 0:
    sys.exit(1)
