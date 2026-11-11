namespace Ipsofactura;

/// <summary>
/// Webhook operations: create, update, delete, deliveries, test.
/// </summary>
public sealed class WebhooksResource
{
    private readonly IpsofacturaClient _client;

    internal WebhooksResource(IpsofacturaClient client) => _client = client;

    /// <summary>Lista todos los webhooks configurados.</summary>
    public Task<Dictionary<string, object?>> ListAsync(CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, "/webhooks", cancellationToken: ct);

    /// <summary>Crea un nuevo webhook.</summary>
    public Task<Dictionary<string, object?>> CrearAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/webhooks", payload, cancellationToken: ct);

    /// <summary>Actualiza un webhook existente.</summary>
    public Task<Dictionary<string, object?>> ActualizarAsync(
        string id, Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(new HttpMethod("PATCH"), $"/webhooks/{Uri.EscapeDataString(id)}", payload, cancellationToken: ct);

    /// <summary>Elimina un webhook.</summary>
    public Task<Dictionary<string, object?>> EliminarAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Delete, $"/webhooks/{Uri.EscapeDataString(id)}", cancellationToken: ct);

    /// <summary>Lista el historial de deliveries de un webhook.</summary>
    public Task<Dictionary<string, object?>> ListDeliveriesAsync(string webhookId, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/webhooks/{Uri.EscapeDataString(webhookId)}/deliveries", cancellationToken: ct);

    /// <summary>Obtiene un delivery específico.</summary>
    public Task<Dictionary<string, object?>> GetDeliveryAsync(string webhookId, string deliveryId, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get,
            $"/webhooks/{Uri.EscapeDataString(webhookId)}/deliveries/{Uri.EscapeDataString(deliveryId)}",
            cancellationToken: ct);

    /// <summary>Re-envía un delivery fallido.</summary>
    public Task<Dictionary<string, object?>> ResendDeliveryAsync(string webhookId, string deliveryId, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post,
            $"/webhooks/{Uri.EscapeDataString(webhookId)}/deliveries/{Uri.EscapeDataString(deliveryId)}/resend",
            cancellationToken: ct);

    /// <summary>Envía un evento de prueba al endpoint del webhook.</summary>
    public Task<Dictionary<string, object?>> TestAsync(string webhookId, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, $"/webhooks/{Uri.EscapeDataString(webhookId)}/test", cancellationToken: ct);
}
