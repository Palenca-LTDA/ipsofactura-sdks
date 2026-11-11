<?php

declare(strict_types=1);

namespace Ipsofactura;

class EmpresasResource
{
    public function __construct(private IpsofacturaClient $client) {}

    /**
     * Listar empresas.
     *
     * @param array $params  q (búsqueda), include_certificados (bool)
     */
    public function list(array $params = []): array
    {
        return $this->client->request('GET', '/empresas', null, $params);
    }

    /** Crear una nueva empresa */
    public function crear(array $payload): array
    {
        return $this->client->request('POST', '/empresas', $payload);
    }

    /** Obtener empresa por ID */
    public function get(string $id): array
    {
        return $this->client->request('GET', '/empresas/' . urlencode($id));
    }

    /** Actualizar datos de una empresa */
    public function actualizar(string $id, array $payload): array
    {
        return $this->client->request('PATCH', '/empresas/' . urlencode($id), $payload);
    }

    /** Subir certificado CSD a una empresa */
    public function subirCertificado(string $empresaId, array $payload): array
    {
        return $this->client->request('POST', '/empresas/' . urlencode($empresaId) . '/certificados', $payload);
    }

    /** Refrescar la contraseña del certificado */
    public function refreshPassword(string $empresaId, array $payload): array
    {
        return $this->client->request(
            'POST',
            '/empresas/' . urlencode($empresaId) . '/certificados/refresh-password',
            $payload
        );
    }

    /** Eliminar un certificado por número de serie */
    public function eliminarCertificado(string $empresaId, string $numero): array
    {
        return $this->client->request(
            'DELETE',
            '/empresas/' . urlencode($empresaId) . '/certificados/' . urlencode($numero)
        );
    }
}
