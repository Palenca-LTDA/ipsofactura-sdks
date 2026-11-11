# ipsofactura-php

Cliente oficial de **Ipsofactura** para PHP 8.0+. Permite timbrar CFDI 4.0, gestionar empresas y administrar webhooks sin dependencias externas (solo la extensión `cURL`, siempre disponible).

## Instalación

```bash
composer require ipsofactura/ipsofactura-php
```

O sin Composer, incluye los archivos directamente desde `src/`.

## Quick start

```php
<?php
require __DIR__ . '/vendor/autoload.php';

use Ipsofactura\IpsofacturaClient;
use Ipsofactura\IpsofacturaException;

// Producción
$client = new IpsofacturaClient('tu_api_key');

// Sandbox
$client = new IpsofacturaClient('tu_api_key', IpsofacturaClient::SANDBOX_URL);
```

## Uso

### CFDI

```php
// Timbrar un CFDI
$cfdi = $client->cfdi->timbrar([
    'tipo_comprobante' => 'I',
    'serie'  => 'A',
    'folio'  => '1',
    // ... resto del payload
]);
echo $cfdi['uuid'];

// Listar CFDIs
$lista = $client->cfdi->list(['page' => 1, 'per_page' => 20]);

// Cancelar
$client->cfdi->cancelar(['uuid' => '...', 'motivo' => '02']);

// Descargar XML
$xml = $client->cfdi->getXml($cfdi['id']);
```

### Empresas

```php
// Listar empresas
$empresas = $client->empresas->list();

// Crear empresa
$empresa = $client->empresas->crear([
    'rfc'    => 'EKU9003173C9',
    'nombre' => 'Mi Empresa SA de CV',
]);

// Subir certificado CSD
$client->empresas->subirCertificado($empresa['id'], [
    'cer' => base64_encode(file_get_contents('certificado.cer')),
    'key' => base64_encode(file_get_contents('certificado.key')),
    'password' => 'mi_contraseña',
]);
```

### Webhooks

```php
// Crear webhook
$wh = $client->webhooks->crear([
    'url'    => 'https://mi-app.com/webhook',
    'eventos' => ['cfdi.timbrado', 'cfdi.cancelado'],
]);

// Verificar firma de un evento recibido
use Ipsofactura\WebhookUtils;

$valid = WebhookUtils::verifySignature(
    $payload,    // body crudo del request
    $timestamp,  // header X-Timestamp
    $signature,  // header X-Signature
    $secret      // secreto del webhook
);
```

## Manejo de errores

```php
use Ipsofactura\IpsofacturaException;

try {
    $cfdi = $client->cfdi->timbrar($payload);
} catch (IpsofacturaException $e) {
    echo $e->getStatusCode(); // HTTP status code
    echo $e->code;            // "VALIDATION_ERROR"
    echo $e->getMessage();    // Mensaje de error
    echo $e->satCode;         // "CFDI40xxx" si aplica
    echo $e->retryAfter;      // segundos para reintentar, si aplica
}
```

## Requisitos

- PHP >= 8.0
- Extensión `curl` habilitada (incluida en la mayoría de instalaciones PHP)

## Licencia

MIT
