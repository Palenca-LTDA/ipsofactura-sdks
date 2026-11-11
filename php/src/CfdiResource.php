<?php

declare(strict_types=1);

namespace Ipsofactura;

class CfdiResource
{
    public function __construct(private IpsofacturaClient $client) {}

    /** Timbrar un CFDI 4.0 */
    public function timbrar(array $payload): array
    {
        return $this->client->request('POST', '/cfdi/timbrar', $payload);
    }

    /** Timbrar un CFDI de nómina */
    public function timbrarNomina(array $payload): array
    {
        return $this->client->request('POST', '/cfdi/timbrar-nomina', $payload);
    }

    /** Timbrar desde XML ya generado */
    public function timbrarXml(array $payload): array
    {
        return $this->client->request('POST', '/cfdi/timbrar-xml', $payload);
    }

    /** Timbrar un comprobante pre-sellado */
    public function timbrarSellado(array $payload): array
    {
        return $this->client->request('POST', '/cfdi/timbrar-sellado', $payload);
    }

    /** Cancelar un CFDI timbrado */
    public function cancelar(array $payload): array
    {
        return $this->client->request('POST', '/cfdi/cancelar', $payload);
    }

    /** Agregar complemento de pago */
    public function complementoPago(array $payload): array
    {
        return $this->client->request('POST', '/cfdi/complemento-pago', $payload);
    }

    /** Crear un export masivo de CFDIs */
    public function crearExport(array $payload): array
    {
        return $this->client->request('POST', '/cfdi/exports', $payload);
    }

    /** Consultar el estado de un export */
    public function getExport(string $id): array
    {
        return $this->client->request('GET', '/cfdi/exports/' . urlencode($id));
    }

    /**
     * Listar CFDIs con filtros opcionales.
     *
     * @param array $params  page, per_page, tipo_comprobante, serie, folio,
     *                       rfc_emisor, folio_fiscal
     */
    public function list(array $params = []): array
    {
        return $this->client->request('GET', '/cfdi', null, $params);
    }

    /** Obtener un CFDI por ID */
    public function get(string $id): array
    {
        return $this->client->request('GET', '/cfdi/' . urlencode($id));
    }

    /** Consultar el estatus SAT de un CFDI */
    public function getEstatus(string $id): array
    {
        return $this->client->request('GET', '/cfdi/' . urlencode($id) . '/estatus');
    }

    /** Descargar el XML de un CFDI */
    public function getXml(string $id): array
    {
        return $this->client->request('GET', '/cfdi/' . urlencode($id) . '/xml');
    }

    /** Descargar el PDF de un CFDI */
    public function getPdf(string $id): array
    {
        return $this->client->request('GET', '/cfdi/' . urlencode($id) . '/pdf');
    }

    /** Obtener el acuse de cancelación */
    public function getAcuse(string $id): array
    {
        return $this->client->request('GET', '/cfdi/' . urlencode($id) . '/acuse');
    }
}
