<?php

declare(strict_types=1);

namespace Ipsofactura;

class WebhooksResource
{
    public function __construct(private IpsofacturaClient $client) {}

    /** Listar todos los webhooks */
    public function list(): array
    {
        return $this->client->request('GET', '/webhooks');
    }

    /** Crear un webhook */
    public function crear(array $payload): array
    {
        return $this->client->request('POST', '/webhooks', $payload);
    }

    /** Actualizar un webhook */
    public function actualizar(string $id, array $payload): array
    {
        return $this->client->request('PATCH', '/webhooks/' . urlencode($id), $payload);
    }

    /** Eliminar un webhook */
    public function eliminar(string $id): array
    {
        return $this->client->request('DELETE', '/webhooks/' . urlencode($id));
    }

    /** Listar deliveries de un webhook */
    public function listDeliveries(string $webhookId): array
    {
        return $this->client->request('GET', '/webhooks/' . urlencode($webhookId) . '/deliveries');
    }

    /** Obtener un delivery específico */
    public function getDelivery(string $webhookId, string $deliveryId): array
    {
        return $this->client->request(
            'GET',
            '/webhooks/' . urlencode($webhookId) . '/deliveries/' . urlencode($deliveryId)
        );
    }

    /** Re-enviar un delivery fallido */
    public function resendDelivery(string $webhookId, string $deliveryId): array
    {
        return $this->client->request(
            'POST',
            '/webhooks/' . urlencode($webhookId) . '/deliveries/' . urlencode($deliveryId) . '/resend'
        );
    }

    /** Enviar un evento de prueba al webhook */
    public function test(string $webhookId): array
    {
        return $this->client->request('POST', '/webhooks/' . urlencode($webhookId) . '/test');
    }
}
