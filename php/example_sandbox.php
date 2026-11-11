<?php

declare(strict_types=1);

/**
 * Ejemplo de uso del SDK de Ipsofactura en sandbox.
 *
 * Uso:
 *   IPSO_API_KEY=tu_clave php example_sandbox.php
 */

// Si usas Composer, descomenta la siguiente línea:
// require __DIR__ . '/vendor/autoload.php';

// Sin Composer, carga las clases manualmente:
require __DIR__ . '/src/IpsofacturaException.php';
require __DIR__ . '/src/IpsofacturaClient.php';
require __DIR__ . '/src/CfdiResource.php';
require __DIR__ . '/src/EmpresasResource.php';
require __DIR__ . '/src/WebhooksResource.php';
require __DIR__ . '/src/WebhookUtils.php';

use Ipsofactura\IpsofacturaClient;
use Ipsofactura\IpsofacturaException;

$apiKey = getenv('IPSO_API_KEY') ?: 'sandbox_test_key';

$client = new IpsofacturaClient($apiKey, IpsofacturaClient::SANDBOX_URL);

// ── Listar empresas ────────────────────────────────────────────────────────────
echo "=== Empresas registradas ===\n";
try {
    $empresas = $client->empresas->list(['include_certificados' => 'true']);
    echo json_encode($empresas, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE) . "\n";
} catch (IpsofacturaException $e) {
    echo "Error {$e->getStatusCode()} [{$e->code}]: {$e->getMessage()}\n";
    if ($e->satCode !== null) {
        echo "SAT code: {$e->satCode}\n";
    }
}

echo "\n";

// ── Timbrar un CFDI de prueba ──────────────────────────────────────────────────
echo "=== Timbrar CFDI (folio PHP-001) ===\n";
$cfdiPayload = [
    'tipo_comprobante' => 'I',
    'serie'            => 'PHP',
    'folio'            => 'PHP-001',
    'fecha'            => date('Y-m-d\TH:i:s'),
    'forma_pago'       => '99',
    'metodo_pago'      => 'PPD',
    'moneda'           => 'MXN',
    'sub_total'        => 862.07,
    'total'            => 1000.00,
    'lugar_expedicion' => '06600',
    'emisor' => [
        'rfc'          => 'EKU9003173C9',
        'nombre'       => 'ESCUELA KEMPER URGATE',
        'regimen_fiscal' => '601',
    ],
    'receptor' => [
        'rfc'                        => 'XAXX010101000',
        'nombre'                     => 'Público en General',
        'uso_cfdi'                   => 'G03',
        'domicilio_fiscal_receptor'  => '06600',
        'regimen_fiscal_receptor'    => '616',
    ],
    'conceptos' => [
        [
            'clave_prod_serv'  => '01010101',
            'cantidad'         => 1,
            'clave_unidad'     => 'ACT',
            'descripcion'      => 'Servicio de prueba SDK PHP',
            'valor_unitario'   => 862.07,
            'importe'          => 862.07,
            'impuestos' => [
                'traslados' => [
                    [
                        'base'        => 862.07,
                        'impuesto'    => '002',
                        'tipo_factor' => 'Tasa',
                        'tasa_o_cuota' => 0.160000,
                        'importe'     => 137.93,
                    ],
                ],
            ],
        ],
    ],
    'impuestos' => [
        'total_impuestos_trasladados' => 137.93,
        'traslados' => [
            [
                'impuesto'    => '002',
                'tipo_factor' => 'Tasa',
                'tasa_o_cuota' => 0.160000,
                'importe'     => 137.93,
            ],
        ],
    ],
];

try {
    $cfdi = $client->cfdi->timbrar($cfdiPayload);
    echo "CFDI timbrado exitosamente.\n";
    echo "UUID (Folio Fiscal): " . ($cfdi['uuid'] ?? $cfdi['folio_fiscal'] ?? 'n/a') . "\n";
    echo json_encode($cfdi, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE) . "\n";
} catch (IpsofacturaException $e) {
    echo "Error {$e->getStatusCode()} [{$e->code}]: {$e->getMessage()}\n";
    if ($e->satCode !== null) {
        echo "SAT code: {$e->satCode}\n";
    }
    if ($e->retryAfter !== null) {
        echo "Retry after: {$e->retryAfter}s\n";
    }
}
